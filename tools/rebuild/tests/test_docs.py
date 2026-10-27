"""Tests for the generated program-map tables and the drift check.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import docs  # noqa: E402


class DocsTestCase(unittest.TestCase):
    """Build every record in the test, so no store file is needed."""

    def record(self, address: str, **overrides: object) -> dict:
        base: dict = {
            "address": address,
            "status": "named",
            "checks": [],
            "evidence": [],
            "notes": [],
        }
        base.update(overrides)
        return base

    def document(self, records: list[dict]) -> str:
        """A document with the marker pair of every section."""
        parts = ["# Program map", ""]
        for section in docs.TABLE_SECTIONS:
            parts.append(f"<!-- begin generated: {section} -->")
            parts.append(docs.render_table(section, records))
            parts.append(f"<!-- end generated: {section} -->")
            parts.append("")
        return "\n".join(parts)

    def cells(self, table: str) -> list[list[str]]:
        """The data rows of a table, one cell list per row."""
        rows = table.splitlines()[2:]
        return [
            [cell.strip() for cell in row.split("|")[1:-1]] for row in rows
        ]

    def test_startup_runtime_address(self) -> None:
        records = [self.record("0x300f8", object_offset="0x200f8")]
        table = docs.render_table("Startup path", records)
        self.assertIn("| `0x300f8` | `0x200f8` | `0150:1f20f8` |", table)

    def test_startup_order_is_step_order(self) -> None:
        records = [
            self.record(f"0x{address:x}", object_offset="0x0")
            for address, _, _ in reversed(docs.STARTUP_STEPS)
        ]
        addresses = [address for address, _, _ in docs.STARTUP_STEPS]
        self.assertNotEqual(addresses, sorted(addresses))
        table = docs.render_table("Startup path", records)
        steps = [row[0] for row in self.cells(table)]
        self.assertEqual(steps, [step for _, step, _ in docs.STARTUP_STEPS])

    def test_function_row_cells(self) -> None:
        records = [
            self.record(
                "0x300ec",
                name="free",
                section="Program functions",
                confidence="observed",
                evidence=[
                    {"kind": "note", "text": "forwards a non-null pointer"},
                    {
                        "kind": "command",
                        "text": "python3 tools/le.py info STARTH.EXE",
                    },
                ],
            )
        ]
        table = docs.render_table("Program functions", records)
        self.assertEqual(
            self.cells(table),
            [
                [
                    "`0x300ec`",
                    "`free`",
                    "forwards a non-null pointer",
                    "observed",
                ]
            ],
        )

    def test_startup_evidence_cell(self) -> None:
        records = [
            self.record(f"0x{address:x}", object_offset="0x0")
            for address, _, _ in docs.STARTUP_STEPS
        ]
        table = docs.render_table("Startup path", records)
        cells = self.cells(table)
        self.assertEqual(
            [row[4] for row in cells],
            [evidence for _, _, evidence in docs.STARTUP_STEPS],
        )
        self.assertEqual(cells[-1][0], "program `main`")

    def test_unknown_section_goes_last(self) -> None:
        records = [
            self.record("0x300ec", name="free", section="Program functions"),
            self.record("0x1234", name="ghost", section="Not a section"),
        ]
        tables = docs.render_tables(records)
        self.assertIn("`ghost`", tables[docs.TABLE_SECTIONS[-1]])
        self.assertNotIn("`ghost`", tables["Program functions"])

    def test_record_without_section_is_absent(self) -> None:
        records = [self.record("0x300ec", name="free")]
        tables = docs.render_tables(records)
        for section in docs.TABLE_SECTIONS:
            self.assertNotIn("`free`", tables[section])

    def test_missing_marker_raises(self) -> None:
        text = "# Program map\n\nNo marker here.\n"
        with self.assertRaises(ValueError):
            docs.update_document(text, [])
        with self.assertRaises(ValueError):
            docs.check_document(text, [])

    def test_check_reports_drift(self) -> None:
        records = [
            self.record("0x300ec", name="free", section="Program functions")
        ]
        text = self.document(records)
        self.assertEqual(docs.check_document(text, records), [])
        drifted = text.replace("`free`", "`free_wrong`")
        messages = docs.check_document(drifted, records)
        self.assertEqual(len(messages), 1)
        self.assertIn("Program functions", messages[0])

    def test_render_then_check_has_no_drift(self) -> None:
        records = [
            self.record("0x300ec", name="free", section="Program functions"),
            self.record("0x300f8", object_offset="0x200f8"),
        ]
        empty = self.document([])
        self.assertNotEqual(docs.check_document(empty, records), [])
        rendered = docs.update_document(empty, records)
        self.assertEqual(docs.check_document(rendered, records), [])


if __name__ == "__main__":
    unittest.main()
