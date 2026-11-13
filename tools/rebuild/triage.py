"""Triage the function records against the recorded provenance evidence.

The provenance evidence document names the spans and the records whose
provenance is not `game`.  This tool proposes a provenance for every record,
checks the evidence document against the records, and writes the evidence into
the records.  A record holds at least one `finding`, `file`, or `command` item,
in its own `evidence` or in its resolved entry.  The resolved entry is the
explicit record entry or the span that holds the address.

Usage:
    python3 tools/rebuild/triage.py propose [--original PATH] [--evidence PATH]
        [--out PATH]
    python3 tools/rebuild/triage.py check [--original PATH] [--evidence PATH]
        [--from ADDR] [--to ADDR]
    python3 tools/rebuild/triage.py apply [--original PATH] [--evidence PATH]

Exit code 0 on success, 1 on a check violation, and 2 on a bad input.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import le  # noqa: E402
import store  # noqa: E402

EVIDENCE_DOCUMENT = store.REBUILD_DIR / "provenance.json"
DRAFT_DOCUMENT = store.WORK_DIR / "provenance-draft.json"
ACTIONABLE_KINDS = ("command", "finding", "file")


def _address_text(value: object, path: Path) -> str:
    """Return the normalized text of a hexadecimal address field."""
    if not isinstance(value, str):
        raise ValueError(
            f"{path}: the address {value!r} is not a hexadecimal address"
        )
    try:
        number = int(value, 16)
    except ValueError:
        raise ValueError(
            f"{path}: the address {value!r} is not a hexadecimal address"
        ) from None
    if number < 0:
        raise ValueError(
            f"{path}: the address {value!r} is not a hexadecimal address"
        )
    return f"0x{number:x}"


def _read_document(path: Path) -> object:
    try:
        text = path.read_text()
    except FileNotFoundError:
        raise ValueError(f"{path}: the file is missing") from None
    except OSError as error:
        raise ValueError(f"{path}: {error}") from error
    try:
        return json.loads(text)
    except json.JSONDecodeError as error:
        raise ValueError(f"{path}: broken JSON: {error}") from error


def _evidence_items(entry: dict, path: Path, label: str) -> list[dict]:
    """Return the evidence list of an entry, or raise on a bad structure."""
    items = entry.get("evidence", [])
    if not isinstance(items, list) or not all(
        isinstance(item, dict) for item in items
    ):
        raise ValueError(
            f"{path}: the {label} evidence is not a list of objects"
        )
    return items


def load_evidence(path: Path) -> dict:
    """Read the provenance evidence document.

    The document holds the top-level keys `spans` and `records`.  A span holds
    the address fields `from` and `to`.  The address fields and the record keys
    are normalized.  Only the top-level keys, the address fields, and the list
    and dict types are validated here; `check` reports a missing `reason`, a
    missing `provenance`, and a bad evidence item.
    """
    document = _read_document(path)
    if not isinstance(document, dict):
        raise ValueError(f"{path}: the document is not a JSON object")
    for key in ("spans", "records"):
        if key not in document:
            raise ValueError(f"{path}: the document has no {key}")
    spans = document["spans"]
    records = document["records"]
    if not isinstance(spans, list):
        raise ValueError(f"{path}: the spans field is not a list")
    if not isinstance(records, dict):
        raise ValueError(f"{path}: the records field is not a dict")
    normalized_spans = []
    for index, span in enumerate(spans):
        if not isinstance(span, dict):
            raise ValueError(f"{path}: the span {index} is not an object")
        for key in ("from", "to"):
            if key not in span:
                raise ValueError(
                    f"{path}: the span {index} has no {key} address"
                )
        normalized = dict(span)
        normalized["from"] = _address_text(span["from"], path)
        normalized["to"] = _address_text(span["to"], path)
        _evidence_items(span, path, f"span {index}")
        normalized_spans.append(normalized)
    normalized_records = {}
    for key, entry in records.items():
        if not isinstance(entry, dict):
            raise ValueError(
                f"{path}: the record entry {key!r} is not an object"
            )
        normalized_key = _address_text(key, path)
        if normalized_key in normalized_records:
            raise ValueError(
                f"{path}: the record key {key!r} repeats the address "
                f"{normalized_key}"
            )
        _evidence_items(entry, path, f"record entry {normalized_key}")
        normalized_records[normalized_key] = dict(entry)
    return {"spans": normalized_spans, "records": normalized_records}


def resolve(address: int, evidence: dict) -> dict | None:
    """Return the entry of an address: the record entry, the span, or None."""
    entry = evidence.get("records", {}).get(f"0x{address:x}")
    if entry is not None:
        return entry
    for span in evidence.get("spans", []):
        if int(span["from"], 16) <= address < int(span["to"], 16):
            return span
    return None


def is_data(image: le.LinearExecutable, record: dict) -> bool:
    """Tell whether the first instruction at the record address does not decode."""
    address = int(record["address"], 16)
    try:
        instructions = le.decode(image, address, 1)
    except ValueError:
        return True
    return not instructions


def propose(record: dict, image: le.LinearExecutable, evidence: dict) -> dict:
    """Return the proposed provenance, reason, and evidence of one record."""
    address = int(record["address"], 16)
    reason = store.non_entry_reason(image, address)
    if reason is not None:
        return {"provenance": "not_a_function", "reason": reason, "evidence": []}
    if is_data(image, record):
        return {
            "provenance": "data",
            "reason": "the first instruction at the address does not decode",
            "evidence": [],
        }
    entry = resolve(address, evidence)
    if entry is not None:
        return {
            "provenance": entry.get("provenance") or "game",
            "reason": entry.get("reason", ""),
            "evidence": list(entry.get("evidence", [])),
        }
    return {
        "provenance": "game",
        "reason": "the record holds no non-game evidence",
        "evidence": [],
    }


def _actionable(items: list[dict]) -> list[dict]:
    return [item for item in items if item.get("kind") in ACTIONABLE_KINDS]


def item_violations(item: dict, root: Path) -> list[str]:
    """Return one message per broken field of one evidence item."""
    violations = []
    kind = item.get("kind")
    text = item.get("text")
    if kind is None:
        violations.append("the evidence item has no kind")
    elif kind not in store.EVIDENCE_KINDS:
        violations.append(
            f"the evidence kind {kind!r} is not one of "
            f"{', '.join(store.EVIDENCE_KINDS)}"
        )
    if text is None or text == "":
        violations.append("the evidence item has no text")
        return violations
    if not isinstance(text, str):
        violations.append("the evidence text is not a string")
        return violations
    if kind == "finding" and text not in store._finding_headings(root):
        violations.append(
            f"the finding {text!r} is not a heading of findings.md"
        )
    if kind == "file" and not (root / text).exists():
        violations.append(f"the file evidence {text!r} does not exist")
    return violations


def _in_range(address: int, start: int | None, end: int | None) -> bool:
    if start is not None and address < start:
        return False
    if end is not None and address >= end:
        return False
    return True


def _intersects(low: int, high: int, start: int | None, end: int | None) -> bool:
    if start is not None and high <= start:
        return False
    if end is not None and low >= end:
        return False
    return True


def _entry_violations(entry: dict, label: str) -> list[str]:
    violations = []
    provenance = entry.get("provenance")
    if provenance is None:
        violations.append(f"{label}: the entry has no provenance")
    elif provenance not in store.PROVENANCE:
        violations.append(
            f"{label}: the provenance {provenance!r} is not a provenance value"
        )
    if not entry.get("reason"):
        violations.append(f"{label}: the entry has no reason")
    items = entry.get("evidence", [])
    if not _actionable(items):
        violations.append(
            f"{label}: the entry holds no finding, file, or command item"
        )
    for item in items:
        for message in item_violations(item, store.REPOSITORY_ROOT):
            violations.append(f"{label}: {message}")
    return violations


def check(
    records: list[dict],
    evidence: dict,
    image: le.LinearExecutable,
    start: int | None = None,
    end: int | None = None,
) -> list[str]:
    """Return one message per violation of the evidence document or a record.

    `start` is inclusive and `end` is exclusive.  A bounded check ignores a
    record outside the range and an entry or a span that does not intersect the
    range.
    """
    violations: list[str] = []
    spans = evidence.get("spans", [])
    entries = evidence.get("records", {})
    records_by_address = {}
    for record in records:
        if "address" not in record:
            continue
        records_by_address[int(record["address"], 16)] = record

    ordered = sorted(spans, key=lambda span: int(span["from"], 16))
    for index, span in enumerate(ordered):
        low = int(span["from"], 16)
        high = int(span["to"], 16)
        if not _intersects(low, high, start, end):
            continue
        for other in ordered[index + 1 :]:
            other_low = int(other["from"], 16)
            other_high = int(other["to"], 16)
            if other_low >= high:
                break
            if not _intersects(other_low, other_high, start, end):
                continue
            violations.append(
                f"0x{low:x}: the span overlaps the span at 0x{other_low:x}"
            )

    for key, entry in sorted(entries.items(), key=lambda pair: int(pair[0], 16)):
        address = int(key, 16)
        if not _in_range(address, start, end):
            continue
        label = f"0x{address:x}"
        if address not in records_by_address:
            violations.append(f"{label}: the entry names no record")
        violations.extend(_entry_violations(entry, label))

    for span in ordered:
        low = int(span["from"], 16)
        high = int(span["to"], 16)
        if not _intersects(low, high, start, end):
            continue
        label = f"0x{low:x}"
        if not any(low <= address < high for address in records_by_address):
            violations.append(f"{label}: the span holds no record")
        violations.extend(_entry_violations(span, label))

    for address, record in sorted(records_by_address.items()):
        if not _in_range(address, start, end):
            continue
        label = f"0x{address:x}"
        entry = resolve(address, evidence)
        provenance = store.provenance_of(record)
        if provenance != "game" and entry is None:
            violations.append(
                f"{label}: the provenance {provenance} has no resolved entry"
            )
        proposal = propose(record, image, evidence)
        if proposal["provenance"] != "game" and entry is None:
            violations.append(
                f"{label}: the proposal {proposal['provenance']} has no "
                f"resolved entry"
            )
        record_items = record.get("evidence", [])
        entry_items = entry.get("evidence", []) if entry is not None else []
        if not _actionable(record_items) and not _actionable(entry_items):
            violations.append(
                f"{label}: the record holds no finding, file, or command "
                f"evidence"
            )
        if (
            provenance in ("data", "not_a_function")
            and record.get("status") != "not_a_function"
        ):
            violations.append(
                f"{label}: the provenance {provenance} requires the "
                f"not_a_function status"
            )
        for item in record_items:
            for message in item_violations(item, store.REPOSITORY_ROOT):
                violations.append(f"{label}: {message}")
    return violations


def apply(
    records: list[dict], evidence: dict, image: le.LinearExecutable
) -> int:
    """Write the evidence into the records and return the updated count.

    The function runs only after a clean `check`.  A record whose own evidence
    and whose resolved entry hold no `finding`, `file`, or `command` item stays
    unchanged.  Otherwise the record takes the entry provenance, or `game` when
    it has no entry, and the absent entry evidence items.  A `data` or
    `not_a_function` provenance sets the `not_a_function` status.
    """
    updated = 0
    for record in records:
        if "address" not in record:
            continue
        address = int(record["address"], 16)
        entry = resolve(address, evidence)
        record_items = list(record.get("evidence", []))
        entry_items = list(entry.get("evidence", [])) if entry is not None else []
        if not _actionable(record_items) and not _actionable(entry_items):
            continue
        before = json.dumps(record, sort_keys=True)
        if entry is not None:
            record["provenance"] = entry.get("provenance") or "game"
        else:
            record["provenance"] = "game"
        for item in entry_items:
            if item not in record_items:
                record_items.append(item)
        record["evidence"] = record_items
        if record["provenance"] in ("data", "not_a_function"):
            record["status"] = "not_a_function"
        if json.dumps(record, sort_keys=True) != before:
            store.save_record(record)
            updated += 1
    return updated


def _load_image(path: Path) -> le.LinearExecutable:
    if not path.is_file():
        raise ValueError(f"{path}: the original image is missing")
    return le.load(path)


def _load_records() -> list[dict]:
    return [store.load_record(address) for address in store.candidate_addresses()]


def _write_document(path: Path, document: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(document, indent=2, sort_keys=True) + "\n")


def command_propose(arguments: argparse.Namespace) -> int:
    evidence = load_evidence(arguments.evidence)
    image = _load_image(arguments.original)
    proposals = {}
    for record in _load_records():
        proposal = propose(record, image, evidence)
        if proposal["provenance"] == "game":
            continue
        proposals[record["address"]] = proposal
    _write_document(
        arguments.out, {"spans": evidence.get("spans", []), "records": proposals}
    )
    print(f"wrote {arguments.out}")
    return 0


def command_check(arguments: argparse.Namespace) -> int:
    evidence = load_evidence(arguments.evidence)
    image = _load_image(arguments.original)
    records = _load_records()
    violations = check(records, evidence, image, arguments.start, arguments.end)
    for violation in violations:
        print(violation)
    print(f"{len(records)} records, {len(violations)} violations")
    return 1 if violations else 0


def command_apply(arguments: argparse.Namespace) -> int:
    evidence = load_evidence(arguments.evidence)
    image = _load_image(arguments.original)
    records = _load_records()
    violations = check(records, evidence, image)
    if violations:
        for violation in violations:
            print(violation)
        print(f"{len(records)} records, {len(violations)} violations")
        return 1
    updated = apply(records, evidence, image)
    print(f"updated {updated} records")
    return 0


def _address(value: str) -> int:
    try:
        return int(value, 0)
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"not a hexadecimal address: {value}"
        ) from None


def _add_source_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--original",
        type=Path,
        default=store.DEFAULT_ORIGINAL,
        help="the original LE executable",
    )
    parser.add_argument(
        "--evidence",
        type=Path,
        default=EVIDENCE_DOCUMENT,
        help="the provenance evidence document",
    )


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Triage the records against the provenance evidence."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    propose_parser = subparsers.add_parser(
        "propose", help="write a draft entry per non-game proposal"
    )
    _add_source_arguments(propose_parser)
    propose_parser.add_argument(
        "--out",
        type=Path,
        default=DRAFT_DOCUMENT,
        help="the draft evidence document",
    )

    check_parser = subparsers.add_parser(
        "check", help="prove the provenance evidence"
    )
    _add_source_arguments(check_parser)
    check_parser.add_argument(
        "--from",
        dest="start",
        type=_address,
        help="the first address to check, inclusive",
    )
    check_parser.add_argument(
        "--to",
        dest="end",
        type=_address,
        help="the last address to check, exclusive",
    )

    apply_parser = subparsers.add_parser(
        "apply", help="write the evidence into the records"
    )
    _add_source_arguments(apply_parser)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "propose":
            return command_propose(arguments)
        if arguments.command == "check":
            return command_check(arguments)
        return command_apply(arguments)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
