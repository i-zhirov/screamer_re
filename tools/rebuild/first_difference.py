"""Report the first oracle difference of the non-strict module records.

The tool selects the module-backed function records whose identity check is not
strict.  It builds each selected module, compares the rebuilt body with the
original body, and classifies the first difference with a source-form idiom.
The report shows where the rebuilt object code departs from the original.

The tool writes one JSON document and no store record.  The report document
stays under `work/rebuild/` by default.

Usage:
    python3 tools/rebuild/first_difference.py report \
        --module error_report --module pool_init \
        --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
        --out work/rebuild/first-differences.json

Exit code 0 when every selected row builds, 1 when a selected row fails or
skips its build, and 2 on a bad input.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from datetime import date
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import diff_module  # noqa: E402
import le  # noqa: E402
import oracle  # noqa: E402
import run  # noqa: E402
import store  # noqa: E402

IDIOMS = (
    "local slot order",
    "constant operand",
    "instruction encoding",
    "instruction selection",
    "basic-block order",
    "relocation target",
    "match",
    "other",
)
STACK_SLOT = re.compile(r"\[ebp\s*-")
INSTRUCTION_PREFIX = re.compile(r"^instruction (\d+): ")
REPORT_DOCUMENT = store.WORK_DIR / "first-differences.json"
BLOCK_SIZE = 65536


def _stack_slot(insn: le.Insn) -> bool:
    """Tell whether the operands hold a stack frame displacement."""
    return STACK_SLOT.search(insn.operands) is not None


def _difference_text(difference: str) -> str:
    """Remove one optional instruction prefix from a difference."""
    return INSTRUCTION_PREFIX.sub("", difference, count=1)


def idiom_of(difference: str, original: le.Insn, rebuilt: le.Insn) -> str:
    """Classify one oracle difference with a source-form idiom.

    The rules run in order.  A `mask:` difference is a constant operand, and a
    relocation-slot difference also produces a `mask:` difference.  That
    overlap is a documented limitation of the classification.
    """
    text = _difference_text(difference)
    if not text:
        return "match"
    if text.startswith("mnemonic:"):
        return "instruction selection"
    if text.startswith("size:"):
        return "instruction encoding"
    if text.startswith("mask:"):
        return "constant operand"
    if (
        text.startswith("byte 2:")
        and _stack_slot(original)
        and _stack_slot(rebuilt)
    ):
        return "local slot order"
    if text.startswith("branch target:"):
        return "basic-block order"
    if text.startswith("relocation"):
        return "relocation target"
    return "other"


def first_pair(
    original: list[le.Insn],
    rebuilt: list[le.Insn],
    result: oracle.OracleResult,
) -> tuple[int, le.Insn, le.Insn] | None:
    """Read the first indexed difference and return the instruction pair.

    Returns None when the list is empty, when the first difference has no
    instruction prefix, or when the index is outside a body.
    """
    if not result.differences:
        return None
    match = INSTRUCTION_PREFIX.match(result.differences[0])
    if match is None:
        return None
    index = int(match.group(1))
    if index >= len(original) or index >= len(rebuilt):
        return None
    return index, original[index], rebuilt[index]


def _instruction_field(insn: le.Insn | None) -> dict | None:
    if insn is None:
        return None
    return {
        "address": hex(insn.address),
        "bytes": insn.raw.hex(),
        "text": insn.text,
    }


def report_row(
    record: dict,
    module: dict,
    result: oracle.OracleResult,
    original: list[le.Insn],
    rebuilt: list[le.Insn],
) -> dict:
    """Build one report row from one measured module result."""
    pair = first_pair(original, rebuilt, result)
    if pair is None:
        left = None
        right = None
        if result.differences:
            idiom = "other"
        elif result.match in run.STRICT_MATCHES:
            idiom = "match"
        else:
            idiom = "other"
    else:
        _, left, right = pair
        idiom = idiom_of(result.differences[0], left, right)
    return {
        "module": module["name"],
        "address": record["address"],
        "symbol": record["symbol"],
        "toolchain": store.module_toolchain(module),
        "sources": module.get("sources", []),
        "build": "pass",
        "match": result.match,
        "instructions_equal": result.instructions_equal,
        "instructions_total": result.instructions_total,
        "bytes_equal": result.bytes_equal,
        "bytes_total": result.bytes_total,
        "first_difference": (
            result.differences[0] if result.differences else None
        ),
        "idiom": idiom,
        "original": _instruction_field(left),
        "rebuilt": _instruction_field(right),
    }


def _failure_row(record: dict, module: dict, build: str) -> dict:
    """Build the row of a failed or skipped build."""
    return {
        "module": module["name"],
        "address": record["address"],
        "symbol": record["symbol"],
        "toolchain": store.module_toolchain(module),
        "sources": module.get("sources", []),
        "build": build,
        "match": None,
        "instructions_equal": None,
        "instructions_total": None,
        "bytes_equal": None,
        "bytes_total": None,
        "first_difference": None,
        "idiom": "other",
        "original": None,
        "rebuilt": None,
    }


def diff_record(
    record: dict,
    module: dict,
    original_path: Path,
    work_dir: Path,
) -> dict:
    """Build one module and compare its body with the original body."""
    build, _ = run.build_module(module)
    if build != "pass":
        return _failure_row(record, module, build)
    original = le.load(original_path)
    rebuilt = le.load(work_dir / module["exe"])
    map_path = work_dir / module["map"]
    segment, offset = diff_module.map_address(map_path, record["symbol"])
    obj = rebuilt.object_by_index(segment)
    if obj is None:
        raise ValueError(f"object {segment} is missing from the rebuilt image")
    original_address = int(record["address"], 16)
    rebuilt_address = obj.reloc_base + offset
    size = record.get("size")
    end = original_address + size if size is not None else None
    count = record.get("count", 0)
    symbols = diff_module.map_symbols(map_path, rebuilt)
    extent = diff_module.next_symbol(symbols, rebuilt_address)
    original_instructions = diff_module.original_body(
        original, original_address, end, count
    )
    rebuilt_instructions = diff_module.rebuilt_body(
        rebuilt, rebuilt_address, end, count, extent=extent
    )
    result = oracle.compare(
        original_instructions,
        rebuilt_instructions,
        original_relocations=original.relocations(),
        rebuilt_relocations=rebuilt.relocations(),
        original_symbols=diff_module.original_symbol_resolver(
            original, diff_module.load_records()
        ),
        rebuilt_symbols=diff_module.rebuilt_symbol_resolver(rebuilt, symbols),
    )
    return report_row(
        record, module, result, original_instructions, rebuilt_instructions
    )


def report(
    records: list[dict],
    modules: list[dict],
    original_path: Path,
    names: list[str] | None = None,
) -> dict:
    """Build the selected non-strict module records and return the document."""
    module_by_name = {module["name"]: module for module in modules}
    rows: list[dict] = []
    for record in records:
        name = record.get("module")
        if not name or name not in module_by_name:
            continue
        if names is not None and name not in names:
            continue
        if not record.get("symbol"):
            continue
        result = record.get("result")
        if not result:
            continue
        identity = (result.get("checks") or {}).get("identity")
        if not identity:
            continue
        match = identity.get("match")
        if not match or match in run.STRICT_MATCHES:
            continue
        rows.append(
            diff_record(
                record, module_by_name[name], original_path, store.WORK_DIR
            )
        )
    rows.sort(key=lambda row: row["module"])
    return {
        "date": date.today().isoformat(),
        "original": {
            "path": str(original_path),
            "sha256": _sha256(original_path),
        },
        "modules": rows,
    }


def _sha256(path: Path) -> str:
    """Return the lowercase SHA-256 hex digest, and raise when it is missing."""
    if not path.is_file():
        raise ValueError(f"{path}: the file is missing")
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(BLOCK_SIZE), b""):
            digest.update(block)
    return digest.hexdigest()


def _write_report(path: Path, document: dict) -> None:
    """Write the report with an indent of two and a trailing newline."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(document, indent=2, sort_keys=True) + "\n")


