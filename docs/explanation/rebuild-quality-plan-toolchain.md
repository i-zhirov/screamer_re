# Toolchain Selection Implementation Plan

> Execute this plan with the `executing-plans` skill. Each task is a
> checkbox. Mark it done when its changes are implemented and its Verify
> command passes.

- **Date**: 2026-11-13
- **Status**: Implemented
- **Design**: docs/explanation/rebuild-quality-design.md
- **Roadmap**: docs/explanation/rebuild-quality-roadmap.md — slice: toolchain
- **Depends on**: Slice 1
- **Manual prerequisites**: DOSBox-X and the period Watcom 10.0a toolchain.
  Run `python3 tools/rebuild/watcom10_setup.py`. It fetches the image
  `WATCOM_C10A.ISO` (SHA-256
  `f51ed9358e7eaf6a0d292268bbbc15832530489dd8239a98f6938c244f1736fc`) and
  extracts `work/watcom10/WATCOM`. Docker is needed for the V2 harness.
  DOSBox-X, Docker, and the installed Ghidra loader are needed for the full
  check.
- **Goal**: Add toolchain selection to the runner, add a toolchain-and-flag
  matrix that records the best measured result in each module recipe, and
  measure the strict delta.

## Constraints

- C-1: Python 3.14.7. Use the standard library and the `ctypes` capstone binding only.
- C-2: Tests use `unittest` under `tools/rebuild/tests/`.
- C-3: A tool takes its configuration from the command line. It reads no environment variable.
- C-4: A tool writes only under `work/` or to an explicit output path. The provenance evidence pass `triage.apply` and the library pass `library.apply` write the function records through `store.save_record`, and C-4 still applies to the shipped artifacts.
- C-5: Prefer C. Use assembly only when C cannot reproduce the original bytes, and record the reason.
- C-6: A harness output name stays within the 8.3 limit.
- C-7: The 10.0a toolchain runs alongside OpenWatcom V2. Do not replace V2.
- C-8: The status ladder stays `unknown`, `not_a_function`, `named`, `rebuilt`, `matching`, `verified`.
- C-9: The identity verdict strings stay unchanged.
- C-10: At the design date, the full check reports 303 checks. `--skip-cross` reports 299, `--skip-runtime` reports 288, and both flags report 284. The plan-date counts are 304, 300, 289, and 285. The observed counts after the library pass are 431, 427, 416, and 412. Update these counts when a change adds or removes a check.
- C-11: Do not commit. The user commits.
- C-4 applies to the module store. `toolchain.py` writes module records through `store.save_module`, in the same way `triage.apply` and `library.apply` write function records through `store.save_record`.
- The module toolchain values are `v2` and `watcom10`. The default is `v2`.
- A measured module records its best measured toolchain and flags. A module with no identity record is `unmeasured` and keeps its recipe.
- Both harnesses stay. V2 is the default. The behavior check always uses the V2 harness, because the period harness has no run command.
- The full matrix report stays under `work/rebuild/`. The committed matrix manifest is `tools/rebuild/toolchain.json`.
- The matrix needs Docker, DOSBox-X, and the period toolchain. It compares every candidate with the strict gate and ranks every measured result. A strict match is preferred but not required.
- The matrix writes module records only through `store.save_module`. It writes no function record.
- A module recipe change makes the record stale. Task 1 through Task 3 change the recipe and the tool inputs, so every stored fingerprint is stale until Task 6 re-baselines.

## Spec Coverage

| Requirement | Task |
| --- | --- |
| FR-015 | Task 1 (module field and migration), Task 2 (runner selection and mode), Task 3 (matrix), Task 4 (manifest and check), Task 5 (V2 pass), Task 6 (period pass) |
| FR-003 | Task 1 (fingerprint inputs), Task 2 (result mode and stage 11), Task 6 (re-baseline) |
| FR-018 | Task 4 (check count), Task 7 (documentation and observed counts) |

FR-003 applies here because the mode carries the toolchain and the fingerprint
covers the toolchain. The runtime mode stays out of the fingerprint. A
`--skip-runtime` run does not invalidate a stored result.

FR-015 has a boundary. The matrix measures one identity record per module. The
identity record is the record with the lowest address that names the module,
declares the identity check, and holds a symbol. The source-form work is Slice
6. This plan changes the compiler, the flags, and the runner. It does not
change the C source form.

FR-018 has a boundary. The plan adds one stage-12 check. Each of the four
full-check counts increases by one. Task 6 measures the matrix delta. Task 7
records the observed counts after the pipeline runs. The plan gives no forecast
count.

## Shared Interfaces

- `store.TOOLCHAINS: tuple[str, ...]` is `("v2", "watcom10")`. It is the only
  definition. `run` and `toolchain` import it.
