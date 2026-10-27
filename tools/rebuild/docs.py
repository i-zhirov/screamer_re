"""Render the generated tables of the program map and check the drift.

The store holds the name, the evidence, and the confidence of every recovered
function.  This tool renders the tables of `docs/reference/program-map.md` between
the generated markers and reports the drift of the page.

Usage:
    python3 tools/rebuild/docs.py render
    python3 tools/rebuild/docs.py check

Exit code 0 on success, 1 on drift, and 2 on a missing document or a missing
marker.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import store  # noqa: E402

DOCUMENT = (
    Path(__file__).resolve().parent.parent.parent / "docs/reference/program-map.md"
)
SELECTOR = "0150"
STARTUP_SECTION = "Startup path"
STARTUP_STEPS = (
    (0x300F8, "entry jump", "`jmp short +0x76` over the banner string"),
    (0x30170, "startup", "`sti`, stack setup, DOS version check"),
    (0xD4324, "main invoker", "loads `argc` and `argv`, calls `main`"),
    (0x10020, "program `main`", "56 instructions, 15 call sites"),
)
TABLE_SECTIONS = (
    "Startup path",
    "Program functions",
    "race_loop callees",
    "frame_update callees",
    "object_update_all callees",
    "motion_update callees",
    "race_frame callees",
    "object_transform callees",
    "object_physics callees",
    "overlay_dispatch callees",
    "The replay records",
    "The graphics decoders",
    "The TABS table loaders",
    "The sound loaders",
    "The vehicle state chain",
    "The timing and filter layer",
    "The runtime memory manager",
    "object_gear_step and object_slide_step callees",
    "The resource layer",
    "Resource loader",
    "Subsystem initialisation",
)
FUNCTION_HEADER = "| Module address | Name | Evidence | Confidence |"
FUNCTION_SEPARATOR = "| --- | --- | --- | --- |"
STARTUP_HEADER = (
    "| Step | Module address | Object offset | Run-time address | Evidence |"
)
STARTUP_SEPARATOR = "| --- | ---: | ---: | --- | --- |"
EVIDENCE_BREAK = "<br>"


def _runtime_address(record: dict) -> str:
    """The DOS selector and the run-time address of the record.

    The run-time address is the load base plus the module linear address, as the
    startup table of the page shows.
    """
    address = int(record["address"], 16)
    return f"{SELECTOR}:{store.LOAD_BASE + address:x}"


def _table_records(section: str, records: list[dict]) -> list[dict]:
    """The records of a function table, sorted by address.

    The last table holds every record with a section outside `TABLE_SECTIONS`.
    A record without a section renders no row.
    """
    last = TABLE_SECTIONS[-1]
    selected = []
    for record in records:
        value = record.get("section")
        if value is None:
            continue
        if value == section or (section == last and value not in TABLE_SECTIONS):
            selected.append(record)
    return sorted(selected, key=lambda record: int(record["address"], 16))


def _evidence_text(record: dict) -> str:
    """The `note` evidence texts of a record, one per line."""
    return EVIDENCE_BREAK.join(
        entry.get("text", "")
        for entry in record.get("evidence", [])
        if entry.get("kind") == "note"
    )


def _render_startup_table(records: list[dict]) -> str:
    index = {int(record["address"], 16): record for record in records}
    lines = [STARTUP_HEADER, STARTUP_SEPARATOR]
    for address, step, evidence in STARTUP_STEPS:
        record = index.get(address)
        if record is None:
            continue
        lines.append(
            f"| {step} | `{record['address']}` | "
            f"`{record.get('object_offset', '')}` | "
            f"`{_runtime_address(record)}` | {evidence} |"
        )
    return "\n".join(lines)


def render_table(section: str, records: list[dict]) -> str:
    """The table block of one section: the header, the separator, and the rows."""
    if section == STARTUP_SECTION:
        return _render_startup_table(records)
    lines = [FUNCTION_HEADER, FUNCTION_SEPARATOR]
    for record in _table_records(section, records):
        name = record.get("name")
        lines.append(
            f"| `{record['address']}` | {f'`{name}`' if name else ''} | "
            f"{_evidence_text(record)} | {record.get('confidence', '')} |"
        )
    return "\n".join(lines)


def render_tables(records: list[dict]) -> dict[str, str]:
    """One table text per section, in document order."""
    return {section: render_table(section, records) for section in TABLE_SECTIONS}


def _section_span(text: str, section: str) -> tuple[int, int]:
    """The text positions between the marker pair of one section."""
    begin = f"<!-- begin generated: {section} -->"
    end = f"<!-- end generated: {section} -->"
    start = text.find(begin)
    if start < 0:
        raise ValueError(f"{DOCUMENT}: the begin marker of {section!r} is missing")
    start += len(begin)
    stop = text.find(end, start)
    if stop < 0:
        raise ValueError(f"{DOCUMENT}: the end marker of {section!r} is missing")
    return start, stop


def update_document(text: str, records: list[dict]) -> str:
    """Replace the text between the markers with the rendered tables."""
    for section, table in render_tables(records).items():
        start, stop = _section_span(text, section)
        text = text[:start] + "\n" + table + "\n" + text[stop:]
    return text


def check_document(text: str, records: list[dict]) -> list[str]:
    """One message per section whose table drifted from the store."""
    messages = []
    for section, table in render_tables(records).items():
        start, stop = _section_span(text, section)
        if text[start:stop].strip() != table.strip():
            messages.append(f"{section}: the generated table drifted from the store")
    return messages


def _read_document() -> str:
    try:
        return DOCUMENT.read_text()
    except OSError as error:
        raise ValueError(f"{DOCUMENT}: {error}") from error


def _load_records() -> list[dict]:
    return [store.load_record(address) for address in store.candidate_addresses()]


def command_render() -> int:
    records = _load_records()
    updated = update_document(_read_document(), records)
    DOCUMENT.write_text(updated)
    print(f"updated {DOCUMENT}: {len(TABLE_SECTIONS)} sections")
    return 0


def command_check() -> int:
    records = _load_records()
    messages = check_document(_read_document(), records)
    for message in messages:
        print(message)
    if messages:
        return 1
    print(f"no drift: {len(TABLE_SECTIONS)} sections")
    return 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Render the program-map tables and check the drift."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("render", help="write the tables between the markers")
    subparsers.add_parser("check", help="write nothing and exit 1 on drift")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "render":
            return command_render()
        return command_check()
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
