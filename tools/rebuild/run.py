"""Run the declared checks of the store records and write the results.

The tool selects the function records and the module rows of the store, runs the
declared checks of each one in address order, prints one progress line per record,
and writes the result through `store.py`.  With `--skip-unchanged` a record whose
input fingerprint still matches the fingerprint of its recorded result prints
`PASS` and runs no check, so an unchanged module or function costs one hash and
no build, no diff, and no DOSBox run.  The full log of the run stays under
`work/rebuild/logs/`.

With `--strict` the identity check passes only when the bytes are exact or the
only differences are the link-time addresses.  With `--recheck-record ADDR` the
runner forces one fresh run and prints the full check output.

Usage:
    python3 tools/rebuild/run.py run (--function ADDR | --name NAME | --module NAME
        | --status STATUS | --stale | --next N | --recheck-record ADDR | --all)
        [--no-write] [--show] [--strict] [--skip-runtime] [--skip-unchanged]
        [--original PATH]

Exit code 0 on success or an empty selection, 1 when a declared check fails, and 2
on a bad argument, a missing record, or a missing original image.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections.abc import Callable
from datetime import date, datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import store  # noqa: E402

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent
REBUILD_DIR = Path(__file__).resolve().parent
WORK_DIR = REPOSITORY_ROOT / "work/rebuild"
LOG_DIR = WORK_DIR / "logs"
DIFF_TOOL = REBUILD_DIR / "diff_module.py"
HARNESS_TOOL = REBUILD_DIR / "harness.py"
PASS_VERDICTS = (
    "the modules are byte-identical",
    "the instruction sequences match; the differing operands are the link-time addresses",
    "the instruction shapes match; the differing bytes are not relocations",
    "the modules hold the same instructions in a different order",
)
STRICT_VERDICTS = (
    "the modules are byte-identical",
    "the instruction sequences match; the differing operands are the link-time addresses",
)
BUILD_DETAIL = {
    "pass": "the module builds",
    "fail": "the module build failed",
    "skip": "the engine is missing",
}
BEHAVIOR_DETAIL = {
    "pass": "the output holds every marker",
    "fail": "the output differs",
    "skip": "the engine is missing",
}
BYTE_COMPARE = re.compile(r"^byte compare: (\d+) of (\d+) bytes equal")
INSTRUCTION_COMPARE = re.compile(
    r"^instruction compare: (\d+) of (\d+) instructions equal"
)


def next_selection(records: list[dict], count: int) -> list[dict]:
    """The `named` records with the smallest `size`.

    A record with no `size` goes last, and the ties keep the address order.
    """
    named = [record for record in records if record.get("status") == "named"]
    sized = sorted(
        (record for record in named if "size" in record),
        key=lambda record: (record["size"], int(record["address"], 16)),
    )
    unsized = sorted(
        (record for record in named if "size" not in record),
        key=lambda record: int(record["address"], 16),
    )
    return (sized + unsized)[:count]


def select_records(
    records: list[dict],
    modules: dict[str, dict],
    stale_fn: Callable[[dict], bool] | None,
    function: int | None,
    name: str | None,
    module: str | None,
    status: str | None,
    stale: bool,
    next_count: int | None,
    all_records: bool,
) -> tuple[list[dict], list[dict]]:
    """Return the selected function records and the selected module rows.

    A module row is a module record that holds no function record of the store.
    The stale selector takes a predicate, because the input fingerprint of a
    record needs its own record and its own module.
    """
    module_rows = [
        row
        for row_name, row in sorted(modules.items())
        if not any(record.get("module") == row_name for record in records)
    ]
    functions: list[dict]
    rows: list[dict]
    if function is not None:
        functions = [
            record
            for record in records
            if int(record["address"], 16) == function
        ]
        rows = []
    elif name is not None:
        functions = [record for record in records if record.get("name") == name]
        rows = []
    elif module is not None:
        functions = [
            record for record in records if record.get("module") == module
        ]
        rows = [] if functions else [
            row for row in module_rows if row["name"] == module
        ]
    elif status is not None:
        functions = [
            record for record in records if record.get("status") == status
        ]
        rows = [row for row in module_rows if row.get("status") == status]
    elif stale:
        if stale_fn is None:
            functions = []
            rows = []
        else:
            functions = [record for record in records if stale_fn(record)]
            rows = [row for row in module_rows if stale_fn(row)]
    elif next_count is not None:
        functions = next_selection(records, next_count)
        rows = []
    elif all_records:
        functions = list(records)
        rows = list(module_rows)
    else:
        functions = []
        rows = []
    functions.sort(key=lambda record: int(record["address"], 16))
    return functions, rows


def progress_line(index: int, total: int, label: str, verdict: str, detail: str) -> str:
    return f"[{index}/{total}] {label}: {verdict} ({detail})"


def summarize(verdicts: list[str]) -> str:
    passes = verdicts.count("PASS")
    fails = verdicts.count("FAIL")
    skips = verdicts.count("SKIP")
    return f"{len(verdicts)} records: {passes} pass, {fails} fail, {skips} skip"


def diff_verdict(text: str) -> str:
    """The last `verdict:` line of a diff output, or the last non-empty line."""
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    for line in reversed(lines):
        if line.startswith("verdict:"):
            return line[len("verdict:") :].strip()
    return lines[-1] if lines else ""


def parse_identity(text: str) -> dict[str, int]:
    """The four identity counts of the two compare lines of a diff output."""
    counts: dict[str, int] = {}
    for line in text.splitlines():
        line = line.strip()
        match = BYTE_COMPARE.match(line)
        if match:
            counts["bytes_equal"] = int(match.group(1))
            counts["bytes_total"] = int(match.group(2))
            continue
        match = INSTRUCTION_COMPARE.match(line)
        if match:
            counts["instructions_equal"] = int(match.group(1))
            counts["instructions_total"] = int(match.group(2))
    return counts


def identity_accepted(verdict: str, strict: bool) -> bool:
    """Tell whether an identity verdict passes the gate.

    The default gate accepts the three historical verdicts.  The strict gate
    accepts only the byte-identical verdict and the verdict whose differences are
    the link-time addresses.  A reordered instruction sequence is not exact.
    """
    accepted = STRICT_VERDICTS if strict else PASS_VERDICTS
    return verdict in accepted


def run_identity(
    record: dict, module: dict, original: Path, strict: bool = False
) -> tuple[str, dict]:
    """Run the identity check and return its verdict and its check result."""
    if not original.is_file():
        raise ValueError(f"the original image {original} is missing")
    command = [
        sys.executable,
        str(DIFF_TOOL),
        "--original",
        str(original),
        "--original-address",
        record["address"],
        "--rebuilt",
        str(WORK_DIR / module["exe"]),
        "--rebuilt-map",
        str(WORK_DIR / module["map"]),
        "--rebuilt-symbol",
        record["symbol"],
    ]
    if "count" in record:
        command += ["--count", str(record["count"])]
    completed = subprocess.run(command, capture_output=True, text=True)
    verdict = diff_verdict(completed.stdout)
    if not verdict:
        error_lines = [
            line.strip()
            for line in completed.stderr.splitlines()
            if line.strip()
        ]
        verdict = error_lines[-1] if error_lines else "the diff tool printed no verdict"
    check = {
        "verdict": (
            "pass"
            if completed.returncode == 0
            and identity_accepted(verdict, strict)
            else "fail"
        ),
        "detail": verdict,
    }
    check.update(parse_identity(completed.stdout))
    return check["verdict"], check


def run_build(module: dict, show: bool = False) -> tuple[str, str]:
    """Run the build check and return its verdict and the captured output."""
    return _harness("build", module, show)


def run_behavior(module: dict, show: bool = False) -> tuple[str, str]:
    """Run the behavior check and return its verdict and the captured output."""
    return _harness("run", module, show)


def _harness(command: str, module: dict, show: bool) -> tuple[str, str]:
    completed = subprocess.run(
        [
            sys.executable,
            str(HARNESS_TOOL),
            command,
            "--module",
            module["name"],
        ],
        capture_output=True,
        text=True,
    )
    output = completed.stdout + completed.stderr
    if show and output:
        sys.stdout.write(output)
    if completed.returncode == 0:
        verdict = "pass"
    elif completed.returncode == 3:
        verdict = "skip"
    else:
        verdict = "fail"
    return verdict, output


def _detail(verdict: str, output: str, fallback: dict[str, str]) -> str:
    if verdict == "fail":
        lines = [line.strip() for line in output.splitlines() if line.strip()]
        if lines:
            return lines[-1]
    return fallback[verdict]


def record_verdict(checks: dict[str, dict]) -> str:
    """The verdict of one record, from its check results."""
    verdicts = [check.get("verdict") for check in checks.values()]
    if "fail" in verdicts:
        return "FAIL"
    if "skip" in verdicts:
        return "SKIP"
    return "PASS"


def record_detail(record: dict, checks: dict[str, dict]) -> str:
    """The detail of the progress line of one record."""
    if not record.get("name"):
        return "no name"
    if not record.get("checks"):
        return "no checks declared"
    skipped = [check for check in checks.values() if check.get("verdict") == "skip"]
    if skipped:
        return skipped[0].get("detail", "")
    identity = checks.get("identity")
    if identity is not None:
        return (
            f"instructions {identity.get('instructions_equal', 0)}/"
            f"{identity.get('instructions_total', 0)}, "
            f"bytes {identity.get('bytes_equal', 0)}/"
            f"{identity.get('bytes_total', 0)}"
        )
    return ", ".join(record.get("checks", []))


def _next_status(declared: list[str], checks: dict[str, dict]) -> str | None:
    """The status the run writes, or None to keep the stored one.

    The verdicts go through `store.advance_status`, so a skip or a failed
    build keeps the stored status, and a failed identity gives `rebuilt`.
    """
    verdicts = {name: check.get("verdict") for name, check in checks.items()}
    return store.advance_status(declared, verdicts)


def _label(record: dict | None, row: dict | None) -> str:
    if record is not None:
        name = record.get("name")
        if name:
            return f"{record['address']} {name}"
        return f"{record['address']} (unnamed)"
    return row["name"]


def _run_record(
    record: dict | None,
    row: dict | None,
    modules: dict[str, dict],
    arguments: argparse.Namespace,
    log_lines: list[str],
) -> tuple[dict[str, dict], str, str, bool]:
    """Run the declared checks of one record.

    Return the check results, the verdict, the detail, and whether a result must
    be written.  A skip stops the remaining checks of the record, because the
    engine is missing.  A record with any skipped check writes no result, because
    a skip is not a pass.  The verdicts give the written status.
    """
    target = record if record is not None else row
    label = _label(record, row)
    if not target.get("name"):
        return {}, "SKIP", "no name", False
    if not target.get("checks"):
        return {}, "SKIP", "no checks declared", False
    module_name = target.get("module") if record is not None else target["name"]
    module = modules.get(module_name) if module_name is not None else None
    if module is None:
        return {}, "SKIP", f"the module {module_name} is missing", False
    checks: dict[str, dict] = {}
    for check in target["checks"]:
        if check == "build":
            verdict, output = run_build(module, arguments.show)
            _log_output(log_lines, label, check, output)
            checks[check] = {
                "verdict": verdict,
                "detail": _detail(verdict, output, BUILD_DETAIL),
            }
        elif check == "identity":
            verdict, check_result = run_identity(
                target, module, arguments.original, arguments.strict
            )
            _log_output(log_lines, label, check, check_result.get("detail", ""))
            checks[check] = check_result
        else:
            if arguments.skip_runtime:
                checks[check] = {
                    "verdict": "skip",
                    "detail": "the runtime is skipped",
                }
            else:
                verdict, output = run_behavior(module, arguments.show)
                _log_output(log_lines, label, check, output)
                checks[check] = {
                    "verdict": verdict,
                    "detail": _detail(verdict, output, BEHAVIOR_DETAIL),
                }
        if checks[check].get("verdict") == "skip":
            break
    verdict = record_verdict(checks)
    detail = record_detail(target, checks)
    skipped = any(
        check.get("verdict") == "skip" for check in checks.values()
    )
    return checks, verdict, detail, verdict != "SKIP" and not skipped


def _result(
    checks: dict[str, dict], module: dict, command: str, fingerprint: str
) -> dict:
    return {
        "date": date.today().isoformat(),
        "command": command,
        "tool": "run.py",
        "source_digest": store.source_digest(module),
        "fingerprint": fingerprint,
        "checks": checks,
    }


def _command_text(record: dict | None, row: dict | None) -> str:
    if record is not None:
        return f"python3 tools/rebuild/run.py run --function {record['address']}"
    return f"python3 tools/rebuild/run.py run --module {row['name']}"


def _write_result(
    record: dict | None,
    row: dict | None,
    checks: dict[str, dict],
    command: str,
    original: Path,
) -> None:
    target = record if record is not None else row
    module_name = target.get("module") if record is not None else target["name"]
    module = store.load_module(module_name)
    fingerprint = store.compute_fingerprint(target, module, original=original)
    if record is not None:
        store.save_result(
            int(record["address"], 16),
            _result(checks, module, command, fingerprint),
            _next_status(record.get("checks", []), checks),
        )
        return
    store.save_module_result(
        row["name"],
        _result(checks, module, command, fingerprint),
        _next_status(row.get("checks", []), checks),
    )


def _log_output(
    log_lines: list[str], label: str, check: str, text: str
) -> None:
    lines = [line for line in text.splitlines() if line.strip()]
    if not lines:
        return
    log_lines.append(f"--- {label}: {check} ---")
    log_lines.extend(lines)


def _write_log(lines: list[str]) -> Path:
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    path = LOG_DIR / f"run-{datetime.now():%Y%m%d-%H%M%S}.log"
    path.write_text("\n".join(lines) + "\n")
    return path


def _module_for(
    record: dict | None, row: dict | None, modules: dict[str, dict]
) -> dict | None:
    """The module record of a function record or of a module row."""
    target = record if record is not None else row
    if target is None:
        return None
    name = target.get("module") if record is not None else target.get("name")
    if name is None:
        return None
    return modules.get(name)


def _current_fingerprint(
    record: dict | None,
    row: dict | None,
    modules: dict[str, dict],
    original: Path,
) -> str | None:
    """The fingerprint of one record, or None when its module is missing."""
    module = _module_for(record, row, modules)
    target = record if record is not None else row
    if module is None or target is None:
        return None
    return store.compute_fingerprint(target, module, original=original)


def _stored_fingerprint(record: dict | None, row: dict | None) -> str | None:
    target = record if record is not None else row
    if target is None:
        return None
    result = target.get("result")
    if not result:
        return None
    return result.get("fingerprint")


def _is_unchanged(
    record: dict | None, row: dict | None, modules: dict[str, dict], original: Path
) -> bool:
    """Tell whether the recorded result matches the current inputs.

    A record without a recorded result or without a stored fingerprint is
    changed, because it must run at least once to record one.
    """
    stored = _stored_fingerprint(record, row)
    if stored is None:
        return False
    fingerprint = _current_fingerprint(record, row, modules, original)
    return fingerprint is not None and stored == fingerprint


def command_run(arguments: argparse.Namespace) -> int:
    if getattr(arguments, "recheck_record", None) is not None:
        arguments.function = arguments.recheck_record
        arguments.show = True
        arguments.skip_unchanged = False
    records = [
        store.load_record(address) for address in store.candidate_addresses()
    ]
    module_names = sorted(path.stem for path in store.MODULES_DIR.glob("*.json"))
    modules = {name: store.load_module(name) for name in module_names}

    def stale_fn(record: dict) -> bool:
        if not record.get("result"):
            return False
        if "address" in record:
            module = _module_for(record, None, modules)
        else:
            module = _module_for(None, record, modules)
        if module is None:
            return False
        fingerprint = store.compute_fingerprint(
            record, module, original=arguments.original
        )
        return store.is_stale(record, fingerprint)

    functions, rows = select_records(
        records,
        modules,
        stale_fn,
        arguments.function,
        arguments.name,
        arguments.module,
        arguments.status,
        arguments.stale,
        arguments.next_count,
        arguments.all_records,
    )
    if arguments.function is not None and not functions and not rows:
        raise ValueError(f"the record {arguments.function:#x} is missing")
    if not functions and not rows:
        print("no records selected")
        return 0
    entries: list[tuple[dict | None, dict | None]] = [
        (record, None) for record in functions
    ] + [(None, row) for row in rows]
    log_lines: list[str] = []
    verdicts: list[str] = []
    failed = False
    for index, (record, row) in enumerate(entries, 1):
        if arguments.skip_unchanged and _is_unchanged(
            record, row, modules, arguments.original
        ):
            verdicts.append("PASS")
            line = progress_line(
                index,
                len(entries),
                _label(record, row),
                "PASS",
                store.UNCHANGED_DETAIL,
            )
            print(line)
            log_lines.append(line)
            continue
        checks, verdict, detail, write = _run_record(
            record, row, modules, arguments, log_lines
        )
        verdicts.append(verdict)
        if verdict == "FAIL":
            failed = True
        if write and not arguments.no_write:
            _write_result(
                record, row, checks, _command_text(record, row), arguments.original
            )
        line = progress_line(
            index, len(entries), _label(record, row), verdict, detail
        )
        print(line)
        log_lines.append(line)
    summary = summarize(verdicts)
    print(summary)
    log_lines.append(summary)
    print(f"log: {_write_log(log_lines)}")
    return 1 if failed else 0


def _address(value: str) -> int:
    try:
        return int(value, 0)
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"not a hexadecimal address: {value}"
        ) from None


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run the declared checks of the store records."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    run_parser = subparsers.add_parser(
        "run", help="run the declared checks of a selection"
    )
    selector = run_parser.add_mutually_exclusive_group(required=True)
    selector.add_argument(
        "--function",
        type=_address,
        help="the module linear address, in hex",
    )
    selector.add_argument("--name", help="the recovered name")
    selector.add_argument("--module", help="the module record name")
    selector.add_argument(
        "--status", choices=store.STATUS_LADDER, help="the status ladder value"
    )
    selector.add_argument(
        "--stale",
        action="store_true",
        help="the records whose source digest changed",
    )
    selector.add_argument(
        "--next",
        type=int,
        dest="next_count",
        metavar="N",
        help="the N smallest named records",
    )
    selector.add_argument(
        "--recheck-record",
        type=_address,
        dest="recheck_record",
        metavar="ADDR",
        help="force a fresh run of one record and print its check output",
    )
    selector.add_argument(
        "--all",
        action="store_true",
        dest="all_records",
        help="every record",
    )
    run_parser.add_argument(
        "--no-write", action="store_true", help="write nothing"
    )
    run_parser.add_argument(
        "--show", action="store_true", help="print the captured check output"
    )
    run_parser.add_argument(
        "--skip-runtime",
        action="store_true",
        help="skip the behavior checks",
    )
    run_parser.add_argument(
        "--skip-unchanged",
        action="store_true",
        help="report a record whose inputs are unchanged since the previous "
        "run, and run no check for it",
    )
    run_parser.add_argument(
        "--strict",
        action="store_true",
        help="pass the identity check only for exact bytes or link-time operands",
    )
    run_parser.add_argument(
        "--original",
        type=Path,
        default=store.DEFAULT_ORIGINAL,
        help="the original LE executable",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "run":
            return command_run(arguments)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
