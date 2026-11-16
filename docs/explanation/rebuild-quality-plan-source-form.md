# Source Form Implementation Plan

> Execute this plan with the `executing-plans` skill. Each task is a
> checkbox. Mark it done when its changes are implemented and its Verify
> command passes.

- **Date**: 2026-11-15
- **Status**: Implemented
- **Design**: docs/explanation/rebuild-quality-design.md
- **Roadmap**: docs/explanation/rebuild-quality-roadmap.md — slice: source-form
- **Depends on**: Slice 2, Slice 5
- **Manual prerequisites**: Human review of the source-form catalogue at the
  gate after Task 2, and approval of the revised Slice 6 `Done when` before
  Task 3. The `Done when` approval gates the whole slice, not only the Task 8
  roadmap edit. If the user does not approve the revised bar, the plan stops
  before Task 3 and the slice is not executed under the old bar. Docker,
  DOSBox-X, the period Watcom 10.0a toolchain, and the game directory for the
  first-difference report and the module checks. Capstone for the Task 8
  `callgraph.py check`. The installed Ghidra loader only for the full check.
- **Goal**: Build the source-form idiom catalogue and the first-difference
  report, and close the six near-miss modules with source-form work, with C
  first.

## Constraints

- C-1: Python 3.14.7. Use the standard library and the `ctypes` capstone binding only.
- C-2: Tests use `unittest` under `tools/rebuild/tests/`.
- C-3: A tool takes its configuration from the command line. It reads no environment variable.
- C-4: A tool writes only under `work/` or to an explicit output path. The provenance evidence pass `triage.apply` and the library pass `library.apply` write the function records through `store.save_record`. The toolchain matrix writes the module records through `store.save_module`. C-4 still applies to the shipped artifacts.
- C-5: Prefer C. Use assembly only when C cannot reproduce the original bytes, and record the reason.
- C-6: A harness output name stays within the 8.3 limit.
- C-7: The 10.0a toolchain runs alongside OpenWatcom V2. Do not replace V2.
- C-8: The status ladder stays `unknown`, `not_a_function`, `named`, `rebuilt`, `matching`, `verified`.
- C-9: The identity verdict strings stay unchanged.
- C-10: At the design date, the full check reports 303 checks. `--skip-cross` reports 299, `--skip-runtime` reports 288, and both flags report 284. The plan-date counts are 304, 300, 289, and 285. The observed counts after the library pass are 431, 427, 416, and 412. The observed counts after the toolchain pass are 432, 428, 417, and 413. Update these counts when a change adds or removes a check.
- C-11: Do not commit. The user commits.
- The source-form rule is: keep C first. Use assembly only after the exact toolchain, flags, and source forms fail, and record the reason. Leave a module `rebuilt` when C is insufficient. Do not convert a module to assembly only to raise the verified count.
- The six-module selection rule is: the target set is the six rebuilt module-backed records whose identity check reports `instructions_equal == instructions_total` and a non-strict byte verdict. Do not select a seventh module.
- The median byte match is the median of the six `bytes_equal / bytes_total`
  ratios, the mean of the third and fourth values in sorted order. The Target
  Set holds the before ratios, and the final records hold the after ratios.
  Task 8 records both.
- The four full-check counts stay 432, 428, 417, and 413 unless a new stage check is added. This plan adds no stage check. `first_difference.py` enters `TOOLS`, the two `py_compile` lists in `AGENTS.md`, and the workflow syntax gate only.
- The first-difference report writes no store record. The module tasks write the function records through `run.py` and the trait notes through `store.py set --note`.
- The catalogue is `docs/reference/source-forms.md`. Every `IDIOMS` name in `first_difference.py` appears in the catalogue text.

## Spec Coverage

| Requirement | Task |
| --- | --- |
| FR-016 | Task 1 (first-difference report), Task 2 (source-form idiom catalogue), Tasks 3 to 7 (the six measured outcomes), Task 8 (the measured outcomes table, any new first-difference signature, and any new source form) |
| FR-018 | Task 1 (TOOLS, the two `py_compile` lists, and the tooling reference), Task 2 (the catalogue link), Tasks 3 to 7 (the findings entries), Task 8 (the observed counts, the AGENTS.md rules, and the roadmap Done when) |
| Slice 6 acceptance bar (the six modules) | Tasks 3 to 7 |

The six-module acceptance bar is not a design requirement ID. It comes from
the roadmap slice 6 entry and the user selection. Tasks 3 and 4 target the
strict gate. Tasks 5 to 7 target a strict match or a recorded source-form
trait (C or assembly) with the first difference and the reason.

## Target Set

| Module | Record | Symbol | Toolchain | Flags | Match | Instructions | Bytes | First oracle difference |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `error_report` | 0x10978 | `error_report_` | watcom10 | `-of+ -ot` | same-shape | 171/171 | 418/554 | instruction 23: `mask: [1, 2, 3, 4] against []` (also 31, 39) |
| `list_create` | 0x2ecf8 | `list_create_` | watcom10 | `-od` | same-shape | 46/46 | 102/127 | instruction 7: `byte 2: 0xf8 against 0xf4` |
| `memory_alloc_small` | 0x2f172 | `memory_alloc_small_` | watcom10 | `-od` | same-shape | 49/49 | 118/147 | instruction 6: `byte 2: 0xf4 against 0xf0` |
| `memory_available` | 0x2f8ca | `memory_available_` | v2 | `-od` | same-shape | 27/27 | 43/58 | instruction 8: `byte 2: 0xfc against 0xf4` |
| `pool_init` | 0x2f904 | `pool_init_` | watcom10 | `-od` | same-shape | 44/44 | 107/129 | instruction 8: `byte 2: 0xfc against 0xf8` |
| `dpmi_int31_entry` | 0xd4410 | `dpmi_int31_entry_` | watcom10 | none | same-shape | 28/28 | 18/56 | instruction 10: `size: 3 against 4` |

The first-difference values come from `python3 tools/rebuild/diff_module.py`
after `run.build_module` on each module. The record `0xd4410` holds the
provenance `compiler_helper`. The other five hold `game`.

## Diagnosis

- `error_report`: the source passes the integer literal `0x7fe8` in the
  codes 1, 2, 3, 4, 5, 7, 10, and the default. The original operand is a
  relocatable data address. The original instruction therefore holds a
  4-byte fixup slot, and the rebuilt constant holds no slot. The report
  classifies the `mask:` difference as `constant operand`.
- `dpmi_int31_entry`: the original `mov ds, word ptr [ebp + 0xc]` encodes as
  `8e 5d 0c` and holds 3 bytes. The period `wasm` encodes it as
  `66 8e 5d 0c` and holds 4 bytes, because the assembler adds the 16-bit
  operand-size prefix. The report classifies the `size:` difference as
  `instruction encoding`.
- The four local-slot modules: the original and the rebuilt forms place the
  local values in a different order, so the stack displacements shift. The
  first difference is a `byte 2:` displacement on the first parameter spill.
  The report classifies a `byte 2:` difference on a pair whose operands both
  hold `[ebp - ...]` as `local slot order`.
- A candidate cause for `pool_init` and `memory_alloc_small` is the struct
  local: the struct occupies the low local area, so the compiler allocator
  may place it at a lower slot. The findings hold no struct-local evidence
  and no measured slot layout for these two modules; the return-temporary
  evidence holds for `list_destroy` only. Task 7 tests the candidate and
  measures the kept pair. Until that measurement, the plan records the
  struct-local cause as a hypothesis, not a fact.