- `store.TOOLCHAIN` stays `"v2"`.
- `store.module_toolchain(module: dict) -> str` returns `module.get("toolchain", store.TOOLCHAIN)`.
- `store.load_module_file(path: Path) -> dict` reads one module JSON file and raises `ValueError` with the path on a missing file or broken JSON.
- `store.save_module(module: dict) -> None` writes the module through `_write_json`, the existing record writer.
- `store.run_mode(strict: bool, skip_runtime: bool, toolchain: str = store.TOOLCHAIN) -> dict` returns `{"strict", "toolchain", "runtime"}`. The `toolchain` argument has the default `"v2"`.
- `store.TOOL_FILES` keeps its current order and appends `"rebuild/harness_dos.py"`, `"rebuild/watcom10_setup.py"`, and `"rebuild/toolchain.py"` after `"rebuild/library.py"`.
- `contracts/module.schema.json` adds the optional `toolchain` property with `enum: ["v2", "watcom10"]` and the default description `v2`.
- `contracts/record.schema.json` changes `$defs.mode.properties.toolchain.enum` to `["v2", "watcom10"]`.
- `run.HARNESS_DOS_TOOL: Path` is `REBUILD_DIR / "harness_dos.py"`.
- `run.harness_for(toolchain: str) -> Path` returns `HARNESS_TOOL` for `"v2"`, `HARNESS_DOS_TOOL` for `"watcom10"`, and raises `ValueError` for any other value.
- `run.build_module(module: dict, toolchain: str | None = None, recipe: Path | None = None, show: bool = False, timeout: int | None = None) -> tuple[str, str]` selects the harness, passes the recipe option when it is present, and passes the timeout to the period harness when it is not `None`. It returns the verdict and the captured output. A `None` toolchain uses `store.module_toolchain(module)`.
- `run.run_build(module: dict, show: bool = False) -> tuple[str, str]` calls `build_module(module, show=show)`.
- `run.run_behavior(module: dict, show: bool = False) -> tuple[str, str]` calls `_harness(HARNESS_TOOL, "run", module, show)`. It does not use the period harness.
- `run.clear_artifacts(module: dict) -> None` deletes `work/rebuild/<exe>`, `work/rebuild/<map>` when present, and `work/rebuild/M.MAP`. `build_module` calls it before the harness command, so a failed build cannot compare an old artifact.
- `run._harness(tool: Path, command: str, module: dict, show: bool, extra: tuple[str, ...] = ()) -> tuple[str, str]` runs one harness command and maps exit code 0 to `pass`, 3 to `skip`, and every other code to `fail`.
- `run.run_mode(arguments: argparse.Namespace, toolchain: str = store.TOOLCHAIN) -> dict` returns `store.run_mode(arguments.strict, arguments.skip_runtime, toolchain)`.
- `run._mode_for_module(mode: dict, module: dict | None) -> dict` returns a copy of `mode` with `toolchain` set to `store.module_toolchain(module)`. A `None` module keeps the mode.
- `pipeline_check.unchanged_is_verified(record, game_dir, skip_runtime) -> bool` uses `store_module.module_toolchain(module)` in `store_module.run_mode(True, skip_runtime, toolchain)`. It already imports `store` as `store_module`, so it needs no new import.
- `harness.load_recipe(name: str, recipe: Path | None) -> dict` and `harness_dos.load_recipe(name: str, recipe: Path | None) -> dict` return `store.load_module(name)` when `recipe` is `None`, else `store.load_module_file(recipe)`. They raise `ValueError` when the record name differs from `name`.
- `harness.command_build(name: str, recipe: Path | None = None) -> int` and `harness_dos.command_build(name: str, timeout: int, recipe: Path | None = None) -> int` load the recipe, build it, and keep the existing exit codes. Both `main` functions pass `arguments.recipe` to `command_build`.
- `harness_dos.clear_artifacts(module: dict) -> None` deletes `work/rebuild/<exe>`, `work/rebuild/<map>` when the module holds one, and `work/rebuild/M.MAP`. `command_build` calls it before the compile and deletes the log.
- `harness_dos.log_failures(text: str) -> list[str]` lowercases each line and returns the lines that start with `error` or contain `error!`. A warning line is not a failure. `command_build` raises `ValueError` when the log holds a failure, so a failed period build cannot look like a pass.
- `toolchain.MATRIX_DOCUMENT: Path` is `store.WORK_DIR / "toolchain-matrix.json"`.
- `toolchain.MANIFEST_DOCUMENT: Path` is `store.REBUILD_DIR / "toolchain.json"`.
- `toolchain.CANDIDATE_DIR: Path` is `store.WORK_DIR / "toolchain"`.
- `toolchain.DEFAULT_FLAG_SETS: tuple[tuple[str, tuple[str, ...]], ...]` is `(("-of+", ("-of+",)), ("-od", ("-od",)), ("-of+ -oi", ("-of+", "-oi")), ("-of+ -ot", ("-of+", "-ot")), ("empty", ()))`.
- `toolchain.MATCH_RANK: dict[str, int]` is `{"exact-bytes": 0, "exact-relocations": 1, "same-shape": 2, "same-mnemonics": 3, "different": 4}`. `toolchain.BUILD_FAIL_RANK` is `5`. `toolchain` imports `run.STRICT_MATCHES` and does not define a second tuple.
- `toolchain.candidate_sets(module: dict) -> list[tuple[str, tuple[str, ...] | None]]` returns the existing source flags first, labelled `existing`, then the five default sets. The duplicate rule compares the per-source `(file, flags)` list. The first occurrence stays. The matrix tries `v2` first and `watcom10` second. Within one toolchain the order is the existing set, then the five default sets.
- `toolchain.candidate_sources(module: dict, flags: tuple[str, ...] | None) -> list[dict]` returns one `{"file", "compiler", "flags"}` item per source. A `None` flags value keeps the source flags. A flag tuple applies to every `wcc386` source. A `wasm` source keeps its flags and its compiler.
- `toolchain.candidate_descriptors(module: dict, toolchain: str) -> list[dict]` returns one descriptor per candidate set. Each descriptor holds `order` (the position within the toolchain), `toolchain`, `label`, and `sources`. `matrix_module` concatenates the V2 descriptors and the period descriptors and rewrites `order` to the global index, V2 first. `evaluate`, `write_recipe`, and the report row use the descriptor.
- `toolchain.write_recipe(module: dict, descriptor: dict, path: Path) -> None` writes a recipe with `name`, `exe`, `map` when present, `run` when present, and the descriptor `sources`. It keeps each source `compiler` and replaces only the `wcc386` flags. A `wasm` source keeps its flags.
- `toolchain.identity_record(records: list[dict], module_name: str) -> dict | None` returns the record with the lowest address that names the module, declares `identity`, and holds a symbol. It returns `None` when no such record exists.
- `toolchain.is_strict(row: dict) -> bool` returns true when `row["match"]` is `exact-bytes` or `exact-relocations`.
- `toolchain.rank(row: dict) -> tuple[int, int, int, int]` returns the match rank, the differing byte count, the differing instruction count, and the candidate order. When either count of a pair is missing, the differing count is 0. Otherwise the differing byte count is `bytes_total - bytes_equal`, and the differing instruction count is `instructions_total - instructions_equal`. A failed or skipped build gets `BUILD_FAIL_RANK`.
- `toolchain.winner(rows: list[dict]) -> dict | None` returns the best measured row by `rank`. It returns `None` when no row has a measured match.
- `toolchain.apply_winner(module: dict, winner: dict) -> dict` returns a copy of the module with `toolchain` set to the winner toolchain and each source `flags` set from the winner `sources` item with the same `file`. It uses `source.get("flags", [])`. An empty list removes the field. It keeps each source `compiler` and removes `period_flags`.
- `toolchain.clear_artifacts(module: dict) -> None` calls `run.clear_artifacts(module)` and deletes `work/rebuild/dos_build.log`. The matrix calls it before every candidate build. `run.build_module` also clears the same outputs.
- `toolchain.evaluate(module: dict, record: dict, descriptor: dict, original: Path, timeout: int | None = None) -> dict` calls `clear_artifacts`, writes the candidate recipe with `write_recipe` under `CANDIDATE_DIR`, builds it with `run.build_module`, and runs `run.run_identity(record, module, original, strict=True)` after a pass. It returns one report row with `strict`.
- `toolchain.matrix_module(module: dict, record: dict | None, original: Path, toolchains: tuple[str, ...] = store.TOOLCHAINS, timeout: int | None = None, only_missing: bool = False, base_manifest: dict | None = None, resume_manifest: dict | None = None, evaluate: Callable[..., dict] = evaluate) -> dict` returns one module item. It returns `measured` false and the reason `no identity record` when `record` is `None`. It returns `measured` false and the reason `no measured result` when every candidate fails or skips. It builds the descriptors with `candidate_descriptors`. It tries the V2 candidates first and stops the module at the first `exact-bytes`. For a period pass, it skips a module with a strict `base_manifest` row and measures a module with no strict V2 row. It seeds the row set with the `base_manifest` winner and keeps the better row by `rank`. A resume run skips a module that `resume_manifest` already measured, whether the row is strict or not, and holds `measured` true, `skipped` true, and the reason `already measured`. It copies an unmeasured row and holds `measured` false, `skipped` true, and the reason `no measured result`.
- `toolchain.matrix(modules: list[dict], records: list[dict], original: Path, module_name: str | None = None, toolchains: tuple[str, ...] = store.TOOLCHAINS, timeout: int | None = None, only_missing: bool = False, base_manifest: dict | None = None, resume_manifest: dict | None = None, on_module: Callable[[dict], None] | None = None, evaluate: Callable[..., dict] = evaluate) -> dict` takes the modules and records, measures the selected modules, and returns the report. It calls `on_module(item)` after each module item, in module order. It writes no module and no report. It writes only the candidate recipe files and the build artifacts under `work/rebuild/`.
- `toolchain.command_matrix(arguments) -> int` loads the modules and records, calls `matrix` with an `on_module` callback, and writes the report to `--out` after each module item through an atomic replace. The callback appends the item to the report and writes it. It applies every winner through `store.save_module` unless `--dry-run` and prints the summary. The module and report writes live here, not in `matrix`. An interrupted pass leaves a partial report. Before the first build it raises `ValueError` when the original image is missing or does not parse with `le.load`, or when a selected module lacks `name`, `exe`, or `sources`.
- `toolchain.report_document(modules: list[dict], records: list[dict]) -> dict` uses `identity_record(records, module["name"])` and reads that record's `result.mode.toolchain` and `result.checks.identity.match`. When the result has no mode, it uses `store.module_toolchain(module)`. It does not read a module recipe result.
- `toolchain.format_report(document: dict) -> str` returns the header `toolchain modules strict exact-bytes exact-relocations shape different no-result`, one line per toolchain with the eight values, and the total line. The total line starts with `total` and prints the sum of the seven counts.
- `toolchain.manifest_document(report: dict, report_path: Path, report_digest: str) -> dict` returns the committed manifest.
- `toolchain.check_matrix(modules: list[dict], records: list[dict], manifest: dict, path: Path) -> list[str]` returns one message per violation.
- `toolchain.migrate_modules(modules: list[dict], check: bool = False) -> int` writes `toolchain` `"v2"` into every module without the field and returns the count. With `check`, it writes nothing and returns the count of modules without the field.

