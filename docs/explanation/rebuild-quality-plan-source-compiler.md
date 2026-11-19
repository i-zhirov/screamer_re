# Source And Compiler Implementation Plan

> Execute this plan with the `executing-plans` skill. Each task is a
> checkbox. Mark it done when its changes are implemented and its Verify
> command passes.

- **Date**: 2026-11-17
- **Status**: Implemented
- **Design**: docs/explanation/rebuild-quality-design.md
- **Roadmap**: docs/explanation/rebuild-quality-roadmap.md — follow-up after
  Slice 5 (toolchain) and Slice 7 (whole image); no slice
- **Depends on**: toolchain (Slice 5), whole image (Slice 7)
- **Manual prerequisites**: The game directory and the original image at
  `~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`; Docker for the
  V2 harness; DOSBox-X and the fetched period Watcom 10.0a toolchain for the
  period harness; the installed Ghidra loader for the full check; capstone for
  the call graph check and the disassembly. The matrix repair needs both
  harnesses and the period toolchain. The full-check Verify needs the image
  written by `whole_image.py` at `work/rebuild/STARTH.EXE`.
- **Goal**: Make the first-difference report measure the first true
  instruction-sequence difference, classify the 193 shorter bodies, reuse the
  strict padded bodies without changing the image bytes, copy the original
  fixup slot bytes exactly, repair the 96-recipe matrix and its committed
  manifest, measure and improve a source/compiler batch under the module
  outcome rule, and verify the whole image, the store, the documentation, and
  the full check.

## Constraints

- C-1: Python 3.14.7. Use the standard library and the `ctypes` capstone
  binding only.
- C-2: Tests use `unittest` under `tools/rebuild/tests/`.
- C-3: A tool takes its configuration from the command line. It reads no
  environment variable.
- C-4: A tool writes only under `work/` or to an explicit output path. The
  provenance evidence pass `triage.apply` and the library pass `library.apply`
  write the function records through `store.save_record`. The toolchain matrix
  writes the module records through `store.save_module`. The whole-image build
  writes the rebuilt image and its report under `work/`. C-4 still applies to
  the shipped artifacts.
- C-5: Prefer C. Use assembly only when C cannot reproduce the original bytes,
  and record the reason.
- C-6: A harness output name stays within the 8.3 limit.
- C-7: The 10.0a toolchain runs alongside OpenWatcom V2. Do not replace V2.
- C-8: The status ladder stays `unknown`, `not_a_function`, `named`,
  `rebuilt`, `matching`, `verified`.
- C-9: The identity verdict strings stay unchanged.
- C-10: The four full-check counts are 529, 525, 514, and 510.  The plan-time
  counts were 433, 429, 418, and 414.  The merged tree adds 96 function
  records, and stage 11 holds one check per record.  Update them when a change
  adds or removes a check.
- C-11: Do not commit. The user commits.
- The C-first rule stays. A body stays `rebuilt` when C cannot reproduce it.
  Do not convert a module to assembly to raise the count.
- The strict gate stays the default identity gate. `--allow-shape` does not
  relax the source/compiler acceptance.
- The original library bytes stay in the image. A library-backed record keeps
  its skip outcome, and the first-difference report excludes it.
- Add no third-party dependency.
- The rebuilt image, the reports, the matrix, and the module backup stay under
  `work/`. Do not commit them.
- The matrix rank stays the bytes-first order: the match value, then fewer
  differing bytes, then fewer differing instructions, then the earlier
  candidate. Do not change `toolchain.rank`.
- The source batch keeps the C-first rule and the `AGENTS.md` module outcome
  rule: keep a strict form when the behavior check passes. Otherwise keep the
  non-strict form with the higher `instructions_equal`, then the higher
  `bytes_equal`, and record the trait, the measured first difference, and the
  reason.
- The matrix repair is unconditional and independent of the source batch. The
  96 merged recipes enter the matrix, `toolchain.py migrate` writes the default
  `v2`, and the committed manifest holds one row per module with an identity
  record (347 observed). Write the manifest from the final recipes.
- Plan-local ruling: the record-span fit stays for the `write` mode. A padded
  strict body uses a new `reuse` mode. The original body bytes stand, so the
  patch changes no image byte. A `reuse` row holds `fit` false, because the
  rebuilt body is shorter than the record span. The `mode` and `outcome` fields
  carry the acceptance.
- Plan-local ruling: the relocation copies the original four slot bytes. It
  does not reconstruct an object base plus target.
- Plan-local ruling: the first-difference report uses the first true
  instruction-sequence difference. `oracle.compare` verdicts and metrics stay
  unchanged.
- Plan-local ruling: the 12 padding reuse rows are a forecast until the Task 3
  dry-run check measures them. The Task 5 census records the post-repair
  measured count.
- Plan-local ruling: the 20 larger bodies and the 2 shape-misaligned bodies are
  measured follow-ups. The plan does not fix them.

## Spec Coverage

| Requirement | Task |
| --- | --- |
| FR-014 | Task 3 keeps the library skip before the fit, and Task 7 verifies the library manifest and the original library bytes |
| FR-015 | Task 4 repairs the 96-recipe matrix and writes the 347-row manifest; Task 6 keeps the bytes-first rank and records the source-form outcomes |
| FR-016 | Tasks 1 and 2 extend the first-difference report, Task 5 records the census, Task 6 records the source-form outcomes |
| FR-017 | Task 3 builds the image with the exact relocation and the reuse mode, Task 7 builds the full image and records the score |
| FR-018 | Task 4 records the matrix repair, Task 6 records the batch outcomes, Task 7 updates the counts and the documentation and runs the full check |
| SM-5 | Task 3 builds the focused image, Task 7 builds the full image and records the measured byte score |
| The source-form rule | Task 1 adds the idiom names to both the classifier and the catalogue, Task 5 records the measured forms |

The source-form rule is the `AGENTS.md` rule that the catalogue and the
classifier use the same idiom names. FR-016 is the design requirement for the
catalogue and the report. The plan does not change the library decision
(FR-014); it keeps the boundary.

## Facts Checked

The code and the shipped artifacts agree with these facts. The measured values
come from the merged tree at the plan date. The whole-image baseline values are
the plan-time values before the matrix repair.

- The plan-time whole-image build gives 13 patched, 238 skipped, and 16
  relocated rows. `image_compare.py` reports 1034738 of 1034923 bytes equal.
  The report holds 251 module items.
- The 238 skipped rows split into 11 library-backed rows, 2 shape-misaligned
  rows, and 225 size mismatches. The 225 size mismatches split into 193 rebuilt
  bodies smaller than the original body, 20 larger, and 12 where the record
  span is larger than the original body. The 12 padding rows are strict: 7
  exact-relocations and 5 exact-bytes.
- The 12 padding rows are `video_block_swap_b`, `error_report`,
  `sound_driver_set`, `hud_update`, `video_fill`, `music_bank_reset`, `free`,
  `stream_status`, `fp_helper`, `memcpy`, `memset`, and `main_invoker`.
