# Record Scope and Provenance Implementation Plan

> Execute this plan with the `executing-plans` skill. Each task is a
> checkbox. Mark it done when its changes are implemented and its Verify
> command passes.

- **Date**: 2026-11-12
- **Status**: Implemented
- **Design**: docs/explanation/rebuild-quality-design.md
- **Roadmap**: docs/explanation/rebuild-quality-roadmap.md — slice: scope
- **Depends on**: Slice 1 — baseline
- **Manual prerequisites**: The Ghidra dev shell, Docker, DOSBox-X, and the game directory.
- **Goal**: Give every function record a provenance value, triage the unknown
  records against recorded evidence, and report the provenance counts and the
  byte coverage.

## Constraints

- C-1: Python 3.14.7. Use the standard library and the `ctypes` capstone binding only.
- C-2: Tests use `unittest` under `tools/rebuild/tests/`.
- C-3: A tool takes its configuration from the command line. It reads no environment variable.
- C-4: A tool does not modify the shipped artifacts. It writes only under `work/` or to an explicit output path. `triage.apply` writes a record through `store.save_record`, the only record writer.
- C-5: Prefer C. Use assembly only when C cannot reproduce the original bytes, and record the reason.
- C-6: A harness output name stays within the 8.3 limit.
- C-7: The 10.0a toolchain runs alongside OpenWatcom V2. Do not replace V2.
- C-8: The status ladder stays `unknown`, `not_a_function`, `named`, `rebuilt`, `matching`, `verified`.
- C-9: The identity verdict strings stay unchanged.
- C-10: At the design date, the full check reports 303 checks, `--skip-cross` reports 299, `--skip-runtime` reports 288, and both flags report 284. The plan-date target is 304, 300, 289, and 285. Update these counts when a change adds or removes a check.
- C-11: Do not commit. The user commits.
- The provenance values are `game`, `library`, `compiler_helper`, `not_a_function`, and `data`, in this order.
- A provenance value is a field of a function record. A module record is a build unit and carries no provenance.
- The provenance is a documentation field. It stays out of `store._record_fields` and the fingerprint.
- A `data` or `not_a_function` provenance requires the `not_a_function` status.
- The analysis count is the `unknown` records with provenance `game`. A library or compiler-helper record without a recovered name keeps the `unknown` status and is excluded by provenance.
- The game denominator is a row with an `address`, provenance `game`, and status not `not_a_function`. The match counters stay over every row whose status is not `not_a_function`, so a library record with a result stays in its match bucket.
- Every record holds at least one `finding`, `file`, or `command` evidence item, in the record `evidence` or in its resolved entry. The resolved entry is the explicit record entry or the span that holds the address.
- The evidence document is `tools/rebuild/provenance.json`. The triage tool is `tools/rebuild/triage.py`. The coverage report is `tools/rebuild/coverage.py`.
- The `coverage` value is a float in `0..1`.
- The report byte coverage uses the code-object virtual size from the original image. The status page uses the covered byte total, because it renders without the image.
- The plan-date counts are 971 function records, 688 `unknown`, 31 `named`, 230 `rebuilt`, 22 `verified`, 0 `matching`, and 252 module records. The design's counts (935 function records, 30 `verified`, 652 `unknown`, 31 `named`, 221 `rebuilt`, 1 `matching`) are historical. No tool hardcodes a count.
- This plan adds one stage-12 check. The target full-check counts are 304, 300, 289, and 285.
- `triage.py` and `coverage.py` are not `store.TOOL_FILES`. They do not steer a build, identity, or behavior check.

## Spec Coverage

| Requirement | Task |
| --- | --- |
| FR-004 | Task 5 (the match counters stay over every row with a result) |
| FR-010 | Task 1, Task 3 |
| FR-011 | Task 2, Task 3 |
| FR-012 | Task 4, Task 5 (the game denominator) |
| FR-018 | Task 6 |

## Shared Interfaces

