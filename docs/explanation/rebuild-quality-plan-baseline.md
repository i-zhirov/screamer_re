# Truthful Verification Baseline Implementation Plan

> Execute this plan with the `executing-plans` skill. Each task is a
> checkbox. Mark it done when its changes are implemented and its Verify
> command passes.

- **Date**: 2026-11-11
- **Status**: Implemented
- **Design**: docs/explanation/rebuild-quality-design.md
- **Roadmap**: docs/explanation/rebuild-quality-roadmap.md — slice: baseline
- **Depends on**: —
- **Manual prerequisites**: Docker for the V2 harness, DOSBox-X for the behavior checks, and the installed Ghidra loader for the full check.
- **Goal**: Make the reported rebuild number true. A stored `fail` must never
  print `PASS`, a failed check must demote the record, the strict mode and the
  toolchain must enter the fingerprint, strict becomes the default gate, the
  behavior check uses fresh output and complete lines, and the report separates
  exact, relocation-only, shape, and behavior-only results.

## Constraints

- Python 3.14.7. Use the standard library and the `ctypes` capstone binding only.
- Tests use `unittest` under `tools/rebuild/tests/` with temporary directories.
- A tool takes its configuration from the command line. It reads no environment variable.
- A tool writes only under `work/` or to an explicit output path.
- The identity `match` values are `exact-bytes`, `exact-relocations`, `same-shape`, `same-mnemonics`, and `different`.
- The report buckets are `exact`, `relocation-only`, `shape`, and `behavior-only`.
- The `same-mnemonics` match maps to the `shape` bucket.
- The `mode` object holds `strict` (boolean), `toolchain` (`v2`), and `runtime` (`run` or `skip`).
- The input fingerprint hashes the strict mode and the toolchain. It does not hash the runtime mode. The result mode records the runtime for reporting.
- Strict is the default identity gate. `--allow-shape` restores the historical gate. `--strict` stays accepted.
- A failed build sets `named` when the record has a name, else `unknown`. The failed result stays on the record.
- A failed behavior check sets `matching` when identity passes, else `named` or `unknown`.
- Stage 11 stays a drift check. It accepts a stored `fail` as the expected result and shows `FAIL`.
- A behavior run deletes the old output first. A marker matches a complete output line.
- The status ladder and the identity verdict strings do not change.
- The schema keeps `match` and `mode` optional during the transition. The re-baseline writes them.
- The full check reports 303 checks. `--skip-cross` reports 299, `--skip-runtime` reports 288, and both flags report 284. This slice adds no pipeline check, so the counts stay.
- Do not commit. The user commits.

## Shared Interfaces

- `store.TOOLCHAIN = "v2"`.
- `store.run_mode(strict: bool, skip_runtime: bool) -> dict` returns `{"strict": ..., "toolchain": "v2", "runtime": "run" | "skip"}`.
- `store.compute_fingerprint(record, module, *, ..., mode: dict | None = None) -> str` feeds the strict and toolchain fields of the mode into the hash after the record fields and before the tool sources. It does not feed the runtime field.
- `store.advance_status(checks: list[str], verdicts: dict[str, str], has_name: bool = False) -> str | None`.
- `store.stored_verdict(record: dict) -> str` returns `FAIL` when a stored check verdict is `fail`, else `PASS`.
- `store.MATCH_BUCKETS = ("exact", "relocation-only", "shape", "behavior-only")`.
- `store.match_bucket(match: str | None) -> str | None`.
- `store.result_bucket(checks: dict[str, dict], declared: list[str]) -> str | None` maps the identity `match` to a bucket regardless of the identity verdict. A `different` or absent match yields `None`. It returns `behavior-only` only when `identity` is not declared and the behavior check passed.
- `run.match_of(verdict: str) -> str`.
- `run.identity_accepted(match: str, strict: bool) -> bool`.
- `run.summarize(verdicts: list[str], buckets: list[str | None]) -> str`.
- `run.run_mode(arguments: argparse.Namespace) -> dict`.
- `harness.clear_output(path: Path) -> None`.
- `pipeline_check.unchanged_matches(record: dict, verdict: str) -> bool`.
- `pipeline_check.unchanged_is_verified(record: dict, game_dir: Path, skip_runtime: bool) -> bool`.
- `view._match(record: dict) -> str`, `view._match_counts(rows: list[dict]) -> dict[str, int]`, and `view._match_counters(rows: list[dict]) -> str`.