- `memset` at 0x8c160 holds a record span of 220829 and an original body of 25
  bytes. An original slot at 0x8c1c4 falls inside the span and outside the
  body. The current build filters the original slots by the record span, so a
  relaxed fit would pass an unassigned slot to the relocation.
- The 13 patched rows change 185 bytes. The exact slot copy changes 16 of them,
  one byte per relocated slot across the 16 slots, so the image score becomes
  1034754. Measured examples: `vesa_set_mode` holds 12 slots and all 12 differ
  by one byte; `default_data_copy` holds 2 slots and both differ.
- The prefix experiment patched the 13 write rows and the 12 padding rows, and
  the score fell to 1034576. The 12 padding rows changed 162 bytes. The oracle
  masks direct branch targets, so a strict body is not byte-identical in the
  image. `free` shows the cause: the original `jne 0xd2008` is masked, and the
  rebuilt body holds `jne 0x10034`.
- The store holds 981 function records and 348 module recipes. 347 modules have
  an identity record with a symbol. 96 of those have no identity result. One
  recipe has no record. The committed manifest holds 251 rows; the merged tree
  needs 347 rows.
- `python3 tools/rebuild/toolchain.py report` reports 348 modules: 30 strict
  (8 exact-bytes and 22 exact-relocations), 4 shape, 217 different, and 97
  no-result. The committed matrix document holds 252 rows: 191 winners under
  v2, 60 under watcom10, and 1 row with no winner. It predates the merged 96
  module recipes. `python3 tools/rebuild/toolchain.py check-matrix` fails for
  the 96 modules.
- The first true difference of the 193 smaller bodies gives 132 `byte 0`
  differences between two `push` instructions of one byte, 53 mnemonic
  differences, 4 `byte 1` differences between two `mov` instructions, 3
  `byte 2` differences between two `sub esp, imm` instructions, and 1 `byte 0`
  difference between two `mov ebp, esp` instructions.
- The registers-before-frame prologue with `-od` without `-of+` is the
  catalogue form for the 132 cases. The `-od` candidate is already in
  `toolchain.DEFAULT_FLAG_SETS`.
- The 3 frame-size rows are `race_lap_complete` at 0x19678, `loading_screen` at
  0x2a364, and `stream_read` at 0x4c4e5. The 4 `byte 1` rows are
  `net_player_add` at 0x2dddc, `resource_read` at 0x2efc2, `resource_find` at
  0x2f25f, and `lzw_bit_read` at 0x4d844. The `byte 0` `mov` row is
  `text_draw` at 0x25020.
- The task data says the first-difference report shows `match` for a
  length-only oracle verdict. The code does not. `oracle.compare` leaves
  `differences` empty when the instruction counts differ. `report_row` then
  sets the idiom to `other`, and the test
  `test_report_row_classifies_a_different_count_result_as_other` pins that
  behavior. The real gap is that the report holds no true sequence difference
  for a count mismatch.
- The task data says the matrix holds 252 modules and 1 no-result row. The
  committed matrix holds 252 rows, but the current store holds 348 recipes and
  the report counts 97 no-result rows. The plan uses the current tree.

## Design Updates

The plan names every design section whose decision changes.

- `docs/explanation/rebuild-quality-design.md` §Design Decisions, the
  whole-image build bullet: add the exact slot copy, the `reuse` mode for a
  strict padded body, the original-body-bytes rule, and the `reused` report
  count.
- `docs/explanation/rebuild-quality-design.md` §Observations: record the 348
  module recipes, the 981 records, the 252-row matrix before the follow-up,
  the 96 recipes the merged tree added, and the 347-row manifest after the
  follow-up.
- `docs/explanation/rebuild-quality-design.md` §Constraints C-10: keep the four
  counts unless the observed counts differ.
- `AGENTS.md` §Architecture, the whole-image bullet, and §Testing, the
  whole-image rule: add the exact slot copy, the `reuse` mode, the `reused`
  count, and the body-end slot bound.
- `AGENTS.md` §Testing, the matrix rule: keep the bytes-first rank. Add the 96
  migrated recipes and the 347-row manifest.
- `AGENTS.md` §Project Structure: change the two hundred and fifty-two module
  recipes to three hundred and forty-eight.
- `docs/reference/tooling.md` "## The rebuild harness" table row
  `modules/*.json`: change the two hundred and fifty-two count to three
  hundred and forty-eight. Keep the row wording.
- `README.md` §Status: add the 348 module recipe count and the measured
  whole-image byte score. The text does not exist today.
- `docs/reference/tooling.md` "## The toolchain matrix": add the 96 migrated
  recipes and the 347-row manifest. Keep the bytes-first rank.
- `docs/reference/source-forms.md` §The difference signatures and the new
  sections: add `register save order`, `frame size`, and `instruction count`.
  The `AGENTS.md` source-form rule already requires both the catalogue and the
  classifier to hold every idiom name.

## Shared Interfaces

- `oracle.first_difference(original: list[le.Insn], rebuilt: list[le.Insn],
  *, original_relocations: list[le.Relocation] | None = None,
  rebuilt_relocations: list[le.Relocation] | None = None,
  original_symbols: Callable[[le.Relocation], str | None] | None = None,
  rebuilt_symbols: Callable[[le.Relocation], str | None] | None = None) ->
  tuple[int, str] | None` returns the index and the bare difference text of the
  first instruction pair whose comparison is not equal. When the common prefix
  is equal and the lengths differ, it returns `(min(len(original),
  len(rebuilt)), f"instruction count: {len(original)} against
  {len(rebuilt)}")`. When both bodies are equal, it returns `None`. It reuses
  `_mask_for`, `_bounds`, `_index_by_address`, `_slots_for`, and
  `_instruction_difference`. It does not change `compare`.
- `first_difference.IDIOMS` becomes the tuple in this order: `local slot
  order`, `constant operand`, `instruction encoding`, `instruction
  selection`, `basic-block order`, `relocation target`, `register save order`,
  `frame size`, `instruction count`, `match`, `other`.
- `first_difference.idiom_of(difference: str, original: le.Insn | None,
  rebuilt: le.Insn | None) -> str` classifies the new texts. The rules run in
  this order: the frame-size rule first, then the local-slot rule. A `byte 2:`
  difference classifies as `frame size` only when both mnemonics are `sub` and
  both operands start `esp, 0x`. A `byte 2:` difference classifies as `local
  slot order` only when both operands hold a stack slot, neither mnemonic is
  `sub`, and it is not a frame size. A `byte 0:` difference classifies as
  `register save order` only when both mnemonics are `push` and both operands
  are a 32-bit register. A text that starts `instruction count:` classifies as
  `instruction count`. `_stack_slot` returns false for a `None` instruction.