The matrix report document holds `date`, `original` (`path`, `sha256`),
`toolchains`, `flag_sets`, and `modules`. A module item holds `name`,
`measured`, `record` (the identity record address or `None`), `skipped` when a
resume run copied a winner, `reason` when the module has no winner, `current`
(`toolchain`, `sources`), `winner` (the best measured row or `None`), and
`rows`. A row holds `order`, `toolchain`, `label`, `sources`, `build`,
`verdict`, `match`, `strict`, `detail`, `bytes_equal`, `bytes_total`,
`instructions_equal`, and `instructions_total`. A `sources` item holds `file`,
`compiler`, and `flags`.

The manifest document holds `date`, `report` (`path`, `sha256`), and `modules`.
It holds one row per measured module. It also holds one `unmeasured` row for a
module with an identity record but no measured result. A module with no
identity record has no row. A row holds `name`, `order`, `toolchain`, `label`,
`sources`, `measured`, `build`, `verdict`, `match`, `strict`, `detail`,
`bytes_equal`, `bytes_total`, `instructions_equal`, and
`instructions_total`. The `order`, `label`, `build`, `verdict`, `match`, and
`detail` values are the report row values, so the period pass can seed the
ranked row set directly from the manifest. The `toolchain` and `sources` are
the winner recipe for a measured module and the current recipe for an
unmeasured row. An `unmeasured` row holds `order` -1, `label` an empty string,
`measured` false, `build` `skip`, `verdict` `skip`, `match` null, `strict`
false, `detail` `no measured result`, and the four counts null. There is no
separate `winner` field. An absent `flags` field equals an empty list.

The period filter skips a module when the `base_manifest` row names the module,
the row recipe equals the module recipe, and the row is `strict`. The resume
rule skips a module when the `resume_manifest` row names the module, the row
recipe equals the module recipe, and the row is `measured`. The resume rule
skips a measured module whether its row is strict or not.

The strict-delta report document holds `toolchains` and `total`. Each toolchain
item holds `toolchain`, `modules`, `strict`, `exact-bytes`, `exact-relocations`,
`shape`, `different`, and `no-result`. The `total` item holds the same keys
without `toolchain`.

## Tasks

