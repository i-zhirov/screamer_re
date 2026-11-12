# Reachable Extent and Symbol-Aware Oracle Implementation Plan

> Execute this plan with the `executing-plans` skill. Each task is a
> checkbox. Mark it done when its changes are implemented and its Verify
> command passes.

- **Date**: 2026-11-12
- **Status**: Implemented
- **Design**: docs/explanation/rebuild-quality-design.md
- **Roadmap**: docs/explanation/rebuild-quality-roadmap.md — slice: oracle
- **Depends on**: Slice 1 — baseline
- **Manual prerequisites**: —
- **Goal**: Make the identity oracle compare the intra-function branch targets
  and the internal fixup symbol identity, cover the whole reachable function
  extent, stitch the LE pages, and record the new strict baseline.

## Constraints

- C-1: Python 3.14.7. Use the standard library and the `ctypes` capstone binding only.
- C-2: Tests use `unittest` under `tools/rebuild/tests/`.
- C-3: A tool takes its configuration from the command line. It reads no environment variable.
- C-4: A tool writes only under `work/` or to an explicit output path.
- C-5: Prefer C. Use assembly only when C cannot reproduce the original bytes, and record the reason.
- C-6: A harness output name stays within the 8.3 limit.
- C-7: The 10.0a toolchain runs alongside OpenWatcom V2. Do not replace V2.
- C-8: The status ladder stays `unknown`, `not_a_function`, `named`, `rebuilt`, `matching`, `verified`.
- C-9: The identity verdict strings stay unchanged.
- C-10: The full check reports 303 checks today. `--skip-cross` reports 299, `--skip-runtime` reports 288, and both flags report 284. Update these counts when a change adds or removes a check.
- C-11: Do not commit. The user commits.
- The identity `match` values are `exact-bytes`, `exact-relocations`, `same-shape`, `same-mnemonics`, and `different`.
- The report buckets are `exact`, `relocation-only`, `shape`, and `behavior-only`.
- Strict is the default identity gate. `--allow-shape` restores the historical gate. `--strict` stays accepted.
- `oracle.py` and `le.py` are `store.TOOL_FILES`. A change to either invalidates every stored fingerprint.

## Shared Interfaces

- `le.LinearExecutable.read(linear_address: int, length: int) -> bytes | None`
  keeps its signature. It stitches the LE pages.
- `le.function_body(exe: LinearExecutable, address: int, max_instructions: int = 4000, end: int | None = None) -> list[Insn]`.
- `le._jump_target(insn: Insn) -> int | None` returns the direct jump or loop
  target and returns `None` for a call and for an indirect operand.
- `le._next_function_start(exe: LinearExecutable, address: int) -> int | None`
  returns the first candidate function start strictly above an address.
- `le._table_jump_targets(exe: LinearExecutable, insn: Insn, block: list[Insn], address: int, end: int) -> list[int]`
  returns the compiler jump-table case targets of an indirect `jmp`.
- `oracle.compare(original, rebuilt, *, original_relocations=None, rebuilt_relocations=None, original_symbols=None, rebuilt_symbols=None) -> OracleResult`.
- `oracle.slot_key(slot: le.Relocation, symbols: Callable[[le.Relocation], str | None] | None = None) -> tuple`.
- `oracle._bounds(instructions: list[le.Insn]) -> tuple[int, int]` returns
  `(first.address, last.address + last.size)` and `(0, 0)` for an empty list.
- `oracle._index_by_address(instructions: list[le.Insn]) -> dict[int, int]`.
- `oracle._branch_target(insn: le.Insn) -> int | None` returns the target of a
  direct jump or loop and `None` for a call or any other instruction.
- `oracle._branch_difference(original, rebuilt, original_bounds, rebuilt_bounds, original_index, rebuilt_index) -> str | None`.
- `oracle._instruction_difference(original, rebuilt, original_mask, rebuilt_mask, original_slots, rebuilt_slots, original_bounds, rebuilt_bounds, original_index, rebuilt_index, original_symbols=None, rebuilt_symbols=None) -> str | None`.
- `diff_module.map_symbols(map_path: Path, exe: le.LinearExecutable) -> list[tuple[int, str]]`.
- `diff_module.symbol_at(symbols: list[tuple[int, str | None]], address: int) -> str | None`.
- `diff_module.next_symbol(symbols: list[tuple[int, str | None]], address: int) -> int | None`
  returns the nearest symbol address strictly above an address.
- `diff_module.rebuilt_symbol_resolver(exe, symbols) -> Callable[[le.Relocation], str | None]`.
- `diff_module.original_symbol_resolver(exe, records) -> Callable[[le.Relocation], str | None]`.
- `diff_module._internal_target(exe: le.LinearExecutable, slot: le.Relocation) -> int | None`.
- `diff_module.original_body(exe, address, end, count) -> list[le.Insn]` uses the
  end when it is present, else the count, else the default extent.
