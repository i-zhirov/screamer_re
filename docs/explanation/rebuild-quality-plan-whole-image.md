# Whole-Image Implementation Plan

> Execute this plan with the `executing-plans` skill. Each task is a
> checkbox. Mark it done when its changes are implemented and its Verify
> command passes.

- **Date**: 2026-11-15
- **Status**: Implemented
- **Design**: docs/explanation/rebuild-quality-design.md
- **Roadmap**: docs/explanation/rebuild-quality-roadmap.md — slice: whole-image
- **Depends on**: Slice 3, Slice 4, Slice 5
- **Manual prerequisites**: The game directory and the original image at
  `~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`; Docker for
  the V2 harness; DOSBox-X and the fetched period Watcom 10.0a toolchain for
  the period harness; the installed Ghidra loader for the full check;
  capstone for the call graph check. The full-build Verify also needs the
  image written by Task 5 at `work/rebuild/STARTH.EXE`, because stage 11 now
  fails when the image is missing.
- **Goal**: Produce `work/rebuild/STARTH.EXE` by placing each rebuilt module
  body at its original address in a copy of the original image, relocate the
  internal link-time operands, and score the result with
  `image_compare.py`.

## Constraints

- C-1: Python 3.14.7. Use the standard library and the `ctypes` capstone binding only.
- C-2: Tests use `unittest` under `tools/rebuild/tests/`.
- C-3: A tool takes its configuration from the command line. It reads no environment variable.
- C-4: A tool writes only under `work/` or to an explicit output path. The provenance evidence pass `triage.apply` and the library pass `library.apply` write the function records through `store.save_record`. The toolchain matrix writes the module records through `store.save_module`. The whole-image build writes the rebuilt image and its report under `work/`. C-4 still applies to the shipped artifacts.
- C-5: Prefer C. Use assembly only when C cannot reproduce the original bytes, and record the reason.
- C-6: A harness output name stays within the 8.3 limit.
- C-7: The 10.0a toolchain runs alongside OpenWatcom V2. Do not replace V2.
- C-8: The status ladder stays `unknown`, `not_a_function`, `named`, `rebuilt`, `matching`, `verified`.
- C-9: The identity verdict strings stay unchanged.
- C-10: At the design date, the full check reports 303 checks. `--skip-cross` reports 299, `--skip-runtime` reports 288, and both flags report 284. The plan-date counts are 304, 300, 289, and 285. The observed counts after the library pass are 431, 427, 416, and 412. The observed counts after the toolchain pass are 432, 428, 417, and 413. The whole-image stage adds one counted check, so the expected counts are 433, 429, 418, and 414. Update these counts when a change adds or removes a check.
- C-11: Do not commit. The user commits.
- The whole-image build is an in-place relocating build. It uses the original
  `STARTH.EXE` as the container, keeps the stub, the header, the data objects,
  and the library bytes, and writes each rebuilt module body at its original
  address.
- The module order is the original address order of the lowest identity record
  of each module. A module with no record is excluded.
- The fit rule: `fit(record, rebuilt)` returns `None` only when the sum of the
  rebuilt instruction sizes equals `record["size"]`. Otherwise the module is
  skipped with the reason.
- The relocation rule: only an internal 4-byte slot is rewritten. A
  non-internal slot, a width other than 4, or an instruction misalignment
  skips the module. The original image holds only source type 7 (a 32-bit
  offset) slots, so the build writes the object base plus the target and
  assumes no 16:16 pointer or self-relative slot.
- A direct branch target is relative and is not rewritten. The body moves as a
  unit.
- The original fixup section stays valid, so the build does not rewrite it.
- The library manifest `tools/rebuild/library.json` holds the 3 library files
  and their digests and 138 entries. The original library bytes stay in place.
  The whole-image build writes no store record.
- The whole-image stage is a score gate. It always runs and is counted in the
  four modes. It fails when the rebuilt image is missing or unreadable. It
  passes when `image_compare.py` prints a byte score, including when the
  images differ. The exact oracle remains the tool's exit code.
- The four full-check counts are 433, 429, 418, and 414.
- The rebuilt image and its report are work artifacts under `work/rebuild/`.
  Do not commit them.

## Spec Coverage

| Requirement | Task |
| --- | --- |
| FR-014 | Task 3 (the in-place build keeps the original library bytes and the library manifest proves the verified records), Task 5 (the full build) |
| FR-017 | Tasks 1 to 3 (the tool and the image), Task 4 (the mandatory score gate), Task 5 (the full image), Task 6 (the documentation) |
| FR-018 | Task 3 (the `TOOLS` entry), Task 4 (the stage check and its tests), Task 5 (the findings), Task 6 (the counts, the `py_compile` lists, and the docs) |
| SM-5 | Tasks 4 and 5 (the score gate and the measured score) |
| Slice 7 Done when | Tasks 5 and 6 (the image exists, the stage is not a skip, the findings and the roadmap record the score) |

RC-9 (the whole-image gate is not exercised) is the reason for this slice. It
is not a requirement ID, so it is not a row in the table; Tasks 3 to 5
exercise the gate.

FR-014 is the Slice 4 library requirement. Task 3 keeps the original library
bytes, and Task 5 checks the library manifest. The Slice 7 requirements are
FR-017 and FR-018; the FR-014 row records the boundary.

## Facts Checked

The code and the shipped artifacts agree with these facts.

- `STARTH.EXE` is 1,034,923 bytes (0xfcaab). The MZ stub is 0x0 to 0x2a50.
  The LE header is at 0x2a50, the page size is 0x1000, there are 227 pages,
  and the data pages start at 0x1a800.