- `store.PROVENANCE: tuple[str, ...]` holds the five values in the design order.
- `store.provenance_of(record: dict) -> str` returns `record.get("provenance") or "game"`. A module record takes `game`.
- `store.non_entry_reason(image: le.LinearExecutable, address: int) -> str | None` returns `sits inside a prologue`, `sits in alignment padding`, or `sits inside a jump`, or `None`.
- `store.non_entry_addresses(image: le.LinearExecutable) -> list[tuple[int, str]]` uses `non_entry_reason`.
- `triage.EVIDENCE_DOCUMENT: Path` is `store.REBUILD_DIR / "provenance.json"`.
- `triage.load_evidence(path: Path) -> dict` returns `{"spans": list[dict], "records": dict[str, dict]}`. It validates only the top-level keys, the address fields, and the list and dict types. It does not validate a missing `reason`, a missing `provenance`, or an evidence item field; `check` reports those.
- `triage.resolve(address: int, evidence: dict) -> dict | None` returns the explicit record entry, else the span that holds the address, else `None`.
- `triage.is_data(image: le.LinearExecutable, record: dict) -> bool` returns true when the first instruction at the record address does not decode.
- `triage.propose(record: dict, image: le.LinearExecutable, evidence: dict) -> dict` returns `{"provenance": str, "reason": str, "evidence": list[dict]}`.
- `triage.apply(records: list[dict], evidence: dict, image: le.LinearExecutable) -> int` runs only after a clean `check`. It writes a provenance and the evidence only when the record `evidence` or its resolved entry holds a `finding`, `file`, or `command` item. Otherwise it leaves the record unchanged. It writes through `store.save_record`. It returns the updated count.
- `triage.check(records: list[dict], evidence: dict, image: le.LinearExecutable, start: int | None = None, end: int | None = None) -> list[str]` returns one message per violation. `start` is inclusive and `end` is exclusive. A bounded check ignores a record outside the range and an entry or a span that does not intersect the range.
- `triage.item_violations(item: dict, root: Path) -> list[str]` returns one message per bad evidence item. It reports a missing `kind` or `text`, an invalid `kind` against `store.EVIDENCE_KINDS`, a `finding` heading against `store._finding_headings(root)`, and a `file` against `(root / text).exists()`. `check` passes `store.REPOSITORY_ROOT` as `root`.
- `coverage.record_spans(records: list[dict]) -> dict[str, int]` sorts the records by address, then returns the byte span per address. The span reaches the next record. The last record uses its `size`, or one byte. A row without an address is skipped.
- `coverage.code_object_size(original: Path) -> int` returns the virtual size of object 1.
- `coverage.provenance_rows(records: list[dict], code_size: int | None) -> list[dict]` maps each record with `store.provenance_of(record)` and returns one row per provenance with the keys `provenance`, `count`, `unknown`, `bytes`, `coverage`, `verified_bytes`, `rebuilt_bytes`, and `exact_bytes`. `verified_bytes` is the sum of the span bytes of the records with status `verified`, `rebuilt_bytes` is the sum for status `rebuilt`, and `exact_bytes` is the sum of the `bytes_equal` values of the identity results. `coverage` is a float in `0..1`. The denominator is `code_size` when it is not `None`, else the sum of the row `bytes`. A zero denominator gives `coverage` `0.0`; this includes a zero `code_size` and a zero byte total.
- `coverage.format_report(rows: list[dict], code_size: int | None) -> str` returns the text table and the total line. The total line is `total: N records, M unknown, B bytes, C% coverage, V verified bytes, R rebuilt bytes, E exact bytes`. `B` is the sum of the row `bytes`. `C` is the coverage `B / code_size`, printed as a percentage. When `code_size` is `None`, `C` is the coverage `B / sum(bytes)`, printed as a percentage. A zero denominator, including `code_size` 0, prints `0%`.
- `view._provenance_panel(records: list[dict]) -> str` renders the panel from `coverage.provenance_rows(records, None)`. The records are function records, because a module record carries no provenance.
- `view._counted_rows(rows: list[dict]) -> list[dict]` keeps the rows whose status is not `not_a_function`. The match counters use it, so a library or compiler-helper record with a result stays in its match bucket.
- `view._denominator_rows(rows: list[dict]) -> list[dict]` keeps a row with an `address`, provenance `game`, and status not `not_a_function`. The progress bar and the header use it. A module row has no `address`, so it stays out.
- `view._map_sizes(records: list[dict]) -> tuple[dict[str, int], int, int]` uses `coverage.record_spans` for the size map. `low` is the minimum address and `span` is `max(max(address + size) - low, 1)`.

## Tasks

- [x] **Task 1: Provenance in the contract and the store**
    - **Implements:** FR-010
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_store.py`**
        - Add `"provenance": "game"` to the `record` fixture.
        - Add `"provenance": "library"` to the `full_record` fixture.
        - `test_schema_rejects_a_missing_provenance` — a record without the field gives one schema violation that names `provenance`.
        - `test_schema_rejects_an_unknown_provenance` — the value `pascal` gives one schema violation that names `provenance`.
        - `test_validate_record_data_provenance_requires_not_a_function_status` — a record with `data` and `unknown` gives one violation that names `not_a_function`.
        - `test_validate_record_not_a_function_provenance_requires_not_a_function_status` — the same rule for `not_a_function`.
        - `test_validate_record_library_provenance_accepts_any_status` — a record with `library` and `unknown` gives no violation.
        - `test_fingerprint_ignores_documentation_fields` — add `"provenance": "library"` to the documented record and assert the same fingerprint.
        - `test_seed_writes_the_game_provenance` — a seed of a new candidate writes `provenance` `game`.
        - `test_parse_arguments_accepts_the_provenance` — the `set` command parses `--provenance library`.
        - `test_set_writes_the_provenance` — `store.command_set` writes the parsed value.
        - `test_non_entry_reason_returns_the_prologue_reason` — the `Image` of the non-entry test gives `sits inside a prologue` at `0x105`.
        - `test_non_entry_reason_returns_none_for_an_entry` — the same image gives `None` at `0x130`.
        - `test_no_game_record_sits_outside_an_entry` — every game record sits inside an entry, except the records whose provenance is `not_a_function` or `data`, or whose status is `not_a_function`. The docstring states the exception.
    - **Modify `tools/rebuild/contracts/record.schema.json`**
        - Add `provenance` to `required`.
        - Add the `provenance` property with the enum `game`, `library`, `compiler_helper`, `not_a_function`, and `data`. The description names the value as the origin class of the record.
    - **Modify `tools/rebuild/store.py`**
        - Add `PROVENANCE` in the design order.
        - Add `provenance_of(record) -> str`.
        - Add `non_entry_reason(image, address) -> str | None` with the three forms. Refactor `non_entry_addresses` to use it.
        - `seed` writes `"provenance": "game"` on a new record.
        - `command_set` writes `record["provenance"]` when the argument is present.
        - `parse_arguments` adds `--provenance` with `choices=PROVENANCE`.
        - `_status_violations` adds the rule: a `data` or `not_a_function` provenance requires the `not_a_function` status.
        - `command_check` skips a non-entry violation when the record provenance is `not_a_function` or `data`, or the status is `not_a_function`.
        - Update the module docstring and the usage block for `--provenance`.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_store.py"`
    - Do not run `store.py check` before Task 3. The shipped records have no provenance yet.
    - Pipeline stage 12 fails until Task 3 writes the provenance. No task runs the pipeline before Task 3.

