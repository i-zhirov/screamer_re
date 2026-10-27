# Rebuild Instrumentation Implementation Plan

> Execute this plan with the `executing-plans` skill. Each task is a
> checkbox; mark it done when its changes are implemented and its
> Verify command passes.

- **Date**: 2026-10-26
- **Status**: Implemented
- **Design**: `docs/explanation/design-rebuild-instrumentation.md`
- **Roadmap**: —
- **Depends on**: —
- **Manual prerequisites**: The game directory at
  `~/Documents/Screamer.app/Contents/Resources/game` exists, Docker runs,
  DOSBox-X is on PATH, and the `nix develop` shell provides Python 3.14.7 and
  capstone. A reviewer is available to accept the `program-map.md` diff of
  Task 3. Every command runs from the repository root.
- **Goal**: Replace the twenty shell scripts and the `REBUILT_MODULES` tuple
  with a committed JSON store, a generic harness, one runner, a status view,
  and generated `program-map.md` tables.

## Constraints

- Python 3.14.7. The framework imports the standard library and `ctypes` only.
- Store path: `tools/rebuild/functions/0x<address>.json`.
- Module path: `tools/rebuild/modules/<name>.json`.
- Status ladder: `unknown`, `not_a_function`, `named`, `rebuilt`, `matching`,
  `verified`.
- Check names: `build`, `identity`, `behavior`.
- Progress line: `[13/642] 0x300ec free: PASS (instructions 3/3, bytes 6/9)`.
- Skip lines: `[16/642] 0x15570 (unnamed): SKIP (no name)`,
  `[17/642] 0x2ecf8 list_create: SKIP (no checks declared)`, and
  `[18/642] 0x300ec free: SKIP (the runtime is skipped)`.
- Module progress line: `[1/1] driver: PASS (behavior)`.
- View output: `work/rebuild/status.html`.
- Log path: `work/rebuild/logs/`.
- Container image: `arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65`.
- Load base: `0x1c2000`. Run-time selector: `0150`.
- Source digest format: `sha256:<64 lowercase hex>`.
- Contracts: `tools/rebuild/contracts/record.schema.json` and
  `tools/rebuild/contracts/module.schema.json`.
- Evidence kinds: `command`, `finding`, `file`, and `note`. The render writes
  the `note` entries into the `Evidence` column of a function table.
- Every tool uses `argparse`, `from __future__ import annotations`, type hints,
  a module docstring with the usage, stdout for the result, stderr for an error,
  and `main() -> int` with `raise SystemExit(main())`.
- Every test file under `tools/rebuild/tests/` puts `tools/rebuild` on `sys.path`
  with `sys.path.insert(0, str(Path(__file__).resolve().parent.parent))`
  before it imports `store`, `docs`, `run`, or `view`.

## Shared Interfaces

The whole plan uses these names. A later task consumes only what an earlier task
produces.

- `store.py` constants: `FUNCTIONS_DIR`, `MODULES_DIR`, `CONTRACTS_DIR`,
  `STATUS_LADDER`, `CHECKS`, `LOAD_BASE = 0x1C2000`, `ENTRY_ADDRESS = 0x300F8`,
  `EXIT_FLUSH_ADDRESS = 0x300D6`, `SEED_LIMIT = 10000`, `DEFAULT_ORIGINAL`.
- `store.py` library: `record_path`, `module_path`, `load_record`, `save_record`,
  `save_result`, `load_module`, `save_module_result`, `candidate_addresses`,
  `advance_status`, `supported_status`, `is_stale`, `source_digest`,
  `validate_record`, `validate_module`, `scan_candidates`, `seed_diff`, `seed`.
- `docs.py` library: `TABLE_SECTIONS`, `STARTUP_STEPS`, `render_table`,
  `render_tables`, `update_document`, `check_document`.
- `harness.py` CLI: `build --module NAME`, `run --module NAME [--show]`,
  `digest --module NAME`. Exit 3 means a skip, because a tool is missing.
- `run.py` library: `select_records`, `next_selection`, `progress_line`,
  `summarize`, `diff_verdict`, `parse_identity`.
- `view.py` library: `instruction_percent`, `byte_percent`, `render_page`.
- Every result object holds `date`, `command`, `tool`, `source_digest`, and
  `checks`. Every check result holds `verdict` and `detail`, plus the four
  identity counts when the identity check ran.
- The importer `tools/rebuild/import_program_map.py` holds the page import and
  the module table. Task 10 removes it.

## Tasks