- Object 1 is code and read-only data at base 0x10000 with virtual size
  0xdba93 and 220 pages. Object 2 is BSS at base 0xf0000 with virtual size
  0x82 and 0 pages. Object 3 is stack and initial data at base 0x100000 with
  virtual size 0x472c0 and 7 pages.
- The original holds 11,259 fixup slots. Every slot is `internal`, source type
  7 (a 32-bit offset), with width 4. A fixup-record scan of the original image
  counts 11,259 source-type-7 records and 11,259 slots. The image holds no
  import or entry slot and no non-resident name table.
- The original function records hold `address`, `size`, `module`, and
  `symbol`. The module recipes hold `sources`, `exe`, `map`, and `toolchain`
  (default `v2`), with an optional `run`. The project holds no original `.obj`
  module and no data pipeline. The in-place build keeps the original data
  objects and library bytes, so the data pipeline is not needed.
- `tools/rebuild/library.json` holds `libraries` as a dict of the 3 library
  files and their digests and `entries` as a dict of 138 entries. Each entry
  holds `file`, `sha256`, `module`, `symbol`, and `object_sha256`.
- The store holds 252 module recipes and 251 module items with a symbol
  record. `driver` has no symbol record, so `module_order` excludes it.
  `exit` has two records: `0x300d6` (`exit_flush_`) and `0x300be` (`exit_`).
  The lowest address, `0x300be`, is the identity record.
- `store.recompute_sizes` defines `record["size"]` as the distance to the next
  entry. It is not the instruction byte sum. At the plan date, 70 of the 251
  module items have an original body whose instruction byte sum equals the
  span. `free` (`0x300ec`) holds a span of 12 and an original body of 9
  instruction bytes. `error_report` (`0x10978`) holds a span of 556 and an
  original body of 554 instruction bytes.
- This is the one contradiction with the task data. The agreed `fit`
  signature compares the rebuilt instruction sum to `record["size"]`, so
  `free` and `error_report` are skipped by the size fit. The plan keeps the
  agreed signature. Task 3 adds `exit` (`0x300be`, span 24, original body 24)
  to the focused build so the task verifies a patched row as well as the two
  skipped rows. The full build patches the modules that fit and skips the rest
  with the recorded reason. The findings record the measured counts.
- `run.build_module` writes its artifacts under `store.WORK_DIR`. The build
  driver reads the rebuilt executable and map from its `work_dir` argument,
  and the command passes `store.WORK_DIR`.
- `diff_module.map_address`, `map_symbols`, `next_symbol`, `original_body`,
  and `rebuilt_body` keep the signatures the design names.
  `image_compare.compare_loaded` returns an `ImageResult` with `equal`,
  `total`, and `exact`.

## Shared Interfaces

- `whole_image.IMAGE_DOCUMENT: Path` is `store.WORK_DIR / "STARTH.EXE"`.
- `whole_image.REPORT_DOCUMENT: Path` is
  `store.WORK_DIR / "whole-image.json"`.
- `whole_image.module_order(modules: list[dict], records: list[dict]) ->
  list[dict]` returns one item per module that names a symbol record. The item
  holds `{"module": module, "record": record}`. The record is the
  lowest-address record whose `module` equals the module name and whose
  `symbol` is present. The items are sorted by the record address. A module
  with no such record is excluded.
- `whole_image.report_row(item: dict) -> dict` returns the base row. The row
  holds `module` (the module name), `address` (the record hex string), `size`
  (the record size), `toolchain` (`store.module_toolchain`), and `sources`
  (the `file` value of each module source). It starts `build` `skip`, `fit`
  false, `fit_reason` `None`, `rebuilt_size` 0, `relocated` 0, and `outcome`
  `skipped`.
- `whole_image.fit(record: dict, rebuilt: list[le.Insn]) -> str | None`
  returns `None` when the sum of the rebuilt instruction sizes equals
  `record["size"]`. Otherwise it returns
  `f"the rebuilt body holds {total} bytes against the recorded {size}"`.
- `whole_image._slots(insn: le.Insn, relocations: list[le.Relocation]) ->
  list[le.Relocation]` returns the slots whose `offset` satisfies
  `insn.address <= offset < insn.address + insn.size`, sorted by offset.
- `whole_image.relocate(original: list[le.Insn], rebuilt: list[le.Insn],
  original_relocations: list[le.Relocation], rebuilt_relocations:
  list[le.Relocation], original_objects: dict[int, int]) -> tuple[bytes |
  None, int]` returns the patched rebuilt body bytes and the number of
  rewritten slots. It returns `(None, 0)` on a different instruction count, a
  different instruction size, a different slot count, a different slot width, a
  different slot relative offset, a non-internal slot, a missing target, an
  unmapped object, a slot outside its instruction, or a slot that belongs to no
  instruction. For each pair the original slot relative offset
  (`original_slot.offset - original_insn.address`) must equal the rebuilt slot
  relative offset. The union of the per-instruction slots must equal the input
  slot lists. The slot lists are the body slots, not the whole-image slots.
- `whole_image.build(original_path: Path, modules: list[dict], records:
  list[dict], work_dir: Path, out: Path, selected: list[str] | None = None) ->
  dict` loads the original, copies its data, orders the modules, builds each
  selected module, reads the original and rebuilt bodies, applies the fit and
  the relocation, writes the patched bytes through
  `LinearExecutable.to_file_offset`, writes the image to `out`, and returns
  the report document. `work_dir` must be `store.WORK_DIR`, because
  `run.build_module` writes the artifacts there. It hashes the original bytes
  before the write and raises `ValueError` when `out.resolve()` equals
  `original_path.resolve()`.