- The findings section "The list_destroy local slot order under the period
  10.0a compiler" records that the period 10.0a allocator places the
  compiler-generated return temporary in the highest local slot and then the
  parameter spills below it. The source form did not change the order for
  `list_destroy`. Tasks 5 to 7 test one named source form per module, measure
  the kept form with `diff_module.py`, and record the trait when the final
  match is non-strict.

## Shared Interfaces

- `first_difference.IDIOMS: tuple[str, ...]` is `("local slot order",
  "constant operand", "instruction encoding", "instruction selection",
  "basic-block order", "relocation target", "match", "other")`. It is the
  only definition of the report idiom names.
- `first_difference.STACK_SLOT = re.compile(r"\[ebp\s*-")` matches a stack
  frame displacement.
- `first_difference._stack_slot(insn: le.Insn) -> bool` returns true when
  `STACK_SLOT` matches `insn.operands`.
- `first_difference._difference_text(difference: str) -> str` removes one
  optional `instruction N: ` prefix.
- `first_difference.idiom_of(difference: str, original: le.Insn, rebuilt:
  le.Insn) -> str` classifies the difference. The rules, in order:
  - no difference after the optional prefix is `match`;
  - a `mnemonic:` difference is `instruction selection`;
  - a `size:` difference is `instruction encoding`;
  - a `mask:` difference is `constant operand`; a relocation-slot
    difference also produces a `mask:` difference, a documented limitation;
  - a `byte 2:` difference with `_stack_slot(original)` and
    `_stack_slot(rebuilt)` both true is `local slot order`;
  - a `branch target:` difference is `basic-block order`;
  - a difference that starts with `relocation` is `relocation target`, and
    this covers `relocations:`, `relocation offset:`, `relocation width:`,
    and `relocation target:`;
  - every other difference is `other`.
- `first_difference.first_pair(original: list[le.Insn], rebuilt:
  list[le.Insn], result: oracle.OracleResult) -> tuple[int, le.Insn,
  le.Insn] | None` reads the index from the first `instruction N: `
  difference. It returns the original and rebuilt instruction at that index.
  It returns `None` when `result.differences` is empty (including a result
  whose instruction counts differ), with a malformed difference, or with an
  index outside the body.
- `first_difference.report_row(record: dict, module: dict, result:
  oracle.OracleResult, original: list[le.Insn], rebuilt: list[le.Insn]) ->
  dict` returns one row. The row holds `module`, `address`, `symbol`,
  `toolchain`, `sources`, `build`, `match`, `instructions_equal`,
  `instructions_total`, `bytes_equal`, `bytes_total`, `first_difference`,
  `idiom`, `original`, and `rebuilt`. `build` is `pass`. `original` and
  `rebuilt` each hold `address`, `bytes`, and `text`, or `None`. `address`
  is a hex string. `bytes` is the lowercase hex of `insn.raw`. `text` is
  `insn.text`. `first_difference` is the first entry of `result.differences`,
  or `None` when the list is empty. The row is a `match` only when
  `result.differences` is empty and `result.match in run.STRICT_MATCHES`. A
  non-empty `differences` list is not a `match`. An empty `differences` list
  with a non-strict `result.match`, for example a result whose instruction
  counts differ, is `other`. When the result is not a strict match, an absent
  pair gives `None` for the pair fields and `other` for `idiom`.
- `first_difference._failure_row(record: dict, module: dict, build: str) ->
  dict` returns the row for a failed or skipped build. The row holds the
  same keys, `build` is `fail` or `skip`, the match and the four counts are
  `None`, `first_difference` is `None`, `idiom` is `other`, and the pair
  fields are `None`.
- `first_difference.diff_record(record: dict, module: dict, original_path:
  Path, work_dir: Path) -> dict` calls `run.build_module(module)`. A failed
  or skipped build returns `_failure_row`. A pass loads the original with
  `le.load(original_path)` and the rebuilt image with `le.load(work_dir /
  module["exe"])`. It reads `map_path = work_dir / module["map"]` and
  `(segment, offset) = diff_module.map_address(map_path, record["symbol"])`,
  takes `obj = rebuilt.object_by_index(segment)`, and defines
  `original_address = int(record["address"], 16)` and `rebuilt_address =
  obj.reloc_base + offset`. The end is `original_address + record["size"]`
  when `size` is present, and the count is `record.get("count", 0)`. Let
  `symbols = diff_module.map_symbols(map_path, rebuilt)` and `extent =
  diff_module.next_symbol(symbols, rebuilt_address)`. It loads the original
  body with `diff_module.original_body(original, original_address, end,
  count)` and the rebuilt body with `diff_module.rebuilt_body(rebuilt,
  rebuilt_address, end, count, extent=extent)`. It calls
  `oracle.compare(original_instructions,
  rebuilt_instructions, original_relocations=original.relocations(),
  rebuilt_relocations=rebuilt.relocations(), original_symbols=
  diff_module.original_symbol_resolver(original,
  diff_module.load_records()), rebuilt_symbols=
  diff_module.rebuilt_symbol_resolver(rebuilt, symbols))`. It returns
  `report_row`.
- `first_difference.report(records: list[dict], modules: list[dict],
  original_path: Path, names: list[str] | None = None) -> dict` selects a
  record that names a module, holds a symbol, and holds an identity match
  that is not in `run.STRICT_MATCHES`. A record with no `result`, no
  identity check entry, or an identity entry with no `match` is skipped. It
  calls `diff_record` with
  `store.WORK_DIR`. A `names` list restricts the module names. The rows are
  in module-name order. The document holds `date`, `original` (`path`,
  `sha256`), and `modules`. `path` is `str(original_path)`. `sha256` is the
  lowercase hex digest from `_sha256`, the same form as
  `toolchain._sha256`.
- `first_difference.REPORT_DOCUMENT: Path` is
  `store.WORK_DIR / "first-differences.json"`.
- `first_difference._sha256(path: Path) -> str` returns the lowercase hex
  digest in the same form as `toolchain._sha256` (no `sha256:` prefix). It
  raises `ValueError` naming the path when the file is missing.
  `command_report` validates the original image before it calls the digest, so
  a missing original exits 2 before the digest.
- `first_difference._write_report(path: Path, document: dict) -> None`
  creates the parent directory and writes the JSON with an indent of 2 and a
  trailing newline.
- `first_difference.command_report(arguments) -> int` loads the modules and
  the records, validates `--module NAME` (repeatable) or `--all`, validates
  the original image, writes the document to `--out` (default
  `REPORT_DOCUMENT`), and prints one line per module and a summary line. The
  exit codes are 0, 1, and 2. It exits 0 after the write when every selected
  row holds `build` `pass`. It exits 1 after the write when a selected row
  holds `build` `fail` or `skip`, because a selected module is not measured.
  It exits 2 on a bad input. It writes no store record.
- `first_difference.parse_arguments() -> argparse.Namespace` and
  `first_difference.main() -> int` follow the `toolchain.py` pattern. `main`
  catches `(OSError, ValueError, struct.error)` and returns 2.