- [x] **Task 2: The triage command and the evidence document**
    - **Implements:** FR-011
    - Write the tests first.
    - **Create `tools/rebuild/tests/test_triage.py`**
        - Add a temporary store with the `store.FUNCTIONS_DIR` patch, and a temporary evidence file under `tempfile.TemporaryDirectory`.
        - `test_load_evidence_reads_the_spans_and_records`
        - `test_load_evidence_normalizes_the_addresses` — `0x0D1F25` becomes `0xd1f25`.
        - `test_load_evidence_rejects_a_bad_span` — a `from` value that is not a hexadecimal address.
        - `test_load_evidence_rejects_a_missing_file`
        - `test_load_evidence_rejects_broken_json`
        - `test_load_evidence_rejects_a_missing_top_level_key` — the document has `spans` but no `records`, or the reverse.
        - `test_load_evidence_accepts_an_entry_without_a_reason` — the entry loads, and `check` reports the missing reason.
        - `test_resolve_prefers_the_record_entry`
        - `test_resolve_uses_a_span`
        - `test_resolve_returns_none_outside_the_spans`
        - `test_is_data_for_an_empty_decode` — mock `le.decode` to return `[]`.
        - `test_is_data_is_false_for_a_decoded_instruction` — mock `le.decode` to return one instruction.
        - `test_propose_uses_the_record_entry`
        - `test_propose_uses_the_span`
        - `test_propose_marks_a_non_entry_address` — mock `store.non_entry_reason`.
        - `test_propose_marks_a_data_record` — mock `triage.is_data`.
        - `test_propose_defaults_to_game`
        - `test_check_reports_a_non_game_record_without_a_resolved_entry`
        - `test_check_reports_a_record_without_evidence` — a game record whose record `evidence` and resolved entry hold no `finding`, `file`, or `command` item.
        - `test_check_accepts_a_game_record_with_record_evidence` — a game record with one valid `finding` item in its own `evidence` gives no violation.
        - `test_check_reports_a_stale_entry` — an entry names no record.
        - `test_check_reports_a_span_with_no_record`
        - `test_check_reports_an_invalid_provenance`
        - `test_check_reports_a_missing_provenance` — an entry without the field gives a violation.
        - `test_check_reports_a_missing_reason`
        - `test_check_reports_a_missing_evidence_item_field` — an item without `kind` or without `text` gives a violation.
        - `test_check_reports_a_note_only_entry` — the evidence holds no `finding`, `file`, or `command` item.
        - `test_check_reports_a_finding_heading_not_in_findings_md`
        - `test_check_reports_a_missing_evidence_file`
        - `test_check_reports_an_invalid_evidence_kind`
        - `test_check_reports_an_overlapping_span`
        - `test_check_reports_a_proposal_without_a_resolved_entry` — a game record whose proposal is `data` and which has no resolved entry.
        - `test_check_reports_a_status_mismatch` — a `data` record with status `unknown`.
        - `test_check_limits_the_address_range` — a violation in a record or an entry outside `--from`/`--to` gives no message.
        - `test_apply_writes_the_entry_provenance`
        - `test_apply_writes_the_default_game_provenance` — a record without the field gets `game` when its record `evidence` holds a `finding`, `file`, or `command` item.
        - `test_apply_keeps_a_game_record_with_evidence` — a game record with record evidence keeps `game` and gains the entry evidence.
        - `test_apply_leaves_a_record_without_evidence_unchanged` — a record with no actionable evidence item stays unchanged.
        - `test_apply_leaves_a_note_only_entry_unchanged` — an explicit entry with only a `note` item leaves the record unchanged.
        - `test_apply_returns_the_updated_count`
        - `test_apply_appends_the_evidence`
        - `test_apply_sets_the_not_a_function_status`
        - `test_apply_is_idempotent` — a second apply adds no duplicate evidence.
        - `test_apply_does_not_change_the_fingerprint` — the stored `result.fingerprint` stays.
        - `test_command_apply_requires_a_clean_check` — a record with actionable record evidence and a note-only resolved entry makes `check` report the note-only entry; `command_apply` writes nothing and exits 1.
    - **Create `tools/rebuild/triage.py`**
        - Add the module docstring and the usage block for `propose`, `check`, and `apply`. The usage block names the optional `--from` and `--to` on `check`.
        - Add `EVIDENCE_DOCUMENT`.
        - Add `load_evidence(path)`. It raises `ValueError` with the path on a missing file, broken JSON, a missing top-level key, or a structurally bad entry. It validates only the top-level keys, the address fields, and the list and dict types. It does not validate a missing `reason`, a missing `provenance`, an evidence item field, or the provenance value; `check` reports those.
        - Add `resolve(address, evidence)`. The explicit record entry wins. The span that holds the address is the entry. The `check` rejects an overlapping span.
        - Add `is_data(image, record)`. It returns true when `le.decode(image, address, 1)` is empty or raises `ValueError`.
        - Add `propose(record, image, evidence)`. The order is the non-entry reason, the data rule, the evidence entry, then `game`.
        - Add `apply(records, evidence, image)`. It runs only after a clean `check`. It writes a provenance only when the record `evidence` or its resolved entry holds a `finding`, `file`, or `command` item. Otherwise it leaves the record unchanged. When it writes, it writes the entry provenance, else `game`, and appends the evidence items that are absent. It sets the `not_a_function` status for `data` and `not_a_function`. It saves only a changed record through `store.save_record` and returns the updated count.
        - Add `check(records, evidence, image, start=None, end=None)`. It reports an overlapping span, a stale entry, an invalid or missing provenance, a missing reason, a missing evidence item field, a note-only entry, a span with no record, a non-game record with no resolved entry, a record without a `finding`, `file`, or `command` item in its record `evidence` or its resolved entry, a proposal that is not `game` and has no resolved entry, and a `data` or `not_a_function` record without the `not_a_function` status. It reports an invalid `kind`, a `finding` heading that is not in `findings.md`, and a `file` that does not exist. `start` is inclusive and `end` is exclusive. A bounded check ignores a record outside the range and an entry or a span that does not intersect the range.
        - Add `item_violations(item, root)`. It reports a missing `kind` or `text`, an invalid `kind` against `store.EVIDENCE_KINDS`, a `finding` heading against `store._finding_headings(root)`, and a `file` against `(root / text).exists()`. `check` calls it with `store.REPOSITORY_ROOT` for every item in the record `evidence` and every entry in the evidence document.
        - Add `command_propose`. It reads the evidence document, then writes the existing spans and one entry per non-game proposal to `--out`. The default `--out` is `work/rebuild/provenance-draft.json`.
        - Add `command_check` and `command_apply`. Both read the evidence document and the original image. `command_check` passes `--from` and `--to` to `check`. `command_apply` runs `check` first and exits 1 without writing when it reports a violation.
        - Add `parse_arguments` and `main`. `--original` defaults to `store.DEFAULT_ORIGINAL`. `--evidence` defaults to `EVIDENCE_DOCUMENT`. The `check` subcommand adds `--from` and `--to`. Exit code 0 on success, 1 on a check violation, and 2 on a bad input.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_triage.py"`

