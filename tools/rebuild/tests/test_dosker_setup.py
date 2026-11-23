"""Tests for the DOS/4G 2.01a extraction, with a fake DOSBox-X binary.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import hashlib
import tempfile
import unittest
from pathlib import Path

import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import dosker_setup  # noqa: E402


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


if __name__ == "__main__":
    unittest.main()
