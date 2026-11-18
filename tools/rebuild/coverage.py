"""Report the byte coverage and the record counts by provenance.

The report reads the function records of the store and the original image.  It
groups the records by provenance, sums the byte span of each group, and divides
the span by the virtual size of the code object.  The span of a record reaches
the next record, and the last record uses its `size`, or one byte.  The report
holds one row per provenance and one total line.  A module record is a build
unit and carries no provenance, so the report reads the function records only.

Usage:
    python3 tools/rebuild/coverage.py report [--original PATH]

Exit code 0 on success and 2 on a bad input.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import le  # noqa: E402
import store  # noqa: E402

REPORT_COLUMNS = (
    "provenance",
    "records",
    "unknown",
    "bytes",
    "coverage",
    "verified",
    "rebuilt",
    "exact",
)


def record_spans(records: list[dict]) -> dict[str, int]:
    """Return the byte span of each record, keyed by address.

    The records are sorted by address.  The span reaches the next record.  The
    last record uses its `size`, or one byte.  A row without an address is
    skipped.
    """
    ordered = sorted(
        (record for record in records if "address" in record),
        key=lambda record: int(record["address"], 16),
    )
    spans: dict[str, int] = {}
    for index, record in enumerate(ordered):
        address = int(record["address"], 16)
        if index + 1 < len(ordered):
            end = int(ordered[index + 1]["address"], 16)
        else:
            end = address + max(record.get("size") or 0, 1)
        spans[record["address"]] = max(end - address, 1)
    return spans


def code_object_size(original: Path) -> int:
    """Return the virtual size of the first object of the original image."""
    image = le.load(original)
    return image.objects[0].virtual_size


def _identity_bytes(record: dict) -> int:
    result = record.get("result") or {}
    identity = result.get("checks", {}).get("identity")
    if not identity:
        return 0
    return identity.get("bytes_equal", 0)


def provenance_rows(
    records: list[dict], code_size: int | None
) -> list[dict]:
    """Return one row per provenance, in the store order.

    The row sums the record count, the `unknown` count, the byte span, the
    verified and rebuilt byte spans, and the exact bytes of the identity
    results.  The coverage is a float in `0..1`.  The denominator is
    `code_size` when it is not `None`, else the sum of the row `bytes`.  A zero
    denominator gives `0.0`.
    """
    spans = record_spans(records)
    totals = {
        provenance: {
            "count": 0,
            "unknown": 0,
            "bytes": 0,
            "verified_bytes": 0,
            "rebuilt_bytes": 0,
            "exact_bytes": 0,
        }
        for provenance in store.PROVENANCE
    }
    for record in records:
        totals_row = totals.get(store.provenance_of(record))
        if totals_row is None:
            continue
        totals_row["count"] += 1
        status = record.get("status")
        if status == "unknown":
            totals_row["unknown"] += 1
        span = spans.get(record.get("address"), 0)
        totals_row["bytes"] += span
        if status == "verified":
            totals_row["verified_bytes"] += span
        elif status == "rebuilt":
            totals_row["rebuilt_bytes"] += span
        totals_row["exact_bytes"] += _identity_bytes(record)
    covered = sum(totals[provenance]["bytes"] for provenance in store.PROVENANCE)
    denominator = code_size if code_size is not None else covered
    rows = []
    for provenance in store.PROVENANCE:
        totals_row = totals[provenance]
        coverage = totals_row["bytes"] / denominator if denominator else 0.0
        rows.append(
            {
                "provenance": provenance,
                "count": totals_row["count"],
                "unknown": totals_row["unknown"],
                "bytes": totals_row["bytes"],
                "coverage": coverage,
                "verified_bytes": totals_row["verified_bytes"],
                "rebuilt_bytes": totals_row["rebuilt_bytes"],
                "exact_bytes": totals_row["exact_bytes"],
            }
        )
    return rows


def _format_line(values: tuple[object, ...]) -> str:
    (
        provenance,
        count,
        unknown,
        byte_count,
        percent,
        verified,
        rebuilt,
        exact,
    ) = values
    return (
        f"{str(provenance):<14} {str(count):>7} {str(unknown):>7}"
        f" {str(byte_count):>8} {str(percent):>8} {str(verified):>8}"
        f" {str(rebuilt):>7} {str(exact):>6}"
    )


def format_report(rows: list[dict], code_size: int | None) -> str:
    """Return the text table and the total line.

    The total coverage is the sum of the row bytes over `code_size`.  When
    `code_size` is `None`, the denominator is the sum of the row bytes.  A zero
    denominator prints `0%`.
    """
    total_records = sum(row["count"] for row in rows)
    total_unknown = sum(row["unknown"] for row in rows)
    total_bytes = sum(row["bytes"] for row in rows)
    total_verified = sum(row["verified_bytes"] for row in rows)
    total_rebuilt = sum(row["rebuilt_bytes"] for row in rows)
    total_exact = sum(row["exact_bytes"] for row in rows)
    denominator = code_size if code_size is not None else total_bytes
    coverage = total_bytes / denominator if denominator else 0.0
    lines = [_format_line(REPORT_COLUMNS)]
    for row in rows:
        lines.append(
            _format_line(
                (
                    row["provenance"],
                    row["count"],
                    row["unknown"],
                    row["bytes"],
                    f"{round(100 * row['coverage'])}%",
                    row["verified_bytes"],
                    row["rebuilt_bytes"],
                    row["exact_bytes"],
                )
            )
        )
    lines.append(
        f"total: {total_records} records, {total_unknown} unknown, "
        f"{total_bytes} bytes, {round(100 * coverage)}% coverage, "
        f"{total_verified} verified bytes, {total_rebuilt} rebuilt bytes, "
        f"{total_exact} exact bytes"
    )
    return "\n".join(lines)


def command_report(arguments: argparse.Namespace) -> int:
    """Load the function records and print the coverage report."""
    records = [
        store.load_record(address) for address in store.candidate_addresses()
    ]
    code_size = code_object_size(arguments.original)
    rows = provenance_rows(records, code_size)
    print(format_report(rows, code_size))
    return 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Report the byte coverage by provenance."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    report_parser = subparsers.add_parser(
        "report", help="print the coverage report"
    )
    report_parser.add_argument(
        "--original",
        type=Path,
        default=store.DEFAULT_ORIGINAL,
        help="the original LE executable",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "report":
            return command_report(arguments)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