- [x] **Task 3: The provenance evidence pass**
    - **Implements:** FR-010, FR-011
    - No new unit test. This task writes data. The Verify commands prove the data.
    - This evidence pass is a human review of every record. The `game` records must cite a call site, a module, or a finding.
    - **Create `tools/rebuild/provenance.json`**
        - The top level holds `spans` and `records`. A span holds `from`, `to`, `provenance`, `reason`, and `evidence`. A record entry holds `provenance`, `reason`, and `evidence`.
        - An evidence item holds `kind` and `text`. The `finding` text is an exact heading of `docs/explanation/findings.md`. The `file` text is an existing repository path. The `command` text is a command.
        - Every record holds at least one `finding`, `file`, or `command` item, either in the record `evidence` or in its resolved entry. A `game` record with such an item needs no entry.
        - Use this entry as the shape:
            ```json
            {
              "spans": [],
              "records": {
                "0xd1f25": {
                  "provenance": "library",
                  "reason": "the original body is byte-identical to the 10.0a CLIB3R.LIB module nmalloc",
                  "evidence": [
                    {"kind": "finding", "text": "## The malloc module at 0xd1f25"},
                    {"kind": "finding", "text": "## The period Watcom 10.0a toolchain"}
                  ]
                }
              }
            }
            ```
    - **Steps:**
        - Create `tools/rebuild/provenance.json` with the top-level keys `spans` and `records`, both empty.
        - Run `python3 tools/rebuild/triage.py propose --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --out work/rebuild/provenance-draft.json`.
        - Read the draft. The draft holds one entry per non-game proposal. A `game` record needs a call site, a module, or a finding in its record `evidence` or in its resolved entry.
        - Read `docs/explanation/findings.md`, `docs/reference/program-map.md`, `tools/rebuild/callgraph.json`, and the record evidence. Use `python3 tools/le.py disasm` and `python3 tools/le.py calls` for the body. Use `nix develop .#ghidra --command sh tools/ghidra/decompile.sh <address>` when the body is unclear.
        - Classify each record in this order:
            - `not_a_function` — `store.non_entry_reason` gives a reason, or the body is a jump thunk or padding.
            - `data` — the first instruction does not decode, or the bytes are a string, a pointer table, or a jump table.
            - `library` — a function of a linked library. The Watcom C run-time, the HMI sound driver, and the FPU emulator are libraries. Use a library symbol, a byte match, or a finding.
            - `compiler_helper` — a routine that the compiler or linker emits. The startup code, the main invoker, the DPMI entry, the FPU truncate helper, and the `__CHP` and `__FPE` helpers are compiler helpers.
            - `game` — a function of the game source. Use a call from a named game function, a game data reference, or a finding.
        - Add a span only when a contiguous region shares one provenance and the boundary records support it. Add a record entry for every other non-game record. Make sure every `game` record has a `finding`, `file`, or `command` item in its record `evidence` or its resolved entry. A record entry wins over a span.
        - Work one address range at a time. Each subtask below owns one range. `--from` is inclusive and `--to` is exclusive.
        - A span may cross a range boundary. Add it once. Each bounded check considers a span that intersects its range.
        - Run the bounded check after every edit in the range. Do not run `store.py check` before the final pass, because the records outside the range have no provenance yet.
        - After the subtasks, run the unbounded `triage.py check`. Fix every message. Run `apply` only after this check is clean; `command_apply` enforces the rule.
        - Run `python3 tools/rebuild/triage.py apply --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
        - Run `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
        - Run `python3 tools/rebuild/store.py list --status unknown | wc -l`. Record the after-unknown count. This is not the analysis count. The analysis count is the `unknown` records with provenance `game`. The Task 4 `coverage.py report` prints the analysis count in the game row.
    - **Subtasks:**
        - [x] **Task 3.1: The records from 0x10000 to 0x1ffff** (200 records)
            - Add the spans and the record entries for the range. Make sure every `game` record has a `finding`, `file`, or `command` item in its record `evidence`, its explicit entry, or its span.
            - **Verify:** `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --from 0x10000 --to 0x20000`
        - [x] **Task 3.2: The records from 0x20000 to 0x2ffff** (224 records)
            - Add the spans and the record entries for the range. Make sure every `game` record has a `finding`, `file`, or `command` item in its record `evidence`, its explicit entry, or its span.
            - **Verify:** `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --from 0x20000 --to 0x30000`
        - [x] **Task 3.3: The records from 0x30000 to 0x7ffff** (56 records)
            - Add the spans and the record entries for the range. Make sure every `game` record has a `finding`, `file`, or `command` item in its record `evidence`, its explicit entry, or its span.
            - **Verify:** `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --from 0x30000 --to 0x80000`
        - [x] **Task 3.4: The records from 0x80000 to 0xbffff** (45 records)
            - Add the spans and the record entries for the range. Make sure every `game` record has a `finding`, `file`, or `command` item in its record `evidence`, its explicit entry, or its span.
            - **Verify:** `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --from 0x80000 --to 0xc0000`
        - [x] **Task 3.5: The records from 0xc0000 to 0xcffff** (111 records)
            - Add the spans and the record entries for the range. Make sure every `game` record has a `finding`, `file`, or `command` item in its record `evidence`, its explicit entry, or its span.
            - **Verify:** `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --from 0xc0000 --to 0xd0000`
        - [x] **Task 3.6: The records from 0xd0000 to 0xdffff** (178 records)
            - Add the spans and the record entries for the range. Make sure every `game` record has a `finding`, `file`, or `command` item in its record `evidence`, its explicit entry, or its span.
            - **Verify:** `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --from 0xd0000 --to 0xe0000`
        - [x] **Task 3.7: The records from 0xe0000 to 0xeffff** (157 records)
            - Add the spans and the record entries for the range. Make sure every `game` record has a `finding`, `file`, or `command` item in its record `evidence`, its explicit entry, or its span.
            - **Verify:** `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --from 0xe0000 --to 0xf0000`
    - **Verify:**
        - `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/store.py list --status unknown | wc -l` — the after-unknown count, not the analysis count

