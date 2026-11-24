"""Tests for the whole-image comparison.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import image_compare  # noqa: E402
import le  # noqa: E402


def image(
    data: bytes,
    *,
    header_offset: int = 0,
    header: dict | None = None,
    objects: list[le.Object] | None = None,
    pages: list[le.Page] | None = None,
) -> le.LinearExecutable:
    exe = le.LinearExecutable(Path("image.exe"), data)
    exe.header_offset = header_offset
    exe.header = dict(header or {})
    exe.objects = list(objects or [])
    exe.pages = list(pages or [])
    return exe


def one_object() -> tuple[list[le.Object], list[le.Page]]:
    objects = [le.Object(1, 0x20, 0x10000, 0x2045, 1, 1, 0)]
    pages = [le.Page(1, 0, 1, 0, 0x40, 0x10, 0x10000, 0x20)]
    return objects, pages


class EqualPrefixTestCase(unittest.TestCase):
    def test_equal_and_total(self) -> None:
        self.assertEqual(image_compare.equal_prefix(b"abc", b"abd"), (2, 3))

    def test_length_difference_counts_in_the_total(self) -> None:
        self.assertEqual(image_compare.equal_prefix(b"abc", b"abcde"), (3, 5))


class FileRegionsTestCase(unittest.TestCase):
    def test_regions_in_file_order(self) -> None:
        objects, pages = one_object()
        exe = image(
            bytes(0x100),
            header_offset=0x10,
            header={
                "object_table_offset": 0x4,
                "fixup_size": 0x8,
                "fixup_page_table_offset": 0x10,
                "non_resident_name_table_offset": 0x60,
                "non_resident_name_size": 0x4,
            },
            objects=objects,
            pages=pages,
        )
        regions = image_compare.file_regions(exe)
        self.assertEqual(
            [(region.name, region.start, region.end) for region in regions],
            [
                ("stub", 0, 0x10),
                ("header", 0x10, 0x14),
                ("loader", 0x14, 0x20),
                ("fixups", 0x20, 0x28),
                ("obj.1", 0x40, 0x50),
                ("nonresident", 0x60, 0x64),
            ],
        )


class CompareLoadedTestCase(unittest.TestCase):
    def test_identical_images(self) -> None:
        objects, pages = one_object()
        left = image(bytes(0x100), objects=objects, pages=pages)
        right = image(bytes(0x100), objects=objects, pages=pages)
        result = image_compare.compare_loaded(left, right)
        self.assertTrue(result.exact)
        self.assertEqual(result.equal, result.total)
        self.assertEqual(result.verdict, "the images are byte-identical")
        self.assertEqual(result.differences, [])

    def test_one_different_byte(self) -> None:
        objects, pages = one_object()
        left = image(bytes(0x100), objects=objects, pages=pages)
        right_data = bytearray(0x100)
        right_data[0x41] = 0xFF
        right = image(bytes(right_data), objects=objects, pages=pages)
        result = image_compare.compare_loaded(left, right)
        self.assertFalse(result.exact)
        self.assertEqual(result.equal, 0xFF)
        region = {score.name: score for score in result.regions}["obj.1"]
        self.assertFalse(region.exact)

    def test_header_difference(self) -> None:
        left = image(b"", header={"cpu": 2})
        right = image(b"", header={"cpu": 3})
        differences = image_compare.compare_headers(left, right)
        self.assertEqual(
            differences, ["header.cpu: original 2, rebuilt 3"]
        )

    def test_object_difference(self) -> None:
        original, pages = one_object()
        changed = [le.Object(1, 0x40, 0x10000, 0x2045, 1, 1, 0)]
        left = image(b"", objects=original, pages=pages)
        right = image(b"", objects=changed, pages=pages)
        differences = image_compare.compare_objects(left, right)
        self.assertEqual(
            differences,
            ["obj.1.virtual_size: original 32, rebuilt 64"],
        )

    def test_page_difference(self) -> None:
        objects, pages = one_object()
        changed = [le.Page(1, 0, 2, 0, 0x40, 0x10, 0x10000, 0x20)]
        left = image(b"", objects=objects, pages=pages)
        right = image(b"", objects=objects, pages=changed)
        differences = image_compare.compare_pages(left, right)
        self.assertEqual(
            differences, ["page obj.1.0.number: original 1, rebuilt 2"]
        )

    def test_fixup_section_difference(self) -> None:
        header = {
            "fixup_size": 4,
            "fixup_page_table_offset": 0,
        }
        left = image(b"ABCD", header=header)
        right = image(b"ABCE", header=header)
        differences = image_compare.compare_fixups(left, right)
        self.assertEqual(
            differences, ["fixup section: 3 of 4 bytes equal"]
        )

    def test_fixup_section_accessor(self) -> None:
        exe = image(
            b"XXXXABCD",
            header={"fixup_size": 4, "fixup_page_table_offset": 4},
        )
        self.assertEqual(exe.fixup_section(), b"ABCD")


def coverage_objects() -> tuple[list[le.Object], list[le.Page]]:
    objects = [
        le.Object(1, 0x20, 0x10000, le.O_EXECUTABLE, 1, 1, 0),
        le.Object(2, 0x10, 0x20000, le.O_WRITABLE, 2, 1, 0),
    ]
    pages = [
        le.Page(1, 0, 1, 0, 0x40, 0x10, 0x10000, 0x20),
        le.Page(2, 0, 2, 0, 0x80, 0x10, 0x20000, 0x10),
    ]
    return objects, pages


class CoverageTestCase(unittest.TestCase):
    def test_identical_images_are_identical(self) -> None:
        objects, pages = coverage_objects()
        left = image(bytes(0x100), objects=objects, pages=pages)
        right = image(bytes(0x100), objects=objects, pages=pages)
        report = image_compare.coverage_loaded(left, right)
        self.assertEqual(report.verdict, "identical")
        self.assertEqual(report.code_gap, 0)
        self.assertEqual(report.data_gap, 0)
        self.assertEqual(report.code_linked, 0x20)

    def test_an_unmodeled_range_marks_a_region(self) -> None:
        objects, pages = coverage_objects()
        left = image(bytes(0x100), objects=objects, pages=pages)
        right = image(bytes(0x100), objects=objects, pages=pages)
        report = image_compare.coverage_loaded(
            left, right, [image_compare.LinearRange(0x10008, 0x1000C, "code")]
        )
        statuses = {region.name: region.status for region in report.regions}
        self.assertEqual(statuses["obj.1.10008"], "unmodeled")
        self.assertEqual(report.verdict, "unmodeled")
        self.assertEqual(report.unmodeled_bytes, 4)

    def test_a_short_rebuilt_region_is_absent(self) -> None:
        objects, pages = coverage_objects()
        left = image(bytes(0x100), objects=objects, pages=pages)
        right = image(bytes(0x50), objects=objects, pages=pages)
        report = image_compare.coverage_loaded(left, right)
        statuses = {region.name: region.status for region in report.regions}
        self.assertEqual(statuses["obj.1.10000"], "absent")
        self.assertEqual(report.verdict, "absent")

    def test_a_different_byte_is_different(self) -> None:
        objects, pages = coverage_objects()
        left = image(bytes(0x100), objects=objects, pages=pages)
        changed = bytearray(0x100)
        changed[0x40] = 0xFF
        right = image(bytes(changed), objects=objects, pages=pages)
        report = image_compare.coverage_loaded(left, right)
        statuses = {region.name: region.status for region in report.regions}
        self.assertEqual(statuses["obj.1.10000"], "different")
        self.assertEqual(report.verdict, "different")


class PlanUnmodeledTestCase(unittest.TestCase):
    def test_ranges_are_parsed(self) -> None:
        ranges = image_compare.plan_unmodeled(
            {
                "unmodeled": [
                    {"start": "0x10000", "end": "0x10010", "kind": "code"},
                    {"start": 0x20000, "end": 0x20010, "kind": "data"},
                ]
            }
        )
        self.assertEqual(
            ranges,
            [
                image_compare.LinearRange(0x10000, 0x10010, "code"),
                image_compare.LinearRange(0x20000, 0x20010, "data"),
            ],
        )

    def test_a_bad_kind_raises(self) -> None:
        with self.assertRaises(ValueError):
            image_compare.plan_unmodeled(
                {"unmodeled": [{"start": "0x10000", "end": "0x10010", "kind": "x"}]}
            )

    def test_an_empty_range_raises(self) -> None:
        with self.assertRaises(ValueError):
            image_compare.plan_unmodeled(
                {"unmodeled": [{"start": "0x10010", "end": "0x10010", "kind": "code"}]}
            )


if __name__ == "__main__":
    unittest.main()
