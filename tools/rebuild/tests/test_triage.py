"""Tests for the provenance evidence document, the triage, and the apply.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import argparse
import contextlib
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import store  # noqa: E402
import triage  # noqa: E402

FINDING = {"kind": "finding", "text": "## Identity"}
COMMAND = {"kind": "command", "text": "python3 tools/rebuild/store.py check"}


class TriageTestCase(unittest.TestCase):
    """A test case with a temporary store and a temporary evidence file."""

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.saved_functions = store.FUNCTIONS_DIR
        store.FUNCTIONS_DIR = self.root / "functions"
        self.evidence_path = self.root / "provenance.json"

    def tearDown(self) -> None:
        store.sync_extents()
        store.FUNCTIONS_DIR = self.saved_functions
        self.temporary.cleanup()

    def write_evidence(self, document: dict) -> Path:
        self.evidence_path.write_text(json.dumps(document))
        return self.evidence_path

    def record(self, **overrides: object) -> dict:
        base = {
            "address": "0x10000",
            "provenance": "game",
            "status": "unknown",
            "checks": [],
            "evidence": [],
            "notes": [],
        }
        base.update(overrides)
        return base

    def save(self, record: dict) -> dict:
        store.save_record(record)
        return record

    def entry(self, **overrides: object) -> dict:
        base = {"provenance": "library", "reason": "a reason", "evidence": [FINDING]}
        base.update(overrides)
        return base

    def run_check(
        self,
        records: list[dict],
        evidence: dict,
        start: int | None = None,
        end: int | None = None,
        *,
        data: bool = False,
        non_entry: str | None = None,
    ) -> list[str]:
        with mock.patch.object(
            store, "non_entry_reason", return_value=non_entry
        ), mock.patch.object(triage, "is_data", return_value=data):
            return triage.check(records, evidence, object(), start, end)

    # load_evidence

    def test_load_evidence_reads_the_spans_and_records(self) -> None:
        document = {
            "spans": [
                {
                    "from": "0x10000",
                    "to": "0x10100",
                    "provenance": "game",
                    "reason": "a span",
                    "evidence": [FINDING],
                }
            ],
            "records": {"0xd1f25": self.entry()},
        }
        loaded = triage.load_evidence(self.write_evidence(document))
        self.assertEqual(len(loaded["spans"]), 1)
        self.assertEqual(loaded["spans"][0]["from"], "0x10000")
        self.assertEqual(loaded["records"]["0xd1f25"]["provenance"], "library")

    def test_load_evidence_normalizes_the_addresses(self) -> None:
        document = {
            "spans": [{"from": "0x00010000", "to": "0x00010100"}],
            "records": {"0x0D1F25": self.entry()},
        }
        loaded = triage.load_evidence(self.write_evidence(document))
        self.assertEqual(loaded["spans"][0]["from"], "0x10000")
        self.assertEqual(loaded["spans"][0]["to"], "0x10100")
        self.assertIn("0xd1f25", loaded["records"])
        self.assertNotIn("0x0D1F25", loaded["records"])

    def test_load_evidence_rejects_a_bad_span(self) -> None:
        document = {
            "spans": [{"from": "not-an-address", "to": "0x10100"}],
            "records": {},
        }
        with self.assertRaises(ValueError) as context:
            triage.load_evidence(self.write_evidence(document))
        self.assertIn(str(self.evidence_path), str(context.exception))

    def test_load_evidence_rejects_a_missing_file(self) -> None:
        with self.assertRaises(ValueError) as context:
            triage.load_evidence(self.evidence_path)
        self.assertIn(str(self.evidence_path), str(context.exception))

    def test_load_evidence_rejects_broken_json(self) -> None:
        self.evidence_path.write_text("{")
        with self.assertRaises(ValueError) as context:
            triage.load_evidence(self.evidence_path)
        self.assertIn(str(self.evidence_path), str(context.exception))

    def test_load_evidence_rejects_a_missing_top_level_key(self) -> None:
        for document in ({"spans": []}, {"records": {}}):
            with self.subTest(document=document):
                with self.assertRaises(ValueError) as context:
                    triage.load_evidence(self.write_evidence(document))
                self.assertIn(str(self.evidence_path), str(context.exception))

    def test_load_evidence_rejects_a_duplicate_record_key(self) -> None:
        document = {
            "spans": [],
            "records": {"0x10000": self.entry(), "0x00010000": self.entry()},
        }
        with self.assertRaises(ValueError) as context:
            triage.load_evidence(self.write_evidence(document))
        self.assertIn(str(self.evidence_path), str(context.exception))

    def test_load_evidence_accepts_an_entry_without_a_reason(self) -> None:
        document = {
            "spans": [],
            "records": {"0xd1f25": {"provenance": "library", "evidence": [FINDING]}},
        }
        loaded = triage.load_evidence(self.write_evidence(document))
        self.assertNotIn("reason", loaded["records"]["0xd1f25"])
        record = self.record(address="0xd1f25", provenance="library")
        violations = self.run_check([record], loaded)
        self.assertTrue(any("reason" in message for message in violations))

    # resolve

    def test_resolve_prefers_the_record_entry(self) -> None:
        evidence = {
            "spans": [{"from": "0x10000", "to": "0x20000", "provenance": "library"}],
            "records": {"0x10000": {"provenance": "game"}},
        }
        resolved = triage.resolve(0x10000, evidence)
        self.assertEqual(resolved["provenance"], "game")

    def test_resolve_uses_a_span(self) -> None:
        evidence = {
            "spans": [{"from": "0x10000", "to": "0x10100", "provenance": "library"}],
            "records": {},
        }
        resolved = triage.resolve(0x10010, evidence)
        self.assertEqual(resolved["provenance"], "library")

    def test_resolve_returns_none_outside_the_spans(self) -> None:
        evidence = {
            "spans": [{"from": "0x10000", "to": "0x10100", "provenance": "library"}],
            "records": {},
        }
        self.assertIsNone(triage.resolve(0x20000, evidence))

    # is_data

    def test_is_data_for_an_empty_decode(self) -> None:
        with mock.patch.object(triage.le, "decode", return_value=[]):
            self.assertTrue(triage.is_data(object(), self.record()))

    def test_is_data_is_false_for_a_decoded_instruction(self) -> None:
        with mock.patch.object(triage.le, "decode", return_value=[object()]):
            self.assertFalse(triage.is_data(object(), self.record()))

    # propose

    def test_propose_uses_the_record_entry(self) -> None:
        evidence = {
            "spans": [],
            "records": {"0x10000": self.entry(provenance="library", reason="entry")},
        }
        with mock.patch.object(
            store, "non_entry_reason", return_value=None
        ), mock.patch.object(triage, "is_data", return_value=False):
            proposal = triage.propose(self.record(), object(), evidence)
        self.assertEqual(proposal["provenance"], "library")
        self.assertEqual(proposal["reason"], "entry")
        self.assertEqual(proposal["evidence"], [FINDING])

    def test_propose_uses_the_span(self) -> None:
        evidence = {
            "spans": [
                {
                    "from": "0x10000",
                    "to": "0x10100",
                    "provenance": "compiler_helper",
                    "reason": "span",
                    "evidence": [],
                }
            ],
            "records": {},
        }
        with mock.patch.object(
            store, "non_entry_reason", return_value=None
        ), mock.patch.object(triage, "is_data", return_value=False):
            proposal = triage.propose(self.record(), object(), evidence)
        self.assertEqual(proposal["provenance"], "compiler_helper")
        self.assertEqual(proposal["reason"], "span")

    def test_propose_marks_a_non_entry_address(self) -> None:
        with mock.patch.object(
            store, "non_entry_reason", return_value="sits inside a jump"
        ):
            proposal = triage.propose(
                self.record(), object(), {"spans": [], "records": {}}
            )
        self.assertEqual(proposal["provenance"], "not_a_function")
        self.assertEqual(proposal["reason"], "sits inside a jump")

    def test_propose_marks_a_data_record(self) -> None:
        with mock.patch.object(
            store, "non_entry_reason", return_value=None
        ), mock.patch.object(triage, "is_data", return_value=True):
            proposal = triage.propose(
                self.record(), object(), {"spans": [], "records": {}}
            )
        self.assertEqual(proposal["provenance"], "data")

    def test_propose_defaults_to_game(self) -> None:
        with mock.patch.object(
            store, "non_entry_reason", return_value=None
        ), mock.patch.object(triage, "is_data", return_value=False):
            proposal = triage.propose(
                self.record(), object(), {"spans": [], "records": {}}
            )
        self.assertEqual(proposal["provenance"], "game")

    # check

    def test_check_reports_a_non_game_record_without_a_resolved_entry(self) -> None:
        violations = self.run_check(
            [self.record(provenance="library")], {"spans": [], "records": {}}
        )
        self.assertTrue(
            any(
                "the provenance library has no resolved entry" in message
                for message in violations
            )
        )

    def test_check_reports_a_record_without_evidence(self) -> None:
        violations = self.run_check([self.record()], {"spans": [], "records": {}})
        self.assertTrue(
            any("the record holds no finding" in message for message in violations)
        )

    def test_check_accepts_a_game_record_with_record_evidence(self) -> None:
        violations = self.run_check(
            [self.record(evidence=[FINDING])], {"spans": [], "records": {}}
        )
        self.assertEqual(violations, [])

    def test_check_reports_a_stale_entry(self) -> None:
        evidence = {"spans": [], "records": {"0x20000": self.entry()}}
        violations = self.run_check([], evidence)
        self.assertTrue(any("names no record" in message for message in violations))

    def test_check_reports_a_span_with_no_record(self) -> None:
        evidence = {
            "spans": [
                {
                    "from": "0x20000",
                    "to": "0x20100",
                    "provenance": "library",
                    "reason": "a span",
                    "evidence": [FINDING],
                }
            ],
            "records": {},
        }
        violations = self.run_check([], evidence)
        self.assertTrue(any("the span holds no record" in message for message in violations))

    def test_check_reports_an_invalid_provenance(self) -> None:
        evidence = {
            "spans": [],
            "records": {"0x10000": self.entry(provenance="pascal")},
        }
        violations = self.run_check([self.record()], evidence)
        self.assertTrue(
            any("is not a provenance" in message for message in violations)
        )

    def test_check_reports_a_missing_provenance(self) -> None:
        entry = self.entry()
        del entry["provenance"]
        evidence = {"spans": [], "records": {"0x10000": entry}}
        violations = self.run_check([self.record()], evidence)
        self.assertTrue(any("has no provenance" in message for message in violations))

    def test_check_reports_a_missing_reason(self) -> None:
        entry = self.entry()
        del entry["reason"]
        evidence = {"spans": [], "records": {"0x10000": entry}}
        violations = self.run_check([self.record()], evidence)
        self.assertTrue(any("has no reason" in message for message in violations))

    def test_check_reports_a_missing_evidence_item_field(self) -> None:
        for item, expected in (
            ({"kind": "finding"}, "has no text"),
            ({"text": "## Identity"}, "has no kind"),
        ):
            with self.subTest(item=item):
                entry = self.entry(evidence=[item])
                evidence = {"spans": [], "records": {"0x10000": entry}}
                violations = self.run_check([self.record()], evidence)
                self.assertTrue(
                    any(expected in message for message in violations)
                )

    def test_check_reports_a_note_only_entry(self) -> None:
        entry = self.entry(evidence=[{"kind": "note", "text": "a note"}])
        evidence = {"spans": [], "records": {"0x10000": entry}}
        violations = self.run_check([self.record(provenance="library")], evidence)
        self.assertTrue(
            any("the entry holds no finding" in message for message in violations)
        )

    def test_check_reports_a_finding_heading_not_in_findings_md(self) -> None:
        record = self.record(
            evidence=[{"kind": "finding", "text": "## Not a heading"}]
        )
        violations = self.run_check([record], {"spans": [], "records": {}})
        self.assertTrue(any("is not a heading" in message for message in violations))

    def test_check_reports_a_missing_evidence_file(self) -> None:
        record = self.record(
            evidence=[{"kind": "file", "text": "docs/explanation/does-not-exist.md"}]
        )
        violations = self.run_check([record], {"spans": [], "records": {}})
        self.assertTrue(any("does not exist" in message for message in violations))

    def test_check_reports_an_invalid_evidence_kind(self) -> None:
        record = self.record(evidence=[{"kind": "screenshot", "text": "a shot"}])
        violations = self.run_check([record], {"spans": [], "records": {}})
        self.assertTrue(any("is not one of" in message for message in violations))

    def test_check_reports_a_non_string_evidence_text(self) -> None:
        record = self.record(evidence=[{"kind": "file", "text": 123}])
        violations = self.run_check([record], {"spans": [], "records": {}})
        self.assertTrue(any("is not a string" in message for message in violations))

    def test_check_reports_an_overlapping_span(self) -> None:
        evidence = {
            "spans": [
                {
                    "from": "0x10000",
                    "to": "0x10200",
                    "provenance": "game",
                    "reason": "first",
                    "evidence": [FINDING],
                },
                {
                    "from": "0x10100",
                    "to": "0x10300",
                    "provenance": "game",
                    "reason": "second",
                    "evidence": [FINDING],
                },
            ],
            "records": {},
        }
        records = [self.record(address="0x10000"), self.record(address="0x10100")]
        violations = self.run_check(records, evidence)
        self.assertTrue(any("overlaps" in message for message in violations))

    def test_check_reports_a_proposal_without_a_resolved_entry(self) -> None:
        violations = self.run_check(
            [self.record()], {"spans": [], "records": {}}, data=True
        )
        self.assertTrue(
            any(
                "the proposal data has no resolved entry" in message
                for message in violations
            )
        )

    def test_check_reports_a_status_mismatch(self) -> None:
        evidence = {
            "spans": [],
            "records": {"0x10000": self.entry(provenance="data")},
        }
        record = self.record(provenance="data", status="unknown")
        violations = self.run_check([record], evidence)
        self.assertTrue(
            any("requires the not_a_function status" in message for message in violations)
        )

    def test_check_limits_the_address_range(self) -> None:
        records = [
            self.record(address="0x10000", evidence=[FINDING]),
            self.record(address="0x30000"),
        ]
        evidence = {
            "spans": [
                {
                    "from": "0x40000",
                    "to": "0x40100",
                    "provenance": "library",
                    "reason": "a span",
                    "evidence": [FINDING],
                }
            ],
            "records": {"0x50000": self.entry()},
        }
        bounded = self.run_check(records, evidence, start=0x10000, end=0x20000)
        self.assertEqual(bounded, [])
        self.assertTrue(self.run_check(records, evidence))

    def test_check_limits_the_overlap_to_the_address_range(self) -> None:
        evidence = {
            "spans": [
                {
                    "from": "0x0f000",
                    "to": "0x20000",
                    "provenance": "game",
                    "reason": "first",
                    "evidence": [FINDING],
                },
                {
                    "from": "0x15000",
                    "to": "0x16000",
                    "provenance": "game",
                    "reason": "second",
                    "evidence": [FINDING],
                },
            ],
            "records": {},
        }
        records = [self.record(address="0x10000", evidence=[FINDING])]
        bounded = self.run_check(records, evidence, start=0x10000, end=0x10100)
        self.assertEqual(bounded, [])
        self.assertTrue(self.run_check(records, evidence))

    # apply

    def test_apply_writes_the_entry_provenance(self) -> None:
        record = self.record()
        evidence = {"spans": [], "records": {"0x10000": self.entry()}}
        updated = triage.apply([record], evidence, object())
        self.assertEqual(updated, 1)
        self.assertEqual(record["provenance"], "library")
        self.assertEqual(store.load_record(0x10000)["provenance"], "library")

    def test_apply_writes_the_default_game_provenance(self) -> None:
        record = self.record(evidence=[COMMAND])
        del record["provenance"]
        updated = triage.apply([record], {"spans": [], "records": {}}, object())
        self.assertEqual(updated, 1)
        self.assertEqual(record["provenance"], "game")

    def test_apply_keeps_a_game_record_with_evidence(self) -> None:
        record = self.record(evidence=[FINDING])
        evidence = {
            "spans": [],
            "records": {"0x10000": self.entry(provenance="game", evidence=[COMMAND])},
        }
        updated = triage.apply([record], evidence, object())
        self.assertEqual(updated, 1)
        self.assertEqual(record["provenance"], "game")
        self.assertEqual(record["evidence"], [FINDING, COMMAND])

    def test_apply_leaves_a_record_without_evidence_unchanged(self) -> None:
        record = self.record(provenance="library")
        updated = triage.apply([record], {"spans": [], "records": {}}, object())
        self.assertEqual(updated, 0)
        self.assertEqual(record["provenance"], "library")
        self.assertEqual(record["evidence"], [])

    def test_apply_leaves_a_note_only_entry_unchanged(self) -> None:
        record = self.record()
        evidence = {
            "spans": [],
            "records": {
                "0x10000": self.entry(evidence=[{"kind": "note", "text": "a note"}])
            },
        }
        updated = triage.apply([record], evidence, object())
        self.assertEqual(updated, 0)
        self.assertEqual(record["provenance"], "game")
        self.assertEqual(record["evidence"], [])

    def test_apply_returns_the_updated_count(self) -> None:
        changed = self.record(address="0x10000")
        unchanged = self.record(address="0x20000", provenance="library")
        evidence = {"spans": [], "records": {"0x10000": self.entry()}}
        updated = triage.apply([changed, unchanged], evidence, object())
        self.assertEqual(updated, 1)

    def test_apply_appends_the_evidence(self) -> None:
        record = self.record(evidence=[FINDING])
        evidence = {
            "spans": [],
            "records": {"0x10000": self.entry(evidence=[FINDING, COMMAND])},
        }
        triage.apply([record], evidence, object())
        self.assertEqual(record["evidence"], [FINDING, COMMAND])

    def test_apply_sets_the_not_a_function_status(self) -> None:
        record = self.record()
        evidence = {
            "spans": [],
            "records": {"0x10000": self.entry(provenance="data")},
        }
        triage.apply([record], evidence, object())
        self.assertEqual(record["provenance"], "data")
        self.assertEqual(record["status"], "not_a_function")

    def test_apply_is_idempotent(self) -> None:
        record = self.record()
        evidence = {"spans": [], "records": {"0x10000": self.entry()}}
        triage.apply([record], evidence, object())
        second = triage.apply([record], evidence, object())
        self.assertEqual(second, 0)
        self.assertEqual(record["evidence"], [FINDING])

    def test_apply_does_not_change_the_fingerprint(self) -> None:
        record = self.record(
            result={"fingerprint": "sha256:" + "0" * 64},
            evidence=[FINDING],
        )
        self.save(record)
        evidence = {
            "spans": [],
            "records": {"0x10000": self.entry(evidence=[COMMAND])},
        }
        triage.apply([store.load_record(0x10000)], evidence, object())
        loaded = store.load_record(0x10000)
        self.assertEqual(loaded["provenance"], "library")
        self.assertEqual(loaded["evidence"], [FINDING, COMMAND])
        self.assertEqual(
            loaded["result"]["fingerprint"], "sha256:" + "0" * 64
        )

    def test_main_returns_two_on_an_unwritable_output(self) -> None:
        self.write_evidence({"spans": [], "records": {}})
        arguments = [
            "triage.py",
            "propose",
            "--original",
            str(self.evidence_path),
            "--evidence",
            str(self.evidence_path),
            "--out",
            str(self.root / "draft.json"),
        ]
        output = io.StringIO()
        error = io.StringIO()
        with mock.patch.object(sys, "argv", arguments), mock.patch.object(
            triage, "_load_image", return_value=object()
        ), mock.patch.object(
            triage, "_write_document", side_effect=OSError("the disk is full")
        ), contextlib.redirect_stdout(output), contextlib.redirect_stderr(error):
            status = triage.main()
        self.assertEqual(status, 2)
        self.assertIn("the disk is full", error.getvalue())

    def test_command_apply_requires_a_clean_check(self) -> None:
        record = self.record(evidence=[FINDING])
        self.save(record)
        evidence = {
            "spans": [],
            "records": {
                "0x10000": self.entry(evidence=[{"kind": "note", "text": "a note"}])
            },
        }
        self.write_evidence(evidence)
        arguments = argparse.Namespace(
            original=self.evidence_path, evidence=self.evidence_path
        )
        output = io.StringIO()
        with mock.patch.object(
            triage.le, "load", return_value=object()
        ), mock.patch.object(
            store, "non_entry_reason", return_value=None
        ), mock.patch.object(
            triage, "is_data", return_value=False
        ), contextlib.redirect_stdout(output):
            status = triage.command_apply(arguments)
        self.assertEqual(status, 1)
        self.assertIn("the entry holds no finding", output.getvalue())
        loaded = store.load_record(0x10000)
        self.assertEqual(loaded["provenance"], "game")
        self.assertEqual(loaded["evidence"], [FINDING])


if __name__ == "__main__":
    unittest.main()