## Tasks

- [x] **Task 1: Record contract for the match and mode fields**
    - **Implements:** FR-003, FR-004
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_store.py`**
        - `test_schema_accepts_a_result_with_a_mode` — a result with `mode` equal to `{"strict": True, "toolchain": "v2", "runtime": "run"}` validates with no violation.
        - `test_schema_rejects_an_unknown_runtime_mode` — `runtime` equal to `sometimes` yields one violation that names `runtime`.
        - `test_schema_accepts_each_identity_match` — each of the five values validates.
        - `test_schema_rejects_an_unknown_identity_match` — `match` equal to `close` yields one violation that names `match`.
    - **Modify `tools/rebuild/contracts/record.schema.json`**
        - Add `match` to `$defs.check_result.properties` with the enum `exact-bytes`, `exact-relocations`, `same-shape`, `same-mnemonics`, `different`.
        - Add `mode` to `$defs.result.properties` as `{"$ref": "#/$defs/mode"}`.
        - Add `$defs.mode` with `additionalProperties: false`, `required: ["strict", "toolchain", "runtime"]`, `strict` boolean, `toolchain` enum `["v2"]`, `runtime` enum `["run", "skip"]`.
        - Keep `match` and `mode` out of the required lists. An older result stays valid until the re-baseline.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_store.py"`

- [x] **Task 2: Mode in the input fingerprint**
    - **Implements:** FR-003
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_store.py`**
        - Add a `mode` parameter to the `fingerprint_of` helper with default `None`.
        - `test_run_mode_holds_the_three_fields` — the strict false case gives `runtime` equal to `run` and `toolchain` equal to `v2`, and the skip case gives `runtime` equal to `skip`.
        - `test_fingerprint_changes_with_strict_mode` — the strict and allow-shape modes produce different fingerprints.
        - `test_fingerprint_ignores_the_runtime_mode` — the run and skip modes produce the same fingerprint when the strict mode and the toolchain are equal.
        - `test_fingerprint_is_stable` stays green with a fixed mode.
    - **Modify `tools/rebuild/store.py`**
        - Add `TOOLCHAIN = "v2"`.
        - Add `run_mode(strict: bool, skip_runtime: bool) -> dict` with the three fields. The runtime field stays in the returned object for reporting.
        - Add `_mode_fields(mode: dict | None) -> bytes`. A `None` mode contributes the marker `b"<none>"`. A present mode contributes `json.dumps({"strict": mode["strict"], "toolchain": mode["toolchain"]}, sort_keys=True, separators=(",", ":")).encode("utf-8")`. The runtime field does not enter the hash.
        - Extend `compute_fingerprint` with `mode: dict | None = None` and feed `("mode", _mode_fields(mode))` after the record fields and before the tool sources.
        - Extend the `compute_fingerprint` docstring to name the strict mode and the toolchain as inputs and the runtime as a reported value that is not hashed.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_store.py"`