- [x] **Task 1: The module toolchain field, the store accessor, and the migration**
    - **Implements:** FR-015, FR-003
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_store.py`**
        - Add `toolchain="watcom10"` to `full_module`, so `test_schema_matches_module_properties` still compares the module keys with the schema properties.
        - `test_run_mode_takes_the_toolchain` — `store.run_mode(True, False, "watcom10")` gives `watcom10` and the default call gives `v2`.
        - `test_module_toolchain_defaults_to_v2` — a module without the field gives `v2`.
        - `test_module_toolchain_reads_the_field` — a module with `watcom10` gives `watcom10`.
        - `test_save_module_writes_through_the_store` — `save_module` writes the module and `load_module` returns it with the canonical indentation.
        - `test_load_module_file_reads_a_module` — the loader returns the parsed module.
        - `test_load_module_file_rejects_a_missing_file` — the error names the path.
        - `test_schema_accepts_a_module_toolchain` — a module with `watcom10` validates with no violation.
        - `test_schema_rejects_an_unknown_module_toolchain` — the violation names `toolchain`.
        - `test_schema_accepts_a_result_mode_with_watcom10` — a result mode with `watcom10` validates.
        - `test_schema_rejects_an_unknown_result_toolchain` — the violation names `toolchain`.
        - `test_tool_files_include_the_period_harness` — `TOOL_FILES` holds `rebuild/harness_dos.py`, `rebuild/watcom10_setup.py`, and `rebuild/toolchain.py`.
        - `test_fingerprint_changes_with_the_module_toolchain` — two module recipes that differ only in `toolchain` give different fingerprints.
    - **Create `tools/rebuild/tests/test_toolchain.py`**
        - Add a temporary-store test case. The migrate tests use no Docker and no DOSBox-X.
        - `test_migrate_writes_the_default_toolchain` — a module without the field gains `"toolchain": "v2"`.
        - `test_migrate_check_reports_an_unmigrated_module` — `check` returns the count and writes no file.
        - `test_migrate_is_idempotent` — a second pass writes 0 modules.
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - `test_tools_include_the_toolchain_tool` — `TOOLS` holds `rebuild/toolchain.py`. This test moves here because Task 1 changes `TOOLS`.
    - **Modify `tools/rebuild/contracts/module.schema.json`**
        - Add the optional `toolchain` property with the enum `["v2", "watcom10"]` and the description `The compiler toolchain. Absent means v2.`
        - Keep `additionalProperties: false` and the required list unchanged.
    - **Modify `tools/rebuild/contracts/record.schema.json`**
        - Change `$defs.mode.properties.toolchain.enum` from `["v2"]` to `["v2", "watcom10"]`.
    - **Modify `tools/rebuild/store.py`**
        - Add `TOOLCHAINS = ("v2", "watcom10")`.
        - Add `module_toolchain`, `load_module_file`, and `save_module`.
        - Change `run_mode` to `run_mode(strict, skip_runtime, toolchain=TOOLCHAIN)`.
        - Append `"rebuild/harness_dos.py"`, `"rebuild/watcom10_setup.py"`, and `"rebuild/toolchain.py"` to `TOOL_FILES` after `"rebuild/library.py"`.
        - Update the `run_mode`, `TOOL_FILES`, and module-record docstrings.
    - **Create `tools/rebuild/toolchain.py`**
        - Add the module docstring and the usage block for `migrate`. Each later task adds its subcommand to the usage block.
        - Import `store` and use `store.TOOLCHAINS`. Do not define a second toolchain tuple.
        - Add the constants `REPOSITORY_ROOT`, `MATRIX_DOCUMENT`, `MANIFEST_DOCUMENT`, `CANDIDATE_DIR`, `DEFAULT_FLAG_SETS`, `MATCH_RANK`, and `BUILD_FAIL_RANK`.
        - Add `migrate_modules(modules, check=False)`, `command_migrate(arguments)`, `parse_arguments`, and `main`. `migrate` loads every module in `store.MODULES_DIR`, writes the default through `store.save_module`, and prints the count. `--check` writes nothing and exits 1 when the count is not zero.
    - **Modify `tools/pipeline_check.py`**
        - Add `rebuild/toolchain.py` to `TOOLS`.
    - **Steps:**
        - Run `python3 tools/rebuild/toolchain.py migrate`. Record the written count. The shipped store holds 252 modules, so the first pass writes every module.
        - Run `python3 tools/rebuild/toolchain.py migrate --check` and confirm that it exits 0.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_store.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_toolchain.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_pipeline_check.py"`
        - `python3 -m py_compile tools/rebuild/toolchain.py tools/rebuild/store.py`
        - `python3 tools/rebuild/toolchain.py migrate --check`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`

- [x] **Task 2: The runner selection, the per-result mode, and the stage-11 fingerprint**
    - **Implements:** FR-015, FR-003
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_run.py`**
        - Add a module fixture method with a `toolchain` override.
        - `test_harness_for_selects_the_v2_harness` — `"v2"` gives `run.HARNESS_TOOL`.
        - `test_harness_for_selects_the_period_harness` — `"watcom10"` gives `run.HARNESS_DOS_TOOL`.
        - `test_harness_for_rejects_an_unknown_toolchain` — the call raises `ValueError`.
        - `test_run_build_selects_the_period_harness` — mock `run._harness` and assert the exact call `_harness(run.HARNESS_DOS_TOOL, "build", module, False, ())`.
        - `test_run_build_uses_the_v2_harness_without_a_field` — a module without the field calls `_harness(run.HARNESS_TOOL, "build", module, False, ())`.
        - `test_run_behavior_uses_the_v2_harness` — a `watcom10` module still calls `_harness(run.HARNESS_TOOL, "run", module, False, ())`.
        - `test_build_module_passes_the_recipe_option` — a recipe path gives the extra tuple `("--recipe", str(recipe))`.
        - `test_run_mode_takes_the_toolchain` — `run.run_mode(arguments, "watcom10")` gives `watcom10`.
        - `test_mode_for_module_replaces_the_toolchain` — `_mode_for_module(mode, module)` gives the module toolchain and keeps the strict and runtime values.
        - `test_mode_for_no_module_keeps_the_mode` — a `None` module returns the mode unchanged.
        - `test_result_mode_records_the_module_toolchain` — `_write_result` stores `watcom10` in `result.mode`.
        - `test_period_module_fingerprint_covers_the_toolchain` — the stored fingerprint of a `watcom10` module differs from the fingerprint of the same module with `v2`.
        - `test_library_record_keeps_the_pinned_library_mode` — a library record still writes `store.run_mode(True, False)`, whose toolchain is `v2`.
        - `test_build_module_clears_the_artifacts` — a stale exe, map, and `M.MAP` are gone before `_harness` runs.
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - Add `test_unchanged_is_verified_uses_the_module_toolchain` — a module with `toolchain` `watcom10`, `compute_fingerprint` mocked, and the call asserts `mode["toolchain"] == "watcom10"`, `mode["strict"] is True`, and the runtime value.
        - Add `test_unchanged_is_verified_uses_the_default_toolchain` — a module without the field gives `v2`.
    - **Modify `tools/rebuild/run.py`**
        - Add `HARNESS_DOS_TOOL = REBUILD_DIR / "harness_dos.py"`.
        - Add `harness_for`, `build_module`, `clear_artifacts`, and `_mode_for_module`.
        - `build_module` calls `clear_artifacts(module)` before the harness command.
        - In `build_module`, append `--recipe` when a recipe is present. Append `--timeout` only for the period harness when the timeout is not `None`.
        - Change `run_build` to call `build_module(module, show=show)`.
        - Change `run_behavior` to call `_harness(HARNESS_TOOL, "run", module, show)`.
        - Change `_harness` to take the tool and the extra arguments.
        - Change `run_mode` to take the toolchain argument.
        - In `_write_result`, apply `_mode_for_module(mode, module)` after `store.load_module`. Keep the library branch before the module load, so the library mode stays `library.library_mode()`.
        - In `_current_fingerprint`, apply `_mode_for_module(mode, module)` after the module resolves.
        - In `stale_fn`, apply `_mode_for_module(mode, module)` after the module resolves.
        - Update the module docstring and the usage block.
    - **Modify `tools/pipeline_check.py`**
        - In `unchanged_is_verified`, after `store_module.load_module`, compute `toolchain = store_module.module_toolchain(module)` and pass `mode=store_module.run_mode(True, skip_runtime, toolchain)`. Keep the library branch before the module load.
        - Update the `unchanged_is_verified` docstring.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_run.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_store.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_pipeline_check.py"`
        - `python3 tools/rebuild/run.py run --function 0x300ec --no-write` — the V2 build passes and the identity check passes.

