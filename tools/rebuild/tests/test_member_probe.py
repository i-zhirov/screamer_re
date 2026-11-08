"""Tests for the member probe capture and its restore fallbacks.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import contextlib
import io
import sys
import tempfile
import threading
import unittest
from contextlib import redirect_stderr
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import member_probe  # noqa: E402

NAME = "cursor.RAW"
OFFSET = 0x10


class FakeSession:
    """Return one member open, then fail the next capture read."""

    def __init__(
        self,
        work_dir: Path,
        registers: str,
        failure: BaseException,
        fail_at: int = 2,
        fail_on_run_after: int | None = None,
        fail_on_smv_after: int | None = None,
    ) -> None:
        self.work_dir = work_dir
        self.registers = registers
        self.failure = failure
        self.fail_at = fail_at
        self.fail_on_run_after = fail_on_run_after
        self.fail_on_smv_after = fail_on_smv_after
        self.captures = 0

    def __enter__(self) -> "FakeSession":
        return self

    def __exit__(self, *arguments: object) -> bool:
        return False

    def request(self, kind: str, text: str = "", timeout: float = 10.0) -> mock.Mock:
        if kind == "BREAK":
            self.captures += 1
            if self.captures >= self.fail_at:
                raise self.failure
        if (
            self.fail_on_run_after is not None
            and kind == "EXEC"
            and text == "RUN"
            and self.captures >= self.fail_on_run_after
        ):
            raise self.failure
        if (
            self.fail_on_smv_after is not None
            and kind == "EXEC"
            and text.startswith("SMV ")
            and self.captures >= self.fail_on_smv_after
        ):
            raise self.failure
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
        self.failure = ConnectionError(
            "DOSBox-X closed the control connection"
        )
        self.fail_at = 2
        self.fail_on_run_after = None
        self.fail_on_smv_after = None
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
        return FakeSession(
            self.work,
            self.registers,
            self.failure,
            self.fail_at,
            self.fail_on_run_after,
            self.fail_on_smv_after,
        )

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

    def test_a_read_timeout_ends_the_capture(self) -> None:
        self.failure = TimeoutError("timed out")
        output = io.StringIO()
        with self.patched(), contextlib.redirect_stdout(output):
            names = member_probe.run_probe(
                self.game, self.work, 9876, 5, 60.0, "dosbox-x"
            )
        self.assertEqual(names, [NAME])
        self.assertIn("the debugger read timed out", output.getvalue())

    def test_the_command_line_exits_zero_after_a_timeout(self) -> None:
        self.failure = TimeoutError("timed out")
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

    def test_a_timeout_on_the_first_read_observes_no_members(self) -> None:
        self.failure = TimeoutError("timed out")
        self.fail_at = 1
        output = io.StringIO()
        with self.patched(), contextlib.redirect_stdout(output):
            names = member_probe.run_probe(
                self.game, self.work, 9876, 5, 60.0, "dosbox-x"
            )
        self.assertEqual(names, [])
        self.assertIn("the capture ends after 0 member opens", output.getvalue())

    def test_the_command_line_exits_zero_with_no_members(self) -> None:
        self.failure = TimeoutError("timed out")
        self.fail_at = 1
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
        self.assertIn("observed 0 member opens", output.getvalue())

    def test_a_timeout_on_the_final_read_keeps_the_full_list(self) -> None:
        self.failure = TimeoutError("timed out")
        self.fail_at = 100
        self.fail_on_run_after = 2
        output = io.StringIO()
        with self.patched(), contextlib.redirect_stdout(output):
            names = member_probe.run_probe(
                self.game, self.work, 9876, 2, 60.0, "dosbox-x"
            )
        self.assertEqual(names, [NAME, NAME])
        self.assertIn(
            "the capture ends after 2 member opens", output.getvalue()
        )

    def test_the_command_line_exits_zero_after_the_full_list(self) -> None:
        self.failure = TimeoutError("timed out")
        self.fail_at = 100
        self.fail_on_run_after = 2
        output = io.StringIO()
        arguments = [
            "member_probe.py",
            "--game-dir",
            str(self.game),
            "--work-dir",
            str(self.work),
            "--limit",
            "2",
        ]
        with self.patched(), mock.patch.object(
            sys, "argv", arguments
        ), contextlib.redirect_stdout(output):
            self.assertEqual(member_probe.main(), 0)
        self.assertIn("observed 2 member opens", output.getvalue())

    def test_a_failure_on_the_final_write_keeps_the_full_list(self) -> None:
        self.failure = TimeoutError("timed out")
        self.fail_at = 100
        self.fail_on_smv_after = 2
        output = io.StringIO()
        with self.patched(), contextlib.redirect_stdout(output):
            names = member_probe.run_probe(
                self.game, self.work, 9876, 2, 60.0, "dosbox-x"
            )
        self.assertEqual(names, [NAME, NAME])
        self.assertIn(
            "the capture ends after 2 member opens", output.getvalue()
        )

    def test_the_command_line_exits_zero_after_the_final_write(self) -> None:
        self.failure = TimeoutError("timed out")
        self.fail_at = 100
        self.fail_on_smv_after = 2
        output = io.StringIO()
        arguments = [
            "member_probe.py",
            "--game-dir",
            str(self.game),
            "--work-dir",
            str(self.work),
            "--limit",
            "2",
        ]
        with self.patched(), mock.patch.object(
            sys, "argv", arguments
        ), contextlib.redirect_stdout(output):
            self.assertEqual(member_probe.main(), 0)
        self.assertIn("observed 2 member opens", output.getvalue())
        self.assertIn(NAME, output.getvalue())

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


class RestoreResponse:
    """The response shape the control session returns."""

    def __init__(self, lines: list[str]) -> None:
        self.lines = lines


class RestoreSession:
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

    def __enter__(self) -> "RestoreSession":
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
    ) -> RestoreResponse:
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
            return RestoreResponse([])
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
            return RestoreResponse([" ".join(f"{value:08X}" for value in values)])
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
            return RestoreResponse([])
        return RestoreResponse([])


class FakePtyProcess:
    """A stand-in for the DOSBox-X child process."""

    def __init__(self, close_error: type[Exception] | None = None) -> None:
        self.close_error = close_error

    def close(self) -> None:
        if self.close_error is not None:
            raise self.close_error(
                "simulated control connection close during the restore"
            )


class FakeKillProcess:
    """A child process whose kill call fails."""

    def kill(self) -> None:
        raise OSError("simulated kill failure")


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
        original = bytearray(member_probe.MEMBER_OPEN_FILE + 5)
        original[member_probe.MEMBER_OPEN_FILE] = 0x57
        self.exe.write_bytes(bytes(original))
        self.original = bytes(original)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def probe(
        self,
        error: type[Exception] | None,
        members_before_error: int = 1,
        error_at: str = "break",
        limit: int = 200,
        close_error: type[Exception] | None = None,
        child: object | None = None,
    ) -> list[str]:
        session = RestoreSession(
            self.work, error, members_before_error, error_at
        )
        pty = child if child is not None else FakePtyProcess(close_error)
        with mock.patch.object(
            member_probe,
            "PtyProcess",
            lambda *args, **kwargs: pty,
        ), mock.patch.object(
            member_probe,
            "ControlSession",
            lambda port, timeout: session,
        ), mock.patch.object(member_probe.time, "sleep", lambda seconds: None):
            return member_probe.run_probe(
                self.game, self.work, 9999, limit, 300.0, "dosbox-x"
            )

    def assert_game_copy_restored(self) -> None:
        self.assertEqual(self.exe.read_bytes(), self.original)

    def test_returns_the_names_when_the_connection_closes_mid_run(self) -> None:
        names = self.probe(ConnectionError, 1, "break")
        self.assertEqual(names, ["MENU.dtr"])
        self.assert_game_copy_restored()

    def test_returns_the_names_when_the_connection_times_out_mid_run(self) -> None:
        names = self.probe(TimeoutError, 1, "break")
        self.assertEqual(names, ["MENU.dtr"])
        self.assert_game_copy_restored()

    def test_observes_no_members_when_the_connection_closes_before_the_first(
        self,
    ) -> None:
        names = self.probe(ConnectionError, 0, "break")
        self.assertEqual(names, [])
        self.assert_game_copy_restored()

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

    def test_propagates_an_os_error_from_the_child_close(self) -> None:
        with self.assertRaises(OSError):
            self.probe(None, 0, "break", limit=1, close_error=OSError)
        self.assert_game_copy_restored()

    def test_restores_the_original_game_copy_on_a_child_close_os_error(self) -> None:
        with self.assertRaises(OSError):
            self.probe(None, 0, "break", limit=1, close_error=OSError)
        self.assert_game_copy_restored()

    def test_restores_the_original_game_copy_on_a_child_kill_os_error(
        self,
    ) -> None:
        child = member_probe.PtyProcess.__new__(member_probe.PtyProcess)
        child._stop = threading.Event()
        child.process = FakeKillProcess()
        child.master = -1
        with self.assertRaises(OSError):
            self.probe(None, 0, "break", limit=1, child=child)
        self.assert_game_copy_restored()

    def test_restores_the_original_game_copy_on_a_patch_write_error(self) -> None:
        real_write_bytes = Path.write_bytes
        calls = {"count": 0}

        def write_bytes(path: Path, data: bytes) -> int:
            if path == self.exe:
                calls["count"] += 1
                if calls["count"] == 1:
                    raise OSError("simulated patch write failure")
            return real_write_bytes(path, data)

        with mock.patch.object(Path, "write_bytes", write_bytes):
            with self.assertRaises(OSError):
                self.probe(None, 0, "break", limit=1)
        self.assert_game_copy_restored()

    def test_restores_the_original_game_copy_after_a_partial_patch_write(
        self,
    ) -> None:
        real_write_bytes = Path.write_bytes
        calls = {"count": 0}

        def write_bytes(path: Path, data: bytes) -> int:
            if path == self.exe:
                calls["count"] += 1
                if calls["count"] == 1:
                    real_write_bytes(
                        path, data[: member_probe.MEMBER_OPEN_FILE + 1]
                    )
                    raise OSError("simulated partial patch write failure")
            return real_write_bytes(path, data)

        with mock.patch.object(Path, "write_bytes", write_bytes):
            with self.assertRaises(OSError):
                self.probe(None, 0, "break", limit=1)
        self.assert_game_copy_restored()

    def test_restores_the_original_game_copy_after_a_partial_rollback_write(
        self,
    ) -> None:
        real_write_bytes = Path.write_bytes
        calls = {"count": 0}

        def write_bytes(path: Path, data: bytes) -> int:
            if path == self.exe:
                calls["count"] += 1
                if calls["count"] == 1:
                    raise OSError("simulated patch write failure")
                if calls["count"] == 2:
                    real_write_bytes(
                        path, data[: member_probe.MEMBER_OPEN_FILE + 1]
                    )
                    raise OSError("simulated partial rollback failure")
            return real_write_bytes(path, data)

        with mock.patch.object(Path, "write_bytes", write_bytes):
            with self.assertRaises(OSError):
                self.probe(None, 0, "break", limit=1)
        self.assert_game_copy_restored()

    def test_restores_the_original_game_copy_after_a_partial_restore_write(
        self,
    ) -> None:
        real_write_bytes = Path.write_bytes
        calls = {"count": 0}

        def write_bytes(path: Path, data: bytes) -> int:
            if path == self.exe:
                calls["count"] += 1
                if calls["count"] == 2:
                    real_write_bytes(
                        path, data[: member_probe.MEMBER_OPEN_FILE + 1]
                    )
                    raise OSError("simulated partial restore failure")
            return real_write_bytes(path, data)

        with mock.patch.object(Path, "write_bytes", write_bytes):
            names = self.probe(None, 0, "break", limit=1)
        self.assertEqual(names, ["MENU.dtr"])
        self.assert_game_copy_restored()

    def test_leaves_the_original_game_copy_on_a_backup_read_error(self) -> None:
        real_read_bytes = Path.read_bytes

        def read_bytes(path: Path) -> bytes:
            if path == self.exe:
                raise OSError("simulated backup read failure")
            return real_read_bytes(path)

        with mock.patch.object(Path, "read_bytes", read_bytes):
            with self.assertRaises(OSError):
                self.probe(None, 0, "break", limit=1)
        self.assert_game_copy_restored()

    def test_propagates_an_os_error_from_the_game_copy_restore(self) -> None:
        real_write_bytes = Path.write_bytes
        calls = {"count": 0}

        def write_bytes(path: Path, data: bytes) -> int:
            if path == self.exe:
                calls["count"] += 1
                if calls["count"] >= 2:
                    raise OSError("simulated restore failure")
            return real_write_bytes(path, data)

        with mock.patch.object(Path, "write_bytes", write_bytes):
            with self.assertRaises(OSError):
                self.probe(None, 0, "break", limit=1)
        self.assertEqual(
            self.exe.read_bytes()[member_probe.MEMBER_OPEN_FILE], 0xCC
        )

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