- The module outcome rule: keep a strict form (`exact-bytes` or
  `exact-relocations`) when the behavior check passes. Otherwise keep the
  `same-shape` form with the higher `instructions_equal`, then the higher
  `bytes_equal`, and record the trait, the measured first difference, and
  the reason.
  A form whose match is not `exact-bytes`, `exact-relocations`, or
  `same-shape` is not kept, and a tie keeps the current form. Leave the
  record `rebuilt` when the final match is not strict. After a revert, re-run
  `python3 tools/rebuild/run.py run --function ADDR` again. The command
  re-runs the declared behavior check. A task that names an extra behavior
  check runs that check again too. The stored result, `source_digest`,
  fingerprint, and artifacts then match the kept source. The task is complete
  only after the final kept source has a stored result. The write-enabled run
  is the final build action. The first-difference report builds a non-strict
  module, so run the report before the final write. Do not run a command that
  rebuilds the module after the final write, because a later build makes the
  stored fingerprint stale.
- For the local-slot tasks in Task 5 to Task 7, the trait note describes the
  kept source. When the tested form is kept, the note names the kept form.
  When the tested form is reverted, the note names the rejected form and says
  the kept source holds the Target Set first difference and idiom. A
  non-strict note cites the measured first difference and the measured
  original and rebuilt stack slot layouts, and says the cause is an inference
  from the measured pair. Do not state a stack-slot cause as a fact before
  the `diff_module.py` measurement.
- The catalogue contract: `docs/reference/source-forms.md` holds one section
  per known source form. Each section holds the source form, the compiler
  and flag condition, the effect on the object code, the original census
  when the findings give one, the evidence command or findings section, and
  the first-difference signature. A difference-signature table names every
  value of `IDIOMS` and records that a `mask:` difference is `constant
  operand` and can also come from a relocation-slot difference. Task 8 adds
  `## The measured outcomes`.
- The module outcome assertion is a read-only `python3 -c` command. It loads
  the record, asserts `status` is `rebuilt` or `verified`, asserts the
  identity `match` is in `("exact-bytes", "exact-relocations",
  "same-shape")`, and asserts a non-strict `same-shape` result holds a note
  with the recorded trait (the note holds `first difference`). A recorded
  non-strict outcome is a pass of the task, not a failure.

## Tasks

- [x] **Task 1: The first-difference report tool**
    - **Implements:** FR-016, FR-018
    - Write the tests first.
    - **Create `tools/rebuild/tests/test_first_difference.py`**
        - Add the `insn(address, mnemonic, operands, raw)` factory and the
          `record(address, module, symbol, match)` factory. Both use no
          Docker, no DOSBox-X, and no game directory.
        - `test_idioms_hold_the_eight_names` — `IDIOMS` holds the eight names
          in the shared order.
        - `test_idiom_of_classifies_a_local_slot_byte` — two
          `mov dword ptr [ebp - 8], eax` and `mov dword ptr [ebp - 0xc], eax`
          instructions with a `byte 2:` difference give `local slot order`.
        - `test_idiom_of_requires_two_stack_slots_for_a_local_slot_byte` —
          one operand without `[ebp - ...]` gives `other`.
        - `test_idiom_of_classifies_a_mask` — a `mask:` difference gives
          `constant operand`.
        - `test_idiom_of_classifies_a_size` — a `size:` difference gives
          `instruction encoding`.
        - `test_idiom_of_classifies_a_mnemonic` — a `mnemonic:` difference
          gives `instruction selection`.
        - `test_idiom_of_classifies_a_branch_target` — a `branch target:`
          difference gives `basic-block order`.
        - `test_idiom_of_classifies_a_relocation` — each of `relocations:`,
          `relocation offset:`, `relocation width:`, and
          `relocation target:` gives `relocation target`.
        - `test_idiom_of_classifies_no_difference` — an empty difference
          gives `match`.
        - `test_idiom_of_classifies_an_unknown_difference` — a `byte 1:`
          difference on two register operands gives `other`.
        - `test_idiom_of_ignores_the_instruction_prefix` — a
          `instruction 23: mask: ...` string gives `constant operand`.
        - `test_first_pair_returns_the_first_indexed_pair` — the result
          holds `instruction 2: ...` and the pair is the third instruction
          of each body.
        - `test_first_pair_uses_the_first_difference` — two differences give
          the first pair.
        - `test_first_pair_is_none_without_a_difference` — an empty
          difference list gives `None`.
        - `test_first_pair_is_none_for_a_malformed_difference` — a
          difference without the prefix gives `None`.
        - `test_report_row_holds_the_pair_and_the_counts` — the row holds
          the module, the address, the symbol, the toolchain, the sources,
          `build` `pass`, the match, the four counts, the full first
          difference, the idiom, and the two instruction fields.
        - `test_report_row_holds_null_fields_without_a_difference` — a clean
          result with a strict match (`exact-bytes` or `exact-relocations`)
          gives `first_difference` `None`, `idiom` `match`, and the pair
          fields `None`.
        - `test_report_row_classifies_a_difference_without_a_pair_as_other`
          — a malformed difference gives `other` and the pair fields `None`.
        - `test_report_row_classifies_a_different_count_result_as_other` — a
          result with different instruction counts, an empty `differences`
          list, and a non-strict match gives `idiom` `other`.
        - `test_report_row_classifies_a_strict_result_with_no_difference_as_match`
          — a strict result with an empty `differences` list gives `idiom`
          `match`.
        - `test_report_selects_the_non_strict_module_records` — a `same-shape`
          record and a `different` record are selected. A `exact-relocations`
          record is skipped.
        - `test_report_skips_a_record_without_a_result` — a record with no
          `result` gives no row.
        - `test_report_skips_a_record_without_an_identity_entry` — a record
          whose `result` holds no identity check gives no row.
        - `test_report_skips_a_record_without_an_identity_match` — a record
          whose identity entry holds no `match` gives no row.
        - `test_report_skips_a_record_without_a_symbol` — the record has no
          symbol and gives no row.
        - `test_report_skips_a_record_without_a_module` — the record names
          no module and gives no row.
        - `test_report_restricts_the_records_to_the_names` — a `names` list
          of one module gives one row.
        - `test_report_orders_the_rows_by_module_name` — the module order is
          `dpmi_int31_entry`, `error_report`, `list_create`,
          `memory_alloc_small`, `memory_available`, `pool_init`.
        - `test_report_document_holds_the_date_and_the_original_digest` —
          the date is `date.today().isoformat()`, `original.path` is
          `str(original)`, and the digest is the lowercase SHA-256 hex of the
          temporary original file.
        - `test_sha256_raises_for_a_missing_path` — a missing path raises
          `ValueError` and the message names the path.
        - `test_diff_record_marks_a_failed_build` — a `run.build_module` that
          gives `fail` returns a row with `build` `fail`, `first_difference`
          `None`, and `idiom` `other`.
        - `test_diff_record_marks_a_skipped_build` — a `run.build_module` that
          gives `skip` returns a row with `build` `skip`.
        - The pure suite does not test the successful `diff_record` path,
          because that path decodes real bodies with capstone. The real report
          command in the Verify is the acceptance for the address, the body
          bounds, and the oracle path. A pure success test would need
          `mock.patch.object(first_difference.le, "load")` and the body helpers,
          so this plan records the note instead.
        - **Mock targets** keep the tests pure:
            - The `report` tests patch
              `mock.patch.object(first_difference, "diff_record")`.
            - The `command_report` tests patch
              `mock.patch.object(first_difference, "report")` or
              `mock.patch.object(first_difference, "diff_record")`.
            - The failed and skipped `diff_record` tests patch
              `mock.patch.object(first_difference.run, "build_module")`.
            - The deferred successful `diff_record` test would also patch
              `mock.patch.object(first_difference.le, "load")`.
        - `test_parse_arguments_accepts_the_report_flags` —
          `report --module error_report --module pool_init --out PATH` and
          `report --all` parse.
        - `test_command_report_rejects_a_missing_selection` — no `--module`
          and no `--all` exit 2.
        - `test_command_report_rejects_a_missing_module` — a name that is
          not a module exits 2.
        - `test_command_report_rejects_a_missing_original` — a missing
          original exits 2 before a build.
        - `test_command_report_writes_the_document` — the command writes
          `--out` and exits 0 when every selected row holds `build` `pass`.
        - `test_command_report_exits_1_for_a_failed_build` — a selected row
          holds `build` `fail`; the command writes `--out` and exits 1.
        - `test_command_report_exits_1_for_a_skipped_build` — a selected row
          holds `build` `skip`; the command writes `--out` and exits 1.
        - `test_command_report_writes_no_store_record` —
          `store.save_record` and `store.save_module` are not called.
    - **Create `tools/rebuild/first_difference.py`**
        - Add the module docstring and the usage block.
        - Add `from __future__ import annotations`.
        - Add the standard-library imports `argparse`, `hashlib`, `json`,
          `re`, `struct`, `sys`, `date`, and `Path`.
        - Insert the repository `tools/` path and the `tools/rebuild/` path.
          Import `le`, `oracle`, `store`, `run`, and `diff_module`.
        - Add `IDIOMS`, `STACK_SLOT`, `_stack_slot`, `_difference_text`,
          `idiom_of`, `first_pair`, `report_row`, `_failure_row`,
          `diff_record`, `report`, `REPORT_DOCUMENT`, `_sha256`,
          `_write_report`, `_load_modules`, `_load_records`,
          `command_report`, `parse_arguments`, and `main`. Use the
          signatures in Shared Interfaces.
        - In `diff_record`, pass `original_relocations=original.relocations()`
          and `rebuilt_relocations=rebuilt.relocations()` to
          `oracle.compare`, exactly as `diff_module.main` does. Define
          `original_address` and `rebuilt_address` before the body calls, and
          pass `extent=extent` to `diff_module.rebuilt_body`.
        - In `report`, skip a record with no `result`, no identity check
          entry, or an identity entry with no `match`. In the document, write
          `str(original_path)` and the lowercase hex digest.
        - `_load_modules` returns `store.load_module_file(path)` for every
          `store.MODULES_DIR.glob("*.json")` in sorted order.
        - `_load_records` returns `store.load_record(address)` for every
          `store.candidate_addresses()`.
        - The report subcommand adds `--module` with `action="append"`,
          `--all`, `--original` with the default `store.DEFAULT_ORIGINAL`,
          and `--out` with the default `REPORT_DOCUMENT`.
        - `command_report` returns 0 when every selected row holds `build`
          `pass`, returns 1 when a selected row holds `build` `fail` or
          `skip`, and returns 2 on a bad input. The document is written
          before the exit code is chosen.
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - `test_tools_include_the_first_difference_tool` — `TOOLS` holds
          `rebuild/first_difference.py`.
    - **Modify `tools/pipeline_check.py`**
        - Add `"rebuild/first_difference.py"` to `TOOLS` after
          `"rebuild/toolchain.py"`.
    - **Modify `AGENTS.md`**
        - Project Structure: add `first_difference.py` under
          `tools/rebuild/` after `diff_module.py`.
        - Build And Test Commands: add
          `python3 tools/rebuild/first_difference.py report --module error_report --out work/rebuild/first-error-report.json`.
        - Add `tools/rebuild/first_difference.py` to both `py_compile`
          lists after `tools/rebuild/toolchain.py`.
    - **Modify `.github/workflows/pipeline.yml`**
        - Add `tools/rebuild/first_difference.py` to the syntax-job
          `py_compile` list after `tools/rebuild/toolchain.py`.
    - **Modify `docs/reference/tooling.md`**
        - Add the row
          `| first_difference.py | build the selected non-strict module-backed records and report the first oracle difference with its source-form idiom |`
          after the `diff_module.py` row.
        - Add the section `## The first-difference report` after
          `## The toolchain matrix`. Describe the selection, the document,
          the eight idiom names, the output path, the no-store-record rule,
          and the no-stage-check rule.
        - Add the command to the rebuild command block.
    - **Steps:**
        - Run the unit tests and the focused `py_compile` command.
        - Run the real report for the six modules:
          `python3 tools/rebuild/first_difference.py report --module error_report --module list_create --module memory_alloc_small --module memory_available --module pool_init --module dpmi_int31_entry --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --out work/rebuild/first-differences.json`.
        - Confirm that the six rows hold the first differences in Target Set.
          The run needs Docker, DOSBox-X, the period toolchain, and the game
          directory.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_first_difference.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_pipeline_check.py"`
        - `python3 -m py_compile tools/rebuild/first_difference.py tools/pipeline_check.py`
        - `python3 tools/rebuild/first_difference.py report --module error_report --module list_create --module memory_alloc_small --module memory_available --module pool_init --module dpmi_int31_entry --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --out work/rebuild/first-differences.json`

