# Mandatory Three-Tool Cross-Check Implementation Plan

> Execute this plan with the `executing-plans` skill. Each task is a
> checkbox. Mark it done when its changes are implemented and its Verify
> command passes.

- **Date**: 2026-11-02
- **Status**: In Progress
- **Design**: `docs/explanation/design-mandatory-cross-checks.md`
- **Depends on**: —
- **Manual prerequisites**: Run
  `nix develop .#cross-check --command sh tools/ghidra/install_loader.sh`
  once per user. The loader install needs network access and a writable home
  directory.
- **Goal**: Add one mandatory pipeline stage that compares four identity facts
  from `tools/le.py`, radare2, and Ghidra on every full run.

## Constraints

- The reference image is `STARTH.EXE`. The entry is `0x300f8`. Page 0 is at
  file `0x1a800` and module `0x10000`. `main` is `0x10020`. The caller is
  `0xd4324`. The call site is `0xd4364`. The call target is `0x10020`.
- `page0_module` is `0x10000`.
- The Ghidra `page0_file` is `sourceInfo.getFileBytesOffset(pageAddress)` for
  the source info that contains `0x10000`.
- Ghidra is 12.0.4. radare2 is 6.2.4.
- The full run reports 61 checks. `--skip-cross` reports 57 checks.
  `--skip-runtime` reports 46 checks. Both flags report 42 checks.
- The full run takes about 8 minutes plus the Ghidra import. The fast path
  takes about 16 seconds.
- The Ghidra timeout is 1800 seconds. The radare2 timeout is 300 seconds. The
  pipeline outer cross-check timeout is 2100 seconds.
- Stage 8 adds exactly four checks, one per fact, even when a tool fails.
- `cross_check.py` exits 0 on agreement, 1 on a difference or a tool failure,
  and 2 on invalid input or a missing tool.
- `cross_check.py` prints exactly four fact lines on exit 0 and exit 1.
- `cross_check.py` takes `--original`, `--main`, `--call-site`, `--work-root`,
  `--r2`, and `--ghidra` only. It has no caller argument.
- `cross_check.py` derives the caller from `--call-site`.
- `cross_check.py` writes its output under `<work-root>/cross-check/`. The raw
  files are `r2.txt` and `ghidra.txt`.
- The four fact lines are, in this order:

```text
entry: agree le=0x300f8 r2=0x300f8 ghidra=0x300f8
page0: agree le=0x1a800->0x10000 r2=0x1a800->0x10000 ghidra=0x1a800->0x10000
main: agree le=0x10020 r2=0x10020 ghidra=0x10020
caller: agree le=0xd4364@0xd4324->0x10020 r2=0xd4364@0xd4324->0x10020 ghidra=0xd4364@0xd4324->0x10020
```

- A differ line keeps the same fields and shows the value from each tool.
- An unresolved field renders as `unknown`. An `unknown` field makes the fact
  `differ`.
- The Ghidra wrapper prints this grammar:

```text
=== identity
entry=0x300f8
page0_module=0x10000
page0_file=0x1a800
main=0x10020
caller=0xd4324
call_site=0xd4364
call_target=0x10020
```

- The radare2 parser selects the `CALL XREF` line whose `@` address equals
  `--call-site`.
- Ghidra must see exactly one external entry point. Zero or several entry
  points print `entry=unknown`.
- The pipeline constants are `MAIN_LINEAR = 0x10020` and
  `MAIN_CALL_SITE = 0xD4364` next to `ENTRY_MODULE_LINEAR`.
- The tool preflight checks capstone, then radare2, then Ghidra, unless
  `--skip-cross` is set.
- `--skip-runtime` skips stages 5, 6, 7, and 9. Stage 8 still runs.
- `--skip-cross` skips stage 8 and the tool preflight.
- The `tools/le.py` `funcs` and `near` output stays byte-identical. The store
  seed reads the `funcs` output.
- No function record field, module record, or record schema changes.
- `le.py` is a fingerprint input. The helper change updates every computed
  fingerprint, so the next verification run re-verifies every record once.
  The record files stay unchanged.
- The `flake.nix` `cross-check` shell contains Python, capstone, radare2,
  Ghidra, `curl`, and `unzip`.
- The Ghidra `.image` block is the raw file at file offset 0. The object
  blocks have no file source. The page-0 file offset comes from the LE page
  table in `.image`.
