"""Tests for the Dosker client, with a fake CLI and no container.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import json
import os
import tempfile
import unittest
from pathlib import Path

import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import dosker  # noqa: E402

FAKE = """#!/usr/bin/env python3
import json
import os
import sys

args = sys.argv[1:]
log = os.environ.get("FAKE_DOSKER_LOG")
if log:
    with open(log, "a", encoding="utf-8") as stream:
        stream.write(
            json.dumps(
                {"args": args, "root": os.environ.get("DOSKER_ROOT")}
            )
            + "\\n"
        )
if args == ["--version"]:
    print("dosker " + os.environ.get("FAKE_DOSKER_VERSION", "0.1.3"))
    sys.exit(0)
if args and args[0] == "exec":
    print("guest output")
    sys.exit(int(os.environ.get("FAKE_EXEC_CODE", "0")))
if args and args[0] == "images":
    print("REFERENCE DIGEST SIZE CREATED")
    print("freedos:1.3 sha256:111 33554432 1980")
    print("screamer-run:1 sha256:222 33554432 2026")
    sys.exit(0)
if args[:2] == ["volume", "ls"]:
    print("NAME SIZE")
    print("screamer-watcom-v2-tools 256M")
    sys.exit(0)
sys.exit(0)
"""


class DoskerClientTestCase(unittest.TestCase):
    """The client with a fake Dosker binary."""

    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        self.binary = self.root / "dosker"
        self.binary.write_text(FAKE)
        self.binary.chmod(0o755)
        self.log = self.root / "calls.jsonl"
        os.environ["FAKE_DOSKER_LOG"] = str(self.log)
        self.client = dosker.DoskerClient(
            str(self.binary), self.root / "root", self.root / "work"
        )

    def tearDown(self) -> None:
        os.environ.pop("FAKE_DOSKER_LOG", None)
        os.environ.pop("FAKE_DOSKER_VERSION", None)
        os.environ.pop("FAKE_EXEC_CODE", None)
        self.directory.cleanup()

    def calls(self) -> list[dict]:
        return [
            json.loads(line)
            for line in self.log.read_text().splitlines()
            if line.strip()
        ]

    def test_version(self) -> None:
        self.assertEqual(self.client.version(), "0.1.3")
        self.assertEqual(self.client.check_version(), "0.1.3")

    def test_version_mismatch(self) -> None:
        os.environ["FAKE_DOSKER_VERSION"] = "0.1.2"
        with self.assertRaises(dosker.DoskerError) as context:
            self.client.check_version()
        self.assertIn("0.1.2", str(context.exception))
        self.assertIn("0.1.3", str(context.exception))

    def test_missing_binary(self) -> None:
        client = dosker.DoskerClient(
            str(self.root / "missing"), self.root / "root", self.root
        )
        with self.assertRaises(dosker.DoskerMissing):
            client.version()

    def test_data_root_is_passed(self) -> None:
        self.client.version()
        self.assertEqual(self.calls()[-1]["root"], str(self.client.data_root))

    def test_exec_returns_guest_code(self) -> None:
        os.environ["FAKE_EXEC_CODE"] = "7"
        result = self.client.exec("c", "BUILD.BAT", workdir="C:\\WORK")
        self.assertEqual(result.exit_code, 7)
        self.assertIn("guest output", result.output)
        self.assertFalse(result.timed_out)
        call = self.calls()[-1]["args"]
        self.assertEqual(call[0], "exec")
        self.assertEqual(call[-2:], ["c", "BUILD.BAT"])
        self.assertIn("C:\\WORK", call)

    def test_exec_timeout(self) -> None:
        os.environ["FAKE_EXEC_CODE"] = "124"
        result = self.client.exec("c", "HANG")
        self.assertTrue(result.timed_out)

    def test_start_arguments(self) -> None:
        self.client.start(
            "freedos:1.3",
            "c",
            volumes=[dosker.Volume("tools", "D:")],
        )
        call = self.calls()[-1]["args"]
        self.assertEqual(call[:3], ["run", "-d", "--name"])
        self.assertIn("-v", call)
        self.assertIn("tools:D:", call)
        self.assertEqual(call[-1], "freedos:1.3")
        self.assertIn("--boot-timeout", call)

    def test_copy_arguments(self) -> None:
        host = self.root / "free.c"
        self.client.copy_in("c", host, "C:\\WORK\\S0.C")
        self.assertEqual(self.calls()[-1]["args"][0], "cp")
        self.assertEqual(self.calls()[-1]["args"][2], "c:C:\\WORK\\S0.C")
        self.client.copy_out("c", "C:\\WORK\\FREE.EXE", host)
        self.assertEqual(self.calls()[-1]["args"][1], "c:C:\\WORK\\FREE.EXE")

    def test_images_and_volume(self) -> None:
        self.assertTrue(self.client.image_exists("freedos:1.3"))
        self.assertEqual(self.client.image_digest("freedos:1.3"), "sha256:111")
        self.assertTrue(self.client.volume_exists("screamer-watcom-v2-tools"))
        self.assertFalse(self.client.volume_exists("missing"))
        self.assertEqual(
            self.client.volume_image("screamer-watcom-v2-tools"),
            self.client.data_root / "volumes/screamer-watcom-v2-tools/volume.img",
        )


if __name__ == "__main__":
    unittest.main()