- [x] **Task 2: The source-form idiom catalogue**
    - **Implements:** FR-016, FR-018
    - Write the tests first.
    - **Create `docs/reference/source-forms.md`**
        - Add the title `# Source Forms`, the date `2026-11-15`, and a short
          scope paragraph.
        - Add one section per known source form:
            - `## The registers-before-frame prologue with -od without -of+`
            - `## The frame-only epilogue against the conservative mov esp,ebp`
            - `## The local slot order`
            - `## The volatile parameter local`
            - `## The #pragma aux modify exact [regs] register contract`
            - `## The -ot body expansion`
            - `## The jump-table bias with inc eax`
            - `## The short push imm8 against the long push imm32 with a relocation`
            - `## The period compiler jump-table placement`
            - `## The 8.3 source-name collision`
        - Each section holds the source form, the compiler and flag
          condition, the effect on the object code, the original census when
          the findings give one, the evidence command or findings section,
          and the first-difference signature.
        - Add `## The difference signatures` with one row per `IDIOMS`
          value. Use the exact name in each row. Record that a `mask:`
          difference is `constant operand` and can also come from a
          relocation-slot difference, a limitation of the classification.
        - Use these facts. The registers-before-frame prologue comes from
          `-od` without `-of+`. The original census holds 205 prologues of
          that form and 116 frame functions without locals that end in
          `pop ebp; ret`. The frame-only epilogue omits `mov esp,ebp`, and
          the `-of+` epilogue is conservative in 8.5a through 11.0b. The
          local slot order holds `list_destroy` `list` at `[ebp-8]`, `unused`
          at `[ebp-4]`, and the return temporary at `[ebp-0xc]`. The volatile
          parameter local reproduces the frame store and reload, and the
          pattern appears 32 times. The `#pragma aux ... modify exact [regs]`
          contract removes the register saves around the printer call. The
          `-ot` body expansion keeps one full body per code. The jump-table
          bias `inc eax` gives the 24-entry table. The short `push imm8`
          becomes the long `push imm32` with a relocation when the operand
          is a symbol. The period compiler places and relocates the
          `error_report` jump table. The findings bullet "The period
          compiler places and relocates the jump table of `error_report`"
          holds the evidence. The 8.3 source-name collision hits
          `shutdown`, `bios_video`, `error_report`, `frame_step`,
          `list_destroy`, and `memory_available`, and the period harness
          copies `S0.C`, `S1.C`, and later names.
    - **Modify `docs/reference/tooling.md`**
        - Add the catalogue link and a one-line description to the
          first-difference report section.
    - **Modify `AGENTS.md`**
        - Project Structure: add `source-forms.md` under `docs/reference/`.
    - **Modify `README.md`**
        - Add `- [Source forms](docs/reference/source-forms.md) — the
          source-form idiom catalogue.` to the Documentation list.
    - **Modify `tools/rebuild/tests/test_first_difference.py`**
        - Add `CATALOGUE = Path(__file__).resolve().parents[3] / "docs/reference/source-forms.md"`.
        - `test_catalogue_holds_every_idiom_name` — every value of `IDIOMS`
          appears in the catalogue text.
        - `test_catalogue_holds_the_known_source_forms` — the catalogue holds
          the ten known section headings.
    - **Gate:** A human reviews `docs/reference/source-forms.md` and
      approves it. The user also approves the revised Slice 6 `Done when`
      (Manual prerequisites). Task 3 starts only after both approvals. If the
      user does not approve the revised bar, the plan stops here and the slice
      is not executed under the old bar.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_first_difference.py"`
        - `test -f docs/reference/source-forms.md`