- `diff_module.rebuilt_body(exe, address, end, count, extent=None) -> list[le.Insn]`
  uses the rebuilt extent when it is present, else the default extent when the
  original end is present, else the count decode, else the default extent.
- `diff_module.parse_arguments` gains `--original-end`, parsed with
  `int(value, 0)` and default `None`.
- `store._record_fields` hashes the `size` field.
- `run.run_identity` appends `--original-end` with
  `int(record["address"], 16) + record["size"]` when the record holds `size`.

## Tasks

- [x] **Task 1: LE page stitching**
    - **Implements:** FR-009, design §Requirements
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_le_helpers.py`**
        - Update the module docstring to name the page reads.
        - Add `page(...) -> le.Page` with the defaults `object_index=1`,
          `page_index=0`, `number=1`, `flags=le.P_LEGAL`, `file_offset=0x20`,
          `data_size=0x10`, `virtual_address=0x10000`, `virtual_size=0x10`.
        - Add `page_image(data: bytes, pages: list[le.Page]) -> le.LinearExecutable`.
          It builds `le.LinearExecutable(Path("image.exe"), data)` directly and
          assigns `header`, `objects`, and `pages`, as `test_image_compare.py`
          does.
        - Add `PageReadTestCase`.
        - `test_read_stitches_two_pages` — two legal pages with adjacent
          virtual addresses and different file offsets. A read across the
          boundary returns the tail of the first page and the head of the
          second page.
        - `test_read_starts_inside_a_page` — a read at `virtual_address + 4`
          returns the four bytes at `file_offset + 4`.
        - `test_read_stops_at_the_page_data_size` — a page whose `data_size`
          is below its `virtual_size` returns a short slice. The bytes of the
          next page's file region do not enter the result.
        - `test_read_zero_fills_a_zeroed_page` — a `P_ZEROED` page gives
          `min(remaining, virtual_size - offset_in_page)` zero bytes.
        - `test_read_continues_past_a_zeroed_page` — a zeroed page between two
          legal pages gives zeros and continues into the second legal page.
        - `test_read_stops_at_a_page_without_a_file_offset` — a compressed
          page after a legal page stops the read. The result holds the legal
          page bytes.
        - `test_read_returns_none_when_the_start_page_has_no_file_offset`
          — a start in a compressed page gives `None`.
        - `test_read_stops_at_a_gap_between_pages` — an address in no page
          stops the read and keeps the collected bytes.
        - `test_read_returns_none_when_the_start_is_unmapped` — the result is
          `None`.
        - `test_read_accepts_a_zero_length` — a mapped start with length zero
          gives `b""`.
    - **Modify `tools/le.py`**
        - Change `LinearExecutable.read`. Return `b""` first when the length
          is zero and the start page is mapped. Return `None` when `page_at`
          finds no page for the start, or when the start page has no file
          offset and is not a `P_ZEROED` page. Otherwise loop from the start to
          `start + length`:
            - Read `page = page_at(current)`. A missing page stops the loop.
            - Compute `offset_in_page = current - page.virtual_address`.
            - A `P_ZEROED` page adds
              `min(remaining, page.virtual_size - offset_in_page)` zero bytes
              and the loop continues.
            - A non-zeroed page with `file_offset` `None` stops the loop.
            - Copy `min(remaining, page.data_size - offset_in_page, page.virtual_size - offset_in_page)`
              bytes from `page.file_offset + offset_in_page`. A count at or
              below zero stops the loop.
        - Keep `to_file_offset`. It still maps only the start page.
        - Update the `read` docstring. State the page walk, the zeroed page,
          the short page, the page without a file offset, and the unmapped
          start.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_le_helpers.py"`

