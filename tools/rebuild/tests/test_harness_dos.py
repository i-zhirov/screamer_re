"""Tests for the period toolchain build script builders.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import contextlib
import io
import json
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import harness_dos  # noqa: E402


class HarnessDosTestCase(unittest.TestCase):
    """The pure functions of the period toolchain, with no DOSBox-X."""

    def module(self, **overrides: object) -> dict:
        base: dict = {
            "name": "free",
            "exe": "free.exe",
            "map": "free.map",
            "sources": [
                {"file": "free.c", "compiler": "wcc386"},
                {"file": "free_main.c", "compiler": "wcc386"},
            ],
        }
        base.update(overrides)
        return base

    def write_recipe(self, directory: str, **overrides: object) -> Path:
        path = Path(directory) / "recipe.json"
        path.write_text(json.dumps(self.module(**overrides)))
        return path

    def test_load_recipe_reads_the_record(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = self.write_recipe(directory)
            self.assertEqual(harness_dos.load_recipe("free", path), self.module())

    def test_load_recipe_rejects_a_name_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = self.write_recipe(directory, name="other")
            with self.assertRaises(ValueError):
                harness_dos.load_recipe("free", path)

    def test_parse_arguments_accepts_a_recipe(self) -> None:
        arguments = [
            "harness_dos.py",
            "build",
            "--module",
            "free",
            "--recipe",
            "recipe.json",
        ]
        with mock.patch.object(sys, "argv", arguments):
            parsed = harness_dos.parse_arguments()
        self.assertEqual(parsed.recipe, Path("recipe.json"))

    def test_main_passes_the_recipe_to_build(self) -> None:
        arguments = [
            "harness_dos.py",
            "build",
            "--module",
            "free",
            "--recipe",
            "recipe.json",
            "--timeout",
            "120",
        ]
        with mock.patch.object(sys, "argv", arguments):
            with mock.patch.object(
                harness_dos, "command_build", return_value=0
            ) as build:
                code = harness_dos.main()
        self.assertEqual(code, 0)
        build.assert_called_once_with("free", 120, Path("recipe.json"))

    def test_log_failures_reads_an_error_line(self) -> None:
        text = "file.c(10): Error! E0000: cannot open file\n"
        self.assertEqual(
            harness_dos.log_failures(text),
            ["file.c(10): Error! E0000: cannot open file"],
        )

    def test_log_failures_accepts_a_warning_line(self) -> None:
        text = "file.c(10): Warning! W0000: unused variable\n"
        self.assertEqual(harness_dos.log_failures(text), [])

    def test_compile_line_wcc386(self) -> None:
        line = harness_dos.compile_line({"file": "free.c", "compiler": "wcc386"}, 0)
        self.assertEqual(
            line,
            "wcc386 -bt=dos -s -zq -i=C:\\WATCOM\\H -fo=D:\\O0.OBJ S0.C",
        )

    def test_compile_line_wasm_holds_the_fpu_flag(self) -> None:
        line = harness_dos.compile_line({"file": "memcpy.asm", "compiler": "wasm"}, 1)
        self.assertEqual(line, "wasm -bt=dos -zq -fpi87 -fo=D:\\O1.OBJ S1.ASM")

    def test_compile_line_wasm_holds_fpi87(self) -> None:
        line = harness_dos.compile_line({"file": "fsqrt.asm", "compiler": "wasm"}, 0)
        self.assertIn("-fpi87", line.split())

    def test_wait_for_artifacts_returns_early_when_the_process_exits(self) -> None:
        class ExitedProcess:
            def poll(self) -> int:
                return 1

        with tempfile.TemporaryDirectory() as directory:
            with mock.patch.object(harness_dos, "WORK_DIR", Path(directory)):
                start = time.monotonic()
                found = harness_dos.wait_for_artifacts(
                    self.module(), 10, ExitedProcess()
                )
                elapsed = time.monotonic() - start
        self.assertFalse(found)
        self.assertLess(elapsed, 3)

    def test_wait_for_artifacts_finds_the_artifact(self) -> None:
        module = self.module()
        module.pop("map")
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory)
            (work / module["exe"]).write_bytes(b"")
            with mock.patch.object(harness_dos, "WORK_DIR", work):
                with mock.patch.object(harness_dos.time, "sleep"):
                    self.assertTrue(
                        harness_dos.wait_for_artifacts(module, 10)
                    )

    def test_compile_line_source_flags(self) -> None:
        line = harness_dos.compile_line(
            {
                "file": "list_destroy.c",
                "compiler": "wcc386",
                "flags": ["-of+", "-od"],
            },
            0,
        )
        self.assertEqual(
            line,
            "wcc386 -bt=dos -s -zq -i=C:\\WATCOM\\H -of+ -od -fo=D:\\O0.OBJ S0.C",
        )

    def test_compile_line_period_flags_override(self) -> None:
        line = harness_dos.compile_line(
            {
                "file": "list_destroy.c",
                "compiler": "wcc386",
                "flags": ["-of+", "-od"],
                "period_flags": ["-od"],
            },
            0,
        )
        self.assertEqual(
            line,
            "wcc386 -bt=dos -s -zq -i=C:\\WATCOM\\H -od -fo=D:\\O0.OBJ S0.C",
        )

    def test_link_rsp_standard_libs(self) -> None:
        text = harness_dos.link_rsp(self.module(), 2)
        self.assertEqual(
            text.splitlines(),
            [
                "system dos4g",
                "option quiet",
                "option map=D:\\M.MAP",
                "name free.exe",
                "file O0.OBJ",
                "file O1.OBJ",
            ],
        )

    def test_link_rsp_driver_holds_no_map(self) -> None:
        module = self.module(name="driver", exe="driver.exe")
        module.pop("map")
        text = harness_dos.link_rsp(module, 1)
        lines = text.splitlines()
        self.assertNotIn("option map=", lines)
        self.assertIn("name driver.exe", lines)
        self.assertIn("file O0.OBJ", lines)

    def test_conf_text(self) -> None:
        text = harness_dos.conf_text(self.module(), 2)
        self.assertIn("[dos]\nlfn = true", text)
        self.assertIn(f"mount C {harness_dos.TOOLCHAIN_DIR}", text)
        self.assertIn(f"mount D {harness_dos.WORK_DIR}", text)
        self.assertIn("PATH=C:\\WATCOM\\BIN;C:\\WATCOM\\BINB", text)
        self.assertIn("set WATCOM=C:\\WATCOM", text)
        self.assertIn("wcc386 -bt=dos -s -zq -i=C:\\WATCOM\\H -fo=D:\\O0.OBJ"
                      " S0.C >> D:\\dos_build.log", text)
        self.assertIn("C:\\WATCOM\\BIN\\WLINK @D:\\dos_link.rsp"
                      " >> D:\\dos_build.log", text)
        self.assertTrue(text.endswith("exit\n"))


class HarnessDosBuildTestCase(unittest.TestCase):
    """The build command with a fake DOSBox-X and a temporary toolchain."""

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        root = Path(self.temporary.name)
        self.work = root / "work"
        self.rebuild = root / "rebuild"
        self.toolchain = root / "watcom10"
        self.work.mkdir()
        self.rebuild.mkdir()
        (self.toolchain / "WATCOM/BINB").mkdir(parents=True)
        (self.toolchain / "WATCOM/BINB/WCC386.EXE").write_bytes(b"")
        for name in ("free.c", "free_main.c"):
            (self.rebuild / name).write_text("int free_value;\n")
        self.saved = {
            name: getattr(harness_dos, name)
            for name in ("WORK_DIR", "REBUILD_DIR", "TOOLCHAIN_DIR")
        }
        self.addCleanup(self.restore)
        harness_dos.WORK_DIR = self.work
        harness_dos.REBUILD_DIR = self.rebuild
        harness_dos.TOOLCHAIN_DIR = self.toolchain
        self.recipe = root / "free.json"
        self.recipe.write_text(json.dumps(self.module()))

    def restore(self) -> None:
        for name, value in self.saved.items():
            setattr(harness_dos, name, value)

    def module(self) -> dict:
        return {
            "name": "free",
            "exe": "free.exe",
            "map": "free.map",
            "sources": [
                {"file": "free.c", "compiler": "wcc386"},
                {"file": "free_main.c", "compiler": "wcc386"},
            ],
        }

    def stale_files(self) -> tuple[Path, ...]:
        return (
            self.work / "free.exe",
            self.work / "free.map",
            self.work / harness_dos.MAP_NAME,
            self.work / harness_dos.LOG_NAME,
        )

    def write_stale(self) -> None:
        for path in self.stale_files():
            path.write_text("stale\n")

    def test_command_build_removes_a_stale_exe(self) -> None:
        self.write_stale()
        seen: list[tuple[bool, ...]] = []

        def fake_popen(*arguments: object, **keywords: object) -> mock.MagicMock:
            seen.append(tuple(path.exists() for path in self.stale_files()))
            return mock.MagicMock()

        def fake_wait(module: dict, timeout: int, process: object = None) -> bool:
            (self.work / "free.exe").write_text("built\n")
            (self.work / harness_dos.MAP_NAME).write_text("map\n")
            return True

        with mock.patch.object(
            harness_dos.shutil, "which", return_value="/bin/dosbox-x"
        ):
            with mock.patch.object(
                harness_dos.subprocess, "Popen", side_effect=fake_popen
            ):
                with mock.patch.object(
                    harness_dos, "wait_for_artifacts", side_effect=fake_wait
                ):
                    with contextlib.redirect_stdout(io.StringIO()):
                        with contextlib.redirect_stderr(io.StringIO()):
                            code = harness_dos.command_build(
                                "free", 30, self.recipe
                            )
        self.assertEqual(code, 0)
        self.assertEqual(len(seen), 1)
        self.assertEqual(seen[0], (False, False, False, False))

    def test_command_build_fails_on_an_error_log(self) -> None:
        def fake_wait(module: dict, timeout: int, process: object = None) -> bool:
            (self.work / harness_dos.LOG_NAME).write_text(
                "file.c(10): Error! E0000: cannot open file\n"
            )
            (self.work / harness_dos.MAP_NAME).write_text("map\n")
            return True

        with mock.patch.object(
            harness_dos.shutil, "which", return_value="/bin/dosbox-x"
        ):
            with mock.patch.object(harness_dos.subprocess, "Popen"):
                with mock.patch.object(
                    harness_dos, "wait_for_artifacts", side_effect=fake_wait
                ):
                    with self.assertRaises(ValueError):
                        harness_dos.command_build("free", 30, self.recipe)


if __name__ == "__main__":
    unittest.main()