- The report document holds:
  - `date`: `date.today().isoformat()`;
  - `original`: `{"path": str(original_path), "sha256": _sha256(original_path)}`;
  - `image`: `{"path": str(out), "sha256": _sha256(out)}`;
  - `modules`: the report rows;
  - `patched`: the count of rows with `outcome` `patched`;
  - `skipped`: the count of rows with `outcome` `skipped`;
  - `relocated`: the sum of the row `relocated` values;
  - `score`: `{"equal": result.equal, "total": result.total, "exact":
    result.exact}` from `image_compare.compare_loaded(original,
    le.load(out))`.
- The report row holds `module`, `address`, `size`, `toolchain`, `sources`,
  `build`, `fit`, `fit_reason`, `rebuilt_size`, `relocated`, and `outcome`.
  A skipped row also holds `reason`. A patched row holds no `reason`.
  `build` is `pass`, `fail`, or `skip`. `fit` is true only after the fit
  passes. `outcome` is `patched` or `skipped`. A skipped row holds `reason`
  with the same text as `fit_reason` for a fit skip, the shape text for a
  shape skip, and the error text for a build or exception skip.
- `whole_image._load_modules() -> list[dict]` returns
  `store.load_module_file(path)` for every sorted
  `store.MODULES_DIR.glob("*.json")`.
- `whole_image._load_records() -> list[dict]` returns
  `store.load_record(address)` for every `store.candidate_addresses()`.
- `whole_image._sha256(path: Path) -> str` returns the lowercase hex SHA-256
  digest. It raises `ValueError` naming the path when the file is missing.
- `whole_image._write_report(path: Path, document: dict) -> None` creates the
  parent directory and writes the JSON with an indent of 2, sorted keys, and a
  trailing newline.
- `whole_image._last_line(text: str) -> str` returns the last non-empty
  stripped line, or `""`.
- `whole_image.command_build(arguments: argparse.Namespace) -> int` validates
  the original image, loads the modules and records, validates the `--module`
  names, calls `build`, writes `--report`, prints one line per row and a
  summary, and returns 0 when the image is written. It catches
  `(OSError, ValueError, struct.error)`, prints `error: <text>` to stderr, and
  returns 2 on a bad input. It writes no store record.
- `whole_image.parse_arguments() -> argparse.Namespace` adds `--original`
  (default `store.DEFAULT_ORIGINAL`), `--out` (default `IMAGE_DOCUMENT`),
  `--report` (default `REPORT_DOCUMENT`), `--module` (repeatable), and
  `--all`. `--module` and `--all` are mutually exclusive. Neither flag means
  every module.
- `whole_image.main() -> int` follows the `first_difference.py` pattern. It
  catches `(OSError, ValueError, struct.error)` and returns 2.
- `pipeline_check.IMAGE_SCORE_PREFIX` is `"image compare: "`.
- `pipeline_check.check_image(checks: Checks, game_dir: Path, exe: str,
  rebuilt_image: Path) -> None` always adds one check named
  `stage 11: the whole image reports a byte score`. A missing image is a
  failure with `f"no rebuilt image at {rebuilt_image}"`. An unreadable image
  is a failure with the error line. A present image runs
  `image_compare.py`; exit 0 or 1 passes with the score line in the detail, or
  the last line when the score line is absent. Exit 2 fails with the error
  line.

## Tasks

- [x] **Task 1: The module order, the fit rule, and the report row**
    - **Implements:** FR-017
    - Write the tests first.
    - **Create `tools/rebuild/tests/test_whole_image.py`**
        - Insert the `tools` and `tools/rebuild` paths on `sys.path`,
          following `test_first_difference.py`, and import `le` and
          `whole_image`.
        - Add the `insn(address, mnemonic, operands, raw)` factory. It builds
          `le.Insn(address, len(raw), mnemonic, operands, raw)`.
        - Add the `record(address, module, symbol, size)` factory. It returns
          the record fields the tool reads.
        - Add the `module(name, sources, toolchain, exe=None, map=None)`
          factory. It returns `{"name": name, "sources": [{"file": file} for
          file in sources], "toolchain": toolchain, "exe": exe or
          f"{name}.exe", "map": map or f"{name}.map"}`.
        - The tests need no Docker, no DOSBox-X, and no game directory.
        - `ModuleOrderTestCase`
            - `test_module_order_sorts_by_record_address` — three modules with
              records at 0x300ec, 0x10000, and 0x20000 give that address
              order.
            - `test_module_order_takes_the_lowest_record_for_exit` — two
              records for `exit` at 0x300d6 and 0x300be give the 0x300be
              record.
            - `test_module_order_excludes_a_module_without_a_record` — the
              `driver` module gives no item. A module with records that hold
              no symbol also gives no item.
            - `test_module_order_holds_one_item_per_module` — two records for
              one module still give one item.
        - `ReportRowTestCase`
            - `test_report_row_holds_the_module_fields` — the row holds the
              module name, the record address, the record size, the module
              toolchain, and the source file names in source order.
            - `test_report_row_starts_skipped` — `build` is `skip`, `fit` is
              false, `fit_reason` is `None`, `rebuilt_size` is 0, `relocated`
              is 0, and `outcome` is `skipped`.
        - `FitTestCase`
            - `test_fit_accepts_a_body_of_the_recorded_size` — two
              instructions of sizes 2 and 3 against a record size of 5 give
              `None`.
            - `test_fit_rejects_a_short_body` — the reason is
              `"the rebuilt body holds 4 bytes against the recorded 5"`.
            - `test_fit_rejects_a_long_body` — the reason is
              `"the rebuilt body holds 6 bytes against the recorded 5"`.
        - **Mock targets**: none. The tests use plain dicts and `le.Insn`.
    - **Create `tools/rebuild/whole_image.py`**
        - Add the module docstring and the usage block. The usage is
          `python3 tools/rebuild/whole_image.py --all` and
          `python3 tools/rebuild/whole_image.py --module NAME [--module NAME]`.
        - Add `from __future__ import annotations`.
        - Add the standard-library imports `argparse`, `hashlib`, `json`,
          `struct`, `sys`, `date`, and `Path`. Insert the `tools/` path and the
          `tools/rebuild/` path. Import `diff_module`, `image_compare`, `le`,
          `run`, and `store`.
        - Add `IMAGE_DOCUMENT` and `REPORT_DOCUMENT`.
        - Add `module_order`, `report_row`, and `fit` with the signatures in
          Shared Interfaces.
        - `module_order` selects the record with the smallest
          `int(record["address"], 16)` among the records whose `module` equals
          the module name and whose `symbol` is present. It sorts the items by
          that address.
        - `report_row` calls `store.module_toolchain(module)` and lists
          `source["file"]` in module source order.
        - `fit` sums `insn.size` over the rebuilt instructions and compares
          the sum to `record["size"]`.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_whole_image.py"`
        - `python3 -m py_compile tools/rebuild/whole_image.py`