- [x] **Task 1: The store and the record contracts**
    - **Implements:** FR-001, FR-002, FR-004, FR-008, FR-012, FR-013,
      design §Component Interfaces (store.py), §Data Model, §API Contracts
    - **Move `docs/explanation/design-rebuild-instrumentation/contracts/record.schema.json` to `tools/rebuild/contracts/record.schema.json`**
        - the tests read the schema at run time
    - **Move `docs/explanation/design-rebuild-instrumentation/contracts/module.schema.json` to `tools/rebuild/contracts/module.schema.json`**
    - **Modify `docs/explanation/design-rebuild-instrumentation.md`**
        - the two Index rows now link `../../tools/rebuild/contracts/record.schema.json`
          and `../../tools/rebuild/contracts/module.schema.json`
        - the two API Contracts rows link the same two paths
    - **Modify `docs/explanation/design-rebuild-instrumentation/data-model.md`**
        - the sentence that names `contracts/record.schema.json` now names
          `tools/rebuild/contracts/record.schema.json`
    - **Create `tools/rebuild/tests/test_store.py`**
        - the file first puts `tools/rebuild` on `sys.path` with
          `sys.path.insert(0, str(Path(__file__).resolve().parent.parent))`
        - `test_advance_status_build_passes` — build passes and no identity
          declared gives `rebuilt`
        - `test_advance_status_identity_fails` — gives `rebuilt`
        - `test_advance_status_behavior_fails` — identity passes and behavior
          fails gives `matching`
        - `test_advance_status_identity_only` — identity alone passes gives
          `verified`, so the `verified` rule wins over `matching`
        - `test_advance_status_all_pass` — every declared check passes gives
          `verified`
        - `test_advance_status_behavior_only` — behavior alone passes gives
          `verified`
        - `test_advance_status_skip` — a skip gives `None`
        - `test_advance_status_build_fails` — gives `None`
        - `test_supported_status_from_result` — the result verdicts give the
          supported status
        - `test_is_stale_no_result` — a record with no result is not stale
        - `test_is_stale_digest_change` — a changed digest makes it stale
        - `test_source_digest_changes_with_content` — two temporary sources
          with different bytes give different digests, and the format matches
          `^sha256:[0-9a-f]{64}$`
        - `test_validate_record_address_mismatch` — the file name and the
          `address` disagree, and the message names the path and the field
        - `test_validate_record_unknown_status` — a `status` outside the ladder
          is a violation
        - `test_validate_record_named_without_name` — `named` with no `name` is
          a violation
        - `test_validate_record_verified_without_result` — `verified` with no
          result is a violation
        - `test_validate_record_verified_without_passing_identity` — `verified`
          whose result holds a failing identity is a violation
        - `test_validate_record_check_without_result` — a declared check with
          no check result is a violation
        - `test_validate_record_unknown_check` — a `checks` entry outside
          `build`, `identity`, and `behavior` is a violation
        - `test_validate_record_unknown_module` — a `module` with no module
          file is a violation
        - `test_validate_record_module_without_symbol` — a `module` without a
          `symbol` is a violation
        - `test_validate_record_identity_without_module` — `identity` declared
          with no `module` is a violation
        - `test_validate_record_identity_without_map` — `identity` declared
          whose module record holds no `map` is a violation
        - `test_validate_record_status_stronger_than_result` — `matching` whose
          result holds only a passing build is a violation
        - `test_validate_record_finding_heading` — a `finding` evidence text
          that is not a heading of `findings.md` under the given root is a
          violation
        - `test_validate_record_missing_file` — a `file` evidence path that
          does not exist is a violation
        - `test_validate_module_status_above_named` — a module `status` above
          `named` with no result is a violation
        - `test_record_round_trip` — `save_record` then `load_record` gives the
          same record, with indent 2 and sorted keys
    - **Create `tools/rebuild/store.py`**
        - `FUNCTIONS_DIR = Path(__file__).resolve().parent / "functions"`,
          `MODULES_DIR = Path(__file__).resolve().parent / "modules"`,
          `CONTRACTS_DIR = Path(__file__).resolve().parent / "contracts"`,
          `STATUS_LADDER = ("unknown", "not_a_function", "named", "rebuilt", "matching", "verified")`,
          `CHECKS = ("build", "identity", "behavior")`,
          `LOAD_BASE = 0x1C2000`, `ENTRY_ADDRESS = 0x300F8`,
          `EXIT_FLUSH_ADDRESS = 0x300D6`, `SEED_LIMIT = 10000`,
          `DEFAULT_ORIGINAL = Path.home() / "Documents/Screamer.app/Contents/Resources/game/STARTH.EXE"`
        - `record_path(address: int) -> Path` — `functions/0x%x.json`
        - `module_path(name: str) -> Path` — `modules/<name>.json`
        - `load_record(address: int) -> dict` — reads the file, raises
          `ValueError` with the path on a broken JSON
        - `save_record(record: dict) -> None` — `json.dumps(record, indent=2,
          sort_keys=True)` plus a newline, written through a temporary file and
          a replace, so a failed save leaves the previous file
        - `save_result(address: int, result: dict, status: str | None) -> None`
          — sets `result` and writes `status` only when it is not `None`
        - `load_module(name: str) -> dict` and
          `save_module_result(name: str, result: dict, status: str | None) -> None`
        - `candidate_addresses() -> list[int]` — the store file names, sorted
        - `advance_status(checks: list[str], verdicts: dict[str, str]) -> str | None`
          — the ladder, in this exclusive order: a skip or an empty `checks`
          gives `None`, a failing `build` gives `None`, a passing `build` with
          no `identity` gives `rebuilt`, a failing `identity` gives `rebuilt`,
          an identity pass with a failing declared `behavior` gives `matching`, and
          every declared check passing with `identity` or `behavior` declared
          gives `verified`; a `checks` list of `["identity"]` with a pass gives
          `verified`, and a build-only pass stays `rebuilt`
        - `supported_status(record: dict) -> str | None` — `advance_status`
          over the record's `checks` and its result verdicts
        - `is_stale(record: dict, digest: str) -> bool` — `False` with no
          result, else the digest compare
        - `source_digest(module: dict, root: Path | None = None) -> str` — the
          SHA-256 of the `sources` entries sorted by `file`, each as the path
          bytes, a zero byte, and the file bytes; the prefix is `sha256:`
        - `validate_record(path: Path, record: dict | None = None,
          root: Path | None = None) -> list[str]`
          and `validate_module(path: Path, record: dict | None = None,
          root: Path | None = None) -> list[str]`
          — the fourteen rules of the data model, one message per violation;
          rule 1 compares the file name of `path` with `address`
        - `scan_candidates(original: Path) -> list[int]` — runs
          `le.py funcs <original> --limit 10000` and adds `0x300f8` and
          `0x300d6`, the two run-time starts without an EBP frame
        - `seed_diff(candidates: list[int], records: dict[int, dict]) -> tuple[list[int], list[int]]`
          — the added addresses and the lost addresses, where a lost address is
          an `unknown` record outside the candidates
        - `seed(check: bool, original: Path) -> int` — writes one record per
          added address, never rewrites an existing record, sets `object_offset`
          from `le.object_at`, and sets `size` as the distance to the next
          address of the merged list; the last address holds no `size`; a missing
          original image gives a clear error and exits 2
        - CLI `seed [--check] [--original PATH]`, `set --function ADDR
          [--name] [--role] [--status] [--module] [--symbol] [--section]
          [--confidence] [--check] [--evidence KIND TEXT] [--note TEXT]`,
          `note --function ADDR --text TEXT [--date]`,
          `list [--status] [--module] [--json]`, `check`
        - `check` prints one line per violation and the summary
          `<n> records, <v> violations`; it exits 1 on a violation
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests` and
      `python3 tools/rebuild/store.py check`

- [x] **Task 2: The program-map tables**
    - **Implements:** FR-010, FR-014, design §Component Interfaces (docs.py)
    - **Create `tools/rebuild/tests/test_docs.py`**
        - the file first puts `tools/rebuild` on `sys.path`
        - `test_startup_runtime_address` — the record `0x300f8` renders
          `0150:1f20f8`
        - `test_startup_order_is_step_order` — the startup rows follow
          `STARTUP_STEPS` and not the address order
        - `test_function_row_cells` — a record renders the address, the name,
          the `note` evidence, and the confidence in the four columns
        - `test_startup_evidence_cell` — the startup rows render the four
          `Evidence` texts of `STARTUP_STEPS`, so the `program \`main\`` cell
          keeps its own text
        - `test_unknown_section_goes_last` — a record with an unknown
          `section` lands in the last table
        - `test_record_without_section_is_absent` — a record with no `section`
          renders no row
        - `test_missing_marker_raises` — a document with no marker raises
          `ValueError`
        - `test_check_reports_drift` — a changed cell gives one drift message
    - **Create `tools/rebuild/docs.py`**
        - `DOCUMENT = Path(__file__).resolve().parent.parent.parent /
          "docs/reference/program-map.md"`
        - `STARTUP_STEPS = ((0x300f8, "entry_jump", "jmp short +0x76 over
          the banner string"), (0x30170, "startup", "sti, stack setup, DOS
          version check"), (0xd4324, "main_invoker", "loads argc and argv,
          calls main"), (0x10020, "program \`main\`", "56 instructions, 15
          call sites"))`
        - `TABLE_SECTIONS` — the section names in document order:
          `Startup path`, `Program functions`, `race_loop callees`,
          `frame_update callees`, `object_update_all callees`,
          `motion_update callees`, `race_frame callees`,
          `object_transform callees`, `object_physics callees`,
          `overlay_dispatch callees`, `The replay records`,
          `The graphics decoders`, `The TABS table loaders`,
          `The sound loaders`, `The vehicle state chain`,
          `The timing and filter layer`, `The runtime memory manager`,
          `object_gear_step and object_slide_step callees`,
          `The resource layer`, `Resource loader`,
          `Subsystem initialisation`
        - `render_table(section: str, records: list[dict]) -> str` — the table
          block; a function table holds `Module address`, `Name`, `Evidence`,
          `Confidence`; the startup table holds `Step`, `Module address`,
          `Object offset`, `Run-time address`, `Evidence`; the function-table rows
          sort by address, and the startup table keeps the `STARTUP_STEPS` order;
          the `Evidence` cell of a function table joins the `note` evidence
          entries of the record, one per line, and the `Evidence` cell of the
          startup table is the fixed text of `STARTUP_STEPS`
        - `render_tables(records: list[dict]) -> dict[str, str]` — one table
          text per section
        - `update_document(text: str, records: list[dict]) -> str` — replaces
          the text between `<!-- begin generated: <section> -->` and
          `<!-- end generated: <section> -->`; a missing marker raises
          `ValueError`
        - `check_document(text: str, records: list[dict]) -> list[str]` — one
          message per drifted section
        - CLI `render` writes the document and prints the section count,
          `check` writes nothing and exits 1 on drift
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests`

- [x] **Task 3: The seed and the one-time migration (human review gate)**
    - **Implements:** FR-002, FR-003, FR-010, design §Seed Rules, §Data Flow,
      §Rollout And Compatibility (step 1)
    - **Create `tools/rebuild/tests/test_import_program_map.py`**
        - the file first puts `tools/rebuild` on `sys.path`
        - `test_read_rows_function_row` — a function row gives `address`,
          `name`, `role`, `confidence`, `section`, and one `note` evidence
        - `test_read_rows_startup_row` — a startup row gives `address`,
          `object_offset`, `section = "Startup path"`, the fixed step name,
          and one `note` evidence from the `Evidence` column
        - `test_apply_rows_promotes_to_named` — an `unknown` record that gains
          a name and a role becomes `named`, and a `not_a_function` record stays
        - `test_apply_rows_creates_missing` — a missing address is created with
          `status = "named"`, and the counts are `(1, 0)`
        - `test_apply_rows_section_precedence` — a function row keeps its
          `section` when a startup row names the same address
    - **Create `tools/rebuild/import_program_map.py`**
        - `read_rows(document: str) -> list[dict]` — reads the 20 function
          tables first and the startup table second; a function row gives
          `address`, `name`, `role`, `confidence`, `section`, and the third
          column as one `evidence` entry of kind `note`; a startup row gives
          `address`, `role`, `object_offset`, and `section = "Startup
          path"`, and takes the name and the `Evidence` text from the fixed
          startup table; a startup row does not change the section of
          an address that a function row names
        - `apply_rows(rows: list[dict]) -> tuple[int, int]` — merges the rows
          by address, creates a missing record with `status = "named"`,
          `checks = []`, `evidence = []`, `notes = []`, and sets the row
          fields on every record; a record that gains a name and a role becomes
          `named` unless its status is already `not_a_function` or later; the
          `size` of a new record is the distance to the next store address, and
          the last record holds no `size`; returns the created and the updated
          counts
        - `apply_modules() -> int` — the module table: `0x300ec free free_
          [build, identity, behavior]`, `0x300be exit exit_ [build, identity,
          behavior]`, `0x300d6 exit exit_flush_ [identity] count 6`,
          `0x3009b printf printf_ [build, identity, behavior]`, `0x8c150
          memcpy memcpy_ [build, identity, behavior]`, `0x128ac shutdown
          engine_shutdown_ [build, identity, behavior]`, `0x10978 error_report
          error_report_ [build, identity, behavior] count 16`, `0x108e4 bios_video
          bios_video_ [build, identity, behavior]`, `0x2ed77 list_destroy
          list_destroy_ [build, identity, behavior]`, `0x2f8ca memory_available
          memory_available_ [build, identity, behavior]`; every module that holds
          a `map` declares `identity`, so the five known differences are
          recorded and not hidden
        - CLI `--modules` runs `apply_modules` alone
    - **Modify `docs/reference/program-map.md`**
        - adds the 21 marker pairs around the generated tables; the hand-written
          table text stays between the markers at first
        - the `Call site` table of the `Program main` section, the
          `race_session_start` table (`Mode`, `Handler`, `Name`, `Confidence`),
          the `Main invoker` disassembly block, `Engine strings`, `Method`, and
          `Limits` are not in `TABLE_SECTIONS`, so they stay hand-written
    - **Run in order:** `python3 tools/rebuild/store.py seed`, then
      `python3 tools/rebuild/import_program_map.py`, then
      `python3 tools/rebuild/docs.py render`
    - **Human review gate:** read `git diff docs/reference/program-map.md`.
      The diff holds these kinds of change: the 21 marker pairs; the rows of every
      generated function table reordered to address ascending; 11 duplicate rows
      removed from a later table when an earlier table names the same address,
      because the store holds one record per address and the record holds one
      `section`; and three rows corrected from the disassembly of `0x29d08` and
      `0x4c810`: the name and the evidence cell of `0x29d08` become
      `menu_space_init`, the evidence cell of `0x28ba4` follows the new name, and
      the name of `0x4c810` becomes `stream_layer_init`. The startup table,
      every other role, every other confidence value, and every other `Evidence`
      cell stay the same. The reviewer confirms this and records the seed count.
    - **Verify:** `python3 tools/rebuild/store.py seed --check`,
      `python3 tools/rebuild/store.py check`,
      `python3 tools/rebuild/docs.py check`

