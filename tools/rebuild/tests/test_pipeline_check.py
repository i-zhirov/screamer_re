"""Tests for the pipeline preflight check of the debugger control port.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import contextlib
import io
import socket
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import pipeline_check  # noqa: E402


FOUR_AGREE_LINES = (
    "entry: agree le=0x300f8 r2=0x300f8 ghidra=0x300f8\n"
    "page0: agree le=0x1a800->0x10000 r2=0x1a800->0x10000 ghidra=0x1a800->0x10000\n"
    "main: agree le=0x10020 r2=0x10020 ghidra=0x10020\n"
    "caller: agree le=0xd4364@0xd4324->0x10020 r2=0xd4364@0xd4324->0x10020 "
    "ghidra=0xd4364@0xd4324->0x10020\n"
)


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
        written = list(work_root.iterdir()) if work_root.exists() else []
        self.assertEqual(written, [], "the preflight wrote nothing")

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

    def test_an_invalid_exe_exits_two_before_any_stage(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            game = root / "game"
            game.mkdir()
            (game / "STARTH.EXE").write_bytes(b"not an MZ image")
            work_root = root / "work"
            result = self.run_check(game, work_root, self.free_port())
            self.assert_no_stage(
                result, "is not a valid LE image", work_root
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

class ToolPreflightTestCase(unittest.TestCase):
    """The preflight must name the first missing tool and its command."""

    def test_capstone_is_checked_before_radare2(self) -> None:
        with mock.patch.object(
            pipeline_check.le_module, "capstone_available", return_value=False
        ) as capstone, mock.patch.object(pipeline_check.shutil, "which") as which:
            message = pipeline_check.tool_preflight(False, 300.0)
        self.assertIsNotNone(message)
        self.assertIn("capstone", message)
        capstone.assert_called_once_with()
        which.assert_not_called()

    def test_a_missing_radare2_names_the_cross_check_shell(self) -> None:
        with mock.patch.object(
            pipeline_check.le_module, "capstone_available", return_value=True
        ), mock.patch.object(
            pipeline_check.shutil, "which", return_value=None
        ) as which:
            message = pipeline_check.tool_preflight(False, 300.0)
        self.assertIsNotNone(message)
        self.assertIn("nix develop .#cross-check", message)
        which.assert_called_once_with("r2")

    def test_a_missing_ghidra_names_the_install_command(self) -> None:
        failed = subprocess.CompletedProcess([], 2, "", "missing loader")
        with mock.patch.object(
            pipeline_check.le_module, "capstone_available", return_value=True
        ), mock.patch.object(
            pipeline_check.shutil, "which", return_value="/usr/bin/r2"
        ), mock.patch.object(
            pipeline_check, "run", return_value=failed
        ):
            message = pipeline_check.tool_preflight(False, 300.0)
        self.assertIsNotNone(message)
        self.assertIn("install_loader.sh", message)

    def test_skip_cross_checks_no_tool(self) -> None:
        with mock.patch.object(
            pipeline_check.le_module, "capstone_available"
        ) as capstone, mock.patch.object(
            pipeline_check.shutil, "which"
        ) as which, mock.patch.object(pipeline_check, "run") as run_mock:
            message = pipeline_check.tool_preflight(True, 300.0)
        self.assertIsNone(message)
        capstone.assert_not_called()
        which.assert_not_called()
        run_mock.assert_not_called()


class StageEnabledTestCase(unittest.TestCase):
    """The two flags must select the stages each mode runs."""

    def test_skip_runtime_keeps_stage_eight(self) -> None:
        self.assertTrue(pipeline_check.stage_enabled(8, True, False))

    def test_skip_cross_disables_stage_eight(self) -> None:
        self.assertFalse(pipeline_check.stage_enabled(8, False, True))

    def test_skip_cross_keeps_the_input_stages(self) -> None:
        for stage in (1, 2, 3, 4):
            self.assertTrue(pipeline_check.stage_enabled(stage, False, True))

    def test_both_flags_disable_the_optional_stages(self) -> None:
        for stage in (5, 6, 7, 8, 9):
            self.assertFalse(pipeline_check.stage_enabled(stage, True, True))
        for stage in (1, 2, 3, 4, 10, 11, 12):
            self.assertTrue(pipeline_check.stage_enabled(stage, True, True))


class ParseFactLinesTestCase(unittest.TestCase):
    """The stage reads only the four fact lines."""

    def test_parse_fact_lines_reads_the_four_facts(self) -> None:
        facts = pipeline_check.parse_fact_lines(FOUR_AGREE_LINES)
        self.assertEqual(
            set(facts), {"entry", "page0", "main", "caller"}
        )
        self.assertEqual(
            facts["entry"], "entry: agree le=0x300f8 r2=0x300f8 ghidra=0x300f8"
        )

    def test_parse_fact_lines_ignores_other_output(self) -> None:
        text = "noise: agree now\nentry: agree le=0x1 r2=0x1 ghidra=0x1\n"
        facts = pipeline_check.parse_fact_lines(text)
        self.assertEqual(set(facts), {"entry"})


class StageEightTestCase(unittest.TestCase):
    """Stage 8 must add one check per fact from the cross-check output."""

    def run_stage(
        self, result: subprocess.CompletedProcess
    ) -> tuple[pipeline_check.Checks, mock.Mock]:
        checks = pipeline_check.Checks()
        with mock.patch.object(
            pipeline_check, "run", return_value=result
        ) as run_mock, contextlib.redirect_stdout(io.StringIO()):
            pipeline_check.check_cross_tools(
                checks, Path("/game"), "STARTH.EXE", Path("/work")
            )
        return checks, run_mock

    def test_stage_eight_adds_four_checks_for_four_agree_lines(self) -> None:
        result = subprocess.CompletedProcess([], 0, FOUR_AGREE_LINES, "")
        checks, _ = self.run_stage(result)
        self.assertEqual(len(checks.results), 4)
        self.assertTrue(all(ok for _, ok, _ in checks.results))

    def test_stage_eight_fails_the_differing_fact_only(self) -> None:
        text = FOUR_AGREE_LINES.replace(
            "main: agree le=0x10020 r2=0x10020 ghidra=0x10020",
            "main: differ le=0x10020 r2=0x10020 ghidra=0x10021",
        )
        result = subprocess.CompletedProcess([], 1, text, "")
        checks, _ = self.run_stage(result)
        self.assertEqual(len(checks.results), 4)
        verdicts = {name: ok for name, ok, _ in checks.results}
        self.assertFalse(
            verdicts["stage 8: the main function agrees in all three tools"]
        )
        self.assertEqual(sum(1 for ok in verdicts.values() if ok), 3)

    def test_stage_eight_fails_all_four_when_a_line_is_missing(self) -> None:
        text = "\n".join(FOUR_AGREE_LINES.splitlines()[:3]) + "\n"
        result = subprocess.CompletedProcess([], 1, text, "")
        checks, _ = self.run_stage(result)
        self.assertEqual(len(checks.results), 4)
        details = {name: (ok, detail) for name, ok, detail in checks.results}
        ok, detail = details[
            "stage 8: the main caller agrees in all three tools"
        ]
        self.assertFalse(ok)
        self.assertEqual(detail, "no fact line")

    def test_stage_eight_fails_all_four_on_a_tool_failure(self) -> None:
        result = subprocess.CompletedProcess([], 2, "", "tool failed")
        checks, _ = self.run_stage(result)
        self.assertEqual(len(checks.results), 4)
        self.assertTrue(all(not ok for _, ok, _ in checks.results))

    def test_stage_eight_fails_all_four_on_an_unexpected_return_code(
        self,
    ) -> None:
        result = subprocess.CompletedProcess([], 2, FOUR_AGREE_LINES, "")
        checks, _ = self.run_stage(result)
        self.assertEqual(len(checks.results), 4)
        self.assertTrue(all(not ok for _, ok, _ in checks.results))
        self.assertIn("the cross-check exited 2", checks.results[0][2])

    def test_stage_eight_passes_the_main_and_call_site_constants(self) -> None:
        result = subprocess.CompletedProcess([], 0, FOUR_AGREE_LINES, "")
        _, run_mock = self.run_stage(result)
        command = run_mock.call_args.args[0]
        self.assertIn("--main", command)
        self.assertIn("0x10020", command)
        self.assertIn("--call-site", command)
        self.assertIn("0xd4364", command)


class ImageGateTestCase(unittest.TestCase):
    """Stage 11 must skip the image gate until a whole image exists."""

    def test_missing_image_is_a_skip(self) -> None:
        checks = pipeline_check.Checks()
        with contextlib.redirect_stdout(io.StringIO()):
            pipeline_check.check_image(
                checks,
                Path("/game"),
                "STARTH.EXE",
                Path("/missing/STARTH.EXE"),
            )
        self.assertEqual(checks.results, [])

    def test_present_image_runs_the_comparison(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            rebuilt = Path(temporary) / "STARTH.EXE"
            rebuilt.write_bytes(b"rebuilt")
            checks = pipeline_check.Checks()
            result = subprocess.CompletedProcess(
                [], 0, "verdict: the images are byte-identical\n", ""
            )
            with mock.patch.object(
                pipeline_check, "run", return_value=result
            ) as run_mock, contextlib.redirect_stdout(io.StringIO()):
                pipeline_check.check_image(
                    checks, Path("/game"), "STARTH.EXE", rebuilt
                )
        self.assertEqual(len(checks.results), 1)
        self.assertTrue(checks.results[0][1])
        self.assertTrue(
            any("image_compare.py" in part for part in run_mock.call_args.args[0])
        )

    def test_a_differing_image_fails(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            rebuilt = Path(temporary) / "STARTH.EXE"
            rebuilt.write_bytes(b"rebuilt")
            checks = pipeline_check.Checks()
            result = subprocess.CompletedProcess(
                [], 1, "verdict: the images differ\n", ""
            )
            with mock.patch.object(
                pipeline_check, "run", return_value=result
            ), contextlib.redirect_stdout(io.StringIO()):
                pipeline_check.check_image(
                    checks, Path("/game"), "STARTH.EXE", rebuilt
                )
        self.assertEqual(len(checks.results), 1)
        self.assertFalse(checks.results[0][1])


if __name__ == "__main__":
    unittest.main()