- [x] **Task 2: The relocation**
    - **Implements:** FR-017
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_whole_image.py`**
        - `RelocateTestCase`
            - `test_relocate_writes_the_original_target` — an original
              6-byte instruction at 0x300ec holds a width-4 internal slot at
              0x300ee for object 1, target 0x20. The rebuilt 6-byte
              instruction at 0x10000 holds a width-4 internal slot at
              0x10002. With `{1: 0x10000}` the patched bytes at the rebuilt
              slot hold `0x10020` in little-endian order, and the count is 1.
            - `test_relocate_counts_each_slot` — two slots in one instruction
              give a count of 2.
            - `test_relocate_returns_none_on_a_different_instruction_count` —
              two original instructions and one rebuilt instruction give
              `(None, 0)`.
            - `test_relocate_returns_none_on_a_size_mismatch` — a 5-byte
              original instruction and a 4-byte rebuilt instruction give
              `(None, 0)`.
            - `test_relocate_returns_none_on_a_slot_count_mismatch` — one
              original slot and no rebuilt slot give `(None, 0)`.
            - `test_relocate_returns_none_on_a_non_internal_slot` — an
              original import slot or a rebuilt import slot gives
              `(None, 0)`.
            - `test_relocate_returns_none_on_a_non_four_byte_slot` — a
              width-2 slot on either side gives `(None, 0)`.
            - `test_relocate_returns_none_for_an_unmapped_object` — an
              original slot whose object is absent from `original_objects`
              gives `(None, 0)`.
            - `test_relocate_returns_none_on_a_different_slot_offset` — the
              original slot at relative offset 2 and the rebuilt slot at
              relative offset 3 give `(None, 0)`.
            - `test_relocate_returns_none_on_an_unassigned_slot` — an input
              slot that falls in no instruction gives `(None, 0)`.
            - `test_relocate_keeps_a_relative_branch` — a `jne` with raw
              `75 10` and no slot returns the same bytes and a count of 0.
        - **Mock targets**: none. The tests build `le.Insn` and
          `le.Relocation` values directly.
    - **Modify `tools/rebuild/whole_image.py`**
        - Add `_slots` and `relocate` with the signatures in Shared
          Interfaces.
        - `relocate` requires the same instruction count. For each pair it
          requires the same instruction size, the same slot count, and the same
          slot relative offset. It requires every original and rebuilt slot to
          be `internal` with width 4, an original target, and an original
          object in `original_objects`. The original slot must satisfy
          `original_slot.offset + 4 <= original_insn.address +
          original_insn.size`. The union of the per-instruction slot lists must
          equal the input slot lists.
        - The relative offset is `rebuilt_slot.offset -
          rebuilt_insn.address`. It must satisfy `0 <= offset` and
          `offset + 4 <= len(rebuilt_insn.raw)`.
        - The written value is
          `(original_objects[original_slot.object] +
          original_slot.target).to_bytes(4, "little")`.
        - The function copies each rebuilt `raw` to a `bytearray`, writes the
          slots, and concatenates the results. A direct branch target is not
          a slot and is not rewritten.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_whole_image.py"`
        - `python3 -m py_compile tools/rebuild/whole_image.py`