- Ghidra 12.0.4 appends ` (GhidraScript)` to each logged script line. The
  wrapper strips it.
- `identity.sh --check` uses `install_loader.sh --check`.
- The Java script cannot set the process exit code. `identity.sh` maps a
  missing marker or key, or a Java `error:` line, to exit 2.
- Fact lines use lower-case `0x` hexadecimal with no zero padding.

## Tasks

- [x] **Task 1: Public function-start helpers in `tools/le.py`**
    - **Implements:** FR-013, SC-007, design §Component Interfaces
      (`tools/le.py`), §Testing Strategy
    - **Create `tools/rebuild/tests/test_le_helpers.py`**
        - `test_function_starts_returns_the_prologue_starts` — a fake image
          with two `55 89 e5` matches returns both starts, and a register save
          before the prologue is included.
        - `test_function_starts_honors_the_limit` — a limit of one returns one
          start.
        - `test_function_starts_rejects_a_missing_entry_object` — a missing
          entry object raises `ValueError("the entry object is missing")`.
        - `test_function_starts_rejects_an_unmapped_range` — an unmapped read
          raises `ValueError("the range is not mapped to a file offset")`.
        - `test_nearest_function_start_finds_the_start_below_the_address` —
          the nearest prologue start below an address is returned.
        - `test_nearest_function_start_returns_none_without_a_prologue` — an
          object with no prologue returns `None`.
        - `test_nearest_function_start_rejects_an_unmapped_address` — an
          unmapped address raises `ValueError("the address is unmapped")`.
        - `test_capstone_available_reports_a_missing_library` —
          `_load_capstone` returning `None` gives `False`.
        - `test_capstone_available_reports_a_loaded_library` — a non-`None`
          load gives `True`, and `_capstone` is not called.
        - `test_funcs_output_stays_identical` — for the bytes
          `53 55 89 e5 90 55 89 e5` at base `0x10000`, captured
          `print_functions` output equals `candidate function starts: 2` and
          the two exact `0x00010000` and `0x00010005` lines.
        - `test_near_output_stays_identical` — for the same bytes and address
          `0x10004`, captured `print_nearest_function` output equals
          `nearest prologue: 0x10000 (0x4 bytes below 0x10004)`.
    - **Modify `tools/le.py`**
        - `function_starts(exe, start=None, end=None, limit=10000) -> list[int]`
          — the reusable form of the `funcs` scan. It uses
          `object_at(exe.entry_linear)`, `read`, the `b"\x55\x89\xe5"`
          pattern, and `prologue_start`. It raises `ValueError` for a missing
          entry object or an unmapped range.
        - `nearest_function_start(exe, address) -> int | None` — the reusable
          form of the `near` scan. It raises `ValueError("the address is
          unmapped")` and `ValueError("the range is not mapped to a file
          offset")`. It returns `None` when no prologue exists below the
          address.
        - `capstone_available() -> bool` — returns
          `_load_capstone() is not None`. It does not call `_capstone()`.
        - `print_functions(exe: LinearExecutable, start: int | None, end: int |
          None, limit: int) -> int` — keeps its output. It calls
          `function_starts` and prints the count and the `0x%010x` lines. On
          `ValueError` it prints the message to stderr and returns 1.
        - `print_nearest_function(exe: LinearExecutable, address: int) -> int`
          — keeps its output. It calls `nearest_function_start`. On
          `ValueError` it prints the message to stderr and returns 1. On
          `None` it prints `no prologue below <address>` and returns 1.
    - **Verify:**
      `python3 -m unittest discover -s tools/rebuild/tests -p 'test_le_helpers.py'`
      and
      `python3 tools/le.py near ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0xd4364`