- [x] **Task 4: The coverage report**
    - **Implements:** FR-012
    - Write the tests first.
    - **Create `tools/rebuild/tests/test_coverage.py`**
        - Add a record fixture with `address`, `status`, `provenance`, `size`, and an optional `result`.
        - `test_record_spans_reach_the_next_record`
        - `test_record_spans_use_the_last_size`
        - `test_record_spans_use_one_byte_for_a_missing_last_size`
        - `test_record_spans_skip_a_row_without_an_address`
        - `test_record_spans_sorts_the_records` — an unsorted input gives the spans in address order.
        - `test_code_object_size_reads_the_first_object` — mock `le.load` with a stub that holds `objects[0].virtual_size`.
        - `test_provenance_rows_count_the_records`
        - `test_provenance_rows_count_the_unknown_records`
        - `test_provenance_rows_sum_the_spans`
        - `test_provenance_rows_sum_the_verified_and_rebuilt_bytes`
        - `test_provenance_rows_sum_the_exact_bytes` — the sum of `bytes_equal` from the identity result.
        - `test_provenance_rows_compute_the_coverage` — `bytes / code_size` as a float in `0..1`.
        - `test_provenance_rows_without_a_code_size_use_the_covered_total`
        - `test_provenance_rows_zero_bytes` — an empty record list or a zero byte total gives `coverage` `0.0`.
        - `test_format_report_prints_every_provenance`
        - `test_format_report_prints_the_total` — the line is `total: N records, M unknown, B bytes, C% coverage, V verified bytes, R rebuilt bytes, E exact bytes`.
        - `test_format_report_zero_denominator` — a zero `code_size` and a zero byte total each print `0%`.
    - **Create `tools/rebuild/coverage.py`**
        - Add the module docstring and the usage block for `report`.
        - Add `record_spans(records)`. It sorts the records by address. It uses the same rule as the status-page map: the span reaches the next record, and the last record uses its `size`, or one byte. A row without an address is skipped.
        - Add `code_object_size(original)`. It loads the image and returns `objects[0].virtual_size`.
        - Add `provenance_rows(records, code_size)`. It maps each record with `store.provenance_of(record)` and returns one row per `store.PROVENANCE`, in order. The rows hold the Shared Interfaces keys. `verified_bytes` is the sum of the span bytes of the records with status `verified`, `rebuilt_bytes` is the sum for status `rebuilt`, and `exact_bytes` is the sum of the `bytes_equal` values of the identity results. The `coverage` value is a float in `0..1`. The denominator is `code_size` when it is not `None`, else the sum of the row `bytes`. A zero denominator gives `0.0`, including a zero `code_size` and a zero byte total.
        - Add `format_report(rows, code_size)`. The columns are `provenance`, `records`, `unknown`, `bytes`, `coverage`, `verified`, `rebuilt`, and `exact`. It ends with the total line `total: N records, M unknown, B bytes, C% coverage, V verified bytes, R rebuilt bytes, E exact bytes`. `B` is the sum of the row `bytes`. `C` is the coverage `B / code_size`, printed as a percentage. When `code_size` is `None`, `C` is the coverage `B / sum(bytes)`, printed as a percentage. A zero denominator, including `code_size` 0, prints `0%`.
        - Add `command_report`, `parse_arguments`, and `main`. `command_report` loads the function records only, because a module record carries no provenance. `--original` defaults to `store.DEFAULT_ORIGINAL`. Exit code 0 on success and 2 on a bad input.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_coverage.py"`
        - `python3 tools/rebuild/coverage.py report --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`