- [x] **Task 3: The candidate matrix and the winning recipe**
    - **Implements:** FR-015, FR-003
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_toolchain.py`**
        - The matrix tests use a fake `evaluate` callable, so they need no Docker, no DOSBox-X, and no original image.
        - `test_candidate_sets_keep_the_pinned_order` — the existing set comes first, then `-of+`, `-od`, `-of+ -oi`, `-of+ -ot`, and the empty set.
        - `test_candidate_sets_label_the_existing_set` — the first label is `existing`.
        - `test_candidate_sets_drop_a_duplicate_of_the_existing_flags` — a module whose wcc386 source already uses `-of+` does not offer `-of+` twice. The duplicate rule uses the per-source list.
        - `test_candidate_sources_keep_the_source_compilers` — every item holds the source `compiler`.
        - `test_candidate_sources_keep_the_wasm_flags` — a `wasm` source keeps its flags for every candidate.
        - `test_candidate_sources_apply_one_set_to_every_wcc386_source` — every wcc386 source gets the same candidate set.
        - `test_write_recipe_keeps_the_source_compilers` — the recipe holds the wcc386 and the wasm compiler.
        - `test_write_recipe_replaces_the_wcc386_flags` — the recipe holds the candidate flags.
        - `test_write_recipe_keeps_the_wasm_flags` — the wasm flags do not change.
        - `test_write_recipe_keeps_the_run_block` — the recipe keeps the module `run` block.
        - `test_identity_record_chooses_the_lowest_address` — two identity records give the lower address.
        - `test_identity_record_is_none_without_an_identity_check` — a behavior-only module gives `None`.
        - `test_rank_orders_the_match_values` — exact bytes, exact relocations, shape, mnemonics, and different rank in that order.
        - `test_rank_breaks_a_tie_by_fewer_differing_bytes` — the row with fewer differing bytes ranks first.
        - `test_rank_breaks_a_tie_by_fewer_differing_instructions` — the row with fewer differing instructions ranks first.
        - `test_rank_breaks_a_tie_by_the_candidate_order` — equal rows rank by the lower order value.
        - `test_rank_treats_a_missing_count_as_zero` — a row without a count ranks as zero differing.
        - `test_rank_places_a_failed_build_after_every_identity` — a failed build ranks after `different`.
        - `test_winner_prefers_exact_bytes_over_exact_relocations` — the exact-byte row wins.
        - `test_winner_prefers_a_strict_match_over_a_shape_match` — exact relocations wins over same shape.
        - `test_winner_returns_a_shape_row_when_it_is_the_best_measured_row` — a shape row wins when no strict row exists.
        - `test_winner_returns_a_different_row_when_it_is_the_only_measured_row` — a different row wins when it is the only measured row.
        - `test_winner_is_none_without_a_measured_row` — every failed build gives `None`.
        - `test_row_marks_a_strict_match` — exact bytes and exact relocations hold `strict` true.
        - `test_row_marks_a_shape_match_not_strict` — same shape and same mnemonics hold `strict` false.
        - `test_candidate_descriptors_hold_the_toolchain_and_sources` — each descriptor holds `order`, `toolchain`, `label`, and the source list.
        - `test_candidate_descriptors_keep_the_global_order` — the V2 descriptors come first and the order ascends.
        - `test_matrix_module_measures_each_selected_candidate` — the row count equals the candidate count.
        - `test_matrix_module_tries_v2_first` — the V2 rows come before the period rows.
        - `test_matrix_module_stops_at_the_first_exact_bytes` — a V2 exact-byte row stops the module.
        - `test_matrix_module_skips_the_period_with_a_strict_v2_result` — a V2 exact-relocations row stops the period candidates.
        - `test_matrix_module_tries_the_period_without_a_strict_v2_result` — a V2 shape row is followed by the period rows.
        - `test_matrix_module_reports_a_module_without_an_identity_record` — `measured` is false and the reason is `no identity record`.
        - `test_matrix_module_reports_a_module_without_a_measured_result` — `measured` is false and the reason is `no measured result`.
        - `test_matrix_period_filter_skips_a_strict_v2_module` — a strict base row stops the period build.
        - `test_matrix_period_pass_measures_a_non_strict_v2_module` — a shape base row measures the period rows.
        - `test_matrix_period_pass_keeps_a_better_v2_winner` — the V2 winner wins the ranking over a worse period row.
        - `test_matrix_period_pass_takes_a_better_period_winner` — the period row wins the ranking over the V2 winner.
        - `test_matrix_only_missing_skips_a_measured_module` — a measured resume row is skipped, whether strict or not.
        - `test_matrix_only_missing_measures_an_unmeasured_module` — a resume row with `measured` false is measured.
        - `test_matrix_only_missing_without_a_resume_manifest_measures_every_module` — the resume manifest is `None`.
        - `test_matrix_returns_the_report_without_a_module_write` — mock `store.save_module` and assert no call.
        - `test_command_matrix_writes_the_report` — `command_matrix` writes `--out`.
        - `test_command_matrix_writes_the_report_after_each_module` — a two-module pass writes one row after the first item and two after the second.
        - `test_command_matrix_rejects_a_missing_original` — the command exits 2 before the first build.
        - `test_command_matrix_rejects_a_module_without_a_source` — the command exits 2 before the first build.
        - `test_command_matrix_writes_the_winner` — `command_matrix` calls `store.save_module` with the winner recipe.
        - `test_command_matrix_dry_run_writes_no_module` — the report exists and the module bytes do not change.
        - `test_clear_artifacts_removes_the_old_outputs` — the exe, the map, `M.MAP`, and the log are gone.
        - `test_matrix_clears_before_each_candidate` — `clear_artifacts` is called before each `evaluate`.
        - `test_parse_arguments_accepts_the_matrix_flags` — `matrix --module free --dry-run`, `matrix --all`, `matrix --toolchain watcom10`, `matrix --only-missing --resume PATH`, `matrix --manifest PATH`, and `matrix --out PATH` parse.
    - **Modify `tools/rebuild/harness.py`**
        - Add `load_recipe(name, recipe)`.
        - Change `command_build` to `command_build(name, recipe=None)`.
        - Add `--recipe` to the `build` subparser.
        - In `main`, pass `arguments.recipe` to `command_build`.
        - Update the module docstring and the usage block.
    - **Modify `tools/rebuild/harness_dos.py`**
        - Add `load_recipe(name, recipe)`, `clear_artifacts(module)`, and `log_failures(text)`.
        - Change `command_build` to `command_build(name, timeout, recipe=None)`. It calls `clear_artifacts`, deletes the log, and raises `ValueError` when `log_failures` is not empty.
        - Add `--recipe` to the `build` subparser.
        - In `main`, pass `arguments.recipe` to `command_build`.
        - Update the module docstring and the usage block. State that a failed build cannot look like a pass.
    - **Modify `tools/rebuild/tests/test_harness.py`**
        - `test_load_recipe_reads_the_record` — a temporary recipe file gives the module.
        - `test_load_recipe_rejects_a_name_mismatch` — the record name differs and the call raises `ValueError`.
        - `test_parse_arguments_accepts_a_recipe` — the parsed value is a `Path`.
        - `test_main_passes_the_recipe_to_build` — mock `command_build`, call `main`, and assert the recipe path.
    - **Modify `tools/rebuild/tests/test_harness_dos.py`**
        - `test_load_recipe_reads_the_record` — a temporary recipe file gives the module.
        - `test_load_recipe_rejects_a_name_mismatch` — the record name differs and the call raises `ValueError`.
        - `test_parse_arguments_accepts_a_recipe` — the parsed value is a `Path`.
        - `test_main_passes_the_recipe_to_build` — mock `command_build`, call `main`, and assert the recipe path and the timeout argument.
        - `test_command_build_removes_a_stale_exe` — a stale exe, map, and `M.MAP` are gone before the build.
        - `test_command_build_fails_on_an_error_log` — a log with an error line raises `ValueError`.
        - `test_log_failures_reads_an_error_line` — the line `file.c(10): Error! E0000: ...` is returned.
        - `test_log_failures_accepts_a_warning_line` — the warning is not a failure.
    - **Modify `tools/rebuild/toolchain.py`**
        - Add `import le` and import `run`. `is_strict` uses `run.STRICT_MATCHES`.
        - Add `candidate_sets`, `candidate_sources`, `candidate_descriptors`, `write_recipe`, `identity_record`, `is_strict`, `rank`, `winner`, `apply_winner`, `clear_artifacts`, `evaluate`, `matrix_module`, `matrix`, and `command_matrix`.
        - `candidate_descriptors` composes `candidate_sets` and `candidate_sources`. The `matrix_module` concatenates the V2 and the period descriptors and rewrites `order` to the global index, V2 first.
        - `evaluate` writes a candidate recipe with `write_recipe` to `CANDIDATE_DIR / f"{module['name']}-{descriptor['order']:02d}.json"`. It calls `run.build_module(module, descriptor["toolchain"], recipe, timeout=timeout)`. On a pass it calls `run.run_identity(record, module, original, strict=True)` and copies the check fields into the row. On a fail or a skip it records the build verdict and no match.
        - `matrix_module` measures the V2 candidates first. It stops the module at the first `exact-bytes`. It tries the period candidates only when no V2 row is strict. It stops the period at the first `exact-bytes`. For a period pass, it seeds the row set with the `base_manifest` winner and keeps the better row by `rank`, so a better V2 winner stays and a better period row replaces it. The winner is the best measured row, so a shape or different row can win when it is the best measured row.
        - `matrix` takes the selected modules and records and returns the report. It calls `on_module(item)` after each module item, in module order. It writes no module and no report. It calls `clear_artifacts` before each candidate. It skips a module with a strict `base_manifest` row, skips a module that `resume_manifest` already measured, and copies the skipped winner or unmeasured row.
        - `command_matrix` loads the modules and the function records, validates the original image with `le.load`, and checks every selected module for `name`, `exe`, and `sources`, then calls `matrix` with an `on_module` callback. The callback appends the item to the report and writes the report to `--out` (default `MATRIX_DOCUMENT`) after each module item through an atomic replace. It applies the winners through `store.save_module` unless `--dry-run`, and prints one line per module and a summary line with the measured, the unmeasured, and the strict counts.
        - The module selection is `--module NAME` or `--all`. The two are exclusive and one is required. `--toolchain` is repeatable and restricts the candidates. `--manifest` is the base manifest for a period pass. `--only-missing` reads `--resume` and skips a module the current pass already measured. A missing base or resume manifest measures every module. `--original` defaults to `store.DEFAULT_ORIGINAL`. `--timeout` passes to the period harness. `--dry-run` writes the report and no module. `--out` defaults to `MATRIX_DOCUMENT`.
        - The matrix exits 0 when it writes the report, 2 on a bad input, and 3 when every candidate skips because the engine is missing.
        - No shipped module holds `period_flags`. The existing candidate uses the source `flags` for both toolchains. A winner write removes any `period_flags`.
        - The matrix writes no function result and no status. Task 6 runs the runner to write them.
        - Extend `parse_arguments` and `main` with `matrix`.
        - Update the usage block.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_toolchain.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_harness.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_harness_dos.py"`
        - `python3 tools/rebuild/toolchain.py matrix --module error_report --dry-run --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE` — the report shows the V2 and the period rows, and the command prints the dry-run line for the module.

