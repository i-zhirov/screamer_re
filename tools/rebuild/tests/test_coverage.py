"""Tests for the record spans and the byte coverage report by provenance.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import sys
import types
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import coverage  # noqa: E402
import store  # noqa: E402


class CoverageTestCase(unittest.TestCase):
    """Build every record in the test, so no store file is needed."""

    def record(
        self,
        address: str,
        status: str = "unknown",
        provenance: str = "game",
        size: int | None = None,
        result: dict | None = None,
    ) -> dict:
        record: dict = {
            "address": address,
            "status": status,
            "provenance": provenance,
        }
        if size is not None:
            record["size"] = size
        if result is not None:
            record["result"] = result
        return record

    def identity_result(self, bytes_equal: int) -> dict:
        return {"checks": {"identity": {"bytes_equal": bytes_equal}}}

    def rows(self, records: list[dict], code_size: int | None) -> dict[str, dict]:
        return {
            row["provenance"]: row
            for row in coverage.provenance_rows(records, code_size)
        }

    def test_record_spans_reach_the_next_record(self) -> None:
        spans = coverage.record_spans(
            [
                self.record("0x100", size=1),
                self.record("0x120", size=1),
                self.record("0x130", size=4),
            ]
        )
        self.assertEqual(spans["0x100"], 0x20)
        self.assertEqual(spans["0x120"], 0x10)
        self.assertEqual(spans["0x130"], 4)

    def test_record_spans_use_the_last_size(self) -> None:
        spans = coverage.record_spans(
            [self.record("0x100", size=10), self.record("0x200", size=7)]
        )
        self.assertEqual(spans["0x200"], 7)

    def test_record_spans_use_one_byte_for_a_missing_last_size(self) -> None:
        spans = coverage.record_spans(
            [self.record("0x100", size=10), self.record("0x200")]
        )
        self.assertEqual(spans["0x200"], 1)

    def test_record_spans_skip_a_row_without_an_address(self) -> None:
        spans = coverage.record_spans(
            [self.record("0x100", size=16), {"status": "unknown"}]
        )
        self.assertEqual(spans, {"0x100": 16})

    def test_record_spans_sorts_the_records(self) -> None:
        spans = coverage.record_spans(
            [
                self.record("0x120", size=4),
                self.record("0x100", size=16),
                self.record("0x110", size=16),
            ]
        )
        self.assertEqual(
            spans, {"0x100": 16, "0x110": 16, "0x120": 4}
        )

    def test_code_object_size_reads_the_first_object(self) -> None:
        image = types.SimpleNamespace(
            objects=[types.SimpleNamespace(virtual_size=0x1234)]
        )
        original = Path("STARTH.EXE")
        with mock.patch.object(
            coverage.le, "load", return_value=image
        ) as load:
            self.assertEqual(coverage.code_object_size(original), 0x1234)
        load.assert_called_once_with(original)

    def test_provenance_rows_count_the_records(self) -> None:
        rows = coverage.provenance_rows(
            [
                self.record("0x100", provenance="game"),
                self.record("0x110", provenance="game"),
                self.record("0x120", provenance="library"),
            ],
            100,
        )
        self.assertEqual(
            [row["provenance"] for row in rows], list(store.PROVENANCE)
        )
        by_provenance = {row["provenance"]: row for row in rows}
        self.assertEqual(by_provenance["game"]["count"], 2)
        self.assertEqual(by_provenance["library"]["count"], 1)

    def test_provenance_rows_count_the_unknown_records(self) -> None:
        by_provenance = self.rows(
            [
                self.record("0x100", status="unknown"),
                self.record("0x110", status="verified"),
                self.record("0x120", status="unknown", provenance="library"),
            ],
            100,
        )
        self.assertEqual(by_provenance["game"]["unknown"], 1)
        self.assertEqual(by_provenance["library"]["unknown"], 1)

    def test_provenance_rows_sum_the_spans(self) -> None:
        by_provenance = self.rows(
            [
                self.record("0x100", provenance="game", size=16),
                self.record("0x110", provenance="game", size=16),
                self.record("0x120", provenance="library", size=8),
            ],
            100,
        )
        self.assertEqual(by_provenance["game"]["bytes"], 32)
        self.assertEqual(by_provenance["library"]["bytes"], 8)

    def test_provenance_rows_sum_the_verified_and_rebuilt_bytes(self) -> None:
        by_provenance = self.rows(
            [
                self.record("0x100", status="verified", size=16),
                self.record("0x110", status="rebuilt", size=16),
                self.record("0x120", status="unknown", size=8),
            ],
            100,
        )
        self.assertEqual(by_provenance["game"]["verified_bytes"], 16)
        self.assertEqual(by_provenance["game"]["rebuilt_bytes"], 16)

    def test_provenance_rows_sum_the_exact_bytes(self) -> None:
        by_provenance = self.rows(
            [
                self.record(
                    "0x100",
                    status="verified",
                    size=16,
                    result=self.identity_result(7),
                ),
                self.record(
                    "0x110",
                    status="rebuilt",
                    size=16,
                    result=self.identity_result(5),
                ),
                self.record("0x120", status="unknown", size=8),
            ],
            100,
        )
        self.assertEqual(by_provenance["game"]["exact_bytes"], 12)

    def test_provenance_rows_compute_the_coverage(self) -> None:
        by_provenance = self.rows(
            [
                self.record("0x100", provenance="game"),
                self.record("0x132", provenance="library", size=50),
            ],
            200,
        )
        self.assertEqual(by_provenance["game"]["coverage"], 0.25)
        self.assertGreaterEqual(by_provenance["game"]["coverage"], 0.0)
        self.assertLessEqual(by_provenance["game"]["coverage"], 1.0)

    def test_provenance_rows_without_a_code_size_use_the_covered_total(
        self,
    ) -> None:
        by_provenance = self.rows(
            [
                self.record("0x100", provenance="game"),
                self.record("0x132", provenance="library", size=50),
            ],
            None,
        )
        self.assertEqual(by_provenance["game"]["coverage"], 0.5)
        self.assertEqual(by_provenance["library"]["coverage"], 0.5)

    def test_provenance_rows_zero_bytes(self) -> None:
        for row in coverage.provenance_rows([], None):
            self.assertEqual(row["coverage"], 0.0)
        by_provenance = self.rows([self.record("0x100", size=10)], 0)
        self.assertEqual(by_provenance["game"]["coverage"], 0.0)

    def test_format_report_prints_every_provenance(self) -> None:
        text = coverage.format_report(
            coverage.provenance_rows([], None), None
        )
        for provenance in store.PROVENANCE:
            self.assertIn(provenance, text)

    def test_format_report_prints_the_total(self) -> None:
        rows = coverage.provenance_rows(
            [
                self.record("0x0", status="unknown", size=100),
                self.record(
                    "0x64",
                    status="verified",
                    size=100,
                    result=self.identity_result(40),
                ),
                self.record(
                    "0xC8",
                    status="rebuilt",
                    size=100,
                    result=self.identity_result(60),
                ),
            ],
            1000,
        )
        text = coverage.format_report(rows, 1000)
        self.assertIn(
            "total: 3 records, 1 unknown, 300 bytes, 30% coverage, "
            "100 verified bytes, 100 rebuilt bytes, 100 exact bytes",
            text,
        )

    def test_format_report_zero_denominator(self) -> None:
        rows = coverage.provenance_rows(
            [self.record("0x100", size=10)], 0
        )
        self.assertIn("0% coverage", coverage.format_report(rows, 0))
        empty = coverage.provenance_rows([], None)
        self.assertIn("0% coverage", coverage.format_report(empty, None))


if __name__ == "__main__":
    unittest.main()
