"""Tests for the DOS/4G 2.01a extraction, the 8.3 check, and the setup check.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import hashlib
import os
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import dosker_setup  # noqa: E402

FAKE = """#!/usr/bin/env python3
import os
import sys

args = sys.argv[1:]
if args == ["--version"]:
    print("dosker 0.1.5")
    sys.exit(0)
if args and args[0] == "images":
    print("REFERENCE DIGEST SIZE CREATED")
    print(
        os.environ.get(
            "FAKE_IMAGES",
            "freedos:1.3 sha256:111 33554432 1980\\n"
            "screamer-run:1 sha256:222 33554432 2026",
        )
    )
    sys.exit(0)
if args[:2] == ["volume", "ls"]:
    print("NAME SIZE")
    print(
        os.environ.get(
            "FAKE_VOLUMES",
            "screamer-watcom-v2-tools 256M\\nscreamer-watcom10-tools 256M",
        )
    )
    sys.exit(0)
sys.exit(0)
"""


def fake_binary(payload: bytes, marker: bytes = b"MZ") -> bytes:
    """A binary with the pinned extender offset and the given payload."""
    data = bytearray(b"\x00" * (dosker_setup.EXTENDER_OFFSET + len(payload)))
    data[dosker_setup.EXTENDER_OFFSET :] = payload
    data[dosker_setup.EXTENDER_OFFSET : dosker_setup.EXTENDER_OFFSET + 2] = marker
    return bytes(data)


class ExtractExtenderTestCase(unittest.TestCase):
    """The setup extracts the pinned blob and rejects a foreign build."""

    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)

    def tearDown(self) -> None:
        self.directory.cleanup()

    def payload(self) -> bytes:
        return b"MZ" + b"\x90" * (dosker_setup.EXTENDER_SIZE - 2)

    def test_extracts_the_offset(self) -> None:
        payload = self.payload()
        data = fake_binary(payload)
        binary = self.root / "dosbox-x"
        binary.write_bytes(data)
        out = self.root / "DOS4GW.EXE"
        digest = dosker_setup.extract_extender(
            binary,
            out,
            hashlib.sha256(data).hexdigest(),
            hashlib.sha256(payload).hexdigest(),
        )
        self.assertEqual(out.read_bytes(), payload)
        self.assertEqual(digest, hashlib.sha256(payload).hexdigest())

    def test_rejects_a_missing_marker(self) -> None:
        payload = self.payload()
        data = fake_binary(payload, marker=b"ZZ")
        binary = self.root / "dosbox-x"
        binary.write_bytes(data)
        with self.assertRaises(ValueError) as context:
            dosker_setup.extract_extender(
                binary,
                self.root / "DOS4GW.EXE",
                hashlib.sha256(data).hexdigest(),
                hashlib.sha256(payload).hexdigest(),
            )
        self.assertIn("holds no DOS/4G 2.01a", str(context.exception))

    def test_rejects_a_foreign_build(self) -> None:
        data = fake_binary(self.payload())
        binary = self.root / "dosbox-x"
        binary.write_bytes(data)
        with self.assertRaises(ValueError) as context:
            dosker_setup.extract_extender(
                binary, self.root / "DOS4GW.EXE", "0" * 64, None
            )
        self.assertIn("the pinned build is", str(context.exception))


class NameCheckTestCase(unittest.TestCase):
    """The 8.3 check accepts a DOS tree and reports a foreign name."""

    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)

    def tearDown(self) -> None:
        self.directory.cleanup()

    def test_accepts_8_3_names(self) -> None:
        (self.root / "BINB").mkdir()
        (self.root / "BINB" / "WCC386.EXE").write_bytes(b"")
        (self.root / "A.B").write_bytes(b"")
        (self.root / "README").write_bytes(b"")
        (self.root / "lower.exe").write_bytes(b"")
        self.assertEqual(dosker_setup.check_names(self.root), [])

    def test_reports_the_bad_names(self) -> None:
        (self.root / "TOOLONGNAME.EXE").write_bytes(b"")
        (self.root / "A.TEXTT").write_bytes(b"")
        (self.root / "A..B").write_bytes(b"")
        names = dosker_setup.check_names(self.root)
        self.assertEqual(len(names), 3)


class CheckTestCase(unittest.TestCase):
    """The --check reports the V2 and the 10.0a volume lines."""

    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        self.binary = self.root / "dosker"
        self.binary.write_text(FAKE)
        self.binary.chmod(0o755)
        self.arguments = SimpleNamespace(
            dosker=str(self.binary),
            data_root=self.root / "root",
            extender=dosker_setup.EXTENDER_PATH,
        )
        self.images = {
            "screamer-watcom-v2-tools": self.root
            / "root/volumes/screamer-watcom-v2-tools/volume.img",
            "screamer-watcom10-tools": self.root
            / "root/volumes/screamer-watcom10-tools/volume.img",
        }
        for image in self.images.values():
            image.parent.mkdir(parents=True, exist_ok=True)
            image.write_bytes(b"\x00")

    def tearDown(self) -> None:
        os.environ.pop("FAKE_VOLUMES", None)
        os.environ.pop("FAKE_IMAGES", None)
        self.directory.cleanup()

    def run_check(self) -> str:
        with mock.patch.object(dosker_setup, "mtools_available", return_value=True):
            with mock.patch.object(
                dosker_setup, "volume_has_toolchain", return_value=True
            ):
                with mock.patch("sys.stdout") as stream:
                    dosker_setup.check(self.arguments)
                    return "".join(call.args[0] for call in stream.write.call_args_list)

    def test_reports_the_10_0a_volume(self) -> None:
        text = self.run_check()
        self.assertIn("check: volume screamer-watcom10-tools: present", text)
        self.assertIn("check: toolchain 10.0a: present", text)

    def test_reports_the_run_image(self) -> None:
        text = self.run_check()
        self.assertIn("check: run image screamer-run:1: present", text)

    def test_reports_a_missing_run_image(self) -> None:
        os.environ["FAKE_IMAGES"] = "freedos:1.3 sha256:111 33554432 1980"
        text = self.run_check()
        self.assertIn("check: run image screamer-run:1: missing", text)

    def test_builds_the_run_image(self) -> None:
        extender = self.root / "DOS4GW.EXE"
        extender.write_bytes(b"MZ")
        context = self.root / "context"
        client = SimpleNamespace(
            image_exists=lambda reference: False,
            build_image=lambda reference, path: None,
        )
        with mock.patch.object(dosker_setup, "RUN_CONTEXT", context):
            dosker_setup.ensure_run_image(client, extender)
        self.assertEqual((context / "extender/DOS4GW.EXE").read_bytes(), b"MZ")
        self.assertIn(
            "COPY extender/ C:/", (context / "Doskerfile").read_text()
        )

    def test_reports_a_missing_10_0a_volume(self) -> None:
        os.environ["FAKE_VOLUMES"] = "screamer-watcom-v2-tools 256M"
        text = self.run_check()
        self.assertIn("check: volume screamer-watcom10-tools: missing", text)
        self.assertIn("check: toolchain 10.0a: missing", text)


if __name__ == "__main__":
    unittest.main()