- [x] **Task 2: Reachable function extent**
    - **Implements:** FR-008, design §Requirements
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_le_helpers.py`**
        - Update the module docstring to name the page reads and the reachable
          body.
        - Add `from collections.abc import Callable` to the imports.
        - Add `insn(address, mnemonic, operands, raw) -> le.Insn`.
        - Add `scripted_decode(blocks: dict[int, list[le.Insn]]) -> Callable[[le.LinearExecutable, int, int], list[le.Insn]]`.
          It returns a `decode` side effect that returns
          `blocks.get(address, [])[:max_instructions]`.
        - Add `ReachableBodyTestCase`.
        - `test_function_body_follows_a_conditional_jump_target` — a block
          that ends at a `ret` and a `jne` target block after it both enter
          the body.
        - `test_function_body_follows_a_backward_loop_target` — a `jb` to the
          block start adds no duplicate and the result stays in address order.
        - `test_function_body_does_not_follow_a_call` — the call target block
          stays out of the body.
        - `test_function_body_stops_at_a_terminator` — instructions after a
          `ret` in the same decoded block stay out of the body.
        - `test_function_body_stops_after_an_unconditional_jump` — the
          fallthrough after a `jmp` stays out and the target block enters.
        - `test_function_body_keeps_a_leading_jump` — the leading `jmp`
          instruction stays in the body and its target is followed.
        - `test_function_body_keeps_the_leading_jump_target_outside_the_bound`
          — a leading `jmp` whose target is at or above an explicit `end`
          still enters the body.
        - `test_function_body_ignores_a_target_outside_the_end` — a target at
          or above an explicit `end` is not followed.
        - `test_function_body_uses_the_next_start_as_the_default_end` — a
          `StubImage` with a prologue at a later conditional-jump target makes
          the target the default `end`, so the target is not followed.
        - `test_function_body_skips_the_current_register_save_prologue` — the
          current function starts with `push ebx; push ebp; mov ebp,esp`. The
          raw prologue pattern is at `address + 1`, and the next function
          starts later. The default `end` is the next function, not
          `address + 1`.
        - `test_function_body_stops_at_the_object_end` — without a prologue,
          the default `end` is the object end and a target at the object end
          is not followed.
        - `test_function_body_honors_the_instruction_cap` — the total body
          length does not exceed `max_instructions`.
        - `test_function_body_caps_the_leading_jumps` — with
          `max_instructions` below the number of leading jumps, the body does
          not exceed the cap.
        - `test_function_body_removes_a_duplicate_leading_jump` — a leading
          `jmp` that targets itself enters the body once.
        - `test_function_body_follows_a_compiler_jump_table` — a block that
          ends at `cmp eax, 2; ja 0x10040; jmp dword ptr cs:[eax*4 + 0x100]`
          enters the three table blocks. The table holds the object-relative
          entries `0x20`, `0x30`, and `0x40`, so the targets are `0x10020`,
          `0x10030`, and `0x10040`.
        - `test_function_body_jump_table_skips_a_target_outside_the_bound` — a
          table entry at or above the explicit `end` is not followed.
        - `test_function_body_jump_table_uses_the_cmp_bound` — a table with
          more entries than the `cmp` bound does not follow the extra entry.
        - `test_function_body_stops_at_an_indirect_jump_without_a_table` — an
          indirect `jmp` with no preceding `cmp` stops the block and follows
          no target.
        - Add `JumpTargetTestCase` and `NextFunctionStartTestCase`.
        - `test_jump_target_returns_a_direct_target` — a `jne 0x10010` gives
          `0x10010`.
        - `test_jump_target_returns_none_for_a_call` — a `call 0x10010` gives
          `None`.
        - `test_jump_target_returns_none_for_an_indirect_operand` — a
          `jmp dword ptr cs:[eax*4 + 0x100]` gives `None`.
        - `test_next_function_start_returns_the_nearest_later_prologue` — a
          `StubImage` with prologues at `0x10000` and `0x10010` returns
          `0x10010` for an address of `0x10005`.
        - `test_next_function_start_raises_for_an_unmapped_address` — a
          missing object raises `ValueError("the address is unmapped")`.
        - `test_next_function_start_raises_for_an_unmapped_range` — an
          `UnreadableImage` raises
          `ValueError("the range is not mapped to a file offset")`.
    - **Modify `tools/le.py`**
        - Add `_jump_target(insn) -> int | None`. It returns the target when
          the mnemonic starts with `j` or `loop` and the operand starts with
          `0x`. It returns `None` for `call`.
        - Add `_next_function_start(exe, address) -> int | None`. It takes the
          object at `address`, reads from `object.reloc_base` to
          `object.reloc_base + object.virtual_size`, and computes every
          candidate `object.reloc_base + prologue_start(data, position)`. It
          returns the smallest candidate strictly above `address`. A missing
          object raises `ValueError("the address is unmapped")`. A range that
          is not mapped raises
          `ValueError("the range is not mapped to a file offset")`.
        - Add `_table_jump_targets(exe, insn, block, address, end) -> list[int]`.
          It returns the compiler jump-table case targets of an indirect
          `jmp`.
            - Return `[]` when the mnemonic is not `jmp`, when the operand
              does not hold `cs:[`, or when the operand does not end with `]`.
            - Parse the displacement: the text after the last `+` and before
              the closing `]`, or the text between `[` and `]` when there is
              no `+`. Require a `0x` prefix.
            - Scan `block` backwards from `insn` for the nearest `cmp` with an
              immediate last operand. Parse the immediate with `int(token, 0)`.
              Return `[]` when no such `cmp` is found.
            - Set `count = immediate + 1`. Return `[]` when `count <= 0` or
              `count > 256`.
            - Find the object at `insn.address`. Return `[]` when the object
              is missing.
            - Read `count * 4` bytes at `object.reloc_base + displacement`.
              Return `[]` when the read is `None` or short.
            - For each 4-byte little-endian entry, compute
              `object.reloc_base + entry` and keep it when
              `address <= target < end`.
            - Return the targets in table order without duplicates.
        - Change `function_body` to the new signature.
        - Resolve `end` when it is `None`. Use `_next_function_start`. When
          that returns `None`, use `object.reloc_base + object.virtual_size`.
        - Keep the leading-jump behavior. Follow up to four leading
          unconditional jumps, add each jump instruction to the body, and
          start the worklist at the final target. Stop the leading-jump loop
          when the body reaches `max_instructions`, and skip a leading jump
          whose address is already in the body.
        - Walk the body with a worklist of block starts and a set of body
          addresses:
            - Pop a start. Skip it when the start address is already in the
              body.
            - Skip a start outside `[address, end)`. The leading-jump target
              is an allowed start even when it is outside the bound. Its block
              is decoded without the end check, because the leading jump
              keeps its current behavior.
            - Decode the block with `decode(exe, start, remaining)`.
            - Add each instruction in address order. Stop the block at an
              instruction whose address is already in the body, at a
              `TERMINATORS` member, at an address at or above `end` unless the
              start is the leading-jump target, or after an unconditional
              `jmp`.
            - Enqueue every direct jump and loop target inside
              `[address, end)`. Do not enqueue a call target.
            - When an unconditional `jmp` has no direct target, enqueue
              `_table_jump_targets(exe, insn, block, address, end)`.
            - Stop when the body reaches `max_instructions`.
        - Return the body sorted by address.
        - Update the `function_body` docstring. State the reachable blocks,
          the half-open bound `[address, end)`, the terminator stop, the
          unconditional jump stop, the call exclusion, the compiler jump
          table, the duplicate removal, and the cap.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_le_helpers.py"`, `python3 tools/le.py calls ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x8adac --max 400`, and `python3 tools/le.py calls ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x2807c --max 400`
        - The `0x8adac` output holds the second exit at `0x8af05`, past the
          first `ret` at `0x8aec2`.
        - The `0x2807c` output holds the case calls at `0x2e0b4`, `0x2e21c`,
          `0x2e58c`, and `0x2e724`.

