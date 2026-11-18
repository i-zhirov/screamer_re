"""Build the rebuilt whole image from the original image and the rebuilt bodies.

The build uses the original `STARTH.EXE` as the container.  It writes each
rebuilt module body at its original address and rewrites the internal
link-time operands.  It writes the image and a report under `work/rebuild/`.
It writes no store record.

Usage:
    python3 tools/rebuild/whole_image.py --all
    python3 tools/rebuild/whole_image.py --module NAME [--module NAME]
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
import image_compare  # noqa: E402
import le  # noqa: E402
import run  # noqa: E402
import store  # noqa: E402

IMAGE_DOCUMENT = store.WORK_DIR / "STARTH.EXE"
REPORT_DOCUMENT = store.WORK_DIR / "whole-image.json"
BRANCH_TARGET = re.compile(r"^(?:call|j[a-z]+|loop[a-z]*)\s+0x([0-9a-f]+)$")


def module_order(modules: list[dict], records: list[dict]) -> list[dict]:
    """Order the modules by the lowest identity record address.

    An item holds the module and its lowest-address record with a symbol.  A
    module with no such record gives no item.  The items are sorted by the
    record address.
    """
    items = []
    for module in modules:
        name = module["name"]
        candidates = [
            record
            for record in records
            if record.get("module") == name and record.get("symbol")
        ]
        if not candidates:
            continue
        record = min(candidates, key=lambda record: int(record["address"], 16))
        items.append({"module": module, "record": record})
    items.sort(key=lambda item: int(item["record"]["address"], 16))
    return items


def report_row(item: dict) -> dict:
    """Return the base report row of one module item.

    The row starts skipped.  A later pass sets the build, the fit, the
    relocation, and the outcome.
    """
    module = item["module"]
    record = item["record"]
    return {
        "module": module["name"],
        "address": record["address"],
        "size": record["size"],
        "toolchain": store.module_toolchain(module),
        "sources": [source["file"] for source in module["sources"]],
        "build": "skip",
        "fit": False,
        "fit_reason": None,
        "rebuilt_size": 0,
        "relocated": 0,
        "outcome": "skipped",
    }


def fit(record: dict, rebuilt: list[le.Insn]) -> str | None:
    """Compare the rebuilt instruction byte sum to the record size.

    Returns `None` when the sizes are equal.  Otherwise returns the reason.
    """
    total = sum(insn.size for insn in rebuilt)
    size = record["size"]
    if total == size:
        return None
    return f"the rebuilt body holds {total} bytes against the recorded {size}"


def has_original_tail(record: dict, rebuilt: list[le.Insn]) -> bool:
    """Return true when a short ret body can leave the record tail in place."""
    total = sum(insn.size for insn in rebuilt)
    return (
        total < record["size"]
        and bool(rebuilt)
        and rebuilt[-1].mnemonic == "ret"
    )


def _slots(
    insn: le.Insn, relocations: list[le.Relocation]
) -> list[le.Relocation]:
    """Return the fixup slots whose offset starts inside one instruction."""
    end = insn.address + insn.size
    found = [
        slot
        for slot in relocations
        if insn.address <= slot.offset < end
    ]
    found.sort(key=lambda slot: slot.offset)
    return found


def _branch_target(insn: le.Insn) -> int | None:
    """Return the direct control-flow target of one instruction."""
    match = BRANCH_TARGET.match(insn.text)
    if match is None:
        return None
    return int(match.group(1), 16)


def _branch_displacement(insn: le.Insn) -> tuple[int, int] | None:
    """Return the offset and width of a relative branch displacement."""
    raw = insn.raw
    if raw[:1] in (b"\xe8", b"\xe9", b"\xeb"):
        return (1, 1 if raw[0] == 0xEB else 4)
    if len(raw) == 2 and (0x70 <= raw[0] <= 0x7F or 0xE0 <= raw[0] <= 0xE3):
        return (1, 1)
    if len(raw) >= 2 and raw[0] == 0x0F and 0x80 <= raw[1] <= 0x8F:
        return (2, 4)
    return None


def _exact_symbol(symbols: list[tuple[int, str]], address: int) -> str | None:
    """Return the first symbol at an exact address."""
    for entry_address, name in symbols:
        if entry_address == address:
            return name
    return None


def _original_symbol_address(
    symbols: list[tuple[int, str]], name: str
) -> int | None:
    """Return the unique address of a named original symbol.

    A duplicate name gives None, because the rebuilt symbol alone does not say
    which original symbol the module meant.
    """
    found = [address for address, symbol in symbols if symbol == name]
    return found[0] if len(found) == 1 else None


def _write_displacement(raw: bytearray, offset: int, width: int, value: int) -> bool:
    """Write a signed relative displacement and report whether it fits."""
    low = -(1 << (8 * width - 1))
    high = (1 << (8 * width - 1)) - 1
    if not low <= value <= high:
        return False
    raw[offset : offset + width] = value.to_bytes(width, "little", signed=True)
    return True


def relocate(
    original: list[le.Insn],
    rebuilt: list[le.Insn],
    original_relocations: list[le.Relocation],
    rebuilt_relocations: list[le.Relocation],
    original_objects: dict[int, int],
    rebuilt_symbols: list[tuple[int, str]] | None = None,
    original_symbols: list[tuple[int, str]] | None = None,
) -> tuple[bytes | None, int]:
    """Patch every absolute operand of the rebuilt body.

    The internal fixup slots take the original target address.  A direct
    relative branch or call takes the original symbol address that the rebuilt
    target names.  An unmapped branch keeps the original target, because the
    rebuilt map can name a harness stub that has no original symbol.  The
    function returns `(None, 0)` when the instruction geometry differs or when
    a byte outside a rewritten operand still differs.
    """
    if len(original) != len(rebuilt):
        return (None, 0)
    body = bytearray()
    relocated = 0
    covered: set[int] = set()
    original_assigned = 0
    rebuilt_assigned = 0
    for original_insn, rebuilt_insn in zip(original, rebuilt):
        if original_insn.size != rebuilt_insn.size:
            return (None, 0)
        original_slots = _slots(original_insn, original_relocations)
        rebuilt_slots = _slots(rebuilt_insn, rebuilt_relocations)
        original_assigned += len(original_slots)
        rebuilt_assigned += len(rebuilt_slots)
        if len(original_slots) != len(rebuilt_slots):
            return (None, 0)
        raw = bytearray(rebuilt_insn.raw)
        base = len(body)
        for original_slot, rebuilt_slot in zip(original_slots, rebuilt_slots):
            if original_slot.kind != "internal" or rebuilt_slot.kind != "internal":
                return (None, 0)
            if original_slot.width != 4 or rebuilt_slot.width != 4:
                return (None, 0)
            if original_slot.target is None:
                return (None, 0)
            original_end = original_insn.address + original_insn.size
            if original_slot.offset + 4 > original_end:
                return (None, 0)
            relative = original_slot.offset - original_insn.address
            rebuilt_relative = rebuilt_slot.offset - rebuilt_insn.address
            if rebuilt_relative != relative:
                return (None, 0)
            if rebuilt_relative < 0 or rebuilt_relative + 4 > len(rebuilt_insn.raw):
                return (None, 0)
            raw[rebuilt_relative : rebuilt_relative + 4] = original_slot.target.to_bytes(
                4, "little"
            )
            covered.update(range(base + rebuilt_relative, base + rebuilt_relative + 4))
            relocated += 1
        original_target = _branch_target(original_insn)
        rebuilt_target = _branch_target(rebuilt_insn)
        if original_target is not None and rebuilt_target is not None:
            original_delta = original_target - original_insn.address
            rebuilt_delta = rebuilt_target - rebuilt_insn.address
            if original_delta != rebuilt_delta:
                target = original_target
                if rebuilt_symbols and original_symbols:
                    name = _exact_symbol(rebuilt_symbols, rebuilt_target)
                    if name is not None:
                        mapped = _original_symbol_address(original_symbols, name)
                        if mapped is not None:
                            target = mapped
                position = _branch_displacement(rebuilt_insn)
                if position is None:
                    return (None, 0)
                offset, width = position
                if not _write_displacement(
                    raw,
                    offset,
                    width,
                    target - (original_insn.address + original_insn.size),
                ):
                    return (None, 0)
                covered.update(range(base + offset, base + offset + width))
        body.extend(raw)
    if original_assigned != len(original_relocations):
        return (None, 0)
    if rebuilt_assigned != len(rebuilt_relocations):
        return (None, 0)
    original_bytes = b"".join(insn.raw for insn in original)
    for index, (left, right) in enumerate(zip(original_bytes, body)):
        if left != right and index not in covered:
            return (None, 0)
    if len(original_bytes) != len(body):
        return (None, 0)
    return (bytes(body), relocated)


def _load_modules() -> list[dict]:
    """Return every module recipe, sorted by file name."""
    return [
        store.load_module_file(path)
        for path in sorted(store.MODULES_DIR.glob("*.json"))
    ]


def _load_records() -> list[dict]:
    """Return every function record."""
    return [store.load_record(address) for address in store.candidate_addresses()]


def _sha256(path: Path) -> str:
    """Return the lowercase SHA-256 hex digest, and raise when it is missing."""
    if not path.is_file():
        raise ValueError(f"{path}: the file is missing")
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _write_report(path: Path, document: dict) -> None:
    """Write the report with an indent of two and a trailing newline."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(document, indent=2, sort_keys=True) + "\n")