- [x] **Task 2: Cross-check environment and loader check**
    - **Implements:** FR-008, FR-009, SC-002, design §Component Interfaces
      (`flake.nix`, `install_loader.sh`), §Rollout And Compatibility
    - **Create `tools/rebuild/tests/test_ghidra_identity.py`**
        - `test_loader_check_accepts_an_installed_extension` — a temporary
          `GHIDRA_INSTALL_DIR` with `Ghidra/application.properties` version
          `12.0.4` and a temporary `GHIDRA_USER_DIR` with
          `Extensions/ghidra-lx-loader/extension.properties` version `12.0.4`
          give exit 0, and stdout names the extension path.
        - `test_loader_check_rejects_a_missing_extension` — exit 2, and stderr
          names the install command.
        - `test_loader_check_rejects_a_version_mismatch` — an extension
          version `12.0.3` gives exit 2.
        - `test_loader_check_rejects_a_missing_ghidra_install` — a missing
          `Ghidra/application.properties` gives exit 2 and names the path.
        - `test_loader_check_prints_the_install_command` — the failure text
          contains `nix develop .#cross-check` and `install_loader.sh`.
        - `test_loader_check_does_not_download` — run from a temporary
          working directory with a fake `curl` on `PATH` that writes a
          marker. `--check` creates no `work/ghidra/lx-loader.zip`.
        - `test_install_mode_rejects_a_missing_ghidra_install` — install mode
          exits 2 when the Ghidra version file is missing.
    - **Modify `flake.nix`**
        - Add a `cross-check` dev shell with `pkgs.python3`, `pkgs.capstone`,
          `pkgs.radare2`, `ghidraPkgs.ghidra`, `pkgs.curl`, and `pkgs.unzip`.
        - Set `GHIDRA_INSTALL_DIR="${ghidraPkgs.ghidra}/lib/ghidra"` and
          prepend `$GHIDRA_INSTALL_DIR/support` to `PATH` in `shellHook`.
        - Keep the `default` and `ghidra` shells unchanged.
    - **Modify `tools/ghidra/install_loader.sh`**
        - Add `--check`. It resolves `GHIDRA_INSTALL_DIR`, reads
          `Ghidra/application.properties`, resolves `USER_DIR` as install mode
          does, reads `Extensions/ghidra-lx-loader/extension.properties`, and
          compares the extension version to the Ghidra version.
        - On success print `ok: <extension path> (version <version>)` and
          exit 0. It does not download.
        - On a missing extension, a version mismatch, or a missing Ghidra
          install, print the install command to stderr and exit 2. A missing
          Ghidra install also names the unreadable path.
        - Make install mode exit 2, not 1, when the Ghidra version file is
          missing.
    - **Verify:**
      `python3 -m unittest discover -s tools/rebuild/tests -p 'test_ghidra_identity.py'`
      and
      `nix develop .#cross-check --command sh tools/ghidra/install_loader.sh --check`