- [x] **Task 3: Intra-function branch targets**
    - **Implements:** FR-006, design §Requirements
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_oracle.py`**
        - Extend `OracleTestCase.compare` with `original_symbols=None` and
          `rebuilt_symbols=None` and pass them to `oracle.compare`.
        - Rename `test_direct_branch_relocation_passes_strict` to
          `test_outside_function_branch_target_stays_masked`. Keep the same
          bodies and assertions. The target `0xd2008` is outside the original
          body and the target `0x10024` is outside the rebuilt body.
        - `test_intra_function_branch_target_matches_by_index` — a `jne` to
          the second instruction on both sides gives `EXACT_RELOCATIONS` and
          passes strict. The absolute addresses differ.
        - `test_intra_function_branch_target_mismatch_fails_strict` — the
          rebuilt `jne` points at the third instruction while the original
          points at the second. The result is `SAME_SHAPE` and does not pass
          strict.
        - `test_intra_function_branch_target_into_the_middle_is_a_difference`
          — the rebuilt target is inside the bounds but is not an instruction
          address. The result is `SAME_SHAPE` and does not pass strict.
        - `test_branch_target_inside_only_one_side_stays_masked` — the
          original target is inside and the rebuilt target is outside. The
          result is `EXACT_RELOCATIONS` and passes strict.
        - `test_intra_function_call_target_stays_masked` — a `call` target
          inside the bounds does not compare by instruction index. The result
          is `EXACT_RELOCATIONS` and passes strict.
    - **Modify `tools/rebuild/oracle.py`**
        - Add `from collections.abc import Callable` to the imports.
        - Add `_bounds(instructions) -> tuple[int, int]`. An empty list gives
          `(0, 0)`.
        - Add `_index_by_address(instructions) -> dict[int, int]`.
        - Add `_branch_target(insn) -> int | None`. It returns the target when
          the mnemonic starts with `j` or `loop` and the operand starts with
          `0x`. It returns `None` for `call` and for any other instruction.
        - Add `_branch_difference(original, rebuilt, original_bounds, rebuilt_bounds, original_index, rebuilt_index) -> str | None`.
          It runs only when both targets are inside their own bounds. It
          compares the instruction index of each target in its own list. A
          target that is not an instruction address gives the message
          `f"branch target: {target:#x} is not an instruction address"`. An
          index mismatch gives
          `f"branch target: instruction {left} against {right}"`.
        - Change `_instruction_difference` to accept the two bounds and the
          two index maps. Call `_branch_difference` after the unmasked-byte
          loop and before the relocation check.
        - Change `compare` to compute the bounds and the index maps for both
          sides and pass them.
        - Do not change the verdict strings or the five match values.
        - Update the `compare` docstring. State that an intra-function branch
          target compares by instruction position and that an outside target
          stays masked.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_oracle.py"`