- [x] **Task 3: The build driver, the report, and the command**
    - **Implements:** FR-014, FR-017, FR-018
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_whole_image.py`**
        - `BuildTestCase`
            - `test_build_writes_the_patched_body` — a fake original of 16
              zero bytes, a fake rebuilt body of one 4-byte instruction at
              0x10000, and a fake original body of one 4-byte instruction at
              0x10000 write the patched bytes at file offset 0. The image
              file holds the patched bytes and 12 trailing zeros.
            - `test_build_patches_the_original_slot_target` — the original
              slot targets object 1 offset 0x20 and the rebuilt slot holds
              another value. The written image holds `0x10020` in
              little-endian order, and the row holds `relocated` 1.
            - `test_build_skips_a_failed_build` — `run.build_module` returns
              `("fail", "boom")`. The row holds `build` `fail`, `outcome`
              `skipped`, and the reason `boom`; `patched` is 0 and `skipped`
              is 1.
            - `test_build_keeps_the_original_bytes_on_a_failed_build` — the
              image file equals the fake original data.
            - `test_build_skips_a_size_mismatch` — the rebuilt body holds 4
              bytes against a record size of 5. The row holds `fit` false,
              `fit_reason` and `reason` with the same text, `outcome`
              `skipped`, and `rebuilt_size` 4.
            - `test_build_skips_a_shape_misalignment` — the rebuilt body
              holds the record size and the instruction counts differ. The row
              holds `fit` true and `reason`
              `"the rebuilt body does not align with the original body"`.
            - `test_build_selected_restricts_the_modules` — a `selected` list
              of one name gives one row.
            - `test_build_report_holds_the_date_and_the_digests` — the date
              is `date.today().isoformat()`, `original.path` is the original
              path, `original.sha256` is the lowercase digest, and
              `image.path` and `image.sha256` name the written image.
            - `test_build_report_holds_the_score` — a mocked
              `image_compare.compare_loaded` gives `equal` 10, `total` 16,
              and `exact` false. The report score holds those values.
            - `test_build_report_holds_the_module_rows` — the report rows
              hold every row field from Shared Interfaces. A skipped row
              holds `reason`; a patched row does not.
            - `test_build_rejects_the_original_as_the_output` — an `out`
              that resolves to the original path raises `ValueError` before
              the write.
        - The Build tests use the `module` factory, which supplies `exe` and
          `map`, so the mocked build reads the expected artifact paths.
        - `CommandBuildTestCase`
            - `test_command_build_writes_the_image_and_the_report` — a mocked
              `build` gives a document and records its `out` argument. The
              command passes `arguments.out` to `build`, writes the report
              JSON, and returns 0.
            - `test_command_build_prints_a_summary` — the output holds one
              line per row and the summary line with the patched, skipped,
              relocated, and byte counts.
            - `test_command_build_rejects_a_missing_original` — a missing
              original returns 2 before a build.
            - `test_command_build_rejects_an_unknown_module` — a name that is
              not a module returns 2.
            - `test_parse_arguments_selects_all_by_default` — no flag gives
              `module` `None` and `all` false. `--all` gives `all` true.
            - `test_module_and_all_are_mutually_exclusive` — `--module free
              --all` exits 2 from `argparse`.
        - **Mock targets** keep the tests pure:
            - `mock.patch.object(whole_image, "le")` is not used. Patch
              `whole_image.le.load` with a side effect that returns the fake
              original, the fake rebuilt image, and the fake written image.
            - Patch `whole_image.run.build_module`.
            - Patch `whole_image.diff_module.map_address`, `map_symbols`,
              `rebuilt_body`, and `original_body`.
            - Patch `whole_image.image_compare.compare_loaded`.
            - The command tests patch `whole_image.build`,
              `whole_image._load_modules`, `whole_image._load_records`, and
              `whole_image.le.load`.
    - **Modify `tools/rebuild/whole_image.py`**
        - Add `_load_modules`, `_load_records`, `_sha256`, `_write_report`,
          `_last_line`, `build`, `command_build`, `parse_arguments`, and
          `main`.
        - `build` loads the original with `le.load`, copies its data to a
          `bytearray`, hashes the original bytes, reads
          `original.relocations()` once, and builds
          `original_objects = {obj.index: obj.reloc_base}`. It raises
          `ValueError` when `out.resolve()` equals `original_path.resolve()`.
        - `build` applies `module_order` and then the `selected` filter. For
          each item it calls `run.build_module(module)`. A verdict other than
          `pass` writes `build` `fail` or `skip` and a reason: the last line
          of the output, else `f"the {toolchain} build failed"` or
          `f"the {toolchain} engine is missing"`.
        - A passing build loads `work_dir / module["exe"]`. It reads the
          rebuilt address with `diff_module.map_address(work_dir /
          module["map"], record["symbol"])`, `rebuilt.object_by_index`, and
          `obj.reloc_base + offset`. It reads the rebuilt symbols with
          `diff_module.map_symbols`, the extent with
          `diff_module.next_symbol`, and the rebuilt body with
          `diff_module.rebuilt_body(rebuilt, rebuilt_address, None,
          record.get("count", 0), extent=extent)`. It reads the original body
          with `diff_module.original_body(original, address, address +
          record["size"], record.get("count", 0))`.
        - The row `rebuilt_size` is the sum of the rebuilt instruction sizes.
          The fit runs next. A fit reason writes `fit` false, the fit reason to
          both `fit_reason` and `reason`, and `outcome` `skipped`.
        - The relocation receives the original slots whose offsets fall in
          the original body and the rebuilt slots whose offsets fall in the
          rebuilt body. A `(None, 0)` result writes `fit` true and the reason
          `"the rebuilt body does not align with the original body"` to
          `reason`.
        - The original file offset is `original.to_file_offset(address)`. A
          `None` offset writes the reason
          `"the original address is not in a file page"` to `reason`.
        - A patched row writes the bytes at the file offset, sets
          `relocated`, sets `outcome` `patched`, and removes `reason`.
        - A `(OSError, ValueError, KeyError)` inside one module writes
          `outcome` `skipped`, the exception text to `reason`, and keeps the
          original bytes. The loop continues with the next module.
        - After the loop, `build` creates the output parent, writes the
          image, computes the score with
          `image_compare.compare_loaded(original, le.load(out))`, and returns
          the report document. The original digest is the pre-write hash.
        - `command_build` validates `arguments.original.is_file()`, calls
          `le.load` to validate the image, loads the modules and records,
          validates every `--module` name, and calls
          `build(arguments.original, modules, records, store.WORK_DIR,
          arguments.out, arguments.module or None)`. It writes
          `arguments.report` and prints one line per row:
          `"<module>: patched, <n> slots relocated"` or
          `"<module>: skipped, <reason>"`. The summary is
          `"<patched> patched, <skipped> skipped, <relocated> slots
          relocated, <equal> of <total> bytes equal"`, then the image path and
          the report path. It returns 0 when the image is written. It catches
          `(OSError, ValueError, struct.error)`, prints `error: <text>` to
          stderr, and returns 2 on a bad input.
        - `parse_arguments` uses a mutually exclusive group for `--module`
          (`action="append"`) and `--all` (`action="store_true"`).
        - `main` calls `command_build` and returns its code.
    - **Modify `tools/pipeline_check.py`**
        - Add `"rebuild/whole_image.py"` to `TOOLS` after
          `"rebuild/first_difference.py"`.
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - `ToolListTestCase.test_tools_include_the_whole_image_tool` —
          `TOOLS` holds `rebuild/whole_image.py`.
    - **Steps:**
        - Run the focused real build with the three named modules:
          `python3 tools/rebuild/whole_image.py --module free --module error_report --module exit --out work/rebuild/STARTH-focused.EXE --report work/rebuild/whole-image-focused.json`.
        - The current store gives one patched row for `exit` (24 bytes) and
          two skipped rows: `free` (9 against 12) and `error_report` (554
          against 556). Print the rows. If a row differs, record the measured
          row and the reason; do not force the fit.
        - Run
          `python3 tools/rebuild/image_compare.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt work/rebuild/STARTH-focused.EXE ; test $? -le 1`.
        - Confirm that the report holds three rows, that
          `patched + skipped == 3`, and that at least one row is patched.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_whole_image.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_pipeline_check.py"`
        - `python3 -m py_compile tools/rebuild/whole_image.py tools/pipeline_check.py`
        - `python3 tools/rebuild/whole_image.py --module free --module error_report --module exit --out work/rebuild/STARTH-focused.EXE --report work/rebuild/whole-image-focused.json`
        - `python3 tools/rebuild/image_compare.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt work/rebuild/STARTH-focused.EXE ; test $? -le 1`

