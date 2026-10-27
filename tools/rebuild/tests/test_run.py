"""Tests for the runner selection, the progress lines, and the result write.

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
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import run  # noqa: E402
import store  # noqa: E402


class RunTestCase(unittest.TestCase):
    """A test case with a temporary store and no Docker and no DOSBox-X."""

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.saved_functions = store.FUNCTIONS_DIR
        self.saved_modules = store.MODULES_DIR
        self.saved_log = run.LOG_DIR
        store.FUNCTIONS_DIR = self.root / "functions"
        store.MODULES_DIR = self.root / "modules"
        run.LOG_DIR = self.root / "logs"

    def tearDown(self) -> None:
        store.FUNCTIONS_DIR = self.saved_functions
        store.MODULES_DIR = self.saved_modules
        run.LOG_DIR = self.saved_log
        self.temporary.cleanup()

    def write_record(self, address: int, record: dict) -> Path:
        path = store.record_path(address)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n")
        return path

    def write_module(self, name: str, record: dict) -> Path:
        path = store.module_path(name)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n")
        return path

    def record(self, address: str, **overrides: object) -> dict:
        base: dict = {
            "address": address,
            "status": "unknown",
            "checks": [],
            "evidence": [],
            "notes": [],
        }
        base.update(overrides)
        return base

    def module(self, name: str = "free", **overrides: object) -> dict:
        base: dict = {
            "name": name,
            "exe": f"{name}.exe",
            "map": f"{name}.map",
            "sources": [{"file": "free.c", "compiler": "wcc386"}],
            "run": {
                "command": f"{name}.exe > {name}_out.txt",
                "output": f"{name}_out.txt",
                "expect": ["A"],
                "timeout": 180,
            },
        }
        base.update(overrides)
        return base

    def select(
        self,
        records: list[dict],
        modules: dict[str, dict] | None = None,
        digests: dict[str, str] | None = None,
        **overrides: object,
    ) -> tuple[list[dict], list[dict]]:
        arguments: dict = {
            "function": None,
            "name": None,
            "module": None,
            "status": None,
            "stale": False,
            "next_count": None,
            "all_records": False,
        }
        arguments.update(overrides)
        return run.select_records(records, modules or {}, digests or {}, **arguments)

    def test_progress_line_pass(self) -> None:
        self.assertEqual(
            run.progress_line(
                13, 642, "0x300ec free", "PASS", "instructions 3/3, bytes 6/9"
            ),
            "[13/642] 0x300ec free: PASS (instructions 3/3, bytes 6/9)",
        )

    def test_progress_line_unnamed(self) -> None:
        self.assertEqual(
            run.progress_line(16, 642, "0x15570 (unnamed)", "SKIP", "no name"),
            "[16/642] 0x15570 (unnamed): SKIP (no name)",
        )

    def test_progress_line_no_checks(self) -> None:
        self.assertEqual(
            run.progress_line(
                17, 642, "0x2ecf8 list_create", "SKIP", "no checks declared"
            ),
            "[17/642] 0x2ecf8 list_create: SKIP (no checks declared)",
        )

    def test_progress_line_runtime_skipped(self) -> None:
        self.assertEqual(
            run.progress_line(
                18, 642, "0x300ec free", "SKIP", "the runtime is skipped"
            ),
            "[18/642] 0x300ec free: SKIP (the runtime is skipped)",
        )

    def test_progress_line_module_row(self) -> None:
        self.assertEqual(
            run.progress_line(1, 1, "driver", "PASS", "behavior"),
            "[1/1] driver: PASS (behavior)",
        )

    def test_progress_line_fail(self) -> None:
        checks = {
            "identity": {
                "verdict": "fail",
                "detail": "the instruction sequences differ",
                "instructions_equal": 22,
                "instructions_total": 22,
                "bytes_equal": 42,
                "bytes_total": 52,
            },
        }
        verdict = run.record_verdict(checks)
        detail = run.record_detail({"name": "bios_video", "checks": ["identity"]}, checks)
        self.assertEqual(verdict, "FAIL")
        self.assertEqual(detail, "instructions 22/22, bytes 42/52")
        self.assertEqual(
            run.progress_line(4, 10, "0x108e4 bios_video", verdict, detail),
            "[4/10] 0x108e4 bios_video: FAIL (instructions 22/22, bytes 42/52)",
        )

    def test_summary(self) -> None:
        self.assertEqual(
            run.summarize(["PASS", "PASS", "FAIL", "SKIP"]),
            "4 records: 2 pass, 1 fail, 1 skip",
        )
        self.assertEqual(run.summarize([]), "0 records: 0 pass, 0 fail, 0 skip")

    def test_diff_verdict_pass(self) -> None:
        for verdict in (
            "the modules are byte-identical",
            "the instruction sequences match; the differing operands are the link-time addresses",
            "the modules hold the same instructions in a different order",
        ):
            text = f"byte compare: 6 of 9 bytes equal, 3 different\nverdict: {verdict}\n"
            self.assertEqual(run.diff_verdict(text), verdict)

    def test_diff_verdict_fail(self) -> None:
        for verdict in (
            "the instruction sequences differ",
            "the rebuilt module holds one instruction the original does not: mov esp,ebp",
            "the original holds one instruction the rebuilt module does not: mov esp,ebp",
            "the instruction counts differ, 15 and 14",
        ):
            text = (
                "rebuilt:  shutmod.exe at 0x10000 (54 bytes, 15 instructions)\n"
                f"verdict: {verdict}\n"
            )
            self.assertEqual(run.diff_verdict(text), verdict)

    def test_parse_identity(self) -> None:
        text = (
            "original: STARTH.EXE at 0x300ec (9 bytes, 3 instructions)\n"
            "rebuilt:  free.exe at 0x10000 (9 bytes, 3 instructions)\n"
            "\n"
            "byte compare: 6 of 9 bytes equal, 3 different\n"
            "\n"
            "instruction compare: 3 of 3 instructions equal, 0 opcodes different,"
            " 0 operands different\n"
            "verdict: the instruction sequences match; the differing operands are"
            " the link-time addresses\n"
        )
        self.assertEqual(
            run.parse_identity(text),
            {
                "instructions_equal": 3,
                "instructions_total": 3,
                "bytes_equal": 6,
                "bytes_total": 9,
            },
        )

    def test_parse_identity_count_mismatch(self) -> None:
        text = (
            "original: STARTH.EXE at 0x128ac (52 bytes, 14 instructions)\n"
            "rebuilt:  shutmod.exe at 0x10000 (54 bytes, 15 instructions)\n"
            "\n"
            "byte compare: 44 of 52 bytes equal, 8 different\n"
            "byte count differs: 52 and 54\n"
            "\n"
            "verdict: the instruction counts differ, 14 and 15\n"
        )
        counts = run.parse_identity(text)
        self.assertEqual(counts["bytes_equal"], 44)
        self.assertEqual(counts["bytes_total"], 52)
        self.assertNotIn("instructions_total", counts)

    def test_next_selection_smallest_size(self) -> None:
        records = [
            self.record("0x300f0", name="free", status="named", size=12),
            self.record("0x300f4", name="big", status="named", size=50),
            self.record("0x300f8", name="no_size", status="named"),
            self.record("0x300fc", name="small", status="named", size=3),
            self.record("0x30100", name="tie", status="named", size=12),
        ]
        selected = run.next_selection(records, 3)
        self.assertEqual(
            [record["address"] for record in selected],
            ["0x300fc", "0x300f0", "0x30100"],
        )
        selected = run.next_selection(records, 5)
        self.assertEqual(
            [record["address"] for record in selected],
            ["0x300fc", "0x300f0", "0x30100", "0x300f4", "0x300f8"],
        )

    def test_next_selection_skips_other_status(self) -> None:
        records = [
            self.record("0x300f0", name="unknown", status="unknown", size=1),
            self.record("0x300f4", name="rebuilt", status="rebuilt", size=2),
            self.record("0x300f8", name="named", status="named", size=3),
        ]
        selected = run.next_selection(records, 3)
        self.assertEqual([record["address"] for record in selected], ["0x300f8"])

    def test_select_by_address_and_name(self) -> None:
        records = [
            self.record("0x300ec", name="free", status="named"),
            self.record("0x300f0", name="free2", status="named"),
        ]
        selected, rows = self.select(records, function=196844)
        self.assertEqual([record["address"] for record in selected], ["0x300ec"])
        self.assertEqual(rows, [])
        selected, rows = self.select(records, name="free")
        self.assertEqual([record["address"] for record in selected], ["0x300ec"])
        self.assertEqual(rows, [])

    def test_select_status_and_stale(self) -> None:
        stale = "sha256:" + "2" * 64
        records = [
            self.record(
                "0x300ec",
                name="free",
                status="named",
                module="free",
                result={"source_digest": "sha256:" + "1" * 64},
            ),
            self.record(
                "0x300f0",
                name="current",
                status="rebuilt",
                module="free",
                result={"source_digest": stale},
            ),
        ]
        modules = {"free": self.module()}
        digests = {"free": stale}
        selected, rows = self.select(records, modules, digests, status="named")
        self.assertEqual([record["address"] for record in selected], ["0x300ec"])
        self.assertEqual(rows, [])
        selected, rows = self.select(records, modules, digests, stale=True)
        self.assertEqual([record["address"] for record in selected], ["0x300ec"])
        self.assertEqual(rows, [])

    def test_select_module_function_records(self) -> None:
        records = [
            self.record("0x300ec", name="free", module="free"),
            self.record("0x8c150", name="memcpy", module="memcpy"),
        ]
        modules = {"free": self.module(), "driver": self.module("driver")}
        selected, rows = self.select(records, modules, module="free")
        self.assertEqual([record["address"] for record in selected], ["0x300ec"])
        self.assertEqual(rows, [])

    def test_select_module_row(self) -> None:
        records = [self.record("0x300ec", name="free", module="free")]
        modules = {
            "free": self.module(),
            "driver": self.module("driver", checks=["behavior"], status="named"),
        }
        selected, rows = self.select(records, modules, module="driver")
        self.assertEqual(selected, [])
        self.assertEqual([row["name"] for row in rows], ["driver"])

    def test_select_empty_is_a_message(self) -> None:
        arguments = ["run.py", "run", "--name", "ghost"]
        output = io.StringIO()
        with mock.patch.object(sys, "argv", arguments):
            with contextlib.redirect_stdout(output):
                code = run.main()
        self.assertEqual(code, 0)
        self.assertIn("no records selected", output.getvalue())

    def test_verdict_fail_exits_1(self) -> None:
        record = self.record(
            "0x300ec",
            name="free",
            status="named",
            module="free",
            symbol="free_",
            checks=["identity"],
        )
        self.write_record(196844, record)
        self.write_module("free", self.module())
        failing = ("fail", {"verdict": "fail", "detail": "the instruction sequences differ"})
        arguments = ["run.py", "run", "--function", "0x300ec"]
        with mock.patch.object(run, "run_identity", return_value=failing):
            with mock.patch.object(sys, "argv", arguments):
                with contextlib.redirect_stdout(io.StringIO()):
                    code = run.main()
        self.assertEqual(code, 1)
        stored = store.load_record(196844)
        self.assertEqual(stored["status"], "rebuilt")
        self.assertEqual(
            stored["result"]["checks"]["identity"]["verdict"], "fail"
        )

    def test_skipped_check_writes_no_result(self) -> None:
        record = self.record(
            "0x300ec",
            name="free",
            status="named",
            module="free",
            symbol="free_",
            checks=["identity", "behavior"],
        )
        path = self.write_record(196844, record)
        self.write_module("free", self.module())
        before = path.read_text()
        failing = (
            "fail",
            {"verdict": "fail", "detail": "the instruction sequences differ"},
        )
        arguments = [
            "run.py",
            "run",
            "--function",
            "0x300ec",
            "--skip-runtime",
        ]
        with mock.patch.object(run, "run_identity", return_value=failing):
            with mock.patch.object(sys, "argv", arguments):
                with contextlib.redirect_stdout(io.StringIO()):
                    code = run.main()
        self.assertEqual(code, 1)
        self.assertEqual(path.read_text(), before)

    def test_missing_original_exits_2(self) -> None:
        record = self.record(
            "0x300ec",
            name="free",
            status="named",
            module="free",
            symbol="free_",
            checks=["identity"],
        )
        self.write_record(196844, record)
        self.write_module("free", self.module())
        missing = self.root / "missing" / "STARTH.EXE"
        arguments = [
            "run.py",
            "run",
            "--function",
            "0x300ec",
            "--no-write",
            "--original",
            str(missing),
        ]
        output = io.StringIO()
        errors = io.StringIO()
        with mock.patch.object(sys, "argv", arguments):
            with contextlib.redirect_stdout(output):
                with contextlib.redirect_stderr(errors):
                    code = run.main()
        self.assertEqual(code, 2)
        self.assertIn("is missing", errors.getvalue())
        self.assertNotIn("FAIL", output.getvalue())

    def test_behavior_only_record_needs_no_original(self) -> None:
        self.write_module(
            "driver",
            self.module("driver", checks=["behavior"], status="named"),
        )
        missing = self.root / "missing" / "STARTH.EXE"
        passing = ("pass", "the output holds every marker")
        arguments = [
            "run.py",
            "run",
            "--module",
            "driver",
            "--no-write",
            "--original",
            str(missing),
        ]
        output = io.StringIO()
        with mock.patch.object(run, "run_behavior", return_value=passing):
            with mock.patch.object(sys, "argv", arguments):
                with contextlib.redirect_stdout(output):
                    code = run.main()
        self.assertEqual(code, 0)
        self.assertIn("[1/1] driver: PASS (behavior)", output.getvalue())


if __name__ == "__main__":
    unittest.main()