- [x] **Task 4: Internal fixup symbol identity and the original bound**
    - **Implements:** FR-007, FR-008, design §Requirements
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_oracle.py`**
        - Add a `symbols(table: dict[tuple[int | None, int | None], str])`
          helper. It returns a resolver that looks up `(slot.object, slot.target)`.
        - `test_internal_symbol_identity_matches` — two internal slots with
          different object and target values resolve to the same symbol. The
          result is `EXACT_RELOCATIONS` and passes strict.
        - `test_internal_symbol_identity_mismatch_fails_strict` — the
          resolvers give different symbols. The result is `SAME_SHAPE` and
          does not pass strict.
        - `test_internal_symbol_identity_compares_the_addend` — the same
          symbol with different addends gives `SAME_SHAPE`.
        - `test_internal_symbol_identity_treats_a_missing_addend_as_zero` —
          an addend of `None` on one side and `0` on the other gives
          `EXACT_RELOCATIONS` and passes strict.
        - `test_internal_symbol_identity_falls_back_to_the_kind` — one
          resolver returns `None`. Both sides compare by kind and the result
          is `EXACT_RELOCATIONS`.
        - `test_slot_key_holds_the_symbol_and_the_addend` — an internal slot
          with a resolver gives `("internal", "free_", 0)` and without a
          resolver gives `("internal",)`.
        - `test_slot_key_keeps_an_import_key_with_a_resolver` — an
          import-ordinal slot gives `("import-ordinal", module, ordinal)`.
    - **Create `tools/rebuild/tests/test_diff_module.py`**
        - Add `StubObject` with `reloc_base` and `StubImage` with
          `object_by_index`.
        - `test_map_symbols_reads_a_linear_address_and_a_symbol` — a map line
          `0001:00000000  free_` and an object 1 base of `0x10000` give
          `[(0x10000, "free_")]`.
        - `test_map_symbols_skips_a_line_with_a_missing_object` — a segment
          that is not an object is skipped.
        - `test_symbol_at_returns_the_exact_symbol` — a symbol at `0x10000`
          and a target `0x10000` give the symbol.
        - `test_symbol_at_returns_none_for_an_interior_address` — a target
          `0x10018` between two symbols gives `None`.
        - `test_next_symbol_returns_the_nearest_later_symbol` — symbols at
          `0x10000` and `0x10034` and an address of `0x10000` give `0x10034`.
        - `test_next_symbol_returns_none_for_the_last_symbol` — the last symbol
          gives `None`.
        - `test_rebuilt_resolver_maps_an_internal_slot_to_the_symbol` — a slot
          with object 1 and target `0x0` gives the symbol at `0x10000`. A
          target `0x8` gives `None`. A missing object gives `None`.
        - `test_original_resolver_maps_a_slot_to_the_record_symbol` — a record
          at `0x10000` with a symbol and a target `0x10000` give the symbol. A
          target `0x10018` gives `None`.
        - `test_original_resolver_returns_none_for_a_record_without_a_symbol`
          — a record at `0x10010` without a symbol and a target `0x10010` give
          `None`.
        - `test_resolver_returns_none_for_an_imported_slot`.
        - `test_original_body_uses_the_end_over_the_count` — an end of
          `0x10100` and a count of `3` call `function_body` with the end and
          do not truncate.
        - `test_original_body_uses_the_count_without_an_end` — a count of `3`
          truncates the default body to three instructions.
        - `test_rebuilt_body_uses_the_extent_with_an_end` — an end calls
          `function_body` and not `decode`.
        - `test_rebuilt_body_uses_the_rebuilt_extent` — an extent of `0x10010`
          calls `function_body` with `end=0x10010`, so a tail jump to the next
          function stays out of the body.
        - `test_rebuilt_body_uses_the_count_without_an_end` — a count of `3`
          calls `decode` with three instructions.
        - `test_original_body_uses_the_default_extent_without_an_end_or_count`
          — a count of `0` uses the default extent.
        - `test_rebuilt_body_uses_the_default_extent_without_an_end_or_count`
          — a count of `0` uses the default extent.
        - `test_parse_arguments_accepts_the_original_end` — with `sys.argv`
          set to the tool name, `--original`, `--original-address`,
          `--rebuilt`, and `--original-end 0x10100`, the parsed value is
          `0x10100`.
    - **Modify `tools/rebuild/tests/test_store.py`**
        - `test_fingerprint_changes_with_the_record_size` — two records that
          differ only in `size` give different fingerprints.
    - **Modify `tools/rebuild/tests/test_callgraph.py`**
        - Add `from unittest import mock` to the imports.
        - `test_build_edges_uses_the_record_size_as_the_end` — mock `le.load`
          and `le.function_body`. A named record with `size` `0x20` and address
          `0x10000` gives a `function_body` call with
          `end=0x10020` and no count cap.
    - **Modify `tools/rebuild/callgraph.py`**
        - In `build_edges`, pass
          `end=_address(record["address"]) + record["size"]` when the record
          holds `size`, and keep `max_instructions` at its default. Remove the
          `record.get("count", 4000)` cap, because the extent bounds the body.
        - Update the `build_edges` docstring. State that the record size bounds
          the body.
    - **Modify `tools/rebuild/tests/test_run.py`**
        - `test_identity_command_carries_the_original_end` — a record with
          `address` `0x300ec` and `size` `5` gives a command that holds
          `--original-end` and `0x300f1`.
        - `test_identity_command_omits_the_original_end_without_a_size`.
        - `test_identity_command_carries_both_count_and_end` — a record with
          `count` `3` and `size` `5` gives a command that holds `--count 3`
          and `--original-end 0x300f1`.
    - **Modify `tools/rebuild/oracle.py`**
        - Change `slot_key` to the new signature. For an internal slot with a
          resolver that returns a symbol, return
          `(slot.kind, symbol, slot.addend or 0)`. Keep the current keys for
          `import-ordinal`, `import-name`, and `entry`. Return `(slot.kind,)`
          otherwise.
        - Change `_instruction_difference` to accept `original_symbols` and
          `rebuilt_symbols`. For each slot pair, resolve both sides. When both
          symbols are present, compare
          `slot_key(left, original_symbols)` with
          `slot_key(right, rebuilt_symbols)`. Otherwise compare
          `slot_key(left)` with `slot_key(right)`. The one-sided fallback is a
          documented limitation: the two images have no common symbol table
          for a target that resolves on only one side, so the slot compares by
          kind.
        - Change `compare` to accept the two keyword-only resolvers and pass
          them through.
        - Update the `slot_key` and `compare` docstrings. State that an
          internal target compares by symbol and addend when both sides
          resolve a symbol, by kind otherwise, and that a one-sided symbol is
          a documented limitation.
    - **Modify `tools/rebuild/diff_module.py`**
        - Add `from collections.abc import Callable` and `import store`.
        - Add `map_symbols(map_path, exe)`. Reuse `MAP_SYMBOL`. Convert the
          segment with `exe.object_by_index(segment)` and the offset to a
          linear address with `object.reloc_base + offset`. Skip a line whose
          segment is not an object. Return the pairs sorted by address.
        - Add `symbol_at(symbols, address)`. Return the symbol of the entry
          whose address equals the target. Among equal addresses the first map
          entry wins. Return `None` when no entry equals the address.
        - Add `next_symbol(symbols, address)`. Return the smallest symbol
          address strictly above the address. Return `None` when no symbol is
          above the address.
        - Add `_internal_target(exe, slot)`. Return
          `object.reloc_base + slot.target` for an internal slot. Return
          `None` for another kind, a missing object, or a missing target.
        - Add `rebuilt_symbol_resolver(exe, symbols)`. Return a closure that
          maps an internal slot to `symbol_at`.
        - Add `original_symbol_resolver(exe, records)`. Build the table from
          every record with `int(record["address"], 16)` and
          `record.get("symbol")`. The table can hold a `None` symbol. Return a
          closure that maps an internal slot to `symbol_at`.
        - Add `load_records() -> list[dict]` with
          `[store.load_record(address) for address in store.candidate_addresses()]`.
        - Add `--original-end` to `parse_arguments`.
        - Add `original_body(exe, address, end, count)` and
          `rebuilt_body(exe, address, end, count, extent=None)`. The original
          helper uses `function_body(exe, address, end=end)` when `end` is not
          `None`, else `function_body(exe, address)[:count]` when `count` is
          greater than zero, else `function_body(exe, address)`. The rebuilt
          helper uses `function_body(exe, address, end=extent)` when `extent`
          is not `None`, else `function_body(exe, address)` when `end` is not
          `None`, else `decode(exe, address, count)` when `count` is greater
          than zero, else `function_body(exe, address)`.
        - In `main`, replace the body selection with the two helpers. Use the
          local names `original_instructions` and `rebuilt_instructions`, so
          the helpers are not shadowed. The end wins over the count, so a
          record with both compares the full extent. Build the original
          resolver with `original_symbol_resolver(original, load_records())`.
          Build the rebuilt symbol table with
          `map_symbols(Path(arguments.rebuilt_map), rebuilt)` only when
          `--rebuilt-map` is given, and build the rebuilt resolver with
          `rebuilt_symbol_resolver(rebuilt, rebuilt_symbols)`. Pass the rebuilt
          extent `next_symbol(rebuilt_symbols, rebuilt_address)` to
          `rebuilt_body`, so a tail jump into the next function stays out of
          the body. Pass both resolvers to `oracle.compare`. The original
          symbol table loads every record for each diff call. This is a known
          cost of the re-baseline.
        - Update the module docstring and the usage block. Name
          `--original-end`, the intra-function branch targets, and the
          internal symbol identity.
    - **Modify `tools/rebuild/run.py`**
        - In `run_identity`, append `--original-end` and the value
          `f"0x{int(record['address'], 16) + record['size']:x}"` when the
          record holds `size`. Keep `--count` unchanged.
        - Update the `run_identity` docstring. State that the record `size`
          bounds the original body.
    - **Modify `tools/rebuild/store.py`**
        - Add `size` to the fields of `_record_fields`.
        - Update the `_record_fields` docstring. State that the record `size`
          bounds the identity check.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_oracle.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_diff_module.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_store.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_run.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_callgraph.py"`
        - `python3 tools/rebuild/harness.py build --module printf`
        - `python3 tools/rebuild/diff_module.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x3009b --original-end 0x300be --rebuilt work/rebuild/prtfmod.exe --rebuilt-map work/rebuild/printf.map --rebuilt-symbol printf_`
          — the command prints a verdict and the relocation compare. The exit
          code is 0 or 1, because the verdict depends on the build.

