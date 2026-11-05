"""Tests for the entry-probe log scan.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import entry_probe  # noqa: E402


class DetectEntryTestCase(unittest.TestCase):
    """The debugger repeats a logged instruction, and the repeats vary."""

    def test_repeated_lines_do_not_hide_the_entry(self) -> None:
        parsed = [
            ("0150", "1F20F8", "jmp  short 001F2170 ($+76)"),
            ("0150", "1F20F8", "jmp  short 001F2170 ($+76)"),
            ("0150", "1F20F8", "jmp  short 001F2170 ($+76)"),
            ("0150", "1F2170", "sti"),
            ("0150", "1F2170", "sti"),
            ("0150", "1F2170", "sti"),
            ("0150", "1F2174", "mov  ebx,esp"),
            ("0150", "1F2195", "int  21"),
        ]
        found = entry_probe.detect_entry(parsed, entry_probe.CANDIDATES)
        self.assertIsNotNone(found)
        assert found is not None
        self.assertEqual(found["candidate"], "entry jump over the Watcom banner")
        self.assertEqual(found["eip"], "1F20F8")
        self.assertEqual(found["following"][:2], ["sti", "mov  ebx,esp"])


if __name__ == "__main__":
    unittest.main()
