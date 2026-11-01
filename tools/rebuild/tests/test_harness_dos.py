"""Tests for the period toolchain build script builders.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

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

    def test_compile_line_wcc386(self) -> None:
        line = harness_dos.compile_line({"file": "free.c", "compiler": "wcc386"}, 0)
        self.assertEqual(
            line,
            "wcc386 -bt=dos -s -zq -i=C:\\WATCOM\\H -fo=D:\\O0.OBJ S0.C",
        )

    def test_compile_line_wasm_holds_no_stack_flag(self) -> None:
        line = harness_dos.compile_line({"file": "memcpy.asm", "compiler": "wasm"}, 1)
        self.assertEqual(line, "wasm -bt=dos -zq -fo=D:\\O1.OBJ S1.ASM")

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


if __name__ == "__main__":
    unittest.main()
