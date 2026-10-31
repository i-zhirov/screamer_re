"""Tests for the member probe fallback when the control connection ends.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import io
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
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
        error_at: str,
    ) -> None:
        self.work_dir = work_dir
        self.error = error
        self.members_before_error = members_before_error
        self.error_at = error_at
        self.members = 0

    def __enter__(self) -> "FakeSession":
        return self

    def __exit__(self, *_exc: object) -> None:
        if (
            self.error is not None
            and self.error_at == "exit"
            and self.members > 0
        ):
            raise self.error(
                "simulated control connection close during the restore"
            )
        return None

    def request(
        self, command: str, payload: str = "", timeout: float | None = None
    ) -> FakeResponse:
        if self.error is not None:
            if (
                self.error_at == "break"
                and command == "BREAK"
                and self.members >= self.members_before_error
            ):
                raise self.error("simulated control connection close")
            if (
                self.error_at == "restore"
                and command == "EXEC"
                and payload.startswith("SR ESP")
                and self.members > 0
            ):
                raise self.error(
                    "simulated control connection close during the restore"
                )
        if command == "BREAK":
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

    def __init__(self, close_error: type[Exception] | None = None) -> None:
        self.close_error = close_error

    def close(self) -> None:
        if self.close_error is not None:
            raise self.close_error(
                "simulated control connection close during the restore"
            )


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

    def probe(
        self,
        error: type[Exception] | None,
        members_before_error: int = 1,
        error_at: str = "break",
        limit: int = 200,
        close_error: type[Exception] | None = None,
    ) -> list[str]:
        session = FakeSession(
            self.work, error, members_before_error, error_at
        )
        with mock.patch.object(
            member_probe,
            "PtyProcess",
            lambda *args, **kwargs: FakePtyProcess(close_error),
        ), mock.patch.object(
            member_probe,
            "ControlSession",
            lambda port, timeout: session,
        ), mock.patch.object(member_probe.time, "sleep", lambda seconds: None):
            return member_probe.run_probe(
                self.game, self.work, 9999, limit, 300.0, "dosbox-x"
            )

    def assert_game_copy_restored(self) -> None:
        self.assertEqual(
            self.exe.read_bytes()[member_probe.MEMBER_OPEN_FILE], 0
        )

    def test_returns_the_names_when_the_connection_closes_mid_run(self) -> None:
        names = self.probe(ConnectionError, 1, "break")
        self.assertEqual(names, ["MENU.dtr"])
        self.assert_game_copy_restored()

    def test_returns_the_names_when_the_connection_times_out_mid_run(self) -> None:
        names = self.probe(TimeoutError, 1, "break")
        self.assertEqual(names, ["MENU.dtr"])

    def test_raises_when_the_connection_closes_before_the_first_member(self) -> None:
        with self.assertRaises(ConnectionError):
            self.probe(ConnectionError, 0, "break")

    def test_returns_the_names_when_the_connection_closes_during_the_restore(
        self,
    ) -> None:
        names = self.probe(ConnectionError, 1, "restore")
        self.assertEqual(names, ["MENU.dtr"])
        self.assert_game_copy_restored()

    def test_returns_the_names_when_the_session_closes_during_the_restore(
        self,
    ) -> None:
        names = self.probe(ConnectionError, 1, "exit", limit=1)
        self.assertEqual(names, ["MENU.dtr"])
        self.assert_game_copy_restored()

    def test_restores_the_game_copy_when_the_child_close_fails(self) -> None:
        names = self.probe(None, 0, "break", limit=1, close_error=ConnectionError)
        self.assertEqual(names, ["MENU.dtr"])
        self.assert_game_copy_restored()

    def test_swallows_a_connection_error_from_the_child_close(self) -> None:
        errors = io.StringIO()
        with redirect_stderr(errors):
            names = self.probe(
                None, 0, "break", limit=1, close_error=ConnectionError
            )
        self.assertEqual(names, ["MENU.dtr"])
        self.assertIn(
            "note: the control connection ended during the restore",
            errors.getvalue(),
        )
        self.assert_game_copy_restored()

    def test_swallows_a_timeout_error_from_the_child_close(self) -> None:
        errors = io.StringIO()
        with redirect_stderr(errors):
            names = self.probe(
                None, 0, "break", limit=1, close_error=TimeoutError
            )
        self.assertEqual(names, ["MENU.dtr"])
        self.assertIn(
            "note: the control connection ended during the restore",
            errors.getvalue(),
        )
        self.assert_game_copy_restored()

    def test_propagates_a_non_connection_error_during_the_restore(self) -> None:
        with self.assertRaises(RuntimeError):
            self.probe(RuntimeError, 1, "restore")
        self.assert_game_copy_restored()

    def test_propagates_a_non_connection_error_when_the_session_restores(
        self,
    ) -> None:
        with self.assertRaises(RuntimeError):
            self.probe(RuntimeError, 1, "exit", limit=1)
        self.assert_game_copy_restored()

    def test_propagates_a_non_connection_error_when_the_child_close_fails(
        self,
    ) -> None:
        with self.assertRaises(RuntimeError):
            self.probe(None, 0, "break", limit=1, close_error=RuntimeError)
        self.assert_game_copy_restored()


if __name__ == "__main__":
    unittest.main()