- `first_difference.report_row(record: dict, module: dict,
  result: oracle.OracleResult, original: list[le.Insn],
  rebuilt: list[le.Insn], sequence: tuple[int, str] | None = None) -> dict`
  returns the row. When `sequence` is given, the row uses its index and text.
  When `sequence` is `None`, the row keeps the current `first_pair` and
  `result.differences` path, so the existing callers and tests stay valid. A
  pair is present only when both indexes are below the body lengths. The row
  holds `module`, `address`, `symbol`, `toolchain`, `sources`, `build`, `match`,
  `instructions_equal`, `instructions_total`, `bytes_equal`, `bytes_total`,
  `first_difference`, `sequence_index`, `idiom`, `original`, `rebuilt`,
  `record_size`, `original_size`, `rebuilt_size`, and `family`.
- `first_difference.body_family(record: dict, original: list[le.Insn],
  rebuilt: list[le.Insn]) -> str` uses `record.get("size")`. It returns
  `unmeasured` when the record has no `size`. Otherwise it returns `aligned`
  when the rebuilt byte sum equals `record["size"]`, `smaller` when it is below
  the original byte sum, `larger` when it is above the original byte sum, and
  `padding` when it equals the original byte sum and that sum is below
  `record["size"]`.
- `first_difference.summarize(rows: list[dict]) -> dict` returns
  `{"modules": len(rows), "families": {...}, "idioms": {...},
  "first_index_zero": n, "count_differences": n}`. The family and idiom keys
  are sorted. A failure row counts in the `unmeasured` family.
- `whole_image.aligned(original: list[le.Insn], rebuilt: list[le.Insn],
  original_relocations: list[le.Relocation],
  rebuilt_relocations: list[le.Relocation]) -> bool` returns true when the
  instruction count, every instruction size, every slot count, and every slot
  relative offset agree, every slot is internal with width 4, every original
  slot holds a target, every slot lies inside its instruction, and every input
  slot is assigned. It is the precondition of `relocate`.
- `whole_image.relocate(original: list[le.Insn], rebuilt: list[le.Insn],
  original_relocations: list[le.Relocation],
  rebuilt_relocations: list[le.Relocation]) -> tuple[bytes | None, int]`
  returns `(None, 0)` unless `aligned` is true. Otherwise it copies the
  original four slot bytes `original_insn.raw[relative:relative + 4]` into the
  rebuilt slot and returns the patched body and the count of copied slots.
- `whole_image.reuse(record: dict, original: list[le.Insn],
  rebuilt: list[le.Insn]) -> bool` returns true when the rebuilt byte sum
  equals the original byte sum, that sum is below `record["size"]`, the record
  carries no `library` field, and the record identity `match` is in
  `run.STRICT_MATCHES`.
- `whole_image.patch_mode(record: dict, original: list[le.Insn],
  rebuilt: list[le.Insn], original_relocations: list[le.Relocation],
  rebuilt_relocations: list[le.Relocation]) -> str | None` returns `write`
  when `fit(record, rebuilt)` returns `None`, `reuse` when `reuse(...)` and
  `aligned(...)` are true, and `None` otherwise.
- `whole_image.report_row(item: dict) -> dict` adds `original_size` 0 and
  `mode` `None` to the base row.
- The whole-image report holds `patched`, `reused`, `skipped`, `relocated`, and
  `score`. A row holds `mode` (`write`, `reuse`, or `None`) and `outcome`
  (`patched`, `reused`, or `skipped`). A `reuse` row holds `fit` false, no
  `fit_reason`, `relocated` 0, and no `reason`. The `mode` `reuse` and the
  `outcome` `reused` carry the acceptance.
- `toolchain.rank(row: dict) -> tuple[int, int, int, int]` returns the match
  rank, the differing byte count, the differing instruction count, and the
  candidate order. `_difference` keeps its missing-count rule.

## Tasks

- [x] **Task 1: The true sequence difference and the idiom classifier**
    - **Implements:** FR-016 and the source-form rule
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_oracle.py`**
        - Add `FirstDifferenceTestCase`.
        - `test_first_difference_reports_the_first_indexed_pair` — a
          `push ebx` at 0x1000 against a `push edx` at 0x2000 gives
          `(0, "byte 0: 0x53 against 0x52")`.
        - `test_first_difference_reports_a_later_pair` — two equal prefix
          instructions and a `mnemonic:` difference at index 2 give index 2.
        - `test_first_difference_reports_a_count_mismatch_after_an_equal_prefix`
          — 3 original instructions and 2 rebuilt instructions with 2 equal
          prefix pairs give `(2, "instruction count: 3 against 2")`.
        - `test_first_difference_is_none_for_equal_bodies` — identical bodies
          give `None`.
        - `test_first_difference_masks_a_relocation` — a body whose only
          difference is a slot byte gives `None`.
        - `test_first_difference_masks_an_intra_function_branch` — a moved
          intra-function branch target gives a `branch target:` text.
        - `test_compare_keeps_the_length_only_verdict` — `compare` on the count
          mismatch still gives `different` and an empty `differences` list.
    - **Modify `tools/rebuild/tests/test_first_difference.py`**
        - Rename `test_idioms_hold_the_eight_names` to
          `test_idioms_hold_the_eleven_names` and assert the tuple in the
          Shared Interfaces order.
        - `test_idiom_of_classifies_a_register_save_order` — a `byte 0:`
          text with two `push` instructions gives `register save order`.
        - `test_idiom_of_requires_a_push_pair_for_a_register_save_order` — a
          `byte 0:` text with two `mov` instructions gives `other`.
        - `test_idiom_of_classifies_a_frame_size` — a `byte 2:` text with two
          `sub esp, imm` instructions gives `frame size`.
        - `test_idiom_of_requires_a_sub_esp_pair_for_a_frame_size` — a
          `byte 2:` text with two `sub dword ptr [ebp - 4], imm` instructions
          gives `other`, because the local-slot rule excludes `sub`.
        - `test_idiom_of_classifies_an_instruction_count` — an
          `instruction count: 3 against 2` text gives `instruction count`.
        - `test_report_row_holds_the_sequence_pair` — a `sequence` of
          `(0, "byte 2: 0xf8 against 0xf4")` gives the formatted
          `first_difference`, `sequence_index` 0, the idiom, and the pair.
        - `test_report_row_holds_the_sequence_count_difference` — a
          `sequence` of `(2, "instruction count: 3 against 2")` gives the
          formatted text, `sequence_index` 2, `idiom` `instruction count`, and
          a null pair. This test replaces
          `test_report_row_classifies_a_different_count_result_as_other`.
        - Keep the four existing `ReportRowTestCase` tests unchanged:
          `test_report_row_holds_the_pair_and_the_counts`,
          `test_report_row_holds_null_fields_without_a_difference`,
          `test_report_row_classifies_a_difference_without_a_pair_as_other`,
          and `test_report_row_classifies_a_strict_result_with_no_difference_as_match`.
          They use the `sequence` fallback.
        - Add `test_report_skips_a_library_record` — a record with a
          `library` field gives no row.
        - Update the `row_for_module` helper with `sequence_index`.
        - Update the `diff_record` test with a mocked
          `oracle.first_difference`; assert the row holds its sequence.
    - **Modify `tools/rebuild/oracle.py`**
        - Add `first_difference` with the signature in Shared Interfaces.
        - The function computes the masks, bounds, and instruction indexes
          once. It walks `zip(original, rebuilt)` and returns the first
          `_instruction_difference` result with its index. Then it returns the
          count difference or `None`.
        - Do not change `compare`.
    - **Modify `tools/rebuild/first_difference.py`**
        - Insert the three names into `IDIOMS` in the Shared Interfaces order.
        - Extend `idiom_of` with the three rules and the Shared Interfaces
          precedence. `_stack_slot` returns false for a `None` instruction.
        - Extend `report_row` with the `sequence` argument, the fallback, and
          the `sequence_index` field. A `sequence` row formats
          `f"instruction {index}: {text}"`. A pair is present only when both
          indexes are below the body lengths.
        - `diff_record` passes the `oracle.first_difference` result.
        - `report` skips a record with a `library` field.
    - **Modify `docs/reference/source-forms.md`**
        - Change the registers-before-frame first-difference signature from
          `other` to `register save order`, with the `byte 0:` two-`push`
          rule.
        - Add the `## The frame size` section. It names the source form (the
          local declaration set), the compiler condition, the `sub esp, imm`
          effect, the 3 measured rows, the first-difference report evidence,
          and the `frame size` signature.
        - Add the `instruction count` signature to the difference-signatures
          table. Change the five sections that say a count difference is
          `other` to `instruction count`.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_oracle.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_first_difference.py"`
        - `python3 -m py_compile tools/rebuild/oracle.py tools/rebuild/first_difference.py`