- [x] **Task 3: Ghidra identity script and wrapper**
    - **Implements:** FR-002, FR-009, design §Component Interfaces
      (`identity.sh`, `LeIdentity.java`), §Error Handling And Edge Cases
    - **Modify `tools/rebuild/tests/test_ghidra_identity.py`**
        - `test_identity_check_accepts_a_working_setup` — a fake
          `analyzeHeadless` on `PATH`, a temporary Ghidra install, and an
          installed loader give exit 0, and stdout contains `ok` and `12.0.4`.
        - `test_identity_check_rejects_a_missing_analyzeheadless` — exit 2,
          and stderr names the cross-check shell.
        - `test_identity_check_rejects_a_missing_loader` — exit 2, and stderr
          names `install_loader.sh`.
        - `test_identity_run_accepts_the_seven_keys` — a fake
          `analyzeHeadless` prints the marker and seven prefixed keys. The
          wrapper exits 0, and stdout holds `=== identity` and all seven keys.
        - `test_identity_run_strips_the_ghidra_script_suffix` — a fake
          `analyzeHeadless` appends ` (GhidraScript)` to the marker and every
          key. The wrapper strips it, exits 0, and stdout holds the marker and
          the seven keys without the suffix.
        - `test_identity_run_accepts_an_unknown_entry` — a fake run prints
          `entry=unknown` with the other six keys. The wrapper exits 0.
        - `test_identity_run_rejects_a_missing_marker` — keys without the
          marker give exit 2.
        - `test_identity_run_rejects_a_missing_key` — six keys give exit 2.
        - `test_identity_run_rejects_a_java_error_line` — a line with
          `error:` gives exit 2 even when all keys are present.
        - `test_identity_run_rejects_a_missing_exe` — a missing executable
          gives exit 2.
    - **Create executable `tools/ghidra/identity.sh`**
        - `identity.sh --check` verifies `analyzeHeadless` on `PATH`,
          `GHIDRA_INSTALL_DIR`, `Ghidra/application.properties`,
          `LeIdentity.java`, and `install_loader.sh --check`. It prints
          `ok: Ghidra <version>` and exits 0. It prints the install command and
          exits 2 on failure. It does not download.
        - `identity.sh --exe PATH --project-dir DIR --main ADDR --call-site
          ADDR` runs `analyzeHeadless DIR Screamer -import PATH -overwrite
          -scriptPath <script directory> -postScript LeIdentity.java --main
          ADDR --call-site ADDR`.
        - The run captures combined output to a file under the project
          directory, filters from `=== identity`, and strips the
          `INFO  LeIdentity.java> ` prefix and the optional ` (GhidraScript)`
          suffix.
        - The run requires the marker and all seven keys. It fails with exit
          2 on a missing marker, a missing key, a Java `error:` line, or an
          `analyzeHeadless` failure. It removes the project directory on
          failure.
        - The run prints the marker and the seven keys to stdout and exits 0.
        - The wrapper resolves its own directory, so it works from any current
          directory.
    - **Create `tools/ghidra/LeIdentity.java`**
        - Read `--main` and `--call-site` from `getScriptArgs()` and convert
          them with `toAddr()`.
        - Print `=== identity`, then `entry`, `page0_module`, `page0_file`,
          `main`, `caller`, `call_site`, and `call_target` in that order.
        - `entry`: take the addresses from
          `getSymbolTable().getExternalEntryPointIterator()`. Exactly one
          address prints its value. Zero or several print `entry=unknown`.
        - `page0_module`: `0x10000`.
        - `page0_file`: parse the LE object page table from the `.image`
          block. Read `e_lfanew` at file offset `0x3c`. Read the LE header at
          `e_lfanew`: `page_size` at `+0x28`, `object_table_offset` at
          `+0x40`, `object_page_table_offset` at `+0x48`, and
          `data_pages_offset` at `+0x80`. Read object 1 at
          `e_lfanew + object_table_offset` and take its `page_table_index` at
          `+0x0c`. Read the page-0 entry at
          `e_lfanew + object_page_table_offset + (page_table_index - 1) * 4`
          as a big-endian 32-bit value. The file page is `value >> 8`. The
          file offset is `data_pages_offset + (file_page - 1) * page_size`.
          A failed read or an invalid value prints `unknown`.
        - `main`: `getFunctionContaining(mainAddress).getEntryPoint()`.
        - `caller`: `getFunctionContaining(callSite).getEntryPoint()`.
        - `call_target`: the first address from the flows of
          `getListing().getInstructionAt(callSite)` when that instruction is a
          call. A missing instruction or no flow prints `unknown`.
        - An unresolved key prints `key=unknown`. A fatal condition prints
          `error: <message>` to stderr and throws.
        - Format every address as lower-case `0x` hexadecimal.
    - **Verify:**
      `python3 -m unittest discover -s tools/rebuild/tests -p 'test_ghidra_identity.py'`
      and
      `nix develop .#cross-check --command sh tools/ghidra/identity.sh --check`

