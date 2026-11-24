"""Tests for the whole-image link plan, build, and coverage.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import image_plan  # noqa: E402
import le  # noqa: E402
import store  # noqa: E402


def record(address: int, module: str | None, size: int | None = None) -> dict:
    entry = {"address": f"0x{address:x}"}
    if module is not None:
        entry["module"] = module
    if size is not None:
        entry["size"] = size
    return entry


def plan(order: list[str]) -> dict:
    return {
        "version": 1,
        "toolchain": "screamer-watcom-v2-tools",
        "engine": "qemu",
        "exe": "STARTH.EXE",
        "map": "STARTH.MAP",
        "link_options": list(image_plan.LINK_OPTIONS),
        "runtime_libs": [],
        "order": order,
        "unmodeled": [],
    }


class DeriveOrderTestCase(unittest.TestCase):
    def test_order_is_the_lowest_record_address(self) -> None:
        records = [
            record(0x200, "zeta"),
            record(0x100, "alpha"),
            record(0x150, "alpha"),
            record(0x300, "beta"),
        ]
        self.assertEqual(image_plan.derive_order(records), ["alpha", "zeta", "beta"])

    def test_a_record_without_a_module_is_ignored(self) -> None:
        records = [record(0x100, None), record(0x200, "alpha")]
        self.assertEqual(image_plan.derive_order(records), ["alpha"])


class RecordExtentsTestCase(unittest.TestCase):
    def test_size_and_fallback(self) -> None:
        records = [
            record(0x100, "a", 0x10),
            record(0x110, "b", 0x10),
            record(0x200, "c"),
        ]
        self.assertEqual(
            image_plan.record_extents(records, 0x300),
            [(0x100, 0x120), (0x200, 0x300)],
        )

    def test_extents_clip_to_the_limit(self) -> None:
        records = [record(0x100, "a", 0x500)]
        self.assertEqual(image_plan.record_extents(records, 0x200), [(0x100, 0x200)])


class ComplementTestCase(unittest.TestCase):
    def test_gaps_around_the_covered_ranges(self) -> None:
        self.assertEqual(
            image_plan.complement(0, 0x100, [(0x20, 0x40), (0x80, 0x90)]),
            [(0, 0x20), (0x40, 0x80), (0x90, 0x100)],
        )

    def test_adjacent_ranges_merge(self) -> None:
        self.assertEqual(
            image_plan.complement(0, 0x100, [(0x20, 0x40), (0x40, 0x60)]),
            [(0, 0x20), (0x60, 0x100)],
        )


class SeedUnmodeledTestCase(unittest.TestCase):
    def test_code_gap_and_data_object(self) -> None:
        objects = [
            le.Object(1, 0x40, 0x10000, le.O_EXECUTABLE, 1, 1, 0),
            le.Object(2, 0x10, 0x20000, le.O_WRITABLE, 2, 0, 0),
        ]
        ranges = image_plan.seed_unmodeled(objects, [(0x10010, 0x10020)])
        self.assertEqual(
            ranges,
            [
                {"start": "0x10000", "end": "0x10010", "kind": "code"},
                {"start": "0x10020", "end": "0x10040", "kind": "code"},
                {"start": "0x20000", "end": "0x20010", "kind": "data"},
            ],
        )


class LoadPlanTestCase(unittest.TestCase):
    def write(self, directory: Path, value: object) -> Path:
        path = directory / "link.json"
        path.write_text(json.dumps(value))
        return path

    def test_a_valid_plan_loads(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            path = self.write(Path(name), plan(["free"]))
            self.assertEqual(image_plan.load_plan(path)["order"], ["free"])

    def test_a_missing_plan_raises(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            with self.assertRaises(image_plan.LinkPlanMissing):
                image_plan.load_plan(Path(name) / "link.json")

    def test_a_wrong_version_raises(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            value = plan(["free"])
            value["version"] = 2
            with self.assertRaises(ValueError):
                image_plan.load_plan(self.write(Path(name), value))

    def test_a_duplicate_order_raises(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            with self.assertRaises(ValueError):
                image_plan.load_plan(
                    self.write(Path(name), plan(["free", "free"]))
                )

    def test_a_bad_output_name_raises(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            value = plan(["free"])
            value["exe"] = "STARTH_LONG.EXE"
            with self.assertRaises(ValueError):
                image_plan.load_plan(self.write(Path(name), value))


class StagedNamesTestCase(unittest.TestCase):
    def test_every_staged_name_fits_8_3(self) -> None:
        sources = [{"file": "a.c", "compiler": "wcc386"}]
        image_plan.validate_staged_names(plan(["free"]), sources)
        for index in (0, 1, 0xFF, 0xFFF):
            for name in (
                image_plan.staged_source_name(index, sources[0]),
                image_plan.staged_object_name(index),
            ):
                self.assertTrue(store.is_dos_83(name))

    def test_the_harness_source_is_recognized(self) -> None:
        self.assertTrue(image_plan.harness_source("free", "free_main.c"))
        self.assertTrue(image_plan.harness_source("game_main", "game_main_main.c"))
        self.assertFalse(image_plan.harness_source("game_main", "game_main.c"))
        self.assertFalse(image_plan.harness_source("free", "free.c"))


class BuildTextTestCase(unittest.TestCase):
    def sources(self) -> list[dict]:
        return [
            {"file": "a.c", "compiler": "wcc386", "flags": ["-of+"]},
            {"file": "b.asm", "compiler": "wasm"},
        ]

    def test_batch_compiles_every_source_and_links(self) -> None:
        text = image_plan.build_batch(plan(["free"]), self.sources())
        self.assertIn("SET WATCOM=D:\\WATCOM", text)
        self.assertIn(
            "wcc386 -bt=dos -s -zq -of+ -fo=C:\\WORK\\S000.OBJ C:\\WORK\\S000.C"
            " >> C:\\WORK\\BUILD.LOG",
            text,
        )
        self.assertIn(
            "wasm -bt=dos -zq -zcm=watcom -fo=C:\\WORK\\S001.OBJ C:\\WORK\\S001.ASM"
            " >> C:\\WORK\\BUILD.LOG",
            text,
        )
        self.assertIn("ECHO BUILD > C:\\WORK\\BUILD.LOG", text)
        self.assertTrue(
            text.endswith("WLINK @C:\\WORK\\LINK.RSP >> C:\\WORK\\BUILD.LOG\r\n")
        )

    def test_response_names_and_file_order(self) -> None:
        text = image_plan.link_response(plan(["free"]), self.sources())
        lines = text.splitlines()
        self.assertEqual(lines[0], "system dos4g")
        self.assertIn("option map=C:\\WORK\\STARTH.MAP", lines)
        self.assertIn("name C:\\WORK\\STARTH.EXE", lines)
        self.assertIn(
            "file C:\\WORK\\S000.OBJ,C:\\WORK\\S001.OBJ,"
            "C:\\WORK\\CHK.OBJ,C:\\WORK\\CSTART.OBJ,C:\\WORK\\STACK.OBJ",
            lines,
        )


if __name__ == "__main__":
    unittest.main()