- [x] **Task 5: Re-baseline, strict count, and documentation**
    - **Implements:** FR-018
    - **Files:** `tools/rebuild/functions/*.json`, `tools/rebuild/modules/*.json`,
      `tools/rebuild/callgraph.json`, `tools/rebuild/contracts/record.schema.json`,
      `docs/reference/program-map.md`,
      `docs/reference/tooling.md`, `docs/explanation/findings.md`,
      `docs/explanation/rebuild-quality-roadmap.md`, `AGENTS.md`, and
      `README.md`. `run.py` writes the record and module results, and
      `callgraph.py` writes the call graph.
    - **Tests:** none. This is a data migration and a documentation change.
      The Verify commands prove it.
    - **Steps:**
        - Confirm that Docker and DOSBox-X are on `PATH`. These tools and the
          Ghidra loader are inherited from the baseline slice.
        - Confirm that the Ghidra loader is installed for the full check.
          The command is
          `nix develop .#cross-check --command sh tools/ghidra/install_loader.sh --check`.
        - Build the driver first. The driver module declares behavior only, so
          `run.py run --all` does not build it. Run
          `python3 tools/rebuild/harness.py build --module driver`.
        - Run `python3 tools/rebuild/run.py run --all --allow-shape --no-write`.
          Read the summary line and record the default count. The command
          writes no record result; it writes the run log under
          `work/rebuild/logs/`. It rebuilds the modules, so run it before the
          strict run. The exit code may be 1; do not treat it as a failure.
        - Run `python3 tools/rebuild/run.py run --all`. The command prints the
          known identity failures and exits 1. That is the expected result.
          Do not treat the exit code as a failure of this task. Every written
          result holds the new verdict, the new `match`, and the new
          fingerprint. This run writes the fingerprints from the final
          artifacts. The linker map holds a creation time and the fingerprint
          covers the map, so no rebuild may follow this run.
        - Run `python3 tools/rebuild/store.py check`.
        - Run `python3 tools/rebuild/callgraph.py build`. The reachable extent
          can add a call edge from a second exit path. Confirm the change with
          `git diff tools/rebuild/callgraph.json`.
        - Run `python3 tools/rebuild/callgraph.py check`.
        - Run `python3 tools/rebuild/docs.py render`, then
          `python3 tools/rebuild/docs.py check`. The reachable extent can change
          a program-map instruction count. The render updates the tables, and
          the check proves no drift remains.
        - Run `python3 tools/rebuild/view.py html --out work/rebuild/status.html`.
        - Spot check `tools/rebuild/functions/0x8adac.json`. Confirm that the
          identity `instructions_total` is above the old count of 71 and that
          the run log names the second exit path at `0x8af05`.
        - Spot check `tools/rebuild/functions/0x24da0.json`. Confirm that the
          run command in `result.command` carries `--original-end 0x25020`,
          because the end wins over the count.
        - Spot check `tools/rebuild/functions/0x300ec.json`. Confirm that the
          result holds `mode`, `match`, and the new fingerprint.
    - **Modify `docs/reference/tooling.md`**
        - The `le.py` section: state that `read` stitches the pages, a zeroed
          page gives zeros, a page without a file offset stops the read, and
          the start must be mapped.
        - The `le.py` body text: replace the first-`ret` sweep with the
          reachable extent. State the bound, the terminator stop, the
          unconditional jump stop, the call exclusion, and the leading jump.
        - The `diff_module.py` command block: add `--original-end ADDR`. State
          that the record `size` supplies the bound.
        - The oracle text: state that an intra-function branch target compares
          by instruction position, an outside target stays masked, and an
          internal slot compares by symbol and addend when both sides resolve
          a symbol.
        - The fingerprint text: add `size` to the record fields.
    - **Modify `tools/rebuild/contracts/record.schema.json`**
        - Change the `size` description. State that it is the distance to the
          next entry and that it bounds the identity check.
    - **Modify `docs/explanation/findings.md`**
        - Insert a new section `## The reachable extent and the symbol-aware
          oracle` immediately before `## Open questions`.
        - Record the extent evidence. Command:
          `python3 tools/le.py calls ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x8adac --max 400`.
          The output holds the second exit at `0x8af05`.
        - Record the branch and symbol evidence. Command:
          `python3 -m unittest discover -s tools/rebuild/tests -p "test_oracle.py"`.
        - Record the page evidence. Command:
          `python3 -m unittest discover -s tools/rebuild/tests -p "test_le_helpers.py"`.
        - Record the strict count and the default count from the two `run.py`
          summary lines.
        - Record the four check counts from the pipeline commands in Verify.
    - **Modify `AGENTS.md`**
        - Testing: replace the `oracle.py` bullet. State that an
          intra-function branch target compares by instruction position, an
          outside target stays masked, and an internal target compares by
          symbol and addend when both sides resolve a symbol.
        - Testing: add a bullet that `le.read` stitches pages and that
          `function_body` follows every direct jump and loop target inside the
          bound.
        - Testing: add `size` to the record fields in the fingerprint bullet.
        - Keep the four check counts at 303, 299, 288, and 284.
    - **Modify `README.md`**
        - The `Checks` paragraph: state that the oracle compares an
          intra-function branch target by position, masks an outside target,
          and compares an internal target by symbol. Keep the four counts.
    - **Modify `docs/explanation/rebuild-quality-roadmap.md`**
        - Set the Slice 2 `Status` to `Implemented` and the `Plan` to
          `docs/explanation/rebuild-quality-plan-oracle.md`. Do this after the
          final review approves the slice.
    - **Verify:**
        - `python3 -m py_compile tools/le.py tools/inventory.py tools/extract_cd_image.py tools/dosboxx_control.py tools/entry_probe.py tools/gfx_container.py tools/export_assets.py tools/sound_bank.py tools/member_probe.py tools/cross_check.py tools/rebuild/diff_module.py tools/rebuild/oracle.py tools/rebuild/store.py tools/rebuild/schema.py tools/rebuild/image_compare.py tools/rebuild/harness.py tools/rebuild/harness_dos.py tools/rebuild/watcom10_setup.py tools/rebuild/run.py tools/rebuild/view.py tools/rebuild/docs.py tools/rebuild/callgraph.py tools/pipeline_check.py`
        - `python3 -m unittest discover -s tools/rebuild/tests`
        - `python3 tools/rebuild/store.py check`
        - `python3 tools/rebuild/callgraph.py check`
        - `python3 tools/rebuild/docs.py check`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-cross`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime`
        - `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross`
        - The full check reports 303 checks, `--skip-cross` reports 299,
          `--skip-runtime` reports 288, and both flags report 284.