- [x] **Task 3: Status demotion and result buckets**
    - **Implements:** FR-002, FR-004
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_store.py`**
        - Update every `advance_status` call with the name flag.
        - `test_advance_status_build_fails` is replaced by `test_advance_status_build_fails_demotes_a_named_record` and `test_advance_status_build_fails_demotes_an_unnamed_record`. The old test expects `None`. The new tests expect `named` and `unknown`.
        - `test_advance_status_behavior_fails_without_identity_demotes_a_named_record` — the result is `named`.
        - `test_advance_status_behavior_fails_without_identity_demotes_an_unnamed_record` — the result is `unknown`.
        - `test_advance_status_build_passes_and_behavior_fails_demotes_a_named_record` — a passing build and a failed behavior with no identity give `named`.
        - `test_advance_status_build_passes_and_behavior_fails_demotes_an_unnamed_record` — the same case gives `unknown`.
        - `test_advance_status_build_and_behavior_pass_gives_verified` — `["build"]` alone gives `rebuilt`. `["build", "behavior"]` with both passing gives `verified`.
        - `test_validate_record_rejects_verified_after_a_failed_build` — the status `verified` with a failed build result yields a violation that contains `stronger`.
        - `test_stored_verdict_reports_a_stored_fail` — the result is `FAIL`.
        - `test_stored_verdict_reports_a_stored_pass` — the result is `PASS`.
        - `test_match_bucket_maps_each_match` — `exact-bytes` gives `exact`, `exact-relocations` gives `relocation-only`, `same-shape` and `same-mnemonics` give `shape`, and `different` gives `None`.
        - `test_result_bucket_prefers_identity` — a passing identity and a passing behavior give the identity bucket.
        - `test_result_bucket_reports_behavior_only` — a declared behavior that passed with no identity gives `behavior-only`.
        - `test_result_bucket_maps_a_failed_shape_match` — a failed identity with `match` `same-shape` gives `shape`. A failed identity with `match` `same-mnemonics` also gives `shape`.
        - `test_result_bucket_ignores_a_failed_different_match` — a failed identity with `match` `different` gives `None`.
    - **Modify `tools/rebuild/store.py`**
        - Change `advance_status` to `advance_status(checks: list[str], verdicts: dict[str, str], has_name: bool = False) -> str | None`. The default `False` keeps the current `run._next_status` call working until Task 5 passes the real name flag. Apply these rules in order.
            - No declared check gives `None`.
            - Any `skip` gives `None`.
            - A declared `build` with a verdict other than `pass` gives `named` when `has_name`, else `unknown`.
            - A declared `build` that passes with no declared `identity` gives `named` when `has_name`, else `unknown`, when a behavior check is declared and its verdict is not `pass`. It gives `verified` when a behavior check is declared and it passes. A build-only pass gives `rebuilt`.
            - A declared `identity` with a verdict other than `pass` gives `rebuilt`.
            - A declared `identity` that passes with a declared `behavior` verdict other than `pass` gives `matching`.
            - A declared `identity` that passes gives `verified`.
            - A declared `behavior` that passes gives `verified`. A declared `behavior` that fails gives `named` when `has_name`, else `unknown`.
        - Change `supported_status` to pass `bool(record.get("name"))`.
        - Add `stored_verdict(record: dict) -> str`. It returns `FAIL` when any stored check verdict is `fail`, else `PASS`.
        - Add `MATCH_BUCKETS` and `match_bucket`.
        - Add `result_bucket`. It maps the identity `match` to a bucket regardless of the identity verdict. A `different` or absent match yields `None`. It returns `behavior-only` only when `identity` is not declared and the behavior check passed.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_store.py"`

