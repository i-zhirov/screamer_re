"""Build the rebuilt whole image from the original image and the rebuilt bodies.

The build uses the original `STARTH.EXE` as the container.  It writes each
rebuilt module body at its original address and copies the original internal
slot bytes.  A strict padded body keeps the original bytes.  It writes the
image and a report under `work/rebuild/`.  It writes no store record.

Usage:
    python3 tools/rebuild/whole_image.py --all
    python3 tools/rebuild/whole_image.py --module NAME [--module NAME]
"""
from __future__ import annotations

import argparse
import hashlib
import json
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
        "original_size": 0,
        "relocated": 0,
        "mode": None,
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


def aligned(
    original: list[le.Insn],
    rebuilt: list[le.Insn],
    original_relocations: list[le.Relocation],
    rebuilt_relocations: list[le.Relocation],
) -> bool:
    """Tell whether the rebuilt slots align with the original slots.

    The instruction count, every size, every slot count, and every slot
    relative offset must agree.  Every slot must be an internal four-byte
    slot, every original slot must hold a target, every slot must lie inside
    its instruction, and every input slot must be assigned.
    """
    if len(original) != len(rebuilt):
        return False
    original_assigned = 0
    rebuilt_assigned = 0
    for original_insn, rebuilt_insn in zip(original, rebuilt):
        if original_insn.size != rebuilt_insn.size:
            return False
        original_slots = _slots(original_insn, original_relocations)
        rebuilt_slots = _slots(rebuilt_insn, rebuilt_relocations)
        original_assigned += len(original_slots)
        rebuilt_assigned += len(rebuilt_slots)
        if len(original_slots) != len(rebuilt_slots):
            return False
        for original_slot, rebuilt_slot in zip(original_slots, rebuilt_slots):
            if original_slot.kind != "internal":
                return False
            if rebuilt_slot.kind != "internal":
                return False
            if original_slot.width != 4 or rebuilt_slot.width != 4:
                return False
            if original_slot.target is None:
                return False
            original_end = original_insn.address + original_insn.size
            if original_slot.offset + 4 > original_end:
                return False
            relative = original_slot.offset - original_insn.address
            if relative < 0 or relative + 4 > len(original_insn.raw):
                return False
            rebuilt_relative = rebuilt_slot.offset - rebuilt_insn.address
            if rebuilt_relative != relative:
                return False
            if rebuilt_relative < 0 or rebuilt_relative + 4 > len(rebuilt_insn.raw):
                return False
    if original_assigned != len(original_relocations):
        return False
    if rebuilt_assigned != len(rebuilt_relocations):
        return False
    return True


def relocate(
    original: list[le.Insn],
    rebuilt: list[le.Insn],
    original_relocations: list[le.Relocation],
    rebuilt_relocations: list[le.Relocation],
) -> tuple[bytes | None, int]:
    """Copy the original internal slot bytes into the rebuilt body.

    Returns the patched rebuilt body and the number of copied slots.  When
    `aligned` is false, returns `(None, 0)`.
    """
    if not aligned(
        original, rebuilt, original_relocations, rebuilt_relocations
    ):
        return (None, 0)
    body = bytearray()
    relocated = 0
    for original_insn, rebuilt_insn in zip(original, rebuilt):
        original_slots = _slots(original_insn, original_relocations)
        rebuilt_slots = _slots(rebuilt_insn, rebuilt_relocations)
        raw = bytearray(rebuilt_insn.raw)
        for original_slot, rebuilt_slot in zip(original_slots, rebuilt_slots):
            relative = original_slot.offset - original_insn.address
            rebuilt_relative = rebuilt_slot.offset - rebuilt_insn.address
            raw[rebuilt_relative : rebuilt_relative + 4] = original_insn.raw[
                relative : relative + 4
            ]
            relocated += 1
        body.extend(raw)
    return (bytes(body), relocated)


def _identity_match(record: dict) -> str | None:
    """Return the identity match of a record result, or `None`."""
    result = record.get("result")
    if not result:
        return None
    identity = (result.get("checks") or {}).get("identity")
    if not identity:
        return None
    return identity.get("match")


def reuse(
    record: dict, original: list[le.Insn], rebuilt: list[le.Insn]
) -> bool:
    """Tell whether the original body bytes stay for a strict padded body.

    The rebuilt byte sum must equal the original byte sum, that sum must lie
    below the record size, the record must carry no library field, and the
    identity match must be strict.
    """
    if record.get("library"):
        return False
    original_size = sum(insn.size for insn in original)
    rebuilt_size = sum(insn.size for insn in rebuilt)
    if rebuilt_size != original_size:
        return False
    if original_size >= record["size"]:
        return False
    return _identity_match(record) in run.STRICT_MATCHES


def patch_mode(
    record: dict,
    original: list[le.Insn],
    rebuilt: list[le.Insn],
    original_relocations: list[le.Relocation],
    rebuilt_relocations: list[le.Relocation],
) -> str | None:
    """Return `write`, `reuse`, or `None` for one rebuilt body."""
    if fit(record, rebuilt) is None:
        return "write"
    if reuse(record, original, rebuilt) and aligned(
        original, rebuilt, original_relocations, rebuilt_relocations
    ):
        return "reuse"
    return None


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
            original_size = sum(
                instruction.size for instruction in original_instructions
            )
            row["original_size"] = original_size
            original_slots = [
                slot
                for slot in original_relocations
                if address <= slot.offset < address + original_size
            ]
            rebuilt_slots = [
                slot
                for slot in rebuilt.relocations()
                if rebuilt_address <= slot.offset < rebuilt_address + rebuilt_size
            ]
            mode = patch_mode(
                record,
                original_instructions,
                rebuilt_instructions,
                original_slots,
                rebuilt_slots,
            )
            if mode is None:
                fit_reason = fit(record, rebuilt_instructions)
                if fit_reason is not None:
                    row["fit_reason"] = fit_reason
                    row["reason"] = fit_reason
                else:
                    row["reason"] = (
                        "the rebuilt body does not align with the original body"
                    )
                row["outcome"] = "skipped"
                continue
            if mode == "reuse":
                row["mode"] = "reuse"
                row["outcome"] = "reused"
                continue
            row["fit"] = True
            body, relocated = relocate(
                original_instructions,
                rebuilt_instructions,
                original_slots,
                rebuilt_slots,
            )
            if body is None:
                row["reason"] = (
                    "the rebuilt body does not align with the original body"
                )
                row["outcome"] = "skipped"
                continue
            file_offset = original.to_file_offset(address)
            if file_offset is None:
                row["reason"] = "the original address is not in a file page"
                row["outcome"] = "skipped"
                continue
            image[file_offset : file_offset + len(body)] = body
            row["relocated"] = relocated
            row["mode"] = "write"
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
        "reused": sum(1 for row in rows if row["outcome"] == "reused"),
        "skipped": sum(1 for row in rows if row["outcome"] == "skipped"),
        "relocated": sum(row["relocated"] for row in rows),
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
        elif row["outcome"] == "reused":
            print(f"{row['module']}: reused, the original body bytes stay")
        else:
            print(f"{row['module']}: skipped, {row['reason']}")
    score = document["score"]
    print(
        f"{document['patched']} patched, {document['reused']} reused, "
        f"{document['skipped']} skipped, "
        f"{document['relocated']} slots relocated, "
        f"{score['equal']} of {score['total']} bytes equal"
    )
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