## Deviations

- Task 2 follows the compiler jump table. The approved manifest followed only
  direct jumps. The reviewer found that `race_frame` stopped at 41 of 48
  instructions. The user approved the extension. `function_body` now reads the
  `cmp` bound and the object-relative table entries, and the design holds the
  new decision.
- The rebuilt body is bounded by the next linker-map symbol. The manifest used
  `function_body(exe, address)` when `--original-end` was present. That followed
  a tail jump into the next function and demoted `free` and `exit_flush`
  falsely. `diff_module.next_symbol` and the `extent` argument to
  `rebuilt_body` fix this.
- Task 5 seeds the 36 candidate addresses that the new extents add. The
  manifest did not list the seed. The pipeline stage 12 seed check fails
  without it. The seed writes `unknown` records and rebuilds no module.
- The strict count is 23, not the baseline 24. `dpmi_real_int` at `0x25db0`
  loses its 17-instruction `exact-relocations` match when the full extent adds
  81 instructions. That demotion is correct.
- The default count is derived from the stored `match` values, not from a
  second `--allow-shape` run. A second run would rebuild the modules and
  invalidate the strict fingerprints. The strict 23 plus the seven shape
  matches gives 30.
- The `0x24da0` and `0x8adac` spot checks use the record evidence, the
  program map, and the `le.py calls` or `diff_module.py` output. The record
  `result.command` holds the `run.py` command, and the run log holds the
  verdict, so neither holds the diff command or the second exit address.
- Deferred minor findings:
  - The `diff_module.py` usage example still shows `--original-end 0x300f1`
    instead of `0x300f8`.
  - `load_records` has no docstring.
  - The `main` resolver construction sits outside the `try` that guards the
    body selection.
  - `le._next_function_start` raises only for a `None` read, not a short read.
  - The `read` docstring says a zeroed page fills the rest of its virtual
    range; the loop also stops at the requested length.
  - The rebuilt-extent test covers the helper but not the `main` wiring.
  - No test decodes a multi-byte instruction across an LE page boundary; the
    `read` tests cover the stitch.
  The first five touch a fingerprint input and wait for the next re-baseline.
  The last two are test-only and can be added without a re-baseline.