- [x] **Task 4: Runner identity gate, match, mode, and summary**
    - **Implements:** FR-003, FR-004
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_run.py`**
        - `test_parse_arguments_defaults_to_strict` — the parsed `strict` is true.
        - `test_parse_arguments_allow_shape_restores_the_historical_gate` — `--allow-shape` gives a false `strict`.
        - `test_parse_arguments_accepts_the_strict_flag` — `--strict` gives a true `strict`.
        - `test_runner_mode_holds_the_three_fields` — strict, `v2`, and the runtime value.
        - `test_match_of_maps_each_verdict` — the five verdict strings map to the five values, and an unknown string maps to `different`.
        - `test_identity_accepted_uses_the_match` — replaces `test_identity_accepted_default_and_strict`. The strict list accepts only `exact-bytes` and `exact-relocations`. The historical list under `--allow-shape` also accepts `same-shape` and `same-mnemonics`. Neither list accepts `different`.
        - `test_strict_identity_rejects_a_reordered_sequence` is rewritten in place. It calls `run_identity` with no strict argument and expects a pass today. The default becomes strict, so the no-argument call must expect `fail`. The `strict=False` call must expect `pass`.
        - `test_identity_result_holds_the_match` — the check result holds the match value.
        - `test_summary_separates_the_match_buckets` — replaces `test_summary`. The pinned example is `"4 records: 2 pass, 1 fail, 1 skip; exact 1, relocation-only 0, shape 1, behavior-only 0"`. The empty case is `"0 records: 0 pass, 0 fail, 0 skip; exact 0, relocation-only 0, shape 0, behavior-only 0"`.
        - `test_result_holds_the_mode` — `_result` writes the mode object.
        - `test_skip_unchanged_runs_a_record_with_a_changed_mode` — a stored strict result and an `--allow-shape` run call the identity check and do not print the unchanged detail.
        - `test_skip_unchanged_ignores_the_runtime_mode` — a stored result with `mode=store.run_mode(True, False)` still prints the unchanged detail under `--skip-runtime`, because the runtime does not enter the fingerprint.
        - Update `test_skip_unchanged_skips_the_checks` and `test_recheck_record_forces_a_fresh_run` to compute the stored fingerprint with `mode=store.run_mode(True, False)`.
    - **Modify `tools/rebuild/tests/test_oracle.py`**
        - Change `test_strict_gate_accepts_a_relocation_only_verdict` to pass `oracle.EXACT_RELOCATIONS`, `oracle.SAME_SHAPE`, and `oracle.SAME_MNEMONICS` to `run.identity_accepted`.
    - **Modify `tools/rebuild/run.py`**
        - Replace `PASS_VERDICTS` and `STRICT_VERDICTS` with `MATCHES`, `MATCH_BY_VERDICT`, `STRICT_MATCHES`, and `DEFAULT_MATCHES`. `STRICT_MATCHES` is `("exact-bytes", "exact-relocations")`. `DEFAULT_MATCHES` adds `("same-shape", "same-mnemonics")`.
        - Add `match_of(verdict: str) -> str`. It maps the five verdict strings to the five match values and maps every other string to `different`.
        - Change `identity_accepted(match: str, strict: bool) -> bool`.
        - Add `run_mode(arguments: argparse.Namespace) -> dict`. It calls `store.run_mode(arguments.strict, arguments.skip_runtime)`.
        - Change `run_identity(record, module, original, strict: bool = True)` to write `match` into the check result and to gate on `identity_accepted(match, strict)`.
        - Change `_result(checks, module, command, fingerprint, mode)` to write `mode`.
        - Extend `_current_fingerprint` and `_is_unchanged` with a `mode` parameter and pass it to `store.compute_fingerprint`.
        - Change `_write_result` to accept the mode and pass it to the fingerprint and to `_result`.
        - Change `summarize(verdicts: list[str], buckets: list[str | None]) -> str`. The format is `f"{len(verdicts)} records: {passes} pass, {fails} fail, {skips} skip; exact {exact}, relocation-only {relocation}, shape {shape}, behavior-only {behavior}"`.
        - Change `parse_arguments`. `--strict` has `dest="strict"`, `action="store_true"`, `default=True`. Add `--allow-shape` with `dest="strict"` and `action="store_false"`. Update both help strings.
        - Update the `run.py` module docstring. The usage line includes `--allow-shape`. The prose states that strict is the default and `--allow-shape` restores the historical gate.
        - In `command_run`, compute `mode = run_mode(arguments)` once. Pass it to the stale predicate, `_is_unchanged`, and `_write_result`. Collect one bucket per record with `store.result_bucket` and call the new `summarize`.
        - Update the `identity_accepted` docstring. State that strict is the default and that `--allow-shape` restores the historical gate.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_run.py"` and `python3 -m unittest discover -s tools/rebuild/tests -p "test_oracle.py"`

- [x] **Task 5: Runner prints the stored verdict and demotes a failed check**
    - **Implements:** FR-001, FR-002
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_run.py`**
        - `test_skip_unchanged_prints_the_stored_fail` — a stored identity `fail` prints `FAIL (the inputs are unchanged since the previous run)`, exits 1, and the summary holds `1 fail`.
        - `test_skip_unchanged_skips_the_checks` stays green and proves the stored `PASS` line.
        - `test_failed_build_demotes_a_verified_record` — a `verified` record with a declared build and a mocked failed build stores the status `named` and keeps the failed result.
        - `test_failed_behavior_demotes_a_verified_identity_record` — a declared identity that passes and a failed behavior store `matching`.
    - **Modify `tools/rebuild/run.py`**
        - Change `_next_status(record: dict | None, row: dict | None, checks: dict[str, dict]) -> str | None`. It takes the declared checks and the name flag from the target and calls `store.advance_status`. It passes `bool(target.get("name"))` as `has_name`, so the default `False` becomes the real name flag.
        - In `command_run`, replace the hard-coded `PASS` in the unchanged branch with `store.stored_verdict(target)`. Add the verdict to the list. Set `failed` when the verdict is `FAIL`. Print the same `UNCHANGED_DETAIL`. Append the bucket of the stored result with `store.result_bucket(stored_checks, target.get("checks", []))`, where `stored_checks` is `target["result"].get("checks", {})`.
        - Pass `record` and `row` to `_next_status` in `_write_result`.
        - Update the `run.py` module docstring. State that `--skip-unchanged` prints the stored verdict, so a stored `fail` prints `FAIL`.
        - Update the `--skip-unchanged` help text. State that the runner prints the stored verdict of a record whose inputs are unchanged since the previous run.
        - Update the `_next_status` docstring. State that a failed build or behavior demotes the record and that the failed result stays.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_run.py"`