- [x] **Task 3: The error_report relocatable printer data**
    - **Implements:** the Slice 6 acceptance bar (FR-016 evidence)
    - No new unit test. The identity check and the behavior check are the
      tests.
    - **Modify `tools/rebuild/error_report.c`**
        - Add `extern char printer_data[];` after `extern char message_table[];`.
        - Replace the eight `0x7fe8` arguments with `printer_data`. The
          codes are 1, 2, 3, 4, 5, 7, 10, and the default.
        - Update the comment that names the `0x7fe8` argument. State that
          the printer data is a relocatable symbol and that the compiler
          emits the long `push imm32` with a relocation.
        - Keep the `report` declaration, the `#pragma aux` line, the format
          strings, and the switch range.
    - **Modify `tools/rebuild/error_report_main.c`**
        - Add `char printer_data[0x100];` beside `char message_table[0x100];`.
        - Keep the `report` stub. The stub ignores the value.
    - **Name:** The mask difference proves the original operand is
      relocatable; the symbol name is neutral and the printer stub ignores
      the value.
    - **Test:** the behavior markers are `R`, `R`, `R`, and `D`. The
      instruction count must stay 171/171.
    - **Expected match set:** `exact-bytes`, `exact-relocations`,
      `same-shape`. A `same-shape` result is a pass only when the record
      note holds the recorded trait. A recorded non-strict outcome is a pass
      of this task, not a failure.
    - **Steps:**
        - Run the report before the edit and record the first difference and
          the idiom from Target Set.
        - Run `python3 tools/rebuild/run.py run --function 0x10978`. Record
          the match, the instruction and byte counts, and the behavior.
        - Run
          `python3 tools/rebuild/first_difference.py report --module error_report --out work/rebuild/first-error-report.json`.
          A strict module leaves the report, so the document holds no row.
          Confirm the strict result from the record and from
          `python3 tools/rebuild/run.py run --function 0x10978 --no-write`.
        - Apply the module outcome rule. Keep the strict form when the
          behavior check passes. Otherwise keep the `same-shape` form with
          the higher `instructions_equal`, then the higher `bytes_equal`, and
          restore the previous source when the edit does not win.
        - After a revert, run `python3 tools/rebuild/run.py run --function
          0x10978` again. The command re-runs the declared behavior check.
          The stored result, `source_digest`, fingerprint, and artifacts
          describe the kept source. The task is complete only after the final
          kept source has a stored result.
        - When the final match is `same-shape`, run `python3
          tools/rebuild/store.py set --function 0x10978 --note "the source
          form does not reach a strict match; the first difference is
          instruction 23: mask: [1, 2, 3, 4] against []; the reason is
          constant operand"`. Use the Target Set value. When the post-edit
          report row holds a different `first_difference` or `idiom`, use the
          row's values. Leave the status `rebuilt`.
        - Create the section `## The six near-miss source-form outcomes`
          before `## Open questions` and add the first outcome. The header
          holds the module, the record, the first difference before the
          change, the idiom, the source form, the final match, the
          instruction count, the byte count, the original stack slots, the
          rebuilt stack slots, and the outcome or reason. Record `n/a` in the
          two stack-slot cells when the module is not a local-slot form.
    - **Verify:**
        - `python3 -c "import sys;sys.path.insert(0,'tools/rebuild');import store;r=store.load_record(int(sys.argv[1],16));m=r['result']['checks']['identity']['match'];n=r.get('notes') or r.get('note') or [];t=' '.join(x.get('text','') if isinstance(x,dict) else str(x) for x in n) if isinstance(n,list) else str(n);assert r['status'] in ('rebuilt','verified'),r['status'];assert m in ('exact-bytes','exact-relocations','same-shape'),m;assert m in ('exact-bytes','exact-relocations') or 'first difference' in t,'a same-shape result needs the recorded trait';print(m,r['status'])" 0x10978`
        - `python3 tools/rebuild/first_difference.py report --module error_report --out work/rebuild/first-error-report.json`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
    - The `run.py` command in the Steps exits 1 when the final match is
      non-strict. That exit is the expected result of a recorded trait, not a
      task failure.

- [x] **Task 4: The dpmi_int31_entry three-byte segment load**
    - **Implements:** the Slice 6 acceptance bar (FR-016 evidence)
    - No new unit test. The identity check and the module run are the tests.
    - **Modify `tools/rebuild/dpmi_int31_entry.asm`**
        - Replace `mov ds, word ptr [ebp + 0xc]` with the three bytes
          `db 8eh, 5dh, 0ch`.
        - Add a comment that states the original encodes the 16-bit segment
          load as three bytes and that the period assembler adds the
          16-bit operand-size prefix.
        - Keep the rest of the instruction sequence.
    - **Reason for assembly:** the module is already assembly, because the
      original is a run-time segment and flag routine. This task pins an
      assembler encoding. It does not convert a C module to assembly.
    - **Test:** the instruction count must stay 28/28. The module run markers
      are `V` and `D`.
    - **Expected match set:** `exact-bytes`, `exact-relocations`,
      `same-shape`. A `same-shape` result is a pass only when the record
      note holds the recorded trait. A recorded non-strict outcome is a pass
      of this task, not a failure.
    - **Steps:**
        - Run the report before the edit and record the first difference and
          the idiom from Target Set.
        - Run `python3 tools/rebuild/run.py run --function 0xd4410`. Record
          the match and the counts.
        - Run `python3 tools/rebuild/harness.py run --module dpmi_int31_entry`
          as the extra behavior check. The record declares no behavior check.
        - Apply the module outcome rule. Keep the strict form when the run
          passes. Otherwise keep the `same-shape` form with the higher
          `instructions_equal`, then the higher `bytes_equal`, and restore
          the previous line when the edit does not win.
        - After a revert, run `python3 tools/rebuild/run.py run --function
          0xd4410` again and run `python3 tools/rebuild/harness.py run
          --module dpmi_int31_entry` again. The stored result,
          `source_digest`, fingerprint, and artifacts then describe the kept
          source. The task is complete only after the final kept source has a
          stored result.
        - When the final match is `same-shape`, run `python3
          tools/rebuild/store.py set --function 0xd4410 --note "the source
          form does not reach a strict match; the first difference is
          instruction 10: size: 3 against 4; the reason is instruction
          encoding"`. Use the Target Set value. When the post-edit report row
          holds a different `first_difference` or `idiom`, use the row's
          values. Leave the status `rebuilt`.
        - Add the outcome to `## The six near-miss source-form outcomes`.
    - **Verify:**
        - `python3 -c "import sys;sys.path.insert(0,'tools/rebuild');import store;r=store.load_record(int(sys.argv[1],16));m=r['result']['checks']['identity']['match'];n=r.get('notes') or r.get('note') or [];t=' '.join(x.get('text','') if isinstance(x,dict) else str(x) for x in n) if isinstance(n,list) else str(n);assert r['status'] in ('rebuilt','verified'),r['status'];assert m in ('exact-bytes','exact-relocations','same-shape'),m;assert m in ('exact-bytes','exact-relocations') or 'first difference' in t,'a same-shape result needs the recorded trait';print(m,r['status'])" 0xd4410`
        - `python3 tools/rebuild/harness.py run --module dpmi_int31_entry`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
    - The `run.py` command in the Steps exits 1 when the final match is
      non-strict. That exit is the expected result of a recorded trait, not a
      task failure.

