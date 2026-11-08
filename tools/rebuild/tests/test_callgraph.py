"""Tests for the named call graph builder, the file read, and the edge rules.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import contextlib
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import callgraph  # noqa: E402


class CallgraphTestCase(unittest.TestCase):
    """Build every record in the test, so no store file is needed."""

    def record(self, address: str, name: str | None = None) -> dict:
        record = {
            "address": address,
            "status": "named" if name else "unknown",
            "checks": [],
            "evidence": [],
            "notes": [],
        }
        if name:
            record["name"] = name
        return record

    def test_resolve_target_uses_the_preceding_record(self) -> None:
        addresses = [0x10020, 0x10100, 0x10200]
        self.assertEqual(callgraph.resolve_target(0x10020, addresses), "0x10020")
        self.assertEqual(callgraph.resolve_target(0x100ff, addresses), "0x10020")
        self.assertIsNone(callgraph.resolve_target(0x10000, addresses))

    def test_make_edges_drops_missing_unnamed_and_self(self) -> None:
        records = [
            self.record("0x10020", "main"),
            self.record("0x10100", "free"),
            self.record("0x10200"),
        ]
        calls = [
            ("0x10020", 0x1002D, 0x10100),
            ("0x10020", 0x1002E, 0x10200),
            ("0x10020", 0x1002F, 0x9999),
            ("0x10020", 0x10030, 0x10020),
            ("0x10100", 0x10105, 0x10040),
            ("0x10100", 0x10106, 0x10050),
        ]
        self.assertEqual(
            callgraph.make_edges(calls, records),
            [
                {"from": "0x10020", "site": "0x1002d", "to": "0x10100"},
                {"from": "0x10100", "site": "0x10105", "to": "0x10020"},
            ],
        )

    def test_write_and_load_edges(self) -> None:
        edges = [
            {"from": "0x10020", "site": "0x1002d", "to": "0x300ec"},
            {"from": "0x300ec", "site": "0x300f4", "to": "0x300f0"},
        ]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "callgraph.json"
            with contextlib.redirect_stdout(io.StringIO()):
                callgraph.write_edges(edges, path)
            self.assertEqual(callgraph.load_edges(path), edges)

    def test_load_edges_rejects_a_missing_file(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "missing.json"
            with self.assertRaises(ValueError):
                callgraph.load_edges(path)

    def test_load_edges_rejects_broken_json(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "callgraph.json"
            path.write_text("{not json")
            with self.assertRaises(ValueError):
                callgraph.load_edges(path)

    def test_load_edges_rejects_a_broken_edge(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "callgraph.json"
            path.write_text(json.dumps({"edges": [{"from": "0x10020"}]}))
            with self.assertRaises(ValueError):
                callgraph.load_edges(path)


if __name__ == "__main__":
    unittest.main()