- [x] **Task 6: Harness fresh output and exact marker lines**
    - **Implements:** FR-005
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_harness.py`**
        - `test_missing_lines_accepts_a_complete_line` replaces `test_missing_lines_accepts_lines_on_their_own`. Do not keep both. The test accepts an output line that equals the marker.
        - `test_missing_lines_requires_a_complete_line` replaces `test_missing_lines_accepts_embedded_suffix`. The embedded `W 20 0F` is reported missing.
        - `test_clear_output_removes_a_stale_file` — a temp file is gone after the call.
        - `test_clear_output_accepts_a_missing_file` — a missing path raises no error.
        - `test_missing_lines_rejects_crash_dump` stays green.
    - **Modify `tools/rebuild/driver.c`**
        - In `main`, remove the `print("REBUILT$")` call. The first marker then comes from `show_pair` and starts a complete line.
    - **Modify `tools/rebuild/harness.py`**
        - Change `missing_lines(expect: list[str], text: str) -> list[str]` to accept only an output line that equals the marker. Remove the suffix match and update the docstring.
        - Add `clear_output(path: Path) -> None`. It calls `path.unlink(missing_ok=True)`.
        - In `command_run`, move the `output_path = WORK_DIR / recipe["output"]` assignment above the DOSBox-X launch. Call `clear_output(output_path)`. After the launch, when `output_path.is_file()` is false, print `f"error: the run of {name} wrote no output"` to stderr and return 1.
    - **After the source change:**
        - `python3 tools/rebuild/harness.py build --module driver`
        - `python3 tools/rebuild/harness.py run --module driver`
        - The build creates the new `driver.exe`. The run proves that the first `W 20 0F` marker is a complete line and that the old output is deleted.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_harness.py"`, `python3 tools/rebuild/harness.py build --module driver`, and `python3 tools/rebuild/harness.py run --module driver`

- [x] **Task 7: Stage 11 accepts a stored fail and fingerprints the mode**
    - **Implements:** FR-001, FR-003
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - `test_unchanged_matches_accepts_a_stored_pass` — a stored pass matches `PASS`.
        - `test_unchanged_matches_accepts_a_stored_fail` — a stored fail matches `FAIL`.
        - `test_check_run_accepts_a_stored_fail` — mock Docker, the runner output with a `FAIL (the inputs are unchanged since the previous run)` line, the recorded record with a stored fail, and `unchanged_is_verified` true. Every check result passes.
        - `test_check_run_fails_an_unchanged_record_with_the_wrong_verdict` — the same case with a `PASS` line fails one check.
    - **Modify `tools/pipeline_check.py`**
        - Add `unchanged_matches(record: dict, verdict: str) -> bool`. It compares the verdict with `store_module.stored_verdict(record)`.
        - Change `unchanged_is_verified(record: dict, game_dir: Path, skip_runtime: bool) -> bool`. It computes the current fingerprint with `mode=store_module.run_mode(True, skip_runtime)`. The runtime field does not enter the fingerprint, so a stored runtime-run result still verifies under `--skip-runtime`.
        - In `check_run`, use `unchanged_matches` and `unchanged_is_verified(record, game_dir, skip_runtime)` in the unchanged branch. Name the stored result in the detail when the verdict differs. Name the fingerprint mismatch in the detail when the fingerprint differs.
        - Update the stage 11 docstring. State that a stored `fail` is expected and still shown as `FAIL`.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_pipeline_check.py"`