- [x] **Task 2: The body family and the census summary**
    - **Implements:** FR-016
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_first_difference.py`**
        - Extend the `record` helper with a `size` field, for example 12.
        - `FamilyTestCase`.
        - `test_family_marks_a_smaller_body` — a rebuilt body below the
          original body and the record span gives `smaller`.
        - `test_family_marks_a_larger_body` — a rebuilt body above the original
          body gives `larger`.
        - `test_family_marks_a_padding_body` — a rebuilt body equal to the
          original body below the record span gives `padding`.
        - `test_family_marks_an_aligned_body` — a rebuilt body equal to the
          record span gives `aligned`.
        - `test_family_marks_a_body_at_the_span_as_aligned` — a rebuilt body
          equal to the record span with an original body below the span gives
          `aligned`.
        - `test_family_marks_a_missing_size_as_unmeasured` — a record with no
          `size` gives `unmeasured`.
        - `SummaryTestCase`.
        - `test_summary_counts_the_families_and_the_idioms` — three rows with
          two families and two idioms give the two count maps.
        - `test_summary_counts_the_first_index_zero_rows` — two rows with a
          sequence index 0 and one row with index 3 give 2.
        - `test_summary_counts_the_count_differences` — one `instruction
          count` row gives 1.
        - `test_summary_counts_an_unmeasured_family` — one failure row gives
          one `unmeasured` family row.
        - `test_report_document_holds_the_summary` — the `report` document
          holds the `summarize` result.
        - Update the `row_for_module` helper with `record_size`,
          `original_size`, `rebuilt_size`, and `family`.
        - Update the `CommandTestCase` mock report documents with a `summary`
          field.
    - **Modify `tools/rebuild/first_difference.py`**
        - Add `body_family` and `summarize` with the signatures in Shared
          Interfaces.
        - `report_row` sets `record_size` from `record.get("size")`,
          `original_size`, `rebuilt_size`, and `family`.
        - `_failure_row` sets `sequence_index` `None`, `record_size` from
          `record.get("size")`, `original_size` 0, `rebuilt_size` 0, and
          `family` `unmeasured`.
        - `report` computes `summary` after the rows are sorted.
        - `command_report` prints the family and idiom counts from the
          document `summary` after the row lines, before the `modules` line.
          The detail is `"<family>: <count>"` and `"<idiom>: <count>"`.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_first_difference.py"`
        - `python3 -m py_compile tools/rebuild/first_difference.py`

