"""Tests for the rebuild harness script builders.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import harness  # noqa: E402


class HarnessTestCase(unittest.TestCase):
    """The pure functions of the harness, with no Docker and no DOSBox-X."""

    def module(self, **overrides: object) -> dict:
        base: dict = {
            "name": "free",
            "exe": "free.exe",
            "map": "free.map",
            "sources": [
                {"file": "free.c", "compiler": "wcc386"},
                {"file": "free_main.c", "compiler": "wcc386"},
            ],
            "run": {
                "command": "free.exe > free_out.txt",
                "output": "free_out.txt",
                "expect": ["NULL", "DONE", "FREE", "AFTR"],
                "timeout": 180,
            },
        }
        base.update(overrides)
        return base

    def test_constants(self) -> None:
        self.assertEqual(
            harness.IMAGE,
            "arlaneenalra/watcom-docker@sha256:"
            "16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65",
        )
        self.assertEqual(
            harness.SHARED_SOURCES, ("chk.asm", "stack.asm", "cstart.c")
        )
        self.assertEqual(harness.DOSBOX, "dosbox-x")
        self.assertEqual(harness.WORK_DIR, harness.REPOSITORY_ROOT / "work/rebuild")
        self.assertEqual(harness.DOSKER_IMAGE, "freedos:1.3")
        self.assertEqual(harness.DOSKER_TOOLCHAIN_DRIVE, "D:")
        self.assertEqual(harness.DOSKER_TOOLCHAIN_ROOT, "D:\\WATCOM")
        self.assertEqual(
            harness.EXTENDER_201, harness.REPOSITORY_ROOT / "work/dosker/DOS4GW.EXE"
        )
        self.assertEqual(harness.RUN_IMAGE, "screamer-run:1")

    def test_build_script_free(self) -> None:
        script = harness.build_script(self.module())
        self.assertEqual(
            script.splitlines(),
            [
                "wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm",
                "wasm -bt=dos -zq -zcm=watcom -fo=/w/stack.obj stack.asm",
                "wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c",
                "wcc386 -bt=dos -s -zq -fo=/w/free.obj free.c",
                "wcc386 -bt=dos -s -zq -fo=/w/free_main.obj free_main.c",
                "wlink system dos4g option quiet option nodefaultlibs"
                " option start=_cstart_ option map=/w/free.map name free.exe"
                " file free.obj,free_main.obj,chk.obj,cstart.obj,stack.obj",
            ],
        )

    def test_build_script_driver_holds_no_map(self) -> None:
        module = self.module(
            name="driver",
            exe="driver.exe",
            sources=[{"file": "driver.c", "compiler": "wcc386"}],
        )
        module.pop("map")
        script = harness.build_script(module)
        lines = script.splitlines()
        self.assertEqual(lines[-1].count("option map="), 0)
        self.assertIn(
            "name driver.exe file driver.obj,chk.obj,cstart.obj,stack.obj",
            lines[-1],
        )
        self.assertEqual(len(lines), 5)

    def test_build_script_ignores_period_flags(self) -> None:
        module = self.module(
            name="list_destroy",
            exe="ldmod.exe",
            map="list_destroy.map",
            sources=[
                {
                    "file": "list_destroy.c",
                    "compiler": "wcc386",
                    "flags": ["-of+", "-od"],
                    "period_flags": ["-od"],
                },
            ],
        )
        lines = harness.build_script(module).splitlines()
        self.assertIn(
            "wcc386 -bt=dos -s -zq -of+ -od -fo=/w/list_destroy.obj"
            " list_destroy.c",
            lines,
        )

    def test_build_script_source_flags(self) -> None:
        module = self.module(
            name="list_destroy",
            exe="ldmod.exe",
            map="list_destroy.map",
            sources=[
                {"file": "memcpy.asm", "compiler": "wasm"},
                {
                    "file": "list_destroy.c",
                    "compiler": "wcc386",
                    "flags": ["-of+", "-od"],
                },
            ],
        )
        lines = harness.build_script(module).splitlines()
        self.assertIn(
            "wasm -bt=dos -zq -zcm=watcom -fo=/w/memcpy.obj memcpy.asm",
            lines,
        )
        self.assertIn(
            "wcc386 -bt=dos -s -zq -of+ -od -fo=/w/list_destroy.obj"
            " list_destroy.c",
            lines,
        )
        self.assertEqual(
            lines[-1],
            "wlink system dos4g option quiet option nodefaultlibs"
            " option start=_cstart_ option map=/w/list_destroy.map"
            " name ldmod.exe file"
            " memcpy.obj,list_destroy.obj,chk.obj,cstart.obj,stack.obj",
        )

    def test_dosker_build_batch_free(self) -> None:
        lines = harness.dosker_build_batch(self.module()).splitlines()
        self.assertEqual(lines[0], "@ECHO OFF")
        self.assertIn("SET WATCOM=D:\\WATCOM", lines)
        self.assertIn("IF NOT EXIST C:\\TMP\\NUL MD C:\\TMP >NUL", lines)
        self.assertIn(
            "WCC386 -bt=dos -s -zq -fo=C:\\WORK\\CSTART.OBJ "
            "C:\\WORK\\CSTART.C",
            lines,
        )
        self.assertIn(
            "WASM -bt=dos -zq -zcm=watcom -fo=C:\\WORK\\CHK.OBJ "
            "C:\\WORK\\CHK.ASM",
            lines,
        )
        self.assertIn(
            "wcc386 -bt=dos -s -zq -fo=C:\\WORK\\O0.OBJ C:\\WORK\\S0.C",
            lines,
        )
        self.assertIn(
            "wcc386 -bt=dos -s -zq -fo=C:\\WORK\\O1.OBJ C:\\WORK\\S1.C",
            lines,
        )
        self.assertEqual(lines[-1], "WLINK @C:\\WORK\\LINK.RSP")

    def test_dosker_link_rsp_free(self) -> None:
        lines = harness.dosker_link_rsp(self.module()).splitlines()
        self.assertEqual(lines[:4], [
            "system dos4g",
            "option quiet",
            "option nodefaultlibs",
            "option start=_cstart_",
        ])
        self.assertIn("option map=C:\\WORK\\FREE.MAP", lines)
        self.assertIn("name C:\\WORK\\free.exe", lines)
        self.assertIn(
            "file C:\\WORK\\O0.OBJ,C:\\WORK\\O1.OBJ,"
            "C:\\WORK\\CHK.OBJ,C:\\WORK\\CSTART.OBJ,"
            "C:\\WORK\\STACK.OBJ",
            lines,
        )

    def test_dosker_link_rsp_driver_holds_no_map(self) -> None:
        module = self.module(
            name="driver",
            exe="driver.exe",
            sources=[{"file": "driver.c", "compiler": "wcc386"}],
        )
        module.pop("map")
        lines = harness.dosker_link_rsp(module).splitlines()
        self.assertFalse(any(line.startswith("option map=") for line in lines))
        self.assertIn(
            "file C:\\WORK\\O0.OBJ,C:\\WORK\\CHK.OBJ,"
            "C:\\WORK\\CSTART.OBJ,C:\\WORK\\STACK.OBJ",
            lines,
        )

    def test_dosker_source_names_are_8_3(self) -> None:
        self.assertEqual(
            harness.dosker_source_name(0, {"file": "free.c", "compiler": "wcc386"}),
            "S0.C",
        )
        self.assertEqual(
            harness.dosker_source_name(1, {"file": "chk.asm", "compiler": "wasm"}),
            "S1.ASM",
        )
        self.assertEqual(harness.dosker_object_name(2), "O2.OBJ")

    def test_free_port(self) -> None:
        port = harness.free_port()
        self.assertIsInstance(port, int)
        self.assertGreaterEqual(port, 1)
        self.assertLessEqual(port, 65535)

    def test_run_script_text(self) -> None:
        self.assertEqual(
            harness.run_script_text(),
            "break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n",
        )

    def test_conf_text(self) -> None:
        text = harness.conf_text(self.module(), 9881)
        self.assertEqual(
            text,
            "[dosbox]\n"
            "mcp_server=9881\n"
            "memsize=32\n"
            "machine=svga_s3\n"
            "\n"
            "[autoexec]\n"
            f'mount D "{harness.WORK_DIR}"\n'
            "D:\n"
            "free.exe > free_out.txt\n",
        )

    def test_missing_lines_accepts_lines_on_their_own(self) -> None:
        text = "NULL\nDONE\nFREE\nAFTR\n"
        self.assertEqual(
            harness.missing_lines(["NULL", "DONE", "FREE", "AFTR"], text), []
        )

    def test_missing_lines_accepts_embedded_suffix(self) -> None:
        expect = [
            "W 20 0F",
            "W 40 3F",
            "W 60 01",
            "W 80 3E",
            "W E0 0F",
            "W 23 3F",
            "W 43 01",
            "W C0 01",
        ]
        text = (
            "boot bytes 0f 6d tW 20 0F\n"
            "W 40 3F\n"
            "W 60 01\n"
            "W 80 3E\n"
            "W E0 0F\n"
            "W 23 3F\n"
            "W 43 01\n"
            "W C0 01\n"
        )
        self.assertEqual(harness.missing_lines(expect, text), [])

    def test_missing_lines_rejects_crash_dump(self) -> None:
        expect = ["R", "R", "R", "D"]
        text = (
            "DOS/4G error (2001): exception 0Dh (general protection fault)"
            " at 1A0:00000001\n"
            "SS        A8 DS       200 ES       200 FS         0 GS        20\n"
            "CR0: PG:0 ET:1 TS:0 EM:0 MP:0 PE:1   CR2: 0 CR3: 0\n"
            "Crash address (unrelocated) = 3:00000001\n"
        )
        self.assertEqual(harness.missing_lines(expect, text), ["R", "R", "R", "D"])




DOSKER_RUN_FAKE = """#!/usr/bin/env python3
import json
import os
import sys