- [x] **Task 4: The module records and the generic harness**
    - **Implements:** FR-011, FR-013, FR-016, design §Component Interfaces
      (harness.py), §Module Record, §Rollout And Compatibility (step 2)
    - **Create `tools/rebuild/modules/driver.json`, `free.json`, `exit.json`,
      `printf.json`, `memcpy.json`, `shutdown.json`, `error_report.json`,
      `bios_video.json`, `list_destroy.json`, `memory_available.json`**
        - every record holds `name`, `exe`, `sources`, and `run`; `map` is absent
          for `driver`; the driver alone holds `status = "named"` and
          `checks = ["behavior"]`
        - `sources`: `free.c` and `free_main.c`; `exit.c` and `exit_main.c`;
          `printf.c` and `printf_main.c`; `memcpy.asm` (compiler `wasm`) and
          `memcpy_main.c`; `shutdown.c` with `flags ["-of+"]` and
          `shutdown_main.c`; `error_report.c` and `error_report_main.c`;
          `bios_video.c` with `flags ["-of+"]` and `bios_video_main.c`;
          `list_destroy.c` with `flags ["-of+", "-od"]` and
          `list_destroy_main.c`; `memory_available.c` with
          `flags ["-of+", "-od"]` and `memory_available_main.c`; `driver.c`
        - `exe` and `map` come from the removed shell scripts:
          `free.exe` and `free.map`, `exitmod.exe` and `exit.map`,
          `prtfmod.exe` and `printf.map`, `memmod.exe` and `memcpy.map`,
          `shutmod.exe` and `shutdown.map`, `errmod.exe` and
          `error_report.map`, `bvmod.exe` and `bios_video.map`, `ldmod.exe`
          and `list_destroy.map`, `mamod.exe` and `memory_available.map`,
          `driver.exe`
        - `run.command`, `run.output`, `run.expect`, and `run.timeout = 180`
          come from the removed run scripts; the ten output names are `out.txt`
          for `driver`, and `free_out.txt`, `exit_out.txt`, `prtf_out.txt`,
          `mem_out.txt`, `shut_out.txt`, `err_out.txt`, `bv_out.txt`,
          `ld_out.txt`, and `ma_out.txt` for the other nine; the `expect`
          lines are the harness markers in order, for example `["NULL", "DONE",
          "FREE", "AFTR"]` for `free`, `["A", "B", "B", "C", "T"]` for
          `exit`, `["P", "4"]` for `printf`, `["A", "B", "C", "D", "E", "F",
          "G", "H", "Y"]` for `memcpy`, `["1", "2", "3", "4", "5", "6",
          "7", "8"]` for `shutdown`, `["R", "R", "R", "D"]` for
          `error_report`, `["F", "C", "D"]` for `bios_video`, `["F", "R",
          "Z", "N"]` for `list_destroy`, `["F", "S", "T", "D"]` for
          `memory_available`, and the eight `W <reg> <val>` lines
          of the `pair` table of `tools/rebuild/driver.c` in that order for
          `driver`; the captured file loses the leading `REBUILT` line and the
          trailing `DONE` line to the DOS print buffer, as `findings.md` records
    - **Create `tools/rebuild/harness.py`**
        - `IMAGE`, `WORK_DIR = <repo root> / "work/rebuild"`,
          `SHARED_SOURCES = ("chk.asm", "stack.asm", "cstart.c")`,
          `REBUILD_DIR = Path(__file__).resolve().parent`,
          `DOSBOX = "dosbox-x"`
        - `build_script(module: dict) -> str` — one `wasm` or `wcc386` line per
          source in record order with the base flags `-bt=dos -zq -zcm=watcom`
          (`wasm`) or `-bt=dos -s -zq` (`wcc386`) plus the record `flags`,
          then `wlink system dos4g option quiet option nodefaultlibs option
          start=_cstart_ option map=/w/<map> name <exe> file <objs>`; the object
          list is the record sources, then `chk.obj`, `cstart.obj`, `stack.obj`;
          the `option map=` clause appears only when the record holds `map`
        - `free_port() -> int` — a bound port on 127.0.0.1
        - `run_script_text() -> str` — `break`, `clear`, `exec RUN`, `sleep
          10`, `break`, `screen`, `stop`
        - `conf_text(module: dict, port: int) -> str` — `[dosbox] mcp_server=
          <port>`, `memsize=32`, `machine=svga_s3`, `[autoexec] mount D
          <work>`, `D:`, the `run.command`
        - CLI `build --module NAME` copies the sources into `work/rebuild/`,
          runs the container, prints its output, and exits 1 on a compile or a
          link error; a missing Docker exits 3 with a clear line
        - CLI `run --module NAME [--show]` writes
          `work/rebuild/run_<name>.conf` and `work/rebuild/run_<name>.dbg`,
          drives `tools/dosboxx_control.py script`, reads `run.output`, strips
          the NUL bytes, checks every `expect` line, prints the output with
          `--show`, and exits 1 on a difference; a missing DOSBox-X exits 3
        - CLI `digest --module NAME` prints `source_digest(load_module(name))`
    - **Run:** `python3 tools/rebuild/import_program_map.py --modules`, then
      `python3 tools/rebuild/store.py check`
    - **Verify:** `python3 tools/rebuild/harness.py build --module` for each of
      the ten module records, then `python3 tools/rebuild/harness.py digest
      --module free`, then the five `diff_module.py` commands of AGENTS.md and the
      `engine_shutdown` and `memcpy` commands of `docs/reference/tooling.md`. The
      commands for `0x10978 error_report_`, `0x108e4 bios_video_`,
      `0x2ed77 list_destroy_`, and `0x2f8ca memory_available_` follow the same
      shape with the `exe` and `map` values of the module records. Every verdict
      matches the recorded one: `free`, `exit`, `exit_flush`, `printf`, and
      `memcpy` pass, and the other five name the recorded difference.