- [x] **Task 3: The exact relocation and the padding reuse**
    - **Implements:** FR-014, FR-017, SM-5
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_whole_image.py`**
        - `ReportRowTestCase` update: `test_report_row_starts_skipped` also
          asserts `mode` `None` and `original_size` 0.
        - `AlignmentTestCase`.
        - `test_aligned_accepts_equal_shapes` — equal counts, sizes, and slot
          offsets give true.
        - `test_aligned_rejects_a_different_instruction_count` — two against
          one gives false.
        - `test_aligned_rejects_a_different_slot_offset` — relative offsets 2
          and 3 give false.
        - `test_aligned_rejects_a_non_internal_slot` — an import slot gives
          false.
        - `RelocateTestCase` changes. Every call site and every test uses the
          four-argument signature. The omitted tests move from five arguments
          to four arguments: `test_relocate_returns_none_on_a_slot_count_mismatch`,
          `test_relocate_returns_none_on_a_non_internal_slot`,
          `test_relocate_returns_none_on_a_non_four_byte_slot`,
          `test_relocate_returns_none_on_an_unassigned_slot`, plus
          `test_relocate_returns_none_on_a_different_instruction_count`,
          `test_relocate_returns_none_on_a_size_mismatch`,
          `test_relocate_returns_none_on_a_different_slot_offset`, and
          `test_relocate_keeps_a_relative_branch`.
        - Replace `test_relocate_writes_the_original_target` with
          `test_relocate_copies_the_original_slot_bytes` — the original
          instruction raw bytes `\xaa\xbb\x20\x00\x00\x00` at relative offset
          2 and a rebuilt zero body give `\x00\x00\x20\x00\x00\x00` and a
          count of 1.
        - Update `test_relocate_counts_each_slot` to the four-argument
          signature.
        - Remove `test_relocate_returns_none_for_an_unmapped_object`. The
          exact copy does not read an object map.
        - `PatchModeTestCase`.
        - Extend the `record` factory with an optional `match` argument. A
          value writes `result.checks.identity.match`; the default writes no
          result. The current Build tests keep the default.
        - `test_patch_mode_writes_an_aligned_span` — a rebuilt body equal to
          the record size gives `write`.
        - `test_patch_mode_reuses_a_strict_padded_body` — a record size 12, an
          original body 9, a rebuilt body 9, an `exact-relocations` match, and
          equal slot offsets give `reuse`.
        - `test_patch_mode_rejects_a_non_strict_padded_body` — the same bodies
          with a `same-shape` match give `None`.
        - `test_patch_mode_rejects_a_padded_body_that_is_not_aligned` — a
          different slot offset gives `None`.
        - `test_patch_mode_rejects_a_body_larger_than_the_span` — a body above
          the record size gives `None`.
        - `BuildTestCase`.
        - Update `test_build_writes_the_patched_body` with `mode` `write` and
          `outcome` `patched`.
        - `test_build_reuses_a_padded_body` — a record size 8, an original
          body 4 at 0x10000 with bytes `\x01\x02\x03\x04`, a strict identity,
          and a rebuilt body of different bytes keep the original bytes in the
          image. The row holds `fit` false, `mode` `reuse`, `outcome`
          `reused`, and `relocated` 0.
        - `test_build_keeps_the_padding_tail` — the original bytes after the
          body stay.
        - `test_build_reuses_a_body_with_a_far_original_slot` — an original
          slot at 0x10050 inside a record span of 100 and outside the body
          gives a `reuse` row. The original slot filter uses the body end.
        - `test_build_rejects_a_reuse_for_a_non_strict_record` — a
          `same-shape` record skips with the fit reason.
        - `test_build_report_holds_the_reused_count` — the document holds
          `patched`, `reused`, `skipped`, and `relocated`.
        - Rename `test_build_patches_the_original_slot_target` to
          `test_build_copies_the_original_slot_bytes`. Set the fake original
          data to hold the slot bytes and expect the same bytes in the image.
        - Update `test_build_report_holds_the_module_rows` base set with `mode`
          and `original_size`.
        - Add a `reused_row` fixture for `error_report` with `mode` `reuse`,
          `outcome` `reused`, `fit` false, and `relocated` 0. Add it to the
          `CommandBuildTestCase` document and set `reused` 1.
        - Update `test_command_build_prints_the_summary` to assert
          `error_report: reused, the original body bytes stay` and the summary
          `1 patched, 1 reused, 1 skipped, 2 slots relocated, 10 of 16 bytes
          equal`.
        - Add `mode` `write` and `original_size` to the `patched_row` fixture,
          and `mode` `None` and `original_size` 0 to the `skipped_row` fixture.
    - **Modify `tools/rebuild/whole_image.py`**
        - Add `aligned`, `reuse`, and `patch_mode` with the signatures in
          Shared Interfaces.
        - Change `relocate` to the four-argument signature and the exact copy.
          Use `aligned` as the precondition. Remove the `original_objects`
          argument and the `original_objects` value in `build`.
        - `report_row` adds `original_size` 0 and `mode` `None`.
        - `build` computes `original_size` from the original instructions and
          the rebuilt size from the rebuilt instructions. It sets
          `row["original_size"]`.
        - `build` computes the original slots with the body end
          `address + original_size` and the rebuilt slots with the rebuilt
          body end.
        - `build` calls `patch_mode`. A `None` result keeps the current skip
          path: the `fit` reason when the size differs, else the alignment
          reason. A `write` result runs `relocate`. When `relocate` returns
          `(None, 0)`, keep the alignment skip path. Otherwise write the bytes
          and set `mode` `write` and `outcome` `patched`. A `reuse` result sets
          `fit` false, `mode` `reuse`, `outcome` `reused`, `relocated` 0,
          leaves `fit_reason` None, and writes no byte.
        - The document holds `reused`. The command prints
          `"<module>: reused, the original body bytes stay"` and the summary
          `"<patched> patched, <reused> reused, <skipped> skipped,
          <relocated> slots relocated, <equal> of <total> bytes equal"`.
    - **Steps:**
        - Run the `exit` build alone. The selected `exit` record `0x300be` is
          library-backed, and the build skips it before the fit:
          `python3 tools/rebuild/whole_image.py --module exit --out work/rebuild/STARTH-exit.EXE --report work/rebuild/whole-image-exit.json`.
          Confirm `exit: skipped, the original library bytes stay`.
        - Run the focused build with the two reuse rows and the two write rows:
          `python3 tools/rebuild/whole_image.py --module free --module error_report --module vesa_set_mode --module default_data_copy --out work/rebuild/STARTH-focused.EXE --report work/rebuild/whole-image-focused.json`.
        - Confirm `free` and `error_report` are `reused`, `vesa_set_mode` and
          `default_data_copy` are `patched`, and `patched + reused + skipped`
          is 4. If a reuse row is not reused, record the measured reason.
        - Confirm each reused row's image slice equals the original image
          slice at the same file offset.
        - Confirm the `vesa_set_mode` row relocates 12 slots and the
          `default_data_copy` row relocates 2 slots. Confirm the written slot
          bytes equal the original slot bytes.
        - Run the 12 padding rows through the focused build to measure the
          reuse forecast:
          `python3 tools/rebuild/whole_image.py --module video_block_swap_b --module error_report --module sound_driver_set --module hud_update --module video_fill --module music_bank_reset --module free --module stream_status --module fp_helper --module memcpy --module memset --module main_invoker --out work/rebuild/STARTH-padding.EXE --report work/rebuild/whole-image-padding.json`.
          Print the `mode` and `outcome` of each row and record the measured
          reuse count:
          `python3 -c "import json, pathlib; rows = json.loads(pathlib.Path('work/rebuild/whole-image-padding.json').read_text())['modules']; print([(row['module'], row['mode'], row['outcome']) for row in rows]); assert len(rows) == 12"`.
          Do not write 12 as measured until this command runs.
        - Run
          `python3 tools/rebuild/image_compare.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt work/rebuild/STARTH-focused.EXE ; test $? -le 1`.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_whole_image.py"`
        - `python3 -m py_compile tools/rebuild/whole_image.py`
        - `python3 tools/rebuild/whole_image.py --module exit --out work/rebuild/STARTH-exit.EXE --report work/rebuild/whole-image-exit.json`
        - `python3 tools/rebuild/whole_image.py --module free --module error_report --module vesa_set_mode --module default_data_copy --out work/rebuild/STARTH-focused.EXE --report work/rebuild/whole-image-focused.json`
        - `python3 tools/rebuild/whole_image.py --module video_block_swap_b --module error_report --module sound_driver_set --module hud_update --module video_fill --module music_bank_reset --module free --module stream_status --module fp_helper --module memcpy --module memset --module main_invoker --out work/rebuild/STARTH-padding.EXE --report work/rebuild/whole-image-padding.json`
        - `python3 -c "import json, pathlib; rows = json.loads(pathlib.Path('work/rebuild/whole-image-padding.json').read_text())['modules']; print([(row['module'], row['mode'], row['outcome']) for row in rows]); assert len(rows) == 12"`
        - `python3 tools/rebuild/image_compare.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt work/rebuild/STARTH-focused.EXE ; test $? -le 1`