- [x] **Task 5: The status-page provenance panel and the game denominator**
    - **Implements:** FR-004, FR-012, design Design Decisions — the game denominator and the analysis count. The roadmap says this slice fixes the denominator.
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_view.py`**
        - Add `"provenance": "game"` to the `record` fixture.
        - `test_page_holds_the_provenance_panel` — the page holds the panel and one row per provenance.
        - `test_page_reports_provenance_counts` — a game record and a library record give `1 game` and `1 library` in the panel.
        - `test_page_reports_the_unknown_count` — a game `unknown` record shows one unknown in the game row.
        - `test_page_reports_byte_coverage` — the panel holds the byte bar and the percentage.
        - `test_page_targets_carry_the_provenance` — the chip, table row, map block, treemap block, and graph node hold `data-provenance`.
        - `test_page_filters_by_provenance` — the page holds `id="provenance-filter"` and the script reads it.
        - `test_page_binds_the_provenance_filter` — the script source holds `document.getElementById("provenance-filter").addEventListener("input", applyFilters);`.
        - `test_page_script_uses_the_denominator` — the script source tests `data.provenance === "game"`, `data.address !== ""`, and `data.status !== "not_a_function"` for the progress count.
        - `test_page_progress_excludes_library_and_compiler_helper_records` — a verified game record, a rebuilt library record, and a rebuilt compiler_helper record give the verified width 100% and the rebuilt width 0%, and the panel shows all three.
        - `test_page_match_counters_keep_a_library_record` — a rebuilt library record with a result stays in its match bucket, but it stays out of the progress denominator: the game record gives the verified width 100% and the rebuilt width 0%.
        - `test_page_script_match_bucket_ignores_provenance` — the script source holds the exact loop block. The `buckets[data.match]` increment stays inside the `not_a_function` guard and before the `data.provenance === "game"` condition. Assert `script.count("buckets[data.match] = (buckets[data.match] || 0) + 1;") == 1`, and assert the snippet `if (data.status !== "not_a_function") {` holds the match increment and then the `data.provenance === "game" && data.address !== ""` condition.
        - `test_page_progress_excludes_a_module_row` — a module row stays out of the progress denominator.
        - `test_page_excludes_data_from_the_total` — a `data` record with status `not_a_function` stays out of the progress total.
        - `test_page_header_counts_the_game_records` — the header count is the game count.
    - **Modify `tools/rebuild/view.py`**
        - Import `coverage`.
        - Add `_provenance_panel(records)`. It renders one row per provenance with the count, the unknown count, the byte bar, and the verified, rebuilt, and exact byte split.
        - Insert `_provenance_panel(functions)` after `_progress_row(all_rows)` in `render_page`. The panel takes the function records only, because a module record carries no provenance.
        - Add `provenance: str` to `_chip`. The new signature is `_chip(key, label, name, address, module, status, stale, provenance, lang="", match="")`. `_chip` writes `data-provenance`. `_function_chip` passes `store.provenance_of(record)`, and `_module_chip` passes `"game"`.
        - Add `data-provenance` to `_table_row`, `_map_block`, `_treemap_block`, and `_graph_node`. Use `store.provenance_of(record)`.
        - Add `data-provenance="game"` to the module chip and the module table row.
        - Add the provenance select to `_filter_row`. The id is `provenance-filter`.
        - Add `_denominator_rows(rows)` with the Shared Interfaces rule. `_progress_widths` and `_header` use it. Keep `_counted_rows` unchanged for `_match_counts`, so a library record with a result stays in the match buckets.
        - Add the provenance match to `applyFilters`. `applyFilters` reads `document.getElementById("provenance-filter").value` into `provenance` and adds `(provenance === "" || target.dataset.provenance === provenance)` to the match condition. The script reads `target.dataset.provenance` for the denominator. For the progress, verified, and rebuilt denominator, `applyFilters` keeps only a row whose provenance is `game`, whose status is not `not_a_function`, and whose `dataset.address` is not empty. The match counters keep the `_counted_rows` rule over every row with a result, regardless of provenance. In the loop over `seen.values()`, put the `if (data.match)` increment first, inside the `not_a_function` guard, and then the `data.provenance === "game"` and `data.address !== ""` condition. The match increment stays outside the game and address condition. Add `document.getElementById("provenance-filter").addEventListener("input", applyFilters);`.
        - Change `_map_sizes` to use `coverage.record_spans` for the size map. `low` is the minimum address and `span` is `max(max(address + size) - low, 1)`.
        - Add the panel style to `STYLE`. The class names are `provenance-panel`, `provenance-row`, `provenance-name`, `provenance-bar`, and `provenance-fill`.
        - Update the module docstring. State the panel and the game denominator.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_view.py"`
        - `python3 tools/rebuild/view.py html --out work/rebuild/status.html`

- [x] **Task 6: The pipeline check, the documentation, and the counts**
    - **Implements:** FR-018
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - `test_stage_twelve_checks_the_provenance_evidence` — mock `pipeline_check.run` and assert the `triage.py check` command and the check name `stage 12: the provenance evidence is complete`.
    - **Modify `tools/pipeline_check.py`**
        - Add `rebuild/triage.py` and `rebuild/coverage.py` to `TOOLS`.
        - In `check_store`, run `triage.py check --original <game dir>/STARTH.EXE` and add one check: `stage 12: the provenance evidence is complete`.
        - Update the stage 12 docstring. State the provenance evidence check.
    - **Modify `AGENTS.md`**
        - Project Structure: add `triage.py`, `coverage.py`, and `provenance.json` under `tools/rebuild/`.
        - Build And Test Commands: add the two files to the `py_compile` list. Add the `triage.py check` command, the bounded `--from`/`--to` form, and the `coverage.py report` command. Update the four check counts to 304, 300, 289, and 285.
        - Testing: add a bullet for the provenance values, the `not_a_function` status rule, and the fingerprint exclusion.
        - Testing: add a bullet for the evidence document, `triage.py check`, and `triage.py apply`.
        - Testing: add a bullet for the analysis count and the game denominator.
        - Testing: add a bullet for the coverage report and the status-page panel.
        - Code Guidelines → Architecture: add a bullet that a function record holds a provenance value from the five values, that the value is a documentation field, and that a `data` or `not_a_function` provenance requires the `not_a_function` status.
        - Update the four check counts at both places in the file.
    - **Modify `README.md`**
        - Add a Status bullet: a provenance value on every function record, a triage evidence document, and a byte-coverage report (`tools/rebuild/triage.py` and `tools/rebuild/coverage.py`).
        - Update the four check counts in the Checks paragraph.
    - **Modify `docs/reference/tooling.md`**
        - Add `tools/rebuild/triage.py` and `tools/rebuild/coverage.py` to the rebuild tool table.
        - Add a section for the provenance triage: the evidence document, the three commands, the bounded `--from`/`--to` check, the five values, and the analysis count.
        - Add a section for the coverage report: the columns and the code-object denominator.
        - Update the `store.py` paragraph with the `provenance` field and the status rule.
        - Update the four check counts.
    - **Modify `docs/explanation/findings.md`**
        - Insert `## The record scope and the provenance` immediately before `## Open questions`.
        - Record the plan-date counts: 971 function records, 688 `unknown`, 31 `named`, 230 `rebuilt`, 22 `verified`, 0 `matching`, and 252 module records. State that the design's counts (935 function records, 30 `verified`, 652 `unknown`, 31 `named`, 221 `rebuilt`, 1 `matching`) are historical.
        - Record the before and after `unknown` counts and the analysis count. State that a library or compiler-helper record without a name keeps the `unknown` status and leaves the analysis count.
        - Record the provenance counts and the byte coverage. Command: `python3 tools/rebuild/coverage.py report --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
        - Record the evidence document and the commands. Command: `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`, `python3 tools/rebuild/triage.py apply --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`, and `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
        - Record the four check counts from the pipeline commands.
    - **Modify `docs/explanation/rebuild-quality-design.md`**
        - The Problem, Observations, RC-2, and RC-4 present the counts as current. Mark those counts `at the design date` and add the plan-date counts: 971 function records, 22 `verified`, 688 `unknown`, 31 `named`, 230 `rebuilt`, 0 `matching`, and 252 module records. Keep the design-date strict split with the design-date count.
        - Change FR-011 to `The project must triage the unknown records.` Drop the 652 count there; the design-date 652 count stays in Observations and RC-2.
        - Mark the four C-10 counts `at the design date`: 303, 299, 288, and 284. Add the plan-date target 304, 300, 289, and 285.
        - Add the `store.save_record` exception to C-4: the provenance evidence pass writes the function records through `store.save_record`, and C-4 still applies to the shipped artifacts.
        - Change `The analysis denominator` to `The analysis count` and `out of the analysis denominator` to `out of the analysis count` in the Design Decisions.
    - **Modify `docs/explanation/rebuild-quality-roadmap.md`**
        - Drop the 652 count from the Slice 3 description, so it reads `Triage the unknown records`.
        - Change the Slice 3 `Done when` to: "Every record has a provenance value. The analysis count falls to the `unknown` records with provenance `game`. The report shows byte coverage and provenance counts."
        - Set the Slice 3 `Status` to `Implemented` and the `Plan` to `docs/explanation/rebuild-quality-plan-scope.md`. Do this after the final review approves the slice.
    - **Steps:**
        - Confirm that Docker, DOSBox-X, and the Ghidra loader are available. The baseline slice records them.
        - Run `python3 tools/rebuild/harness.py build --module driver`.
        - Run the four pipeline commands in Verify.
        - Run `python3 tools/rebuild/docs.py check` and `python3 tools/rebuild/callgraph.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
    - **Verify:**
        - `python3 -m py_compile tools/le.py tools/inventory.py tools/extract_cd_image.py tools/dosboxx_control.py tools/entry_probe.py tools/gfx_container.py tools/export_assets.py tools/sound_bank.py tools/member_probe.py tools/cross_check.py tools/rebuild/diff_module.py tools/rebuild/oracle.py tools/rebuild/store.py tools/rebuild/schema.py tools/rebuild/image_compare.py tools/rebuild/harness.py tools/rebuild/harness_dos.py tools/rebuild/watcom10_setup.py tools/rebuild/run.py tools/rebuild/view.py tools/rebuild/docs.py tools/rebuild/callgraph.py tools/rebuild/triage.py tools/rebuild/coverage.py tools/pipeline_check.py`
        - `python3 -m unittest discover -s tools/rebuild/tests`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/coverage.py report --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-cross`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime`
        - `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross`
        - The full check reports 304 checks, `--skip-cross` reports 300, `--skip-runtime` reports 289, and both flags report 285.

## Deviations

- The evidence pass writes one `tools/rebuild/provenance.json` with 27 spans,
  then runs the seven bounded checks. The plan describes one edit per range.
  Reason: the evidence document is one data file, and the bounded checks still
  prove each range.
- Three pure thunks (`0xd1ed8`, `0xeab1d`, and `0xd6e52`) hold the
  `not_a_function` provenance. The pass set their status with `store.py set`
  before `triage.py apply`. The pre-set is not required, because `check` reads
  the record provenance and the records had no provenance yet, but it keeps the
  status consistent before `apply`. The final review confirmed the outcome.
- The `store.py` change in Task 1 is a fingerprint input, so every stored
  fingerprint was stale at Task 6. The verification refreshed the 252 results
  with `run.py run --all --skip-unchanged` before the pipeline. The status
  counts did not change; the result dates and fingerprints did.
- The design's plan-date snapshot (688 `unknown`) predates the three-thunk
  reclassification. `findings.md` records the final 685 `unknown` and 3
  `not_a_function`, so both counts are labeled and consistent.
- The roadmap slice `Status` stayed `In Progress` until the final review
  approved the slice.
- The Task 6 verification stopped a stale `member_probe`/`dosbox-x` pair that
  held the debugger control port. No source or artifact was affected.