- [x] **Task 5: The runner**
    - **Implements:** FR-005, FR-006, FR-007, FR-008, FR-014,
      design §Component Interfaces (run.py), §The Identity Check,
      §Status Ladder, §Rollout And Compatibility (step 2)
    - **Create `tools/rebuild/tests/test_run.py`**
        - `test_progress_line_pass` — the pinned line
          `[13/642] 0x300ec free: PASS (instructions 3/3, bytes 6/9)`
        - `test_progress_line_unnamed` — `[16/642] 0x15570 (unnamed): SKIP (no name)`
        - `test_progress_line_no_checks` — `[17/642] 0x2ecf8 list_create:
          SKIP (no checks declared)`
        - `test_progress_line_runtime_skipped` — `[18/642] 0x300ec free:
          SKIP (the runtime is skipped)`
        - `test_progress_line_module_row` — `[1/1] driver: PASS (behavior)`
        - `test_progress_line_fail` — a failing identity gives `FAIL` with the
          identity counts
        - `test_summary` — `<n> records: <p> pass, <f> fail, <s> skip`
        - `test_diff_verdict_pass` — the three pass verdict lines
        - `test_diff_verdict_fail` — the four fail verdict lines
        - `test_parse_identity` — the two compare lines give the four counts
        - `test_parse_identity_count_mismatch` — a count mismatch still gives
          the two byte counts
        - `test_next_selection_smallest_size` — `named` records sort by
          `size`, and a record with no `size` goes last
        - `test_next_selection_skips_other_status` — only `named` records
        - `test_select_by_address_and_name` — the two selectors
        - `test_select_status_and_stale` — the status filter and the stale
          filter
        - `test_select_module_function_records` — the function records of a
          module
        - `test_select_module_row` — a module with no function record selects
          the module row
        - `test_select_empty_is_a_message` — an empty selection prints a
          message and exits 0
        - `test_verdict_fail_exits_1` — a failing identity gives exit 1
          and the status `rebuilt`
    - **Create `tools/rebuild/run.py`**
        - `LOG_DIR = <repo root> / "work/rebuild/logs"`
        - `select_records(records: list[dict], modules: dict[str, dict],
          digests: dict[str, str], function: int | None, name: str | None,
          module: str | None, status: str | None, stale: bool,
          next_count: int | None, all_records: bool) -> tuple[list[dict], list[dict]]`
          — the function records in address order and the module rows
        - `next_selection(records: list[dict], count: int) -> list[dict]` — the
          `named` records with the smallest `size`, no `size` last, address order
        - `progress_line(index: int, total: int, label: str, verdict: str,
          detail: str) -> str` — `[<i>/<n>] <label>: <verdict> (<detail>)`
        - `summarize(verdicts: list[str]) -> str` — `<n> records: <p> pass,
          <f> fail, <s> skip`
        - `diff_verdict(text: str) -> str` — the last `verdict:` line, or the
          last line
        - `parse_identity(text: str) -> dict[str, int]` — the two compare
          lines give `instructions_equal`, `instructions_total`, `bytes_equal`,
          `bytes_total`
        - `run_identity(record: dict, module: dict, original: Path) -> tuple[str, dict]`
          — builds the `diff_module.py` command with `--original`,
          `--original-address`, `--rebuilt <exe>`, `--rebuilt-map <map>`,
          `--rebuilt-symbol <symbol>`, and `--count` when the record holds
          `count`; a pass needs exit 0 and one of the three pass verdicts; the
          check result holds the verdict line in `detail`
        - `run_build(module: dict) -> str` and `run_behavior(module: dict) -> str`
          — call `harness.py`; exit 0 is `pass`, exit 3 is `skip`, else `fail`;
          `--skip-runtime` skips a declared behavior check with the detail
          `the runtime is skipped`
        - the run keeps every check line in
          `work/rebuild/logs/run-<YYYYMMDD-HHMMSS>.log`
        - the write-back calls `save_result` or `save_module_result` with
          `advance_status`; `--no-write` skips it; a failed declared check
          exits 1 and writes the status that `advance_status` gives: a skip or
          a failed build keeps the previous status, a failed identity gives
          `rebuilt`
        - CLI `run (--function ADDR | --name NAME | --module NAME | --status
          STATUS | --stale | --next N | --all) [--no-write] [--show]
          [--skip-runtime] [--original PATH]`; no selector prints a clear
          error and exits 2
    - **Modify `tools/rebuild/diff_module.py`**
        - the `byte compare` and `instruction compare` lines print before every
          early return, so the four counts are available on every path; the
          verdict strings and the comparison logic stay the same
    - **Run:** `python3 tools/rebuild/run.py run --all`, so every module
      record holds a result, then the focused commands
    - **Verify:** `python3 tools/rebuild/run.py run --function 0x300ec`,
      `python3 tools/rebuild/run.py run --name free`,
      `python3 tools/rebuild/run.py run --module driver`,
      `python3 tools/rebuild/run.py run --all --no-write`, and
      `python3 -m unittest discover -s tools/rebuild/tests`