- [x] **Task 4: The 96-recipe matrix and manifest repair**
    - **Implements:** FR-015, FR-018
    - This task is unconditional and independent of the source batch. It
      changes no tool code, so it adds no unit test.
    - **Steps:**
        - Back up the recipes:
          `rm -rf work/rebuild/modules-before-matrix && cp -R tools/rebuild/modules work/rebuild/modules-before-matrix`.
        - Write the default toolchain into the 96 merged recipes. Do not use
          `--check` for the write:
          `python3 tools/rebuild/toolchain.py migrate`.
          It prints `migrated 96 modules`. Confirm the write with
          `python3 tools/rebuild/toolchain.py migrate --check`, which prints
          `migrated 0 modules` and exits 0.
        - Measure the matrix with the committed manifest as the base and the
          resume base. This is the longest step. It builds up to six candidates
          per toolchain for each module that the committed manifest does not
          cover, whose recipe changed, or whose row is unmeasured:
          `python3 tools/rebuild/toolchain.py matrix --all --only-missing --manifest tools/rebuild/toolchain.json --resume tools/rebuild/toolchain.json --out work/rebuild/toolchain-matrix-repair.json`.
          The command measures the 96 merged recipes and any other row that
          the committed manifest does not cover and that has an identity
          record. The one recipe with no record gets an item with the reason
          `no identity record` and no row in the manifest.
        - If the pass stops, write a manifest from the partial report and
          resume from it. The matrix report is not a resume manifest, because
          its items are not flat rows:
          `python3 tools/rebuild/toolchain.py manifest --report work/rebuild/toolchain-matrix-repair.json --out work/rebuild/toolchain-matrix-repair-resume.json`,
          then run
          `python3 tools/rebuild/toolchain.py matrix --all --only-missing --manifest tools/rebuild/toolchain.json --resume work/rebuild/toolchain-matrix-repair-resume.json --out work/rebuild/toolchain-matrix-repair-2.json`.
          If the pass stops again, write the manifest from
          `work/rebuild/toolchain-matrix-repair-2.json` to
          `work/rebuild/toolchain-matrix-repair-2-resume.json` and run the
          matrix with that resume file and
          `--out work/rebuild/toolchain-matrix-repair-3.json`. The alternative
          is to measure all 348 recipes explicitly with
          `python3 tools/rebuild/toolchain.py matrix --all --out work/rebuild/toolchain-matrix-repair.json`.
        - Inspect the summary. It prints `<modules> modules: <measured>
          measured, <unmeasured> unmeasured, <strict> strict`.
        - After the pass completes, copy the completed report to a fixed name:
          `cp work/rebuild/toolchain-matrix-repair.json work/rebuild/toolchain-matrix-final.json`.
          When the pass used a resume, copy
          `work/rebuild/toolchain-matrix-repair-2.json` or
          `work/rebuild/toolchain-matrix-repair-3.json` to the same fixed name.
          Write the manifest from the final report, after the winners are
          applied:
          `python3 tools/rebuild/toolchain.py manifest --report work/rebuild/toolchain-matrix-final.json --out tools/rebuild/toolchain.json`.
          It prints `347 rows`. Do not back up or restore the committed
          manifest. The final recipes produce it.
        - Run `python3 tools/rebuild/toolchain.py check-matrix --manifest tools/rebuild/toolchain.json`.
        - Run `python3 tools/rebuild/toolchain.py report` and record the
          module, strict, exact-bytes, exact-relocations, shape, different,
          and no-result counts. The report reads the store records, not the
          manifest. The 96 records keep no identity result until the Task 7
          full check re-runs them. Do not expect the no-result count to change
          in this task.
        - If a later task changes a recipe, rerun the matrix for that module
          and regenerate the manifest from the final recipes. The source batch
          does not change a recipe.
        - Record the matrix repair in `docs/explanation/findings.md`: the 96
          recipes now have a `toolchain`, the matrix report holds 348 rows,
          the manifest holds 347 rows, and `check-matrix` passes. Task 7
          confirms stage 12 in the full check.
    - **Verify:**
        - `python3 tools/rebuild/toolchain.py migrate --check`
        - `python3 tools/rebuild/toolchain.py check-matrix --manifest tools/rebuild/toolchain.json`
        - `python3 tools/rebuild/toolchain.py report`
        - `python3 -c "import json, pathlib; document = json.loads(pathlib.Path('tools/rebuild/toolchain.json').read_text()); rows = document['modules']; print(f'{len(rows)} manifest rows'); assert len(rows) == 347; assert all(row.get('name') for row in rows)"`
        - `python3 -m py_compile tools/rebuild/toolchain.py`

- [x] **Task 5: The body census and the source-form outcomes**
    - **Implements:** FR-016, FR-018 and the source-form rule
    - This is a data task. It changes no tool code.
    - **Steps:**
        - Build the image after the exact relocation, the reuse mode, and the
          matrix repair:
          `python3 tools/rebuild/whole_image.py --all`.
        - Record the row counts and the score from
          `work/rebuild/whole-image.json`. The plan-time baseline before the
          matrix repair is 13 patched, 238 skipped, 16 relocated, and 1034738
          of 1034923 bytes equal. The matrix repair changes 96 recipes, so the
          post-repair values can differ. Do not assert the plan-time values.
          Record the observed values.
        - Derive the family counts from the image rows, not from the
          first-difference report. A row with `build` `pass` and a
          `rebuilt_size` different from `size` is a size mismatch. It is
          `smaller` when `rebuilt_size` is below `original_size`, `larger`
          when above, and `padding` when equal to `original_size` and below
          `size`. The `aligned` rows are the write rows. The library rows and
          the shape-misaligned rows are not in this census. The plan-time
          census is 193 `smaller`, 20 `larger`, and 12 `padding`; record the
          post-repair values.
        - Write the census report:
          `python3 tools/rebuild/first_difference.py report --all --out work/rebuild/first-differences.json`.
          The command exits 1 when a selected row fails to build. Accept exit
          1 for this data task and record the failed rows.
        - Read the summary. The report selects the non-strict bodies only, so
          the plan-time 12 strict padding rows are absent. The plan-time idiom
          counts of the 193 smaller bodies are 132 `register save order`, 53
          `instruction selection`, 4 `other` `byte 1` rows, 3 `frame size`,
          and 1 `other` `byte 0` row. The 20 larger bodies hold their own
          idiom counts in the same summary. Record the measured counts. Do
          not read a family count from the report as the image census.
        - Add the section `## The body census of the image rows` to
          `docs/reference/source-forms.md` before `## The measured outcomes`.
          It holds one row per family and idiom with the count, the
          first-difference index, the byte and instruction counts, and one
          example module. The examples are `game_main` for `register save
          order`, `object_load` for `instruction selection`,
          `race_lap_complete` for `frame size`, and `net_player_add` for the
          `byte 1` `other` row.
        - Add the section `## The 193 shorter bodies` to
          `docs/explanation/findings.md` before `## Open questions`. The title
          names the plan-time count. It records the post-repair census, the
          measured reuse count and the 12 padding rows, the exact relocation
          16-byte measurement, the 162-byte prefix change, the 20 larger rows,
          and the 2 shape-misaligned rows. It records the evidence commands.
        - Record the 20 larger rows and the 2 shape-misaligned rows as
          measured follow-ups. Do not change their bodies.
    - **Verify:**
        - `python3 tools/rebuild/whole_image.py --all`
        - `python3 tools/rebuild/first_difference.py report --all --out work/rebuild/first-differences.json ; test $? -le 1`
        - `python3 -c "import collections, json, pathlib; rows = json.loads(pathlib.Path('work/rebuild/whole-image.json').read_text())['modules']; sizes = [row for row in rows if row['build'] == 'pass' and row['rebuilt_size'] != row['size']]; families = collections.Counter('smaller' if row['rebuilt_size'] < row['original_size'] else 'larger' if row['rebuilt_size'] > row['original_size'] else 'padding' for row in sizes); print(len(rows), dict(sorted(families.items()))); assert len(rows) == 347; assert sum(families.values()) == len(sizes); assert all(value >= 0 for value in families.values())"`
        - `python3 -c "import json, pathlib; document = json.loads(pathlib.Path('work/rebuild/first-differences.json').read_text()); summary = document['summary']; print(summary); assert summary['modules'] == len(document['modules']); assert sum(summary['families'].values()) == summary['modules']; assert all(value >= 0 for value in summary['idioms'].values())"`
        - `grep -q '## The body census of the image rows' docs/reference/source-forms.md`
        - `grep -q '## The 193 shorter bodies' docs/explanation/findings.md`