- [x] **Task 4: The stage 11 score gate**
    - **Implements:** FR-017, FR-018
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - Change the `ImageGateTestCase` docstring to: "Stage 11 must score
          the rebuilt image and fail only when it cannot read it."
        - Replace `test_missing_image_is_a_skip` with
          `test_a_missing_image_is_a_counted_failure`. It passes a missing
          path and asserts one failed result with the detail
          `f"no rebuilt image at {path}"`, one entry in `checks.failures`, and
          the check name
          `stage 11: the whole image reports a byte score`.
        - Keep `test_present_image_runs_the_comparison`. The mocked run
          returns 0 with `image compare: 20 of 20 bytes equal` and a verdict
          line. The test asserts one passing result, the detail is the score
          line, the command holds `image_compare.py`, and the check name is
          `stage 11: the whole image reports a byte score`.
        - Replace `test_a_differing_image_fails` with
          `test_a_differing_image_passes_with_its_score`. The mocked run
          returns 1 with `image compare: 10 of 20 bytes equal` and a differing
          verdict. The test asserts one passing result, the score line in
          the detail, and the check name
          `stage 11: the whole image reports a byte score`.
        - Add `test_an_unreadable_image_fails`. The file exists and the
          mocked run returns 2 with `error: not an MZ image` on stderr. The
          test asserts one failed result with that error line in the detail
          and the check name `stage 11: the whole image reports a byte score`.
    - **Modify `tools/pipeline_check.py`**
        - Add `IMAGE_SCORE_PREFIX = "image compare: "`.
        - Replace the `check_image` body with the always-counted score gate
          in Shared Interfaces. The check name is exactly
          `stage 11: the whole image reports a byte score`.
        - The detail is the first line of `result.stdout` that starts with
          `IMAGE_SCORE_PREFIX`, else `last_line(result.stdout)`, else
          `last_line(result.stderr)`.
        - A missing image calls
          `checks.check(name, False, f"no rebuilt image at {rebuilt_image}")`.
          An exit code other than 0 or 1 is a failure with the error line.
          Exit 0 and exit 1 are passes.
        - Update the docstring: the stage always scores the image, a missing
          or unreadable image fails, and the exact oracle remains the tool's
          exit code.
    - **Steps:**
        - Run the focused pipeline check with the Task 3 image:
          `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross --rebuilt-image work/rebuild/STARTH-focused.EXE`.
        - Confirm the run reports 414 checks and 0 failures.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_pipeline_check.py"`
        - `python3 -m py_compile tools/pipeline_check.py`
        - `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross --rebuilt-image work/rebuild/STARTH-focused.EXE`