- [x] **Task 6: The status view**
    - **Implements:** FR-008, FR-009, FR-014, design §Component Interfaces
      (view.py), §UI Design, §Derived Values, §Rollout And Compatibility
      (step 3)
    - **Create `tools/rebuild/tests/test_view.py`**
        - `test_instruction_percent` — 3/3 gives `100%`, 2/3 gives `67%`, and
          no result gives `—`
        - `test_byte_percent_zero_total` — a zero total gives `—`
        - `test_page_holds_one_row_per_record` — two records give two rows
          with the names and both percentages
        - `test_page_marks_stale` — a changed digest adds the stale mark
        - `test_page_has_no_external_reference` — the text holds no `http`,
          no `<script src`, and no `<link`
        - `test_page_holds_the_driver_module_row` — a module record with no
          function record appears in the modules group with its own status and
          its own test command
        - `test_page_filters_and_switch` — the filter narrows the rows and the
          counters, and the board and table switch keeps the selection
        - `test_page_detail_pane_click` — a click on a chip loads the detail
          pane without a page load
    - **Create `tools/rebuild/view.py`**
        - `DEFAULT_OUT = <repo root> / "work/rebuild/status.html"`
        - `instruction_percent(record: dict) -> str` and
          `byte_percent(record: dict) -> str` — `round(100 * equal / total)`
          and `%`, or `—` with no result or a zero total
        - `render_page(records: list[dict], modules: list[dict],
          digests: dict[str, str]) -> str` — the approved layout D: the
          header with the record count, the last run date, and the stale count;
          the filter row; the progress bar and the status counters; the board
          with one group per `section`, then a `Modules` group per module
          record with no function record, then an `Unmapped` group; the table;
          the inline detail pane with the record fields, the evidence, the notes,
          and           the test command `run.py run --name <name>` or `run.py run
          --function 0x<address>`; the theme variables of the wireframe; the
          status word next to the status color; the stale mark `⚠`; one inline
          `script` block holds the filter, the board and table switch, and the
          chip click, so the page needs no server and no external reference
        - CLI `html [--out PATH]` writes the page and prints the path and the
          row count
    - **Verify:** `python3 tools/rebuild/view.py html` and
      `python3 -m unittest discover -s tools/rebuild/tests`