- [x] **Task 4: `tools/cross_check.py`**
    - **Implements:** FR-002, FR-003, FR-004, FR-010, FR-015, FR-017,
      SC-004, SC-008, design §Component Interfaces (`cross_check.py`),
      §Data Model, §Error Handling And Edge Cases
    - **Create `tools/rebuild/tests/test_cross_check.py`**
        - `test_strip_ansi_removes_escape_sequences` — ANSI escapes are
          removed and the text is otherwise unchanged.
        - `test_parse_r2_reads_the_entry_and_page_zero` — the `ie` row of type
          `program` gives `entry`, and the `iS` row at `0x00010000` gives
          `page0_file` and `page0_module`.
        - `test_parse_r2_reads_the_main_function` — the `pdf` header gives
          `main`.
        - `test_parse_r2_selects_the_call_xref_at_the_call_site` — two
          `CALL XREF` lines are present, and the line at `--call-site` is
          selected.
        - `test_parse_r2_marks_the_caller_unknown_for_zero_matches` — no
          matching line gives `None` for the caller fields.
        - `test_parse_r2_marks_the_caller_unknown_for_several_matches` — two
          matching lines give `None` for the caller fields.
        - `test_parse_r2_marks_a_missing_entry_unknown` — no `program` row
          gives `None` for `entry`.
        - `test_parse_ghidra_reads_the_seven_keys` — the marker text gives all
          seven integer values.
        - `test_parse_ghidra_maps_unknown_to_none` — `entry=unknown` gives
          `None`. This fixture models zero and several external entry points.
        - `test_parse_ghidra_maps_a_missing_key_to_none` — a missing key gives
          `None`.
        - `test_le_facts_reads_the_entry_and_page_zero` — a fake image gives
          `entry` and the page-0 fields.
        - `test_le_facts_takes_the_greatest_start_at_or_below_main` — the
          greatest start at or below `--main` is the `main` value.
        - `test_le_facts_reads_the_caller_and_call_target` — the nearest start
          below the call site is the caller, and the direct call target at the
          site is the target.
        - `test_le_facts_marks_an_absent_prologue_unknown` — no prologue gives
          `None`.
        - `test_compare_agrees_when_all_three_tools_match` — four facts are
          `agree`.
        - `test_compare_reports_only_the_differing_fact` — one changed `main`
          makes one fact `differ`, and the other three stay `agree`.
        - `test_compare_treats_an_unknown_field_as_differ` — an `unknown`
          field makes the fact `differ`.
        - `test_compare_returns_the_four_facts_in_fixed_order` — the order is
          `entry`, `page0`, `main`, `caller`.
        - `test_render_prints_the_four_agree_lines` — the four captured lines
          equal the design grammar exactly.
        - `test_render_prints_a_differ_line_with_each_tool_value` — the line
          says `differ` and shows `le`, `r2`, and `ghidra`.
        - `test_render_prints_unknown_for_an_unresolved_field` — an unresolved
          field renders as `unknown`.
        - `test_main_exits_zero_when_every_fact_agrees` — the three tool views
          agree and give exit 0 and four lines.
        - `test_main_exits_one_on_a_difference` — one different value gives
          exit 1 and four lines.
        - `test_main_exits_one_on_a_tool_failure` — a failed tool gives exit 1
          and four lines with that tool `unknown`.
        - `test_main_prints_four_lines_on_exit_one` — the line count is four.
        - `test_main_writes_the_raw_r2_and_ghidra_outputs` — a failed run
          leaves `r2.txt` and `ghidra.txt` under `<work-root>/cross-check/`.
        - `test_main_exits_two_on_an_invalid_main_address` — an unmapped
          `--main` gives exit 2 and names the address.
        - `test_main_exits_two_on_an_invalid_call_site` — an unmapped
          `--call-site` gives exit 2 and names the address.
        - `test_main_exits_two_when_capstone_is_missing` — exit 2 with the
          cross-check shell command.
        - `test_main_exits_two_when_r2_is_missing` — exit 2 and no stage
          output.
        - `test_main_exits_two_when_the_ghidra_wrapper_is_missing` — exit 2
          and no stage output.
        - `test_main_has_no_caller_option` — `--caller` is rejected by the
          argument parser.
        - `test_main_derives_the_caller_from_the_call_site` — `le_facts`
          receives `--call-site`, and no caller value is passed.
    - **Create `tools/cross_check.py`**
        - Constants: `FACT_ORDER = ("entry", "page0", "main", "caller")`,
          `FACT_FIELDS`, `PAGE0_MODULE = 0x10000`,
          `R2_TIMEOUT_SECONDS = 300.0`, and
          `GHIDRA_TIMEOUT_SECONDS = 1800.0`.
        - `strip_ansi(text: str) -> str` — remove `\x1b\[[0-9;]*m`.
        - `parse_r2(text: str, call_site: int) -> dict[str, dict[str, int |
          None]]` — parse the `ie` table, the `iS` table, and the `pdf` body.
          The `ie` row of type `program` gives `entry`. The `iS` row whose
          `vaddr` is `0x00010000` gives `page0_file` from `paddr` and
          `page0_module` from `vaddr`. The `pdf` function name gives `main`.
          `call_target` is `main`, because `pdf` disassembles the main
          function. Select the one `CALL XREF from fcn.<caller> @ <site>`
          line whose `<site>` equals `call_site`. Zero or several matches
          give `None` for `caller`, `call_site`, and `call_target`.
        - `parse_ghidra(text: str) -> dict[str, int | None]` — parse the seven
          keys. Strip an optional `INFO  LeIdentity.java> ` prefix. Map
          `unknown` and a missing key to `None`.
        - `le_facts(exe, main: int, call_site: int) -> dict[str, dict[str,
          int | None]]` — use `exe.entry_linear`, `exe.page_at(0x10000)` and
          `exe.to_file_offset(0x10000)`, the greatest
          `le.function_starts(exe)` entry at or below `main`,
          `le.nearest_function_start(exe, call_site)`, and
          `le.decode(exe, call_site, 1)` for the direct call target.
        - `compare_facts(le_view, r2_view, ghidra_view) ->
          list[IdentityFact]` — build four `IdentityFact` values in
          `FACT_ORDER`. A fact is `agree` only when every field is non-`None`
          and equal across all three tools. Otherwise it is `differ`.
        - `IdentityFact` — a dataclass with `name`, `values`, and `status`.
          `values` maps `le`, `r2`, and `ghidra` to the fact field maps.
        - `render_fact(fact: IdentityFact) -> str` — produce the exact fact
          line grammar. Use `unknown` for a `None` field.
        - `run_radare2(original, r2, main) -> str` — run
          `r2 -A -q -c ie -c iS -c "s <main>; pdf" -c q <original>` with
          `R2_TIMEOUT_SECONDS`. Return the ANSI-stripped text.
        - `run_ghidra(original, ghidra, project_dir, main, call_site) ->
          str` — run `sh <ghidra> --exe <original> --project-dir
          <project_dir> --main <main> --call-site <call_site>` with
          `GHIDRA_TIMEOUT_SECONDS`. The project directory is
          `<work-root>/cross-check/ghidra/proj`. Remove it on failure.
        - `main(argv=None) -> int` — validate the image with `le.load`, require
          `exe.page_at(main)` and `exe.page_at(call_site)`, and check
          capstone, `r2`, and the Ghidra wrapper. An invalid address names the
          address and returns 2. Address arguments use `int(value, 0)`.
          `--work-root` defaults to `work`, `--r2` defaults to `r2`, and
          `--ghidra` defaults to `tools/ghidra/identity.sh`. Resolve `--r2`
          with `shutil.which` when it has no path separator, and with
          `Path.is_file()` otherwise. Check `--ghidra` with `Path.is_file()`.
          Catch `subprocess.TimeoutExpired` and a nonzero tool exit as a tool
          failure. Print four fact lines and return 0, 1, or 2. Write
          `r2.txt` and `ghidra.txt` under `<work-root>/cross-check/`.
        - The first real run confirms that the external entry point iterator
          holds one address. If it holds zero or several addresses, keep
          `entry=unknown`, fail the fact, and record the iterator in the
          findings. If the first real Ghidra run reports a different `main` or
          `caller`, stop and record a finding. Do not weaken the compare.
    - **Verify:**
      `python3 -m unittest discover -s tools/rebuild/tests -p 'test_cross_check.py'`
      and
      `nix develop .#cross-check --command python3 tools/cross_check.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --main 0x10020 --call-site 0xd4364 --work-root work`