def _last_line(text: str) -> str:
    """Return the last non-empty stripped line, or an empty string."""
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    return lines[-1] if lines else ""


def build(
    original_path: Path,
    modules: list[dict],
    records: list[dict],
    work_dir: Path,
    out: Path,
    selected: list[str] | None = None,
) -> dict:
    """Build the rebuilt whole image and return its report document.

    The original image is the container.  Each selected module body is placed
    at its original address and its internal link-time operands are rewritten
    from the original fixup slots.  A library-backed record and a module that
    fails the build, the fit, the alignment, or an exception keeps the original
    bytes and is skipped with the reason.
    """
    original_path = Path(original_path)
    out = Path(out)
    if out.resolve() == original_path.resolve():
        raise ValueError("the output path must differ from the original image")
    original = le.load(original_path)
    image = bytearray(original.data)
    original_digest = _sha256(original_path)
    original_relocations = original.relocations()
    original_objects = {obj.index: obj.reloc_base for obj in original.objects}
    original_symbols = [
        (int(record["address"], 16), record["symbol"])
        for record in records
        if record.get("symbol")
    ]
    original_symbols.sort()
    items = module_order(modules, records)
    if selected is not None:
        names = set(selected)
        absent = names - {item["module"]["name"] for item in items}
        if absent:
            raise ValueError(
                "the module selection holds no build item: "
                + ", ".join(sorted(absent))
            )
        items = [item for item in items if item["module"]["name"] in names]
    rows = []
    for item in items:
        module = item["module"]
        record = item["record"]
        row = report_row(item)
        rows.append(row)
        if record.get("library"):
            row["build"] = "skip"
            row["reason"] = "the original library bytes stay"
            row["outcome"] = "skipped"
            continue
        toolchain = row["toolchain"]
        try:
            verdict, output = run.build_module(module)
            if verdict != "pass":
                row["build"] = "fail" if verdict == "fail" else "skip"
                reason = _last_line(output)
                if not reason:
                    reason = (
                        f"the {toolchain} build failed"
                        if verdict == "fail"
                        else f"the {toolchain} engine is missing"
                    )
                row["reason"] = reason
                row["outcome"] = "skipped"
                continue
            row["build"] = "pass"
            rebuilt = le.load(Path(work_dir) / module["exe"])
            address = int(record["address"], 16)
            segment, offset = diff_module.map_address(
                Path(work_dir) / module["map"], record["symbol"]
            )
            obj = rebuilt.object_by_index(segment)
            if obj is None:
                raise ValueError(
                    f"object {segment} is missing from the rebuilt image"
                )
            rebuilt_address = obj.reloc_base + offset
            rebuilt_symbols = diff_module.map_symbols(
                Path(work_dir) / module["map"], rebuilt
            )
            extent = diff_module.next_symbol(rebuilt_symbols, rebuilt_address)
            rebuilt_instructions = diff_module.rebuilt_body(
                rebuilt,
                rebuilt_address,
                None,
                record.get("count", 0),
                extent=extent,
            )
            original_instructions = diff_module.original_body(
                original,
                address,
                address + record["size"],
                record.get("count", 0),
            )
            rebuilt_size = sum(
                instruction.size for instruction in rebuilt_instructions
            )
            row["rebuilt_size"] = rebuilt_size
            fit_reason = fit(record, rebuilt_instructions)
            tail = has_original_tail(record, rebuilt_instructions)
            if fit_reason is not None and not tail:
                row["fit_reason"] = fit_reason
                row["reason"] = fit_reason
                row["outcome"] = "skipped"
                continue
            if fit_reason is None:
                row["fit"] = True
            if tail:
                original_instructions = original_instructions[
                    : len(rebuilt_instructions)
                ]
            original_slots = [
                slot
                for slot in original_relocations
                if address <= slot.offset < address + record["size"]
            ]
            rebuilt_slots = [
                slot
                for slot in rebuilt.relocations()
                if rebuilt_address <= slot.offset < rebuilt_address + rebuilt_size
            ]
            body, relocated = relocate(
                original_instructions,
                rebuilt_instructions,
                original_slots,
                rebuilt_slots,
                original_objects,
                rebuilt_symbols,
                original_symbols,
            )
            if body is None:
                if fit_reason is not None:
                    row["fit_reason"] = fit_reason
                    row["reason"] = fit_reason
                else:
                    row["reason"] = (
                        "the rebuilt body differs beyond its absolute operands"
                    )
                row["outcome"] = "skipped"
                continue
            row["fit"] = True
            file_offset = original.to_file_offset(address)
            if file_offset is None:
                row["reason"] = "the original address is not in a file page"
                row["outcome"] = "skipped"
                continue
            image[file_offset : file_offset + len(body)] = body
            row["relocated"] = relocated
            row["outcome"] = "patched"
            row.pop("reason", None)
        except (OSError, ValueError, KeyError) as error:
            row["outcome"] = "skipped"
            row["reason"] = str(error)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(bytes(image))
    result = image_compare.compare_loaded(original, le.load(out))
    return {
        "date": date.today().isoformat(),
        "original": {"path": str(original_path), "sha256": original_digest},
        "image": {"path": str(out), "sha256": _sha256(out)},
        "modules": rows,
        "patched": sum(1 for row in rows if row["outcome"] == "patched"),
        "skipped": sum(1 for row in rows if row["outcome"] == "skipped"),
        "relocated": sum(row["relocated"] for row in rows),
        "relocation_incomplete": [
            row["module"]
            for row in rows
            if row.get("reason") == "an absolute operand is unresolved"
        ],
        "body_differences": [
            row["module"]
            for row in rows
            if row.get("reason")
            == "the rebuilt body differs beyond its absolute operands"
        ],
        "runnable": result.exact,
        "score": {
            "equal": result.equal,
            "total": result.total,
            "exact": result.exact,
        },
    }