- [x] **Task 5: The memory_available direct return**
    - **Implements:** the Slice 6 acceptance bar (FR-016 evidence)
    - No new unit test. The identity check and the behavior check are the
      tests.
    - **Modify `tools/rebuild/memory_available.c`**
        - Replace the `unsigned total;` local and the two statements with
          `return probe_free(pool) + probe_size(pool);`.
        - Add a comment that states the direct return removes the named
          local and tests the allocator spill order.
    - **Test:** the behavior markers are `F`, `S`, `T`, and `D`. Record the
      identity match, the instruction and byte counts, and the first
      difference.
    - **Expected match set:** `exact-bytes`, `exact-relocations`,
      `same-shape`. A `same-shape` result is a pass only when the record
      note holds the recorded trait. A recorded non-strict outcome is a pass
      of this task, not a failure.
    - **Steps:**
        - Run the report before the edit and record the first difference and
          the idiom from Target Set.
        - Run `python3 tools/rebuild/run.py run --function 0x2f8ca`. The
          command exits 1 while the identity is non-strict. Record the result.
        - Run
          `python3 tools/rebuild/first_difference.py report --module memory_available --out work/rebuild/first-memory-available.json`.
        - Apply the module outcome rule. Keep the direct return only when
          the match is strict and the behavior check passes. Otherwise keep
          the form with the higher `instructions_equal`, then `bytes_equal`.
        - After a revert, run `python3 tools/rebuild/run.py run --function
          0x2f8ca` again. The command re-runs the declared behavior check.
          The stored result, `source_digest`, fingerprint, and artifacts
          describe the kept source. The task is complete only
          after the final kept source has a stored result.
        - Measure the kept form with
          `python3 tools/rebuild/diff_module.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x2f8ca --original-end 0x2f904 --rebuilt work/rebuild/mamod.exe --rebuilt-map work/rebuild/memory_available.map --rebuilt-symbol memory_available_`.
          Record the `oracle difference:` line, the idiom, and the original
          and rebuilt stack slot layout from the two instruction bodies. The
          layout is the ordered list of `[ebp - N]` slots with the value
          stored in each slot.
        - When the direct return is kept and the final match is non-strict,
          run `python3 tools/rebuild/store.py set --function 0x2f8ca --note
          TEXT`. TEXT names the kept direct return, holds the measured first
          difference, the idiom, and the two stack slot layouts, and says the
          cause is an inference from the measured pair.
        - When the direct return is reverted, run `python3
          tools/rebuild/store.py set --function 0x2f8ca --note TEXT`. TEXT
          names the direct return as rejected, says the kept source holds the
          Target Set first difference `instruction 8: byte 2: 0xfc against
          0xf4` and the idiom `local slot order`, gives the reason `the
          compiler allocator local slot order`, holds the measured original
          and rebuilt stack slot layouts, and says the cause is an inference
          from the measured pair. Replace the first difference and the idiom
          with the measured kept-source values when they differ. Leave the
          status `rebuilt`.
        - Add the measured first difference, the idiom, the two stack slot
          layouts, and the outcome to `## The six near-miss source-form
          outcomes`.
    - **Verify:**
        - `python3 -c "import sys;sys.path.insert(0,'tools/rebuild');import store;r=store.load_record(int(sys.argv[1],16));m=r['result']['checks']['identity']['match'];n=r.get('notes') or r.get('note') or [];t=' '.join(x.get('text','') if isinstance(x,dict) else str(x) for x in n) if isinstance(n,list) else str(n);assert r['status'] in ('rebuilt','verified'),r['status'];assert m in ('exact-bytes','exact-relocations','same-shape'),m;assert m in ('exact-bytes','exact-relocations') or 'first difference' in t,'a same-shape result needs the recorded trait';print(m,r['status'])" 0x2f8ca`
        - `python3 tools/rebuild/first_difference.py report --module memory_available --out work/rebuild/first-memory-available.json`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
    - The `run.py` command in the Steps exits 1 when the final match is
      non-strict. That exit is the expected result of a recorded trait, not a
      task failure.