- [x] **Task 5: Pipeline preflight and stage 8**
    - **Implements:** FR-001, FR-005, FR-006, FR-007, FR-014, FR-016,
      SC-001, SC-002, SC-003, SC-009, design §Component Interfaces
      (`pipeline_check.py`), §Data Flow, §Skip modes
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - `test_capstone_is_checked_before_radare2` — a missing capstone
          returns the capstone message and does not call `shutil.which`.
        - `test_a_missing_radare2_names_the_cross_check_shell` — a missing
          `r2` returns a message with `nix develop .#cross-check`.
        - `test_a_missing_ghidra_names_the_install_command` — a failed
          `identity.sh --check` returns a message with `install_loader.sh`.
        - `test_skip_cross_checks_no_tool` — `--skip-cross` returns `None`
          and calls no tool.
        - `test_skip_runtime_keeps_stage_eight` — `stage_enabled(8, True,
          False)` is true.
        - `test_skip_cross_disables_stage_eight` — `stage_enabled(8, False,
          True)` is false.
        - `test_skip_cross_keeps_the_input_stages` — stages 1 through 4 stay
          enabled under `--skip-cross`.
        - `test_both_flags_disable_the_optional_stages` — stages 5, 6, 7, 8,
          and 9 are disabled. Stages 1 through 4, 10, 11, and 12 stay enabled.
        - `test_stage_eight_adds_four_checks_for_four_agree_lines` — four
          `agree` lines give four `PASS` checks.
        - `test_stage_eight_fails_the_differing_fact_only` — one `differ`
          line gives one `FAIL` and three `PASS` checks.
        - `test_stage_eight_fails_all_four_when_a_line_is_missing` — three
          lines give four checks, and the missing fact is `FAIL`.
        - `test_stage_eight_fails_all_four_on_a_tool_failure` — a nonzero
          cross-check exit gives four `FAIL` checks.
        - `test_stage_eight_passes_the_main_and_call_site_constants` — the
          command holds `--main 0x10020` and `--call-site 0xd4364`.
        - `test_parse_fact_lines_reads_the_four_facts` — four well-formed
          lines give four entries.
        - `test_parse_fact_lines_ignores_other_output` — a line with an
          unknown prefix is ignored.
    - **Modify `tools/pipeline_check.py`**
        - Add `"cross_check.py"` to `TOOLS`.
        - Add `MAIN_LINEAR = 0x10020` and `MAIN_CALL_SITE = 0xD4364` next to
          `ENTRY_MODULE_LINEAR`.
        - Add `CROSS_CHECK_TIMEOUT_SECONDS = 2100.0`.
        - Add the four stage-8 check names:
          `stage 8: the entry agrees in all three tools`,
          `stage 8: the page-0 mapping agrees in all three tools`,
          `stage 8: the main function agrees in all three tools`, and
          `stage 8: the main caller agrees in all three tools`.
        - `tool_preflight(skip_cross: bool, timeout: float) -> str | None` —
          return `None` when `skip_cross` is true. Check
          `le_module.capstone_available()`, then `shutil.which("r2")`, then
          `sh tools/ghidra/identity.sh --check`. Return the exact command
          message for the first failure.
        - `stage_enabled(stage: int, skip_runtime: bool, skip_cross: bool) ->
          bool` — false for stages 5, 6, 7, and 9 under `--skip-runtime`.
          False for stage 8 under `--skip-cross`. True otherwise.
        - `parse_fact_lines(text: str) -> dict[str, str]` — map each fact name
          to its full line. Accept only the four fact prefixes.
        - `check_cross_tools(checks, game_dir, exe_name, work_root) -> None` —
          run `python3 tools/cross_check.py --original <game>/<exe> --main
          0x10020 --call-site 0xd4364 --work-root <work_root>` with
          `CROSS_CHECK_TIMEOUT_SECONDS`. Add exactly four checks. A line that
          starts with `<fact>: agree` passes. A `differ` line fails with the
          line as detail. A missing line fails with `no fact line`. A timeout
          fails all four.
        - Remove `check_radare2`. Stage 8 uses `check_cross_tools`.
        - In `main`, add `--skip-cross`. Run the input preflight first, then
          `tool_preflight`. When it returns a message, print it to stderr and
          return 2 before stage 1. Keep the existing `PreflightTestCase`
          tests. They keep their input error because the input checks run
          before the tool preflight.
        - Run stage 8 after stage 7 when stage 7 runs, and after stage 4 when
          `--skip-runtime` is set. Run stage 8 before stage 9.
        - `--skip-runtime` no longer skips stage 8. It skips only stages 5, 6,
          7, and 9. `--skip-cross` skips stage 8 and the tool preflight.
        - Update the module docstring and the usage line.
    - **Verify:**
      `python3 -m unittest discover -s tools/rebuild/tests -p 'test_pipeline_check.py'`
      and
      `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross`
      gives `42 checks, 0 failures`.

