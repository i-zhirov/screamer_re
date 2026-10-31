"""Tests for the pipeline preflight check of the debugger control port.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import socket
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import pipeline_check  # noqa: E402


class PortTestCase(unittest.TestCase):
    """The preflight must tell a held debugger port from a free one."""

    def bound(self) -> tuple[socket.socket, int]:
        listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(listener.close)
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        return listener, listener.getsockname()[1]

    def test_a_held_port_is_not_free(self) -> None:
        _, port = self.bound()
        self.assertFalse(pipeline_check.debugger_port_free(port))

    def test_a_closed_port_is_free(self) -> None:
        listener, port = self.bound()
        listener.close()
        self.assertTrue(pipeline_check.debugger_port_free(port))


class PreflightTestCase(unittest.TestCase):
    """The command must stop before stage 1 when an input is unusable."""

    def run_check(
        self, game_dir: Path, work_root: Path, port: int
    ) -> subprocess.CompletedProcess:
        return subprocess.run(
            [
                sys.executable,
                str(Path(pipeline_check.__file__).resolve()),
                "--game-dir",
                str(game_dir),
                "--work-root",
                str(work_root),
                "--port",
                str(port),
            ],
            capture_output=True,
            text=True,
            timeout=60,
        )

    def assert_no_stage(
        self, result: subprocess.CompletedProcess, message: str, work_root: Path
    ) -> None:
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn(message, result.stderr)
        self.assertNotIn("[PASS]", result.stdout)
        self.assertNotIn("checks,", result.stdout)
        self.assertFalse(work_root.exists())

    def free_port(self) -> int:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as picker:
            picker.bind(("127.0.0.1", 0))
            return picker.getsockname()[1]

    def test_a_held_port_exits_two_before_any_stage(self) -> None:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
            listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listener.bind(("127.0.0.1", 0))
            listener.listen(1)
            port = listener.getsockname()[1]
            with tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                work_root = root / "work"
                result = self.run_check(root / "game", work_root, port)
                self.assert_no_stage(
                    result,
                    f"the debugger control port {port} is already in use",
                    work_root,
                )

    def test_a_missing_game_dir_exits_two_before_any_stage(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            work_root = root / "work"
            result = self.run_check(root / "game", work_root, self.free_port())
            self.assert_no_stage(
                result, "the game directory", work_root
            )

    def test_a_missing_exe_exits_two_before_any_stage(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            game = root / "game"
            game.mkdir()
            work_root = root / "work"
            result = self.run_check(game, work_root, self.free_port())
            self.assert_no_stage(
                result, "the reference executable STARTH.EXE is missing", work_root
            )

    def test_a_blocked_work_root_exits_two_before_any_stage(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            game = root / "game"
            game.mkdir()
            (game / "STARTH.EXE").write_bytes(b"MZ")
            blocked = root / "blocked"
            blocked.write_text("not a directory")
            work_root = blocked / "work"
            result = self.run_check(game, work_root, self.free_port())
            self.assert_no_stage(result, "the work root", work_root)

if __name__ == "__main__":
    unittest.main()