- [x] **Task 7: The unit suite closure and the check wiring**
    - **Implements:** FR-014, design §Testing Strategy, §Rollout And
      Compatibility (the hook and the CI wiring extend steps 3 and 4 to the
      existing repository gates)
    - **Modify `tools/rebuild/tests/test_store.py`**
        - `test_schema_matches_record_properties` — the property names and
          the required set of a full record equal `record.schema.json`
        - `test_schema_matches_module_properties` — the same for a module
          record and `module.schema.json`
        - `test_seed_diff_added_and_lost` — a missing candidate is added, an
          `unknown` record outside the candidates is lost, and a `named` record
          outside the candidates is not lost
    - **Modify `tools/rebuild/tests/test_docs.py`**
        - `test_render_then_check_has_no_drift` — a temporary document with
          the markers and two records renders and then checks clean
    - **Modify `.githooks/pre-push`**
        - the compile list gains `tools/rebuild/store.py
          tools/rebuild/harness.py tools/rebuild/run.py
          tools/rebuild/view.py tools/rebuild/docs.py
          tools/rebuild/import_program_map.py`, and the hook runs
          `python3 -m unittest discover -s tools/rebuild/tests` after the
          syntax gate
    - **Modify `.github/workflows/pipeline.yml`**
        - the syntax job gains the same paths and the unittest step
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests` and
      `sh .githooks/pre-push`

- [x] **Task 8: The pipeline check rewiring**
    - **Implements:** FR-015, design §Testing Strategy, §Rollout And
      Compatibility (step 4)
    - **Modify `tools/pipeline_check.py`**
        - `TOOLS` gains `rebuild/store.py`, `rebuild/harness.py`,
          `rebuild/run.py`, `rebuild/view.py`, `rebuild/docs.py`, and
          `rebuild/import_program_map.py`
        - `check_store(checks: Checks, game_dir: Path, work_dir: Path) -> None`
          — runs `store.py check`, `store.py seed --check --original
          <game_dir>/STARTH.EXE`, `docs.py check`, and `view.py html --out
          <work_dir>/status.html`, and checks the page file exists
        - `check_run(checks: Checks, game_dir: Path, skip_runtime: bool) ->
          None` replaces `check_rebuild`; it prints the skip line when Docker
          is missing, else it runs `run.py run --all --no-write --original
          <game_dir>/STARTH.EXE` plus `--skip-runtime` when the flag is set,
          reads the recorded result of every record through `store.py list
          --json`, and compares the printed verdict of each record with the
          recorded result, so a change and an unexpected skip fail; a record
          with no recorded result is skipped with the detail `no recorded
          result`, and under `--skip-runtime` the detail `the runtime is
          skipped` is the expected detail of a behavior-check record
        - `REBUILT_MODULES` and its comment go away, because the store is
          the only module registry
        - the module docstring names `check_run` as stage 11, which replaces
          `check_rebuild`, and `check_store` as the new stage 12
    - **Verify:** `python3 tools/pipeline_check.py --game-dir
      ~/Documents/Screamer.app/Contents/Resources/game`, then read the
      printed check count and the elapsed time

- [x] **Task 9: The pages and the finding**
    - **Implements:** FR-002, FR-010, design §Rollout And Compatibility
      (steps 6 and 7)
    - **Modify `AGENTS.md`**
        - the Project Structure tree gains `store.py`, `harness.py`, `run.py`,
          `view.py`, `docs.py`, `contracts/`, `functions/`, `modules/`, and
          `tests/` under `tools/rebuild/`
        - the layer table gains the Store row between the Tools layer and the
          Documentation layer: the identity, the status, and the evidence per
          function
        - the record rule: a function record lives at
          `tools/rebuild/functions/0x<address>.json`, and `store.py` is the
          only writer
        - the Testing section names the `unittest` suite and its command
        - the syntax gate list gains `tools/rebuild/store.py`,
          `tools/rebuild/harness.py`, `tools/rebuild/run.py`,
          `tools/rebuild/view.py`, and `tools/rebuild/docs.py`
        - the Build And Test Commands section replaces the twenty shell script
          commands with `harness.py`, `run.py`, `store.py`, `docs.py`, and
          `view.py`, and holds the count that the pipeline check prints
    - **Modify `README.md`**
        - the Status section names the store, the runner, the view, and the
          generated program map
        - the Checks section holds the count that the pipeline check prints
    - **Modify `docs/reference/tooling.md`**
        - the rebuild harness section replaces the twenty shell script rows
          with the ten module records and the five tools
        - the commands section holds the new commands and the count
    - **Modify `docs/explanation/findings.md`**
        - a new heading holds the framework, the seed count, the render diff
          kinds, and the pipeline check count, with the exact commands
    - **Verify:** `python3 tools/pipeline_check.py --game-dir
      ~/Documents/Screamer.app/Contents/Resources/game`, then confirm the
      printed count appears in the three pages

- [x] **Task 10: The removal**
    - **Implements:** design §Rollout And Compatibility (step 5), §Context And
      Problem
    - **Remove `tools/rebuild/build.sh`, `build_free.sh`, `build_exit.sh`,
      `build_printf.sh`, `build_memcpy.sh`, `build_shutdown.sh`,
      `build_error_report.sh`, `build_bios_video.sh`, `build_list_destroy.sh`,
      `build_memory_available.sh`, `run.sh`, `run_free.sh`, `run_exit.sh`,
      `run_printf.sh`, `run_memcpy.sh`, `run_shutdown.sh`,
      `run_error_report.sh`, `run_bios_video.sh`, `run_list_destroy.sh`,
      `run_memory_available.sh`**
        - the module records and `harness.py` replace them
    - **Remove `tools/rebuild/import_program_map.py` and
      `tools/rebuild/tests/test_import_program_map.py`**
        - the migration is complete, and the test goes with the tool
    - **Modify `tools/pipeline_check.py`**
        - `TOOLS` drops `rebuild/import_program_map.py`
    - **Modify `.githooks/pre-push` and `.github/workflows/pipeline.yml`**
        - the compile list drops `tools/rebuild/import_program_map.py`
    - **Verify:** `python3 tools/pipeline_check.py --game-dir
      ~/Documents/Screamer.app/Contents/Resources/game` passes,
      `python3 -m unittest discover -s tools/rebuild/tests` passes, and
      `python3 -m py_compile` of every remaining tool passes

## Requirements Trace

| Requirement | Task |
| --- | --- |
| FR-001 | 1 |
| FR-002 | 1, 3, 5, 6, 9 |
| FR-003 | 3 |
| FR-004 | 1 |
| FR-005 | 5 |
| FR-006 | 5 |
| FR-007 | 5 |
| FR-008 | 1, 5, 6 |
| FR-009 | 6 |
| FR-010 | 2, 3, 9 |
| FR-011 | 4 |
| FR-012 | 1-10 |
| FR-013 | 1-10 |
| FR-014 | 1, 2, 5, 6, 7 |
| FR-015 | 8 |
| FR-016 | 4, 5 |

## Deviations

Each ruling below is a plan-local correction made during execution.  The
reason follows it.

1. **Task 2: `STARTUP_STEPS` and the run-time address cell.**  The step names
   and the evidence texts now match the page (`entry jump`, `main invoker`, and
   the backticked evidence), and the run-time address cell is backticked.  The
   run-time address is `LOAD_BASE + module address`, not
   `LOAD_BASE + object_offset`.  Reason: the render must reproduce the page, and
   the page's prose formula is arithmetically wrong.  The user chose the page.
2. **Task 3: the human review gate.**  The diff now holds the 21 marker pairs,
   the rows reordered to address ascending, 11 duplicate rows removed, and 3 rows
   corrected.  Reason: the store holds one record per address and the record holds
   one `section`, so an address that two tables name renders in one table only.
   The user confirmed the amendment.  The 3 corrected rows follow the disassembly
   of `0x29d08` and `0x4c810`, which the user asked for: `0x29d08` becomes
   `menu_space_init`, the evidence cell of `0x28ba4` follows it, and `0x4c810`
   becomes `stream_layer_init`.
3. **Task 1 and Task 3: the exit helper `0x300d6`.**  The seed adds the helper
   through `EXIT_FLUSH_ADDRESS`, next to the entry address.  Reason: the module
   table needs the record, and the prologue scan misses the helper because it holds
   no EBP frame.  The design's seed rule 2 now names both run-time starts.
4. **Task 3: the `error_report` count.**  The module table holds `count 16` for
   `0x10978`.  Reason: the recorded diff command uses `--count 16`; without it
   the verdict is `the instruction counts differ, 16 and 12`.
5. **Task 4: the `driver` and `list_destroy` expect lists.**  The driver
   record holds the eight `W` lines; the list holds `["F", "R", "Z", "N"]`.
   Reason: the captured driver file loses `REBUILT` and `DONE` to the DOS print
   buffer, as `findings.md` records, and the source prints an uppercase `Z`.
6. **Task 5: the write-back status.**  The write-back passes
   `store.advance_status` for every outcome, so a failed identity check writes
   `rebuilt`.  Reason: the design's advance table and validation rule 12 require
   it; keeping the previous status would leave a state that `store.py check`
   rejects.  FR-006 and the `run.py` failure-modes line now say the same.
7. **Task 5: the runtime-skip example line.**  The example now names
   `0x300ec free`.  Reason: `0x128ac` fails its identity check, so the run
   prints `FAIL`, not `SKIP`.
8. **Task 8: the pipeline check count.**  The 748 skip lines stay out of the
   check count, and a zero-comparison guard fails the stage when the runner
   prints no progress line.  Reason: a store-size-dependent count weakens SC-006,
   and a runner that prints nothing would otherwise pass the stage.
9. **The final review fixes.**  The runner writes no result when a declared check
   is a skip, a missing original image gives a clear error and exit 2, and the
   harness quotes the mount path.  Reason: the record contract allows only the
   `pass` and `fail` verdicts, the design's edge-case table requires exit 2, and
   the removed shell scripts quoted the path.