- [x] **Task 6: The source-form batch under the bytes-first rank**
    - **Implements:** FR-015, FR-016, FR-018 and the source-form rule
    - This task changes source files and no tool code. It does not change
      `toolchain.rank` and does not change a module recipe.
    - **Steps:**
        - Back up the three source files:
          `mkdir -p work/rebuild/source-before && cp tools/rebuild/race_lap_complete.c tools/rebuild/loading_screen.c tools/rebuild/stream_read.c work/rebuild/source-before/`.
        - Record the before measurement:
          `python3 tools/rebuild/first_difference.py report --module race_lap_complete --module loading_screen --module stream_read --module net_player_add --module resource_read --module resource_find --module lzw_bit_read --module text_draw --out work/rebuild/first-batch-before.json`.
        - Change the source of `race_lap_complete.c`, `loading_screen.c`, and
          `stream_read.c` to the local declaration set that matches the
          original frame size. Keep the C form.
        - Re-run the batch report:
          `python3 tools/rebuild/first_difference.py report --module race_lap_complete --module loading_screen --module stream_read --module net_player_add --module resource_read --module resource_find --module lzw_bit_read --module text_draw --out work/rebuild/first-batch-after.json`.
          The command exits 1 when a selected row fails to build. Accept exit
          1 for this data task and record the failed rows.
        - Run the behavior check for each changed module:
          `python3 tools/rebuild/run.py run --recheck-record 0x19678`,
          `python3 tools/rebuild/run.py run --recheck-record 0x4c4e5`, and
          `python3 tools/rebuild/run.py run --recheck-record 0x2a364`.
        - Apply the `AGENTS.md` module outcome rule for each changed module.
          Keep a strict form when the behavior check passes. Otherwise keep
          the non-strict form with the higher `instructions_equal`, then the
          higher `bytes_equal`, and record the trait, the measured first
          difference, and the reason. Restore a rejected file from
          `work/rebuild/source-before`, run the behavior check again for the
          restored source, and record the measured difference.
        - Do not change the `byte 1` rows or `text_draw` to assembly. Record
          their first difference as the measured follow-up.
        - Build the image after the source edits:
          `python3 tools/rebuild/whole_image.py --all`.
        - Record the observed score and counts. Do not compare with a
          rank-and-source combined change, because the rank stays bytes-first.
        - Record the batch outcome in `docs/reference/source-forms.md` §The
          measured outcomes and in `docs/explanation/findings.md` §The 193
          shorter bodies. Record the strict count, the score, the kept forms,
          the reverted forms, the measured first difference, and the reason.
    - **Verify:**
        - `python3 tools/rebuild/first_difference.py report --module race_lap_complete --module loading_screen --module stream_read --module net_player_add --module resource_read --module resource_find --module lzw_bit_read --module text_draw --out work/rebuild/first-batch-after.json ; test $? -le 1`
        - `python3 tools/rebuild/run.py run --recheck-record 0x19678`
        - `python3 tools/rebuild/run.py run --recheck-record 0x4c4e5`
        - `python3 tools/rebuild/run.py run --recheck-record 0x2a364`
        - `python3 tools/rebuild/whole_image.py --all`
        - `python3 tools/rebuild/image_compare.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt work/rebuild/STARTH.EXE ; test $? -le 1`
        - `python3 tools/rebuild/toolchain.py check-matrix --manifest tools/rebuild/toolchain.json`
        - `python3 -m py_compile tools/rebuild/first_difference.py tools/rebuild/whole_image.py`