- [ ] **Task 6: CI workflow**
    - **Implements:** FR-008, SC-001, design §Rollout And Compatibility,
      §Non-Functional Requirements (Portability)
    - **Modify `.github/workflows/pipeline.yml`**
        - Add `tools/cross_check.py` to the syntax-gate `py_compile` list.
        - Run the full pipeline step through the cross-check shell:
          `nix develop .#cross-check --command python3 tools/pipeline_check.py
          --game-dir "$GAME_DIR"`.
        - Keep the `if: vars.SCREAMER_SELF_HOSTED == 'true'` guard. The job
          stays disabled until the self-hosted runner has Nix and the loader.
    - **Verify:**
      `grep -q "tools/cross_check.py" .github/workflows/pipeline.yml`
      and
      `grep -q "nix develop .#cross-check" .github/workflows/pipeline.yml`

- [ ] **Task 7: Documentation**
    - **Implements:** FR-012, SC-005, design §Rollout And Compatibility,
      §Testing Strategy
    - **Modify `README.md`**
        - Add the `cross-check` shell and the loader install to Development
          Environment.
        - Replace the 57/61/42 sentence in Checks with the four modes: 61
          full, 57 with `--skip-cross`, 46 with `--skip-runtime`, and 42 with
          both.
        - Add the `cross_check.py` manual command.
    - **Modify `AGENTS.md`**
        - Add the three-tool cross-check to Technical Context.
        - Add `tools/cross_check.py`, `tools/ghidra/identity.sh`, and
          `tools/ghidra/LeIdentity.java` to Project Structure.
        - Add `tools/cross_check.py` to the syntax-gate command.
        - Add the cross-check commands and the four check counts to Build And
          Test Commands and Testing.
        - State that `--skip-runtime` still runs stage 8, and `--skip-cross`
          skips stage 8 and the tool preflight.
        - State that `le.py` is a fingerprint input, so the helper change
          re-verifies every record once and the record files stay unchanged.
    - **Modify `docs/reference/tooling.md`**
        - Add `tools/cross_check.py`, `tools/ghidra/identity.sh`, and
          `tools/ghidra/LeIdentity.java` to the tool table.
        - Rewrite the `tools/pipeline_check.py` section for the mandatory
          stage, the four counts, and the two flags.
        - Add the `cross-check` shell to Development Environment.
        - State in Static cross-check with radare2 and Decompilation with
          Ghidra that stage 8 now enforces the three views.
        - Add `install_loader.sh --check` and `identity.sh --check`.
    - **Modify `docs/explanation/findings.md`**
        - Add a section for the mandatory three-tool cross-check. Record the
          four `agree` lines, the identity values, the four counts and their
          modes, and the evidence commands.
        - Update the Pipeline confirmation counts to the current four modes.
    - **Verify:**
      `for f in README.md AGENTS.md docs/reference/tooling.md docs/explanation/findings.md; do for n in 61 57 46 42; do grep -q "$n checks" "$f" || { echo "$f misses $n"; exit 1; }; done; done`
      and
      `grep -n "skip-cross\|cross-check" README.md AGENTS.md docs/reference/tooling.md docs/explanation/findings.md`