- [x] **Task 4: The strict-delta report, the manifest, and the stage-12 check**
    - **Implements:** FR-015, FR-018
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_toolchain.py`**
        - `test_report_counts_modules_per_toolchain` — one module under `v2` and one under `watcom10` gives one module in each row.
        - `test_report_counts_the_strict_matches` — `exact-bytes` and `exact-relocations` count in `strict`.
        - `test_report_counts_each_match_value` — shape, different, and no-result counts are separate.
        - `test_report_counts_a_module_without_an_identity_record` — the module counts in `no-result`.
        - `test_report_groups_by_the_identity_record` — a module recipe of `watcom10` with an identity result mode of `v2` counts under `v2` until the re-baseline.
        - `test_format_report_prints_the_total_line` — the header, one toolchain line, and the total line.
        - `test_report_is_stable` — the same records give the same document.
        - `test_manifest_lists_every_measured_module` — one row per measured module.
        - `test_manifest_lists_an_unmeasured_module_with_an_identity_record` — one `unmeasured` row for a module with an identity record and no measured result.
        - `test_manifest_row_shape` — a measured row holds `name`, `order`, `toolchain`, `label`, `sources`, `measured`, `build`, `verdict`, `match`, `strict`, `detail`, and the four counts, and it holds no `winner` field. An `unmeasured` row holds `order` -1, an empty `label`, `measured` false, `build` `skip`, `verdict` `skip`, `match` null, `strict` false, `detail` `no measured result`, and the four null counts.
        - `test_manifest_measured_row_can_be_ranked` — `rank` accepts a measured manifest row and gives the same key as the report row with the same values.
        - `test_manifest_omits_a_module_without_an_identity_record` — the driver has no row.
        - `test_manifest_records_the_report_digest` — the digest matches the report file.
        - `test_check_matrix_accepts_a_matching_manifest` — no message.
        - `test_command_check_matrix_reports_a_missing_manifest` — the command exits 1 with a message that names the path.
        - `test_check_matrix_reports_a_changed_toolchain` — one message names the module.
        - `test_check_matrix_reports_changed_flags` — one message names the module and the file.
        - `test_check_matrix_reports_a_changed_compiler` — one message names the module and the file.
        - `test_check_matrix_reports_a_row_without_a_module` — one message.
        - `test_check_matrix_reports_a_module_with_an_identity_record_without_a_row` — one message.
        - `test_check_matrix_reports_a_row_for_a_module_without_an_identity_record` — one message.
        - `test_check_matrix_treats_an_absent_flags_field_as_empty` — no message.
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - `test_stage_twelve_checks_the_toolchain_matrix` — mock `pipeline_check.run` and assert the `toolchain.py check-matrix --manifest <str(pipeline_check.toolchain.MANIFEST_DOCUMENT)>` command and the check name `stage 12: the toolchain matrix matches the module recipes`.
    - **Modify `tools/rebuild/toolchain.py`**
        - Add `report_document`, `format_report`, `command_report`, `manifest_document`, `command_manifest`, `check_matrix`, and `command_check_matrix`.
        - `report_document` returns `toolchains` and `total`. Each item holds `toolchain`, `modules`, `strict`, `exact-bytes`, `exact-relocations`, `shape`, `different`, and `no-result`. The `total` item holds the same keys without `toolchain`. It uses `identity_record(records, module["name"])` and reads that record's `result.mode.toolchain` and `result.checks.identity.match`. A module with no identity record counts in `no-result` under `store.module_toolchain(module)`. The shape count holds `same-shape` and `same-mnemonics`. `format_report` prints the header `toolchain modules strict exact-bytes exact-relocations shape different no-result`, one line per toolchain with the eight values, and the total line.
        - `manifest_document` holds one row per measured module with the winner recipe. It also holds one `unmeasured` row for a module with an identity record but no measured result, with the current recipe and `measured` false. A module with no identity record has no row. The `toolchain` and `sources` fields are the winner recipe. The row holds `order`, `label`, `build`, `verdict`, `detail`, `measured`, `strict`, `match`, and the counts, and it holds no `winner` field. An `unmeasured` row writes `order` -1, an empty `label`, `measured` false, `build` `skip`, `verdict` `skip`, `match` null, `strict` false, `detail` `no measured result`, and the four null counts, so every row has one shape and the period pass can rank a seeded row directly.
        - `check_matrix` takes the modules, the records, the parsed manifest, and the path. It reports a row without a module, a module with an identity record and no row, a row for a module without an identity record, a toolchain that differs, a source compiler that differs, and a source flag list that differs. An absent `flags` field equals an empty list. `check_matrix` needs the records to prove the "identity record and no row" rule.
        - Add `report`, `manifest`, and `check-matrix` to `parse_arguments` and `main`.
        - `report` prints the document and exits 0. `manifest` reads `--report` (default `MATRIX_DOCUMENT`) and writes `--out` (default `MANIFEST_DOCUMENT`). The default is intentional: the final command writes the committed manifest, and the intermediate passes pass `--out`. `check-matrix` reads `--manifest` (default `MANIFEST_DOCUMENT`). It exits 1 on a message and also exits 1 with a message when the manifest file is missing or broken.
        - Update the usage block.
    - **Modify `tools/pipeline_check.py`**
        - Add `import toolchain` beside the `store` import.
        - In `check_store`, run `toolchain.py check-matrix --manifest str(toolchain.MANIFEST_DOCUMENT)` and add the check `stage 12: the toolchain matrix matches the module recipes`.
        - Update the stage 12 docstring.
    - **Note:** The stage-12 check reads the committed manifest, not the disposable full report, so the check runs in CI without Docker or DOSBox-X. The full report stays under `work/rebuild/` as the detailed evidence.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_toolchain.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_pipeline_check.py"`
        - `python3 tools/rebuild/toolchain.py report` — the baseline prints one line per toolchain.
        - Do not run the pipeline before Task 6. The final manifest is written in Task 6, so the stage-12 check fails until then.

