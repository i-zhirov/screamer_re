"""Tests for the reusable function-start helpers of the LE tool.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests -p 'test_le_helpers.py'
"""
from __future__ import annotations

import contextlib
import io
import sys
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import le  # noqa: E402

PROLOGUE_SAVE = b"\x53\x55\x89\xe5\x90\x55\x89\xe5"
BASE = 0x10000


class StubObject:
    """The object table row the helpers read."""

    def __init__(self, reloc_base: int, virtual_size: int) -> None:
        self.reloc_base = reloc_base
        self.virtual_size = virtual_size


class StubImage:
    """The minimal image the function-start helpers read."""

    def __init__(self, data: bytes, base: int = BASE, objects: bool = True) -> None:
        self.base = base
        self.data = data
        self.entry_linear = base
        self.objects = [StubObject(base, len(data))] if objects else []

    def object_at(self, address: int) -> StubObject | None:
        for obj in self.objects:
            if obj.reloc_base <= address < obj.reloc_base + obj.virtual_size:
                return obj
        return None

    def read(self, address: int, length: int) -> bytes | None:
        offset = address - self.base
        if offset < 0:
            return None
        return self.data[offset : offset + length]


class UnreadableImage(StubImage):
    """An image whose object exists but has no file mapping."""

    def read(self, address: int, length: int) -> bytes | None:
        return None


class FunctionStartsTestCase(unittest.TestCase):
    def test_function_starts_returns_the_prologue_starts(self) -> None:
        image = StubImage(PROLOGUE_SAVE)
        self.assertEqual(le.function_starts(image), [0x10000, 0x10005])

    def test_function_starts_honors_the_limit(self) -> None:
        image = StubImage(PROLOGUE_SAVE)
        self.assertEqual(le.function_starts(image, limit=1), [0x10000])

    def test_function_starts_rejects_a_missing_entry_object(self) -> None:
        image = StubImage(PROLOGUE_SAVE, objects=False)
        with self.assertRaisesRegex(ValueError, "the entry object is missing"):
            le.function_starts(image)

    def test_function_starts_rejects_an_unmapped_range(self) -> None:
        image = UnreadableImage(PROLOGUE_SAVE)
        with self.assertRaisesRegex(
            ValueError, "the range is not mapped to a file offset"
        ):
            le.function_starts(image)


class NearestFunctionStartTestCase(unittest.TestCase):
    def test_nearest_function_start_finds_the_start_below_the_address(self) -> None:
        image = StubImage(PROLOGUE_SAVE)
        self.assertEqual(le.nearest_function_start(image, 0x10004), 0x10000)

    def test_nearest_function_start_returns_none_without_a_prologue(self) -> None:
        image = StubImage(b"\x90" * 8)
        self.assertIsNone(le.nearest_function_start(image, 0x10004))

    def test_nearest_function_start_rejects_an_unmapped_address(self) -> None:
        image = StubImage(b"\x90" * 8, base=0x20000)
        image.entry_linear = 0x20000
        with self.assertRaisesRegex(ValueError, "the address is unmapped"):
            le.nearest_function_start(image, 0x10004)


class CapstoneAvailableTestCase(unittest.TestCase):
    def test_capstone_available_reports_a_missing_library(self) -> None:
        with mock.patch.object(le, "_load_capstone", return_value=None):
            self.assertFalse(le.capstone_available())

    def test_capstone_available_reports_a_loaded_library(self) -> None:
        with mock.patch.object(
            le, "_load_capstone", return_value=object()
        ), mock.patch.object(le, "_capstone") as capstone:
            self.assertTrue(le.capstone_available())
        capstone.assert_not_called()


class CommandOutputTestCase(unittest.TestCase):
    def test_funcs_output_stays_identical(self) -> None:
        image = StubImage(PROLOGUE_SAVE)
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            self.assertEqual(le.print_functions(image, None, None, 10000), 0)
        self.assertEqual(
            output.getvalue(),
            "candidate function starts: 2\n"
            "  0x00010000\n"
            "  0x00010005\n",
        )

    def test_near_output_stays_identical(self) -> None:
        image = StubImage(PROLOGUE_SAVE)
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            self.assertEqual(le.print_nearest_function(image, 0x10004), 0)
        self.assertEqual(
            output.getvalue(),
            "nearest prologue: 0x10000 (0x4 bytes below 0x10004)\n",
        )


if __name__ == "__main__":
    unittest.main()