- [x] **Task 7: The documentation, the counts, and the full verification**
    - **Implements:** FR-018, SM-5
    - **Modify `docs/explanation/rebuild-quality-design.md`**
        - Update §Design Decisions, the whole-image bullet, with the exact slot
          copy, the `reuse` mode, the original-body-bytes rule, and the
          `reused` count. Keep the bytes-first matrix rank.
        - Update §Observations with the 348 module recipes, the 981 records,
          the 252-row matrix before the follow-up, the 96 recipes the merged
          tree added, and the 347-row manifest after the follow-up.
        - Keep C-10 at 433, 429, 418, and 414 unless the observed counts
          differ.
    - **Modify `AGENTS.md`**
        - §Project Structure: change `the two hundred and fifty-two module
          recipes` to `the three hundred and forty-eight module recipes`.
        - §Architecture, the whole-image bullet: add the exact slot copy, the
          `reuse` mode, and the strict padded body rule.
        - §Testing, the whole-image rule: add the exact slot copy, the `reuse`
          mode, the `reused` count, and the body-end slot bound.
        - §Testing, the matrix rule: keep the bytes-first rank. Add the 96
          migrated recipes and the 347-row manifest.
        - §Architecture, the source-form bullet: add the three idiom names and
          the catalogue sync rule. The rule already requires both files to
          hold the names.
        - §Build And Test Commands: keep the existing commands. The plan adds
          no tool, so the `py_compile` lists stay unchanged.
    - **Modify `README.md`**
        - §Status: add the 348 module recipe count and the measured
          whole-image byte score. The text does not exist today.
        - §Status: add a sentence that the whole-image build has the `write`
          and `reuse` modes.
    - **Modify `docs/reference/tooling.md`**
        - "## The rebuild harness" table row `modules/*.json`: change the two
          hundred and fifty-two count to three hundred and forty-eight. Keep
          the row wording.
        - "## The toolchain matrix": add the 96 migrated recipes and the
          347-row manifest. Keep the bytes-first rank.
        - "## The first-difference report": add the true sequence difference,
          the family field, the summary, the three idioms, the `unmeasured`
          family, and the library exclusion.
        - "## The whole-image build": add the exact slot copy, the `reuse`
          mode, the `reused` count, the body-end slot bound, and the new row
          fields `original_size` and `mode`. State that a `reuse` row holds
          `fit` false and that the `mode` and `outcome` fields carry the
          acceptance.
    - **Modify `docs/reference/source-forms.md`**
        - Keep the Task 1 signatures and the Task 5 census.
        - Add the Task 6 batch outcomes to §The measured outcomes.
    - **Modify `docs/explanation/findings.md`**
        - Add the measured whole-image counts and score to §The 193 shorter
          bodies. Do not rewrite the historical count lines.
        - Confirm the Task 4 matrix repair finding and add the final stage-12
          result: the 96 recipes now have rows, the manifest holds 347 rows,
          and the full check passes.
        - Confirm the Task 6 batch outcome, add the final measured score, and
          record the task data contradictions from §Facts Checked. Record that
          the bytes-first rank stays.
    - **Steps:**
        - Run the unit suite, the store check, the docs check, and the call
          graph check. The call graph check needs capstone and the original
          executable.
        - If a Task 6 source edit changes a direct call edge, run
          `python3 tools/rebuild/callgraph.py build` and write the updated
          `tools/rebuild/callgraph.json`.
        - Run the full `py_compile` list in `AGENTS.md`. The lists include
          `oracle.py`, `first_difference.py`, `whole_image.py`, and
          `toolchain.py`.
        - Build the image before the full check:
          `python3 tools/rebuild/whole_image.py --all`.
        - Run the four pipeline commands. The plan-time counts are 433, 429,
          418, and 414. Record the observed counts. If a count differs, stop,
          record the difference, and update the docs and C-10.
        - The full check re-runs the records. Regenerate the status page after
          the pipeline and confirm that it holds no `data-stale="true"`
          marker.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests`
        - `python3 -m py_compile tools/rebuild/oracle.py tools/rebuild/first_difference.py tools/rebuild/whole_image.py tools/rebuild/toolchain.py tools/pipeline_check.py`
        - `python3 tools/rebuild/whole_image.py --all`
        - `python3 tools/rebuild/image_compare.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt work/rebuild/STARTH.EXE ; test $? -le 1`
        - `python3 -c "import hashlib, json, pathlib; report = json.loads(pathlib.Path('work/rebuild/whole-image.json').read_text()); rows = report['modules']; assert len(rows) == 347; assert report['patched'] + report['reused'] + report['skipped'] == len(rows); assert report['relocated'] == sum(row['relocated'] for row in rows); assert report['score']['total'] == 1034923; image = pathlib.Path(report['image']['path']); assert image.is_file(); assert hashlib.sha256(image.read_bytes()).hexdigest() == report['image']['sha256']"`
        - `python3 tools/rebuild/first_difference.py report --all --out work/rebuild/first-differences.json ; test $? -le 1`
        - `python3 tools/rebuild/toolchain.py report`
        - `python3 tools/rebuild/toolchain.py check-matrix --manifest tools/rebuild/toolchain.json`
        - `python3 -c "import json, pathlib; document = json.loads(pathlib.Path('tools/rebuild/toolchain.json').read_text()); assert len(document['modules']) == 347; print('347 manifest rows')"`
        - `python3 tools/rebuild/library.py check-manifest --manifest tools/rebuild/library.json`
        - `python3 tools/rebuild/store.py check`
        - `python3 tools/rebuild/docs.py check`
        - `python3 tools/rebuild/callgraph.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --record-timeout 7200`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-cross --record-timeout 7200`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --record-timeout 7200`
        - `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross --record-timeout 7200`
        - `python3 tools/rebuild/view.py html --out work/rebuild/status.html`
        - `! grep -q 'data-stale="true"' work/rebuild/status.html`

## Measured Expectations

The plan states the measured result where it is known. It does not forecast a
local score.

- The plan-time image before the matrix repair is 13 patched, 238 skipped, 16
  relocated, and 1034738 of 1034923 bytes equal.
- The exact slot copy changes 16 of the 185 differing bytes, one byte per
  relocated slot. The plan-time measured body change gives 1034754 equal bytes
  when the write set is unchanged.
- If the Task 3 dry-run measures 12 reuse rows, the plan-time image becomes 13
  write, 12 reused, 226 skipped, and 1034754 equal bytes when the write set is
  unchanged. Task 5 records the measured reuse count and the post-repair
  values.
- The matrix repair changes 96 recipes. The post-repair whole-image counts and
  score can differ from the plan-time baseline. Task 5 records them.
- The plan-time 193 shorter bodies stay skipped unless the matrix repair or
  the source/compiler batch makes a body fit. The plan does not promise a
  strict match for any of them.
- Task 6 changes a source body under the module outcome rule. Task 7 records
  the observed score and the observed counts. A value above is a measured
  plan-time value, not a promise.

## Contradiction And Risk

- The task data says the first-difference report shows `match` for a
  length-only oracle verdict. The code and the test
  `test_report_row_classifies_a_different_count_result_as_other` show
  `other`. The report's `match` field is `same-mnemonics` or `different`,
  never `match`. Task 1 replaces the empty difference list with the true
  sequence difference and the new count idiom.
- The task data says the matrix holds 252 modules and 1 no-result row. The
  committed matrix holds 252 rows, but the merged store holds 348 recipes and
  the report counts 97 no-result rows. `toolchain.py check-matrix` fails for
  the 96 recipes. Task 4 measures them, writes the 347-row manifest, and the
  design and the reference pages update the count. This is the largest scope
  risk: the matrix task needs both harnesses and the period toolchain for the
  full 348-module report, and the pass is long. The partial-report manifest
  makes it restartable.
- The current relocation writes `reloc_base + target`, and the original file
  holds the target value. The plan-time 13 write rows hold 16 slots, and one
  byte of each slot differs from the original bytes. Task 3 copies the
  original slot bytes and records the score change. The design's whole-image
  bullet changes.
- The current original-slot filter uses the record span. `memset` holds a span
  of 220829 and a body of 25 bytes, so a relaxed fit would pass an unassigned
  slot. Task 3 bounds the filter by the body end and tests the case.
- A strict body is not byte-identical in the image, because the oracle masks
  direct branch targets. The reuse rows keep the original bytes, so they
  change no byte. A `write` patch of a strict body would change the masked
  branch bytes. The plan does not use that patch.
- The 12 padding reuse count is a forecast. Task 3 measures it. If a padding
  row is not aligned, it stays skipped and Task 5 records the measured reason.
- The whole-image build stales the record fingerprints, because the module
  maps hold a build timestamp. The full and `--skip-cross` runs take about 65
  minutes. The documented commands use `--record-timeout 7200`.
- The 20 larger bodies and the 2 shape-misaligned bodies stay skipped unless a
  later plan changes them. The plan records them as measured follow-ups.

## Deviations

- The five catalogue sections keep the measured first pair. The section
  signatures name the measured insertion or removal and reserve the
  `instruction count` signature for the equal-prefix count case. The
  classifier rules, the `IDIOMS` order, and `compare` stay unchanged.
- The merged tree holds 347 image rows, not the 251 plan-time rows. The Task 5
  and Task 7 image guards assert 347 rows, and the report counts sum to
  `len(rows)`.