- [x] **Task 5: The V2 matrix pass**
    - **Implements:** FR-015
    - No new unit test. This task writes data. The Verify commands prove the data.
    - **Steps:**
        - Confirm that Docker and `tools/rebuild/harness.py` are usable. The V2 pass needs no DOSBox-X and no period toolchain.
        - Run `python3 tools/rebuild/toolchain.py report`. Record the baseline module count per toolchain and the strict counts.
        - Run `python3 tools/rebuild/toolchain.py matrix --all --toolchain v2 --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --out work/rebuild/toolchain-v2.json`. The pass measures the V2 candidates for every module, stops a module at the first `exact-bytes`, and writes the V2 winners through `store.save_module`. Record the measured, the unmeasured, and the strict counts. The V2 pass can take one to two hours. The V2 harness has no per-build timeout. The partial report and the resume are the only budget guard.
        - If the session ends, the partial report stays at `work/rebuild/toolchain-v2.json`. Run `python3 tools/rebuild/toolchain.py manifest --report work/rebuild/toolchain-v2.json --out work/rebuild/toolchain-v2-manifest.json`, then resume with `--only-missing --resume work/rebuild/toolchain-v2-manifest.json` and the same `--out`. The resume skips a module that the current pass already measured, whether its row is strict or not.
        - Run the matrix a second time for one module: `python3 tools/rebuild/toolchain.py matrix --module error_report --toolchain v2 --out work/rebuild/toolchain-v2-recheck.json --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`. Confirm that the module recipe does not change.
        - Run `python3 tools/rebuild/toolchain.py manifest --report work/rebuild/toolchain-v2.json --out work/rebuild/toolchain-v2-manifest.json`.
        - Run `python3 tools/rebuild/toolchain.py check-matrix --manifest work/rebuild/toolchain-v2-manifest.json`.
        - Run `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
        - Do not re-baseline the function results in this task. The stored results are stale after the V2 recipe writes. Task 6 re-baselines.
    - **The acceptance bar:** Every measured module records its best measured V2 candidate. A module with no identity record is unmeasured and keeps its recipe. The V2 report and the intermediate manifest agree.
    - **Verify:**
        - `python3 tools/rebuild/toolchain.py check-matrix --manifest work/rebuild/toolchain-v2-manifest.json`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`

- [x] **Task 6: The period matrix pass, the re-baseline, and the strict delta**
    - **Implements:** FR-015, FR-003, FR-018
    - No new unit test. This task writes data. The Verify commands prove the data.
    - **Steps:**
        - Confirm that DOSBox-X and `work/watcom10/WATCOM/BINB/WCC386.EXE` exist.
        - Run `python3 tools/rebuild/toolchain.py matrix --all --toolchain watcom10 --manifest work/rebuild/toolchain-v2-manifest.json --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --out work/rebuild/toolchain-matrix.json`. The pass measures the period candidates only for a module without a strict V2 result. It stops a module at the first period `exact-bytes`, seeds the row set with the V2 winner from the base manifest, keeps the better row by `rank`, and writes the best measured winner through `store.save_module`. Record the measured, the unmeasured, and the strict counts.
        - The period pass can take several hours and may need more than one session. If the session ends, the partial report stays at `work/rebuild/toolchain-matrix.json`. Run `python3 tools/rebuild/toolchain.py manifest --report work/rebuild/toolchain-matrix.json --out work/rebuild/toolchain-period-manifest.json`, then resume with `--manifest work/rebuild/toolchain-v2-manifest.json --only-missing --resume work/rebuild/toolchain-period-manifest.json --out work/rebuild/toolchain-matrix.json`. The resume skips a module that the current period pass already measured, whether its row is strict or not. A module with no period row is measured.
        - Run `python3 tools/rebuild/toolchain.py manifest --report work/rebuild/toolchain-matrix.json --out tools/rebuild/toolchain.json`. The default `--out` is `MANIFEST_DOCUMENT`, so the bare command writes the committed manifest. The intermediate passes pass `--out` explicitly.
        - Run `python3 tools/rebuild/toolchain.py check-matrix --manifest tools/rebuild/toolchain.json`.
        - Re-baseline the store: `python3 tools/rebuild/run.py run --all`. The command exits 1 when a declared check still fails. Every module-backed result now holds the module toolchain and the new fingerprint.
        - Run `python3 tools/rebuild/run.py run --all --skip-unchanged` and record the summary line and the FAIL lines. Every record with a stored result prints the unchanged detail with its stored verdict. A record with no name or no declared checks prints `SKIP`. The command exits 1 when a stored result holds a FAIL. Confirm that every FAIL line names a record whose stored result holds a fail. The stage-11 check proves the stored verdicts and fingerprints.
        - Run `python3 tools/rebuild/toolchain.py report`. Record the after module count per toolchain and the strict counts. The difference from the baseline is the measured delta.
        - Confirm that at least one module records `watcom10` and that its `run.py` result holds `result.mode.toolchain` `watcom10`. If no module wins with the period toolchain, stop and report the matrix rows. Do not force a toolchain.
        - Run `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
        - Run `python3 tools/rebuild/callgraph.py build` and `python3 tools/rebuild/callgraph.py check`.
        - Run `python3 tools/rebuild/docs.py render` and `python3 tools/rebuild/docs.py check`.
        - Run `python3 tools/rebuild/view.py html --out work/rebuild/status.html` and confirm that the page holds no `data-stale="true"` marker.
        - Spot check one period module record: it holds `toolchain` `watcom10`, the winner flags, and `result.mode.toolchain` `watcom10`.
        - Spot check `tools/rebuild/toolchain.json`: it holds one row per measured module, one `unmeasured` row for a module with an identity record and no measured result, and the report digest.
    - **The acceptance bar:** Every measured module records its best measured compiler and flags. A module with no identity record is unmeasured and keeps its recipe. The strict delta from the matrix is measured. The period toolchain runs in the store path for at least one module. If no module wins with the period toolchain, stop and report the matrix rows before Task 7. Do not force a toolchain.
    - **Verify:**
        - `python3 tools/rebuild/toolchain.py check-matrix --manifest tools/rebuild/toolchain.json`
        - `python3 tools/rebuild/toolchain.py report`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross` — stage 11 proves the summary line, the stored verdicts, and the stored fingerprints.
        - `! grep -q 'data-stale="true"' work/rebuild/status.html`
        - `python3 tools/rebuild/docs.py check`
        - `python3 tools/rebuild/callgraph.py check`