args = sys.argv[1:]
log = os.environ.get("FAKE_DOSKER_LOG")
if log:
    with open(log, "a", encoding="utf-8") as stream:
        stream.write(json.dumps({"args": args}) + "\\n")
if args == ["--version"]:
    print("dosker 0.1.5")
    sys.exit(0)
if args and args[0] == "exec" and args[-1].lower() == "free.exe":
    print("NULL\\nDONE\\nFREE\\nAFTR")
sys.exit(0)
"""


class DoskerRunTestCase(unittest.TestCase):
    """The Dosker behavior run with a fake client and a fake extender."""

    def module(self) -> dict:
        return {
            "name": "free",
            "exe": "free.exe",
            "run": {
                "output": "free_out.txt",
                "expect": ["NULL", "DONE", "FREE", "AFTR"],
                "timeout": 10,
            },
        }

    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        self.binary = self.root / "dosker"
        self.binary.write_text(DOSKER_RUN_FAKE)
        self.binary.chmod(0o755)
        self.log = self.root / "calls.jsonl"
        os.environ["FAKE_DOSKER_LOG"] = str(self.log)
        self.previous_extender = harness.EXTENDER_201
        self.previous_work = harness.WORK_DIR
        harness.EXTENDER_201 = self.root / "DOS4GW.EXE"
        harness.EXTENDER_201.write_bytes(b"MZ")
        harness.WORK_DIR = self.root / "rebuild"
        harness.WORK_DIR.mkdir()
        (harness.WORK_DIR / "free.exe").write_bytes(b"MZ")
        self.arguments = SimpleNamespace(
            dosker=str(self.binary),
            data_root=self.root / "root",
            toolchain_volume="screamer-watcom-v2-tools",
            backend="dosker",
        )

    def tearDown(self) -> None:
        harness.EXTENDER_201 = self.previous_extender
        harness.WORK_DIR = self.previous_work
        os.environ.pop("FAKE_DOSKER_LOG", None)
        self.directory.cleanup()

    def calls(self) -> list[dict]:
        return [
            json.loads(line)
            for line in self.log.read_text().splitlines()
            if line.strip()
        ]

    def test_run_writes_the_guest_output(self) -> None:
        module = self.module()
        code = harness.run_dosker(
            "free", module, module["run"], False, self.arguments
        )
        self.assertEqual(code, 0)
        out = (self.root / "rebuild/free_out.txt").read_text()
        for marker in ("NULL", "DONE", "FREE", "AFTR"):
            self.assertIn(marker, out)
        copied = [call["args"] for call in self.calls() if call["args"][0] == "cp"]
        guest = [args[2].split(":", 1)[1] for args in copied]
        self.assertIn("C:\\FREE.EXE", guest)
        self.assertNotIn("C:\\RUN\\FREE.EXE", guest)
        self.assertIn("C:\\DOS4GW.EXE", guest)
        execs = [call["args"] for call in self.calls() if call["args"][0] == "exec"]
        self.assertEqual(execs[-1][-1], "free.exe")
        self.assertIn("C:\\", execs[-1])
        runs = [call["args"] for call in self.calls() if call["args"][0] == "run"]
        self.assertIn("screamer-run:1", runs[-1])


if __name__ == "__main__":
    unittest.main()