- [ ] **Task 8: Full verification**
    - **Implements:** FR-010, FR-011, SC-001, SC-003, SC-004, SC-006,
      SC-007, design §Testing Strategy, §Success Criteria
    - **No new behavior.** Run the complete gate and confirm the documented
      modes.
    - The full run re-verifies every record because the `le.py` fingerprint
      changed. This is expected. The record files stay unchanged.
    - **Verify:**
        - `python3 -m py_compile tools/le.py tools/inventory.py tools/extract_cd_image.py tools/dosboxx_control.py tools/entry_probe.py tools/gfx_container.py tools/export_assets.py tools/sound_bank.py tools/member_probe.py tools/cross_check.py tools/rebuild/diff_module.py tools/rebuild/store.py tools/rebuild/harness.py tools/rebuild/harness_dos.py tools/rebuild/watcom10_setup.py tools/rebuild/run.py tools/rebuild/view.py tools/rebuild/docs.py tools/pipeline_check.py`
        - `python3 -m unittest discover -s tools/rebuild/tests` passes with no
          Ghidra, radare2, Docker, DOSBox-X, or game directory.
        - `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross`
          gives `42 checks, 0 failures`.
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game`
          gives `61 checks, 0 failures` and four `agree` lines.
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-cross`
          gives `57 checks, 0 failures`.
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime`
          gives `46 checks, 0 failures`.
        - `python3 tools/rebuild/store.py check` passes.
        - `git diff --exit-code -- tools/rebuild/functions tools/rebuild/modules tools/rebuild/contracts`
          prints nothing.

## Deviations

- The Ghidra loader creates the object blocks from a byte array, so
  `sourceInfo.getFileBytesOffset(0x10000)` returns -1. The page-0 file
  offset is parsed from the LE object page table in the `.image` block
  instead. The user approved this change.
- Ghidra 12.0.4 appends ` (GhidraScript)` to each logged script line.
  `identity.sh` strips the optional suffix before the marker and key checks.