- [x] **Task 7: The documentation and the observed counts**
    - **Implements:** FR-018
    - **Modify `AGENTS.md`**
        - Project Structure: add `toolchain.py` under `tools/rebuild/` and `toolchain.json` beside `library.json`.
        - Build And Test Commands: add `toolchain.py` to both `py_compile` lists. Add the `toolchain.py migrate`, `toolchain.py matrix`, `toolchain.py report`, `toolchain.py manifest`, and `toolchain.py check-matrix` commands.
        - Testing: add a bullet for the module `toolchain` field and the default `v2`, a bullet for the runner selection, a bullet for the result mode and the fingerprint, a bullet for the matrix ranking and the candidate order, and a bullet for the committed manifest and the stage-12 check. State that the matrix needs Docker, DOSBox-X, and the period toolchain. State that the unit suite needs none of them.
        - Architecture: state that a measured module records its best measured toolchain and flags, and that a module with no identity record is unmeasured and keeps its recipe.
        - Update the four check counts to the observed values from the pipeline runs in Verify. Each run reports one more check than the current count.
    - **Modify `README.md`**
        - Add a Status bullet: a toolchain matrix that records the best measured compiler and flags for each module.
        - Update the four check counts to the observed values.
    - **Modify `docs/reference/tooling.md`**
        - Add `toolchain.py` to the rebuild tool table and `toolchain.json` to the committed data files.
        - Add a section for the toolchain matrix: the candidates, the candidate order, the ranking, the full report path, the committed manifest path, the runner selection, and the behavior harness rule.
        - Add the `toolchain` module field to the `store.py` paragraph. State that the recipe field enters the fingerprint and that `harness_dos.py`, `watcom10_setup.py`, and `toolchain.py` are tool inputs.
        - Update the four check-count lines to the observed values.
    - **Modify `docs/explanation/findings.md`**
        - Insert `## The toolchain matrix and the strict delta` before `## Open questions`.
        - Record the baseline and the after module counts per toolchain and the strict counts. Record the exact commands.
        - Record the modules whose winning toolchain is `watcom10`, with the match and the byte and instruction counts.
        - Record the modules with no identity record and the reason.
        - Record the observed full-check counts.
    - **Modify `docs/explanation/rebuild-quality-design.md`**
        - Mark the C-10 counts 431, 427, 416, and 412 as the library-pass counts. Add the observed counts from the pipeline runs in Task 7.
        - Extend C-4 to name the module store and `store.save_module`: the matrix writes module records through the store in the same way `triage.apply` and `library.apply` write function records through `store.save_record`.
        - Add a Design Decisions bullet: a module recipe holds an optional `toolchain` value with the default `v2`. The runner selects the harness by the module toolchain. The behavior check always uses the V2 harness. The matrix records the best measured candidate for every measured module. The ranking is the match order, then fewer differing bytes, then fewer differing instructions, then the earlier candidate. A strict match is preferred but not required. A module with no identity record is unmeasured and keeps its recipe. The full report stays under `work/rebuild/` and the committed manifest is the stage-12 input.
    - **Modify `docs/explanation/rebuild-quality-roadmap.md`**
        - Set the Slice 5 `Done when` to: each measured module records its best measured compiler and flags. A module with no identity record is unmeasured and keeps its recipe. The strict delta from the matrix is measured. The period toolchain runs in the store path. Add Docker for the V2 harness and the Ghidra loader for the full check to the manual prerequisites, or state that they are for the full check.
        - Set the Slice 5 Status to `Implemented` and the Plan to `docs/explanation/rebuild-quality-plan-toolchain.md`. Do this after the final review approves the slice.
    - **Modify `.github/workflows/pipeline.yml`**
        - Add `tools/rebuild/toolchain.py` to the syntax-job `py_compile` list, after `tools/rebuild/library.py`.
    - **Steps:**
        - Run `python3 -m py_compile tools/le.py tools/inventory.py tools/extract_cd_image.py tools/dosboxx_control.py tools/entry_probe.py tools/gfx_container.py tools/export_assets.py tools/sound_bank.py tools/member_probe.py tools/cross_check.py tools/rebuild/diff_module.py tools/rebuild/oracle.py tools/rebuild/store.py tools/rebuild/schema.py tools/rebuild/image_compare.py tools/rebuild/harness.py tools/rebuild/harness_dos.py tools/rebuild/watcom10_setup.py tools/rebuild/run.py tools/rebuild/view.py tools/rebuild/docs.py tools/rebuild/callgraph.py tools/rebuild/triage.py tools/rebuild/coverage.py tools/rebuild/library.py tools/rebuild/toolchain.py tools/pipeline_check.py`.
        - Run the four pipeline commands in Verify. Record the four observed counts. Do not write a forecast count.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/toolchain.py check-matrix --manifest tools/rebuild/toolchain.json`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-cross`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime`
        - `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross`
        - Each run reports one more check than the current count. Record the observed values.

## Deviations

- The matrix writes each winner inside the per-module callback, not at the end
  of the pass.  Reason: the partial-report resume rule compares the resume row
  recipe with the module recipe, so the store recipe must hold the winner before
  the pass ends.
- The period `wasm` gains `-fpi87`.  Reason: without the flag the period
  assembler rejects the FPU stubs, so a module with a stub fails every period
  candidate.
- `wait_for_artifacts` stops when the DOSBox-X process exits.  Reason: a failed
  period build otherwise waits the full timeout for each candidate.
- The period pass measured 60 `watcom10` winners and one new strict match
  (`main_invoker`).  No toolchain was forced.
