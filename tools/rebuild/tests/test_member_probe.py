"""Tests for the member probe fallback when the control connection ends.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import member_probe  # noqa: E402


class FakeResponse:
    """The response shape the control session returns."""

    def __init__(self, lines: list[str]) -> None:
        self.lines = lines


class FakeSession:
    """A control session that completes member opens, then raises."""

    def __init__(
        self,
        work_dir: Path,
        error: type[Exception] | None,
        members_before_error: int,
    ) -> None:
        self.work_dir = work_dir
        self.error = error
        self.members_before_error = members_before_error
        self.members = 0

    def __enter__(self) -> "FakeSession":
        return self

    def __exit__(self, *_exc: object) -> None:
        return None

    def request(
        self, command: str, payload: str = "", timeout: float | None = None
    ) -> FakeResponse:
        if command == "BREAK":
            if self.error is not None and self.members >= self.members_before_error:
                raise self.error("simulated control connection close")
            return FakeResponse([])
        if command == "EXEC" and payload.startswith("EV "):
            values = [
                0x1,
                member_probe.STACK_DUMP_BASE + 0x100,
                0x3,
                0x4,
                0x5,
                0x6,
                0x7,
                0x2F5000,
                0x20E08D,
            ]
            return FakeResponse([" ".join(f"{value:08X}" for value in values)])
        if command == "EXEC" and payload.startswith("MEMDUMPBIN"):
            target = payload.split()[-1]
            if target.startswith("P"):
                (self.work_dir / target).write_bytes(
                    member_probe.MEMBER_OPEN_PATCHED
                )
            elif target.startswith("N"):
                buffer = bytearray(member_probe.STACK_DUMP_SIZE)
                buffer[0x100 : 0x100 + len(b"MENU.dtr")] = b"MENU.dtr"
                (self.work_dir / target).write_bytes(buffer)
                self.members += 1
            return FakeResponse([])
        return FakeResponse([])


class FakePtyProcess:
    """A stand-in for the DOSBox-X child process."""

    def __init__(self, *_args: object, **_kwargs: object) -> None:
        pass

    def close(self) -> None:
        pass


class MemberProbeTestCase(unittest.TestCase):
    """Run the probe loop with a session that ends after the first member."""

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.game = self.root / "game"
        self.work = self.root / "work"
        self.game.mkdir()
        self.work.mkdir()
        self.exe = self.game / "STARTH.EXE"
        self.exe.write_bytes(bytes(member_probe.MEMBER_OPEN_FILE + 5))

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def probe(self, error: type[Exception], members_before_error: int) -> list[str]:
        session = FakeSession(self.work, error, members_before_error)
        with mock.patch.object(member_probe, "PtyProcess", FakePtyProcess), \
                mock.patch.object(
                    member_probe,
                    "ControlSession",
                    lambda port, timeout: session,
                ), \
                mock.patch.object(member_probe.time, "sleep", lambda seconds: None):
            return member_probe.run_probe(
                self.game, self.work, 9999, 200, 300.0, "dosbox-x"
            )

    def test_returns_the_names_when_the_connection_closes_mid_run(self) -> None:
        names = self.probe(ConnectionError, 1)
        self.assertEqual(names, ["MENU.dtr"])
        self.assertEqual(
            self.exe.read_bytes()[member_probe.MEMBER_OPEN_FILE], 0
        )

    def test_returns_the_names_when_the_connection_times_out_mid_run(self) -> None:
        names = self.probe(TimeoutError, 1)
        self.assertEqual(names, ["MENU.dtr"])

    def test_raises_when_the_connection_closes_before_the_first_member(self) -> None:
        with self.assertRaises(ConnectionError):
            self.probe(ConnectionError, 0)


if __name__ == "__main__":
    unittest.main()