- [x] **Task 6: The list_create named result**
    - **Implements:** the Slice 6 acceptance bar (FR-016 evidence)
    - No new unit test. The identity check and the behavior check are the
      tests.
    - **Modify `tools/rebuild/list_create.c`**
        - Add `void *result;` at the top of the function.
        - Set `result = list;` after the if/else.
        - Return `result` instead of `list`.
        - Add a comment that states the named result tests whether the
          allocator changes the spill order.
    - **Test:** the behavior markers are `0`, `1`, and `D`. Record the
      identity match, the instruction and byte counts, and the first
      difference.
    - **Expected match set:** `exact-bytes`, `exact-relocations`,
      `same-shape`. A `same-shape` result is a pass only when the record
      note holds the recorded trait. A recorded non-strict outcome is a pass
      of this task, not a failure.
    - **Steps:**
        - Run the report before the edit and record the first difference and
          the idiom from Target Set.
        - Run `python3 tools/rebuild/run.py run --function 0x2ecf8`. The
          command exits 1 while the identity is non-strict. Record the result.
        - Run
          `python3 tools/rebuild/first_difference.py report --module list_create --out work/rebuild/first-list-create.json`.
        - Apply the module outcome rule. Keep the named result only when the
          match is strict and the behavior check passes. Otherwise keep the
          form with the higher `instructions_equal`, then `bytes_equal`.
        - After a revert, run `python3 tools/rebuild/run.py run --function
          0x2ecf8` again. The command re-runs the declared behavior check.
          The stored result, `source_digest`, fingerprint, and artifacts
          describe the kept source. The task is complete only
          after the final kept source has a stored result.
        - Measure the kept form with
          `python3 tools/rebuild/diff_module.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x2ecf8 --original-end 0x2ed77 --rebuilt work/rebuild/lcrmod.exe --rebuilt-map work/rebuild/list_create.map --rebuilt-symbol list_create_`.
          Record the `oracle difference:` line, the idiom, and the original
          and rebuilt stack slot layout from the two instruction bodies. The
          layout is the ordered list of `[ebp - N]` slots with the value
          stored in each slot.
        - When the named result is kept and the final match is non-strict,
          run `python3 tools/rebuild/store.py set --function 0x2ecf8 --note
          TEXT`. TEXT names the kept named result, holds the measured first
          difference, the idiom, and the two stack slot layouts, and says the
          cause is an inference from the measured pair.
        - When the named result is reverted, run `python3
          tools/rebuild/store.py set --function 0x2ecf8 --note TEXT`. TEXT
          names the named result as rejected, says the kept source holds the
          Target Set first difference `instruction 7: byte 2: 0xf8 against
          0xf4` and the idiom `local slot order`, gives the reason `the
          compiler allocator local slot order`, holds the measured original
          and rebuilt stack slot layouts, and says the cause is an inference
          from the measured pair. Replace the first difference and the idiom
          with the measured kept-source values when they differ. Leave the
          status `rebuilt`.
        - Add the measured first difference, the idiom, the two stack slot
          layouts, and the outcome to `## The six near-miss source-form
          outcomes`.
    - **Verify:**
        - `python3 -c "import sys;sys.path.insert(0,'tools/rebuild');import store;r=store.load_record(int(sys.argv[1],16));m=r['result']['checks']['identity']['match'];n=r.get('notes') or r.get('note') or [];t=' '.join(x.get('text','') if isinstance(x,dict) else str(x) for x in n) if isinstance(n,list) else str(n);assert r['status'] in ('rebuilt','verified'),r['status'];assert m in ('exact-bytes','exact-relocations','same-shape'),m;assert m in ('exact-bytes','exact-relocations') or 'first difference' in t,'a same-shape result needs the recorded trait';print(m,r['status'])" 0x2ecf8`
        - `python3 tools/rebuild/first_difference.py report --module list_create --out work/rebuild/first-list-create.json`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
    - The `run.py` command in the Steps exits 1 when the final match is
      non-strict. That exit is the expected result of a recorded trait, not a
      task failure.

- [x] **Task 7: The pool_init and memory_alloc_small struct local**
    - **Implements:** the Slice 6 acceptance bar (FR-016 evidence)
    - No new unit test. Each module has its own identity check and behavior
      check. A strict result for one module does not force the other module.
    - **Modify `tools/rebuild/pool_init.c`**
        - Add the file-scope `struct dpmi_block { unsigned int block[7]; unsigned int segments[3]; };`.
        - Replace the two locals with `struct dpmi_block regs;`.
        - Use `regs.block` and `regs.segments` in the function.
        - Keep the DPMI function `0x500`, the status test at `regs.block[6]`,
          and the two page conversions.
    - **Modify `tools/rebuild/memory_alloc_small.c`**
        - Add the same `struct dpmi_block` definition.
        - Replace the two arrays with `struct dpmi_block regs;`.
        - Use `regs.block` and `regs.segments` in the function.
        - Keep the `unsigned int value;` local, the `value =
          (regs.block[1] << 16) | (regs.block[2] & 0xffff);` statement, and
          the `return value;` statement. The direct-return change is out of
          scope for this experiment, so the edit changes one variable only.
        - Keep the DPMI function `0x501`, the status test at `regs.block[6]`,
          and the address reassembly.
    - **Test:** the `pool_init` behavior markers are `0`, `1`, `2`, and `D`.
      The `memory_alloc_small` behavior markers are `0`, `1`, and `D`.
      Record the identity match, the instruction and byte counts, and the
      first difference for each module.
    - **Expected match set:** `exact-bytes`, `exact-relocations`,
      `same-shape` for each module. A `same-shape` result is a pass only when
      the record note holds the recorded trait. A recorded non-strict
      outcome is a pass of this task, not a failure.
    - **Steps:**
        - Run the report before the edits and record the first differences
          and the idioms from Target Set.
        - Apply the `pool_init` change, then run
          `python3 tools/rebuild/run.py run --function 0x2f904`. The command
          exits 1 while the identity is non-strict. Record the result and run
          `python3 tools/rebuild/first_difference.py report --module pool_init --out work/rebuild/first-pool-init.json`.
        - Apply the module outcome rule to `pool_init`, then restore the
          previous source when the edit does not win. After a revert, run
          `python3 tools/rebuild/run.py run --function 0x2f904` again. The
          command re-runs the declared behavior check. The stored result,
          `source_digest`, fingerprint, and artifacts describe the kept
          source. The task is complete only after the final kept source has a
          stored result.
        - Measure the kept `pool_init` form with
          `python3 tools/rebuild/diff_module.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x2f904 --original-end 0x2f985 --rebuilt work/rebuild/pimod.exe --rebuilt-map work/rebuild/pool_init.map --rebuilt-symbol pool_init_`.
          Record the `oracle difference:` line, the idiom, and the original
          and rebuilt stack slot layout from the two instruction bodies. The
          layout is the ordered list of `[ebp - N]` slots with the value
          stored in each slot.
        - When the struct local is kept and the final match is non-strict,
          run `python3 tools/rebuild/store.py set --function 0x2f904 --note
          TEXT`. TEXT names the kept struct local, holds the measured first
          difference, the idiom, and the two stack slot layouts, and says the
          cause is an inference from the measured pair.
        - When the struct local is reverted, run `python3
          tools/rebuild/store.py set --function 0x2f904 --note TEXT`. TEXT
          names the struct local as rejected, says the kept source holds the
          Target Set first difference `instruction 8: byte 2: 0xfc against
          0xf8` and the idiom `local slot order`, gives the reason `the
          compiler allocator local slot order`, holds the measured original
          and rebuilt stack slot layouts, and says the cause is an inference
          from the measured pair. Replace the first difference and the idiom
          with the measured kept-source values when they differ. Leave the
          status `rebuilt`.
        - Apply the `memory_alloc_small` change, then run
          `python3 tools/rebuild/run.py run --function 0x2f172`. The command
          exits 1 while the identity is non-strict. Record the result and run
          `python3 tools/rebuild/first_difference.py report --module memory_alloc_small --out work/rebuild/first-memory-alloc-small.json`.
        - Apply the module outcome rule to `memory_alloc_small`, then restore
          the previous source when the edit does not win. After a revert, run
          `python3 tools/rebuild/run.py run --function 0x2f172` again. The
          command re-runs the declared behavior check. The stored result,
          `source_digest`, fingerprint, and artifacts describe the kept
          source. The task is complete only after the final kept source has a
          stored result.
        - Measure the kept `memory_alloc_small` form with
          `python3 tools/rebuild/diff_module.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x2f172 --original-end 0x2f205 --rebuilt work/rebuild/masmod.exe --rebuilt-map work/rebuild/memory_alloc_small.map --rebuilt-symbol memory_alloc_small_`.
          Record the `oracle difference:` line, the idiom, and the original
          and rebuilt stack slot layout from the two instruction bodies. The
          layout is the ordered list of `[ebp - N]` slots with the value
          stored in each slot.
        - When the struct local is kept and the final match is non-strict,
          run `python3 tools/rebuild/store.py set --function 0x2f172 --note
          TEXT`. TEXT names the kept struct local, holds the measured first
          difference, the idiom, and the two stack slot layouts, and says the
          cause is an inference from the measured pair.
        - When the struct local is reverted, run `python3
          tools/rebuild/store.py set --function 0x2f172 --note TEXT`. TEXT
          names the struct local as rejected, says the kept source holds the
          Target Set first difference `instruction 6: byte 2: 0xf4 against
          0xf0` and the idiom `local slot order`, gives the reason `the
          compiler allocator local slot order`, holds the measured original
          and rebuilt stack slot layouts, and says the cause is an inference
          from the measured pair. Replace the first difference and the idiom
          with the measured kept-source values when they differ. Leave the
          status `rebuilt`.
        - Add the measured first differences, the idioms, the two stack slot
          layouts for each module, and the two outcomes to `## The six
          near-miss source-form outcomes`.
    - **Verify:**
        - `python3 -c "import sys;sys.path.insert(0,'tools/rebuild');import store;r=store.load_record(int(sys.argv[1],16));m=r['result']['checks']['identity']['match'];n=r.get('notes') or r.get('note') or [];t=' '.join(x.get('text','') if isinstance(x,dict) else str(x) for x in n) if isinstance(n,list) else str(n);assert r['status'] in ('rebuilt','verified'),r['status'];assert m in ('exact-bytes','exact-relocations','same-shape'),m;assert m in ('exact-bytes','exact-relocations') or 'first difference' in t,'a same-shape result needs the recorded trait';print(m,r['status'])" 0x2f904`
        - `python3 -c "import sys;sys.path.insert(0,'tools/rebuild');import store;r=store.load_record(int(sys.argv[1],16));m=r['result']['checks']['identity']['match'];n=r.get('notes') or r.get('note') or [];t=' '.join(x.get('text','') if isinstance(x,dict) else str(x) for x in n) if isinstance(n,list) else str(n);assert r['status'] in ('rebuilt','verified'),r['status'];assert m in ('exact-bytes','exact-relocations','same-shape'),m;assert m in ('exact-bytes','exact-relocations') or 'first difference' in t,'a same-shape result needs the recorded trait';print(m,r['status'])" 0x2f172`
        - `python3 tools/rebuild/first_difference.py report --module pool_init --module memory_alloc_small --out work/rebuild/first-struct-locals.json`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
    - Each `run.py` command in the Steps exits 1 when that module stays
      non-strict. That exit is the expected result of a recorded trait, not a
      task failure.

