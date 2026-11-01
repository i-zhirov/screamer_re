"""Tests for the member probe capture, especially a closed connection.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import member_probe  # noqa: E402

NAME = "cursor.RAW"
OFFSET = 0x10


class FakeSession:
    """Return one member open, then close the control connection."""

    def __init__(self, work_dir: Path, registers: str) -> None:
        self.work_dir = work_dir
        self.registers = registers
        self.captures = 0

    def __enter__(self) -> "FakeSession":
        return self

    def __exit__(self, *arguments: object) -> bool:
        return False

    def request(self, kind: str, text: str = "", timeout: float = 10.0) -> mock.Mock:
        if kind == "BREAK":
            self.captures += 1
            if self.captures > 1:
                raise ConnectionError("DOSBox-X closed the control connection")
        if kind == "EXEC" and text.startswith("EV "):
            return mock.Mock(lines=["", self.registers])
        if kind == "EXEC" and text.startswith("MEMDUMPBIN"):
            target = text.split()[-1]
            if target.startswith("P"):
                (self.work_dir / target).write_bytes(
                    member_probe.MEMBER_OPEN_PATCHED
                )
            elif target.startswith("N"):
                payload = bytearray(member_probe.STACK_DUMP_SIZE)
                payload[OFFSET : OFFSET + len(NAME) + 1] = NAME.encode() + b"\x00"
                (self.work_dir / target).write_bytes(bytes(payload))
        return mock.Mock(lines=[])


class CaptureTestCase(unittest.TestCase):
    """A closed connection ends the capture with the observed members."""

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.game = self.root / "game"
        self.work = self.root / "work"
        self.game.mkdir()
        self.work.mkdir()
        (self.game / "STARTH.EXE").write_bytes(
            bytes(member_probe.MEMBER_OPEN_FILE + 5)
        )
        self.registers = " ".join(
            [
                "0",
                f"{member_probe.STACK_DUMP_BASE + OFFSET:X}",
                "0",
                "0",
                "0",
                "0",
                "0",
                "3F5000",
                "1000",
            ]
        )

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def session(self, *arguments: object, **keywords: object) -> FakeSession:
        return FakeSession(self.work, self.registers)

    def patched(self) -> contextlib.ExitStack:
        stack = contextlib.ExitStack()
        stack.enter_context(
            mock.patch.object(member_probe, "ControlSession", self.session)
        )
        stack.enter_context(mock.patch.object(member_probe, "PtyProcess"))
        stack.enter_context(
            mock.patch.object(member_probe.time, "sleep", return_value=None)
        )
        return stack

    def test_a_closed_connection_ends_the_capture(self) -> None:
        output = io.StringIO()
        with self.patched(), contextlib.redirect_stdout(output):
            names = member_probe.run_probe(
                self.game, self.work, 9876, 5, 60.0, "dosbox-x"
            )
        self.assertEqual(names, [NAME])
        self.assertIn("the game closed the control connection", output.getvalue())

    def test_the_command_line_exits_zero(self) -> None:
        output = io.StringIO()
        arguments = [
            "member_probe.py",
            "--game-dir",
            str(self.game),
            "--work-dir",
            str(self.work),
            "--limit",
            "5",
        ]
        with self.patched(), mock.patch.object(
            sys, "argv", arguments
        ), contextlib.redirect_stdout(output):
            self.assertEqual(member_probe.main(), 0)
        self.assertIn("observed 1 member opens", output.getvalue())


if __name__ == "__main__":
    unittest.main()