- [x] **Task 5: The full image build and the score**
    - **Implements:** FR-014, FR-017, SM-5
    - This is a data task. It changes no tool code.
    - **Steps:**
        - Run the full build:
          `python3 tools/rebuild/whole_image.py --all`.
        - Run the exact oracle:
          `python3 tools/rebuild/image_compare.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt work/rebuild/STARTH.EXE ; test $? -le 1`.
        - Run
          `python3 tools/rebuild/library.py check-manifest --manifest tools/rebuild/library.json`
          to confirm that the 138 entries resolve to their named objects. The
          in-place build keeps those original library bytes.
        - Read the report `work/rebuild/whole-image.json`. Record the
          `patched`, `skipped`, `relocated`, and `score` values and the
          region scores from the image compare.
        - Assert the report shape with a read-only command: the `modules`
          list holds 251 rows, `patched + skipped` is 251, `relocated` equals
          the sum of the row values, `score.total` is 1,034,923, and the image
          exists with the `image.sha256` digest.
        - Group the skipped rows by `build` and by `fit` and record the first
          reason of each group. Record the measured group sizes. The fit group
          holds the modules whose rebuilt instruction sum differs from the
          record span.
    - **Modify `docs/explanation/findings.md`**
        - Add the section `## The whole-image build and the score` before
          `## Open questions`.
        - Record the original image facts, the 251 module items, the module
          order rule, the `driver` exclusion, and the `exit` identity record.
        - Record the measured `patched`, `skipped`, and `relocated` counts
          and the skipped groups with their reasons.
        - Record the fit-span observation with the `free` and
          `error_report` values and the measured count of module items whose
          instruction sum equals the span.
        - Record the score line and the region scores. The score is a byte
          score, not a correctness measure.
        - Record the open question: whether the fit should compare the
          record span, the original instruction byte sum, or the rebuilt body
          span; whether an external direct branch target must be rewritten for
          a runnable image; and whether an absolute operand that holds no fixup
          slot must be rewritten. State that the current plan follows the
          agreed in-place rule and the agreed `fit` signature, and that the
          image is a byte-score artifact, not a proven runnable image.
        - Record the evidence commands. Do not commit the image or the
          report.
    - **Verify:**
        - `python3 tools/rebuild/whole_image.py --all`
        - `python3 tools/rebuild/image_compare.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt work/rebuild/STARTH.EXE ; test $? -le 1`
        - `python3 -c "import hashlib, json, pathlib; report = json.loads(pathlib.Path('work/rebuild/whole-image.json').read_text()); rows = report['modules']; assert len(rows) == 251; assert report['patched'] + report['skipped'] == 251; assert report['relocated'] == sum(row['relocated'] for row in rows); assert report['score']['total'] == 1034923; image = pathlib.Path(report['image']['path']); assert image.is_file(); assert hashlib.sha256(image.read_bytes()).hexdigest() == report['image']['sha256']"`
        - `grep -q '## The whole-image build and the score' docs/explanation/findings.md`

- [x] **Task 6: The documentation and the counts**
    - **Implements:** FR-018, SM-5, Slice 7 Done when
    - **Modify `AGENTS.md`**
        - Project Structure: add
          `│   │   ├── whole_image.py     # build the rebuilt whole image and its report`
          under `tools/rebuild/` after `image_compare.py`.
        - Build And Test Commands: add
          `python3 tools/rebuild/whole_image.py --all` and
          `python3 tools/rebuild/whole_image.py --module free --module error_report --module exit --out work/rebuild/STARTH-focused.EXE --report work/rebuild/whole-image-focused.json`
          to the build block. Add `tools/rebuild/whole_image.py` to both
          `py_compile` lists after `tools/rebuild/first_difference.py`.
        - Full-check prose: change 432 to 433, 428 to 429, 417 to 418, and
          413 to 414. Replace "The image check is a skip until a whole image
          is built, so it does not change these counts" with: stage 11 always
          scores the rebuilt whole image and the four counts include its
          check. State that a missing image is a failure and that
          `python3 tools/rebuild/whole_image.py --all` runs before the full
          check.
        - Testing: replace the count bullet with 433, 429, 418, and 414 and
          state that the whole-image stage adds one counted check. Add a
          whole-image rule bullet: the tool places the rebuilt module bodies
          at their original addresses, writes
          `work/rebuild/STARTH.EXE` and `work/rebuild/whole-image.json`, and
          writes no store record. A module is patched only when its
          instruction byte sum equals the record `size` and its instruction
          count, sizes, and fixup slots align. A skipped module keeps the
          original bytes and records the reason. Stage 11 always scores the
          image and fails when it is missing or unreadable.
        - Architecture: replace the whole-image bullet with the in-place
          build rule, the fit and relocation rules, the unchanged fixup
          section, the library manifest evidence, and the score gate.
    - **Modify `README.md`**
        - Change the check counts to 433, 429, 418, and 414.
        - Replace "Stage 11 also runs the whole-image oracle when
          `--rebuilt-image` exists, and reports a skip when no whole image is
          built" with the score gate rule and the build command.
        - Add `python3 tools/rebuild/whole_image.py --all` to the command
          block near `image_compare.py`.
    - **Modify `docs/reference/tooling.md`**
        - Add the row
          `| whole_image.py | build the rebuilt whole image from the original image and the rebuilt module bodies, and write its report |`
          after the `image_compare.py` row.
        - Add `python3 tools/rebuild/whole_image.py --all` to the rebuild
          command block.
        - Add the section `## The whole-image build` after
          `## The first-difference report`. Describe the in-place container,
          the module order, the fit rule, the relocation rule, the skipped
          row with its reason, the two output paths, the no-store-record
          rule, the report fields, and the score gate.
        - Update `### tools/pipeline_check.py`: stage 11 always scores the
          image, a missing or unreadable image is a failure, exit 0 or 1
          passes with the score, and exit 2 fails with the error line.
        - Update the four count lines to 433, 429, 418, and 414. Remove the
          sentence that the whole-image check is a skip and does not change
          the counts.
    - **Modify `.github/workflows/pipeline.yml`**
        - Add `tools/rebuild/whole_image.py` to the syntax-job `py_compile`
          list after `tools/rebuild/first_difference.py`.
        - Add a `build the rebuilt whole image` step to the `pipeline` job
          before the `run the pipeline check` step. The step holds the same
          `env: SCREAMER_GAME_DIR: ${{ vars.SCREAMER_GAME_DIR }}` block,
          resolves
          `GAME_DIR="${SCREAMER_GAME_DIR:-$HOME/Documents/Screamer.app/Contents/Resources/game}"`,
          and runs
          `nix develop .#cross-check --command python3 tools/rebuild/whole_image.py --all --original "$GAME_DIR/STARTH.EXE"`.
          Stage 11 fails when the image is missing, so the job must build the
          image first. The cross-check shell, Docker, DOSBox-X, and the period
          Watcom 10.0a toolchain are already available to this job.
    - **Modify `docs/explanation/rebuild-quality-design.md`**
        - Confirm that C-10 holds 433, 429, 418, and 414. Change it only if
          the observed counts differ.
    - **Modify `docs/explanation/findings.md`**
        - Add the four observed counts to the Task 5 section. Do not rewrite
          the historical count lines for the earlier passes.
    - **Modify `docs/explanation/rebuild-quality-roadmap.md`**
        - Set the Slice 7 `Done when` to: a rebuilt `STARTH.EXE` exists. The
          whole-image stage reports a byte score and is not a skip. The build
          patches the modules that fit the record size and skips the rest
          with a recorded reason. The findings hold the measured patched,
          skipped, and byte counts.
        - Do not change the Slice 7 Status or Plan. The execution skill owns
          those fields.
    - **Steps:**
        - Run the unit suite, the store check, the docs check, the call graph
          check, and the status page command. The call graph check needs
          capstone and the original executable.
        - Run the full `py_compile` list in `AGENTS.md`. Both lists include
          `tools/rebuild/whole_image.py`.
        - Run the four pipeline commands in Verify. Record the four observed
          counts. The counts must be 433, 429, 418, and 414. If a count
          differs, stop and report the difference. Do not write a forecast
          count.
        - If the status page holds a `data-stale="true"` marker, re-run the
          stored verification for the affected records and regenerate the
          page before the final check.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests`
        - `python3 -m py_compile tools/rebuild/whole_image.py tools/pipeline_check.py`
        - `python3 tools/rebuild/whole_image.py --all`
        - `python3 tools/rebuild/image_compare.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt work/rebuild/STARTH.EXE ; test $? -le 1`
        - `python3 -c "import hashlib, json, pathlib; report = json.loads(pathlib.Path('work/rebuild/whole-image.json').read_text()); rows = report['modules']; assert len(rows) == 251; assert report['patched'] + report['skipped'] == 251; assert report['relocated'] == sum(row['relocated'] for row in rows); assert report['score']['total'] == 1034923; image = pathlib.Path(report['image']['path']); assert image.is_file(); assert hashlib.sha256(image.read_bytes()).hexdigest() == report['image']['sha256']"`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/docs.py check`
        - `python3 tools/rebuild/callgraph.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/view.py html --out work/rebuild/status.html`
        - `! grep -q 'data-stale="true"' work/rebuild/status.html`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-cross`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime`
        - `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross`