- [x] **Task 8: The documentation, the outcomes, and the counts**
    - **Implements:** FR-016, FR-018
    - **Modify `docs/reference/source-forms.md`**
        - Add `## The measured outcomes` with one row per measured module:
          the module, the record, the source form, the final match, the
          instruction count, the byte count, the first difference, the idiom,
          the original stack slots, the rebuilt stack slots, and the outcome.
          Record `n/a` in the two stack-slot cells for a module that is not a
          local-slot form.
        - Add any new first-difference signature to `## The difference
          signatures`. If a measured result reveals a new source form, add a
          section. If it only confirms an existing form, update the outcome
          table.
        - Keep `test_catalogue_holds_every_idiom_name` green.
    - **Modify `docs/explanation/findings.md`**
        - Complete `## The six near-miss source-form outcomes` before
          `## Open questions`. Add one table with the module, the record, the
          first difference before the change, the idiom, the source form, the
          final match, the instruction count, the byte count, the original
          stack slots, the rebuilt stack slots, and the outcome or reason.
          Record `n/a` in the two stack-slot cells for a module that is not a
          local-slot form.
        - Record the measured slot layouts for `memory_available`,
          `list_create`, `pool_init`, and `memory_alloc_small`. State that a
          stack-slot cause is an inference from the measured original and
          rebuilt pair, not a compiler fact the findings already prove.
        - Record the report command and the catalogue path.
        - Record the before and after median byte match of the six, as
          defined in Constraints.
        - Record the four-check counts 432, 428, 417, and 413. State that
          `first_difference.py` is not a stage check.
    - **Modify `AGENTS.md`**
        - Testing: add a bullet for the first-difference report. State that
          the pure tests need no Docker, no DOSBox-X, and no game directory,
          and that the real report and the module checks need them.
        - Testing: add a bullet for the catalogue sync test.
        - Testing: add a bullet for the source-form rule and the module
          outcome rule. State that the record stays `rebuilt` when C is
          insufficient.
        - Testing: state that the four full-check counts stay 432, 428, 417,
          and 413 because the tool adds no stage check.
        - Architecture: add a bullet that `docs/reference/source-forms.md`
          names the idioms and `first_difference.py` classifies the first
          oracle difference with the same names.
    - **Modify `docs/reference/tooling.md`**
        - Update the four check-count lines in the pipeline section. State
          that the counts stay 432, 428, 417, and 413 and that the
          first-difference tool is not a stage check.
        - Confirm that the first-difference command is in the tool command
          block.
    - **Modify `docs/explanation/rebuild-quality-roadmap.md`** — the user
      approved the revised Slice 6 `Done when` before Task 3 (Manual
      prerequisites). If the user did not approve it, the plan stopped before
      Task 3 and this edit does not run.
        - Set the Slice 6 `Done when` to: each of the six near-miss modules
          holds a strict match or a recorded source-form trait (C or assembly)
          with its first difference and its reason. The first-difference
          report and the source-form idiom catalogue are in the docs. The
          median byte match of the six does not fall.
        - Do not change the Slice 6 Status or Plan. The execution skill owns
          those fields.
    - **Steps:**
        - Confirm that the approval of the revised Slice 6 `Done when` was
          recorded before Task 3.
        - Run the unit suite, the store check, the docs check, the call graph
          check, and the status page command. The call graph check needs
          capstone and the original executable.
        - Run the full `py_compile` list in `AGENTS.md`. Both lists include
          `tools/rebuild/first_difference.py`.
        - Run the four pipeline commands in Verify. Record the four observed
          counts. The counts must be 432, 428, 417, and 413. If a count
          differs, stop and report the difference. Do not write a forecast
          count.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_first_difference.py"`
        - `grep -q '## The measured outcomes' docs/reference/source-forms.md`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/docs.py check`
        - `python3 tools/rebuild/callgraph.py check`
        - `python3 tools/rebuild/view.py html --out work/rebuild/status.html`
        - `! grep -q 'data-stale="true"' work/rebuild/status.html`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-cross`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime`
        - `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross`

## Deviations

- The final write-enabled `run.py run --function ADDR` is the last build action
  for a module.  Reason: the input fingerprint covers the built executable and
  map.  A later `--no-write` confirmation or a non-strict report rebuilds the
  artifacts and stales the stored fingerprint.  The Shared Interfaces and the
  module tasks state the rule.
- The `list_create` trait note was replaced through the store API, not through
  `store.py set --note`.  Reason: `set --note` appends a note, and the plan asks
  for one note.  The replacement keeps one note and leaves the fingerprint
  unchanged.
- The rejected-form evidence uses distinct report paths
  (`work/rebuild/first-list-create-rejected.json` and
  `work/rebuild/first-pool-struct-rejected.json`).  Reason: the kept-form report
  overwrites the default path, so the rejected row needs its own evidence.
- The older findings section "The dpmi_int31_entry module" was updated to the
  current byte-identical result.  Reason: the findings page must not hold two
  different results for one module.
- The catalogue records the short-push signature as `size: 5 against 2` and the
  local-slot cause as an inference.  Reason: the oracle prints the original size
  first, and the findings present the allocator cause as an inference, not a
  proved fact.