def command_build(arguments: argparse.Namespace) -> int:
    """Build the image, write the report, print the rows, and return the code."""
    try:
        original = Path(arguments.original)
        if not original.is_file():
            raise ValueError(f"the original image {original} is missing")
        try:
            le.load(original)
        except (ValueError, struct.error) as error:
            raise ValueError(
                f"the original image {original} does not parse: {error}"
            ) from error
        modules = _load_modules()
        records = _load_records()
        if arguments.module:
            known = {module.get("name") for module in modules}
            for name in arguments.module:
                if name not in known:
                    raise ValueError(f"the module {name} is missing")
        document = build(
            original,
            modules,
            records,
            store.WORK_DIR,
            arguments.out,
            arguments.module or None,
        )
        _write_report(Path(arguments.report), document)
    except (OSError, ValueError, struct.error) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    for row in document["modules"]:
        if row["outcome"] == "patched":
            print(
                f"{row['module']}: patched, "
                f"{row['relocated']} slots relocated"
            )
        else:
            print(f"{row['module']}: skipped, {row['reason']}")
    score = document["score"]
    print(
        f"{document['patched']} patched, {document['skipped']} skipped, "
        f"{document['relocated']} slots relocated, "
        f"{score['equal']} of {score['total']} bytes equal"
    )
    runnable = document.get("runnable", document.get("score", {}).get("exact", False))
    if runnable:
        print("the image is runnable")
    else:
        incomplete = document.get("relocation_incomplete", [])
        if incomplete:
            print(
                "the image is NOT runnable: the relocation is incomplete for "
                f"{len(incomplete)} modules"
            )
        else:
            print("the image is NOT runnable")
    print(document["image"]["path"])
    print(str(arguments.report))
    return 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build the rebuilt whole image and its report."
    )
    parser.add_argument(
        "--original",
        type=Path,
        default=store.DEFAULT_ORIGINAL,
        help="the original LE executable",
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=IMAGE_DOCUMENT,
        help="the rebuilt image path",
    )
    parser.add_argument(
        "--report",
        type=Path,
        default=REPORT_DOCUMENT,
        help="the report path",
    )
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument(
        "--module",
        action="append",
        default=None,
        help="the module record name; repeat the flag",
    )
    selection.add_argument(
        "--all",
        action="store_true",
        help="every module-backed record",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        return command_build(arguments)
    except (OSError, ValueError, struct.error) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