- [x] **Task 8: Status page match buckets**
    - **Implements:** FR-004
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_view.py`**
        - Extend the `result` helper with a `match` keyword argument that sets the identity `match`, and add a `mode` object.
        - `test_page_reports_match_buckets` — one exact identity result, one relocation-only, one shape, and one behavior-only module row produce one count in each bucket.
        - `test_page_targets_carry_the_match_bucket` — the chip, the table row, the map block, the treemap block, and the graph node of a record that declares identity carry `data-match="exact"`.
        - `test_page_filters_and_switch` — add the assertions for `document.querySelectorAll(".match-counter")` and the bucket update in the script.
    - **Modify `tools/rebuild/view.py`**
        - Add `_match(record: dict) -> str`. It returns `store.result_bucket((record.get("result") or {}).get("checks", {}), record.get("checks", [])) or ""`.
        - Add `_match_counts(rows: list[dict]) -> dict[str, int]`. It counts the four `store.MATCH_BUCKETS` over `_counted_rows(rows)`.
        - Add `_match_counters(rows: list[dict]) -> str`. It renders one `f'<span class="match-counter" data-bucket="{bucket}">{count} {bucket}</span>'` per bucket, joined by ` · `.
        - Change `_progress_row` to add `<span id="match-counters" class="counters match-counters">` plus `_match_counters(rows)` after the status counters.
        - Add `data-match` set to `_match(record)` to every record element: `_chip`, `_table_row`, `_module_table_row`, `_map_block`, `_treemap_block`, and `_graph_node`.
        - Add `match: str = ""` to `_chip` and pass `_match(record)` from `_function_chip` and `_module_chip`.
        - In the inline `applyFilters`, declare `const buckets = {}`. Add the bucket of each unique visible key. After the status counter loop, update every `.match-counter` from `counter.dataset.bucket`.
    - **Verify:** `python3 -m unittest discover -s tools/rebuild/tests -p "test_view.py"`

- [x] **Task 9: Re-baseline the stored results**
    - **Implements:** FR-003, FR-004
    - **Files:** no source file changes. The task rewrites `tools/rebuild/functions/*.json` and `tools/rebuild/modules/*.json` through `store.py`.
    - **Tests:** none. This is a data migration. The Verify commands prove it.
    - **Steps:**
        - Confirm Docker and DOSBox-X are on PATH.
        - Build the driver first. The driver module declares behavior only, so `run.py run --all` does not build it. Run `python3 tools/rebuild/harness.py build --module driver`.
        - Run `python3 tools/rebuild/run.py run --all`. The command prints the known identity failures and exits 1. That is the expected result, because the store holds many known failures. Do not treat the exit code as a failure of this task. Every written result now holds `mode` and every identity result holds `match`.
        - Run `python3 tools/rebuild/store.py check`.
        - Render the status page and confirm that all four buckets show a count. A failed shape match counts in `shape`, so the strict re-baseline still fills that bucket.
        - Spot check `tools/rebuild/functions/0x300ec.json`. Confirm that `result.mode` holds `strict`, `toolchain`, and `runtime`, and that `result.checks.identity.match` holds a value.
    - **Verify:**
        - `python3 tools/rebuild/store.py check`
        - `python3 tools/rebuild/view.py html --out work/rebuild/status.html`
        - The manual spot check of `result.mode` and `result.checks.identity.match` in `tools/rebuild/functions/0x300ec.json`.

- [x] **Task 10: Documentation and check counts**
    - **Implements:** FR-018
    - **Modify `AGENTS.md`**
        - Build And Test Commands: add `python3 tools/rebuild/run.py run --all --allow-shape`.
        - Testing: replace the bullet that says the default gate accepts the three historical verdicts. State that strict is the default, `--allow-shape` restores the historical gate, and `--strict` stays accepted.
        - Testing: add bullets for the `match` field and the four report buckets, the failed-check demotion, the fresh behavior output with a complete line, and the strict mode and the toolchain in the input fingerprint.
        - Testing: add a bullet that `--skip-unchanged` prints the stored verdict, so a stored `fail` prints `FAIL`.
        - Shared View Logic: add `data-match` to the list of record attributes.
        - Project Structure: add `rebuild-quality-design.md`, `rebuild-quality-roadmap.md`, and `rebuild-quality-plan-baseline.md` under `docs/explanation/`.
        - Keep the four check counts at 303, 299, 288, and 284.
    - **Modify `README.md`**
        - The feature list: state that the strict identity gate is the default and `--allow-shape` restores the historical gate.
        - The quick-start command block: replace `python3 tools/rebuild/run.py run --all --strict` with `python3 tools/rebuild/run.py run --all` and add `python3 tools/rebuild/run.py run --all --allow-shape`.
        - The `Checks` paragraph: state that strict is the default and `--allow-shape` restores the old gate. Keep the four counts.
    - **Modify `docs/reference/tooling.md`**
        - The command block: remove the `python3 tools/rebuild/run.py run --all --strict` line. Add `python3 tools/rebuild/run.py run --all --allow-shape`. Keep the existing `python3 tools/rebuild/run.py run --all` line as the strict-default example.
        - The `run.py` paragraph that starts `run.py --strict accepts`: state that strict is the default, describe `--allow-shape` and `--strict`, the match values, the four buckets, and the strict mode and the toolchain in the fingerprint. Add one sentence: `--skip-unchanged` prints the stored verdict, so a stored `fail` prints `FAIL`.
        - The `diff_module.py` and `oracle.py` text: distinguish the `run.py` identity gate from the `diff_module.py` exit code. State that `run.py` uses the strict list by default and the historical list under `--allow-shape`. State that `diff_module.py` still exits 0 when `oracle.passes_default` is true. State that the oracle exposes `passes_strict` and `passes_default`.
        - The `harness.py` text: state that the behavior run deletes the old output and matches a complete line.
        - The pipeline stage 11 text: state that a stored `fail` is expected and shown as `FAIL`.
        - Keep the four check counts.
    - **Modify `docs/explanation/findings.md`**
        - Insert a new section `## The truthful verification baseline` immediately before `## Open questions`, after the body of `## Pipeline confirmation`.
        - Record the stored fail evidence. Command: `python3 tools/rebuild/run.py run --function 0x2003c --skip-unchanged`.
        - Record the demotion evidence. Command: `python3 -m unittest discover -s tools/rebuild/tests -p "test_store.py" -k build_fails`.
        - Record the mode evidence. Command: `python3 tools/rebuild/store.py list --json`.
        - Record the fresh behavior output evidence. Command: `python3 tools/rebuild/harness.py run --module driver`.
        - Record the check counts from the four pipeline commands in the Verify block.
    - **Modify `docs/explanation/rebuild-quality-roadmap.md`**
        - Set the Slice 1 `Status` to `Implemented`.
        - Set the Slice 1 `Plan` to `docs/explanation/rebuild-quality-plan-baseline.md`.
    - **Before the full check:**
        - `python3 tools/rebuild/harness.py build --module driver`
        - The driver module declares behavior only, so the full check needs the built driver.
    - **Verify:**
        - `python3 -m py_compile tools/le.py tools/inventory.py tools/extract_cd_image.py tools/dosboxx_control.py tools/entry_probe.py tools/gfx_container.py tools/export_assets.py tools/sound_bank.py tools/member_probe.py tools/cross_check.py tools/rebuild/diff_module.py tools/rebuild/oracle.py tools/rebuild/store.py tools/rebuild/schema.py tools/rebuild/image_compare.py tools/rebuild/harness.py tools/rebuild/harness_dos.py tools/rebuild/watcom10_setup.py tools/rebuild/run.py tools/rebuild/view.py tools/rebuild/docs.py tools/rebuild/callgraph.py tools/pipeline_check.py`
        - `python3 -m unittest discover -s tools/rebuild/tests`
        - `python3 tools/rebuild/harness.py build --module driver`
        - `python3 tools/rebuild/run.py run --all --allow-shape --no-write` — expected to print the known failures and exit 1. The task passes when the command completes and prints its summary, the strict-default full check passes, and the unit suite passes.
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-cross`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime`
        - `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross`

## Deviations

- The unchanged-record branch collects the match bucket from the stored result
  during the runner task, not during the later stored-verdict task.  Reason:
  the four-bucket summary needs one bucket for every record, including an
  unchanged record.
- The re-baseline ran twice.  The first strict run wrote the results.  The
  `--allow-shape` verification then rebuilt every module.  The Open Watcom
  linker writes a creation time into the map file, and the fingerprint covers
  the map, so every stored fingerprint changed.  The second strict run restored
  the stored fingerprints to the current artifacts, so the full check can skip
  the unchanged records.
- The roadmap slice status change waits for the final review.  The executing
  workflow advances the plan and roadmap statuses only after the final review
  reports `Plan: COMPLETE` and `Quality: APPROVED`.
- The findings section also records the strict and default pass counts and the
  map-timestamp observation.  Reason: the repository rules require a new
  finding with its evidence, and both facts affect the trust in the reported
  number.