## Contradiction And Risk

- The agreed `fit` rule compares the rebuilt instruction sum to
  `record["size"]`, and the store defines that field as the span to the next
  record. The two named modules in the task, `free` and `error_report`, do not
  fit: 9 against 12 and 554 against 556. The plan keeps the agreed signature
  and adds `exit` to the focused build. The full build patches the modules
  whose rebuilt instruction sum equals the span and skips the rest with the
  reason. The plan-date estimate is 70 of 251, and Task 5 records the measured
  count. This is the largest risk to the value of the image: only the patched
  bodies change, and most bodies keep their original bytes, so the byte score
  stays close to the original image. Task 5 records the measured result, and
  the findings hold the open question.
- A direct branch target is relative and the agreed rule does not rewrite it.
  A branch to a function outside the rebuilt body therefore keeps the rebuilt
  displacement. An absolute operand that holds no fixup slot keeps the rebuilt
  link-time value for the same reason. The image is a byte-score artifact, not
  a proven runnable image. The findings record this limit.
- The CI full check runs stage 11, which now fails when the image is missing.
  The full-check Verify builds the image first. Task 6 adds a
  `build the rebuilt whole image` step to the `pipeline` job before the
  pipeline check, so the self-hosted runner writes
  `work/rebuild/STARTH.EXE` before stage 11 runs. The step needs Docker,
  DOSBox-X, the period Watcom 10.0a toolchain, and the cross-check shell,
  which the job already uses. This adds
  the full-build time to the job.

## Deviations

- The whole-image build skips a record that carries a `library` field before
  the build and the patch. The 11 library-backed identity records keep their
  original bytes and carry the reason "the original library bytes stay". The
  full build gives 13 patched, 238 skipped, 16 relocated, and 1034738 of
  1034923 bytes equal. The focused build gives 0 patched and 3 skipped. The
  earlier run patched 5 library-backed records and changed 32 bytes. The
  design requires the original library bytes to stay, so the fix moved those
  records to the skip group.
- The fit rule patches a module only when its rebuilt body byte sum equals the
  record span. 13 of the 251 modules patch. The findings hold the open
  question about the fit basis, the external branch target, and a slotless
  absolute operand.
- The whole-image build rewrites the module maps, and each map holds a build
  timestamp. The record fingerprints become stale, so the record stage
  re-runs every record. The documented full check and the CI step use
  `--record-timeout 7200`. The full and `--skip-cross` runs take about 65
  minutes.
- The stage 11 gate passes a tool exit of 0 or 1 and fails a missing or
  unreadable image. The exact oracle stays the tool's exit code.
- The documented full-check blocks place the whole-image build before the
  pipeline check. The findings keep the historical counts, and the
  whole-image section holds the new counts.