def _load_modules() -> list[dict]:
    return [
        store.load_module_file(path)
        for path in sorted(store.MODULES_DIR.glob("*.json"))
    ]


def _load_records() -> list[dict]:
    return [store.load_record(address) for address in store.candidate_addresses()]


def command_report(arguments: argparse.Namespace) -> int:
    """Write the report and return the command exit code."""
    try:
        modules = _load_modules()
        records = _load_records()
        if arguments.module:
            known = {module.get("name") for module in modules}
            for name in arguments.module:
                if name not in known:
                    raise ValueError(f"the module {name} is missing")
            names = list(arguments.module)
        elif arguments.all:
            names = None
        else:
            raise ValueError("select --module NAME or --all")
        original = Path(arguments.original)
        if not original.is_file():
            raise ValueError(f"the original image {original} is missing")
        try:
            le.load(original)
        except (ValueError, struct.error) as error:
            raise ValueError(
                f"the original image {original} does not parse: {error}"
            ) from error
        document = report(records, modules, original, names)
        _write_report(Path(arguments.out), document)
    except (OSError, ValueError, struct.error) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    for row in document["modules"]:
        difference = row["first_difference"] or "no difference"
        print(
            f"{row['module']}: {row['build']} {row['match']} "
            f"{row['idiom']}: {difference}"
        )
    not_measured = sum(
        1 for row in document["modules"] if row["build"] != "pass"
    )
    print(f"{len(document['modules'])} modules: {not_measured} not measured")
    return 0 if not_measured == 0 else 1


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Report the first oracle difference of the modules."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    report_parser = subparsers.add_parser(
        "report", help="build the selected modules and write the report"
    )
    report_parser.add_argument(
        "--module",
        action="append",
        default=None,
        help="the module record name; repeat the flag",
    )
    report_parser.add_argument(
        "--all",
        action="store_true",
        help="every module-backed non-strict record",
    )
    report_parser.add_argument(
        "--original",
        type=Path,
        default=store.DEFAULT_ORIGINAL,
        help="the original LE executable",
    )
    report_parser.add_argument(
        "--out",
        type=Path,
        default=REPORT_DOCUMENT,
        help="the report path",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        return command_report(arguments)
    except (OSError, ValueError, struct.error) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
