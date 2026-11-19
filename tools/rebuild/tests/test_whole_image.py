"""Tests for the whole-image build helpers.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import argparse
import contextlib
import hashlib
import io
import json
import sys
import tempfile
import unittest
from datetime import date
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import le  # noqa: E402
import whole_image  # noqa: E402


def insn(address: int, mnemonic: str, operands: str, raw: bytes) -> le.Insn:
    return le.Insn(address, len(raw), mnemonic, operands, raw)


def record(
    address: str,
    module: str | None,
    symbol: str | None,
    size: int,
    match: str | None = None,
) -> dict:
    value = {
        "address": address,
        "module": module,
        "symbol": symbol,
        "size": size,
    }
    if match is not None:
        value["result"] = {"checks": {"identity": {"match": match}}}
    return value


def module(
    name: str,
    sources: list[str],
    toolchain: str,
    exe: str | None = None,
    map: str | None = None,
) -> dict:
    return {
        "name": name,
        "sources": [{"file": file} for file in sources],
        "toolchain": toolchain,
        "exe": exe or f"{name}.exe",
        "map": map or f"{name}.map",
    }


class ModuleOrderTestCase(unittest.TestCase):
    def test_module_order_sorts_by_record_address(self) -> None:
        modules = [
            module("gamma", ["gamma.c"], "v2"),
            module("alpha", ["alpha.c"], "v2"),
            module("beta", ["beta.c"], "v2"),
        ]
        records = [
            record("0x300ec", "gamma", "gamma_", 12),
            record("0x10000", "alpha", "alpha_", 4),
            record("0x20000", "beta", "beta_", 4),
        ]

        items = whole_image.module_order(modules, records)

        self.assertEqual(
            [item["record"]["address"] for item in items],
            ["0x10000", "0x20000", "0x300ec"],
        )

    def test_module_order_takes_the_lowest_record_for_exit(self) -> None:
        modules = [module("exit", ["exit.c"], "v2", "exitmod.exe", "exit.map")]
        records = [
            record("0x300d6", "exit", "exit_flush_", 6),
            record("0x300be", "exit", "exit_", 24),
        ]

        items = whole_image.module_order(modules, records)

        self.assertEqual(len(items), 1)
        self.assertEqual(items[0]["record"]["address"], "0x300be")

    def test_module_order_excludes_a_module_without_a_record(self) -> None:
        modules = [
            module("driver", ["driver.c"], "v2"),
            module("helper", ["helper.c"], "v2"),
        ]
        records = [record("0x30100", "helper", None, 4)]

        items = whole_image.module_order(modules, records)

        self.assertEqual(items, [])

    def test_module_order_holds_one_item_per_module(self) -> None:
        modules = [module("free", ["free.c", "free_main.c"], "v2")]
        records = [
            record("0x300ec", "free", "free_", 12),
            record("0x30100", "free", "free_main_", 8),
        ]

        items = whole_image.module_order(modules, records)

        self.assertEqual(len(items), 1)
        self.assertEqual(items[0]["module"]["name"], "free")
        self.assertEqual(items[0]["record"]["address"], "0x300ec")


class ReportRowTestCase(unittest.TestCase):
    def test_report_row_holds_the_module_fields(self) -> None:
        item = {
            "module": module(
                "free", ["free.c", "free_main.c"], "watcom10"
            ),
            "record": record("0x300ec", "free", "free_", 12),
        }

        row = whole_image.report_row(item)

        self.assertEqual(row["module"], "free")
        self.assertEqual(row["address"], "0x300ec")
        self.assertEqual(row["size"], 12)
        self.assertEqual(row["toolchain"], "watcom10")
        self.assertEqual(row["sources"], ["free.c", "free_main.c"])

    def test_report_row_starts_skipped(self) -> None:
        item = {
            "module": module("free", ["free.c"], "v2"),
            "record": record("0x300ec", "free", "free_", 12),
        }

        row = whole_image.report_row(item)

        self.assertEqual(row["build"], "skip")
        self.assertFalse(row["fit"])
        self.assertIsNone(row["fit_reason"])
        self.assertEqual(row["rebuilt_size"], 0)
        self.assertEqual(row["original_size"], 0)
        self.assertEqual(row["relocated"], 0)
        self.assertIsNone(row["mode"])
        self.assertEqual(row["outcome"], "skipped")


class FitTestCase(unittest.TestCase):
    def test_fit_accepts_a_body_of_the_recorded_size(self) -> None:
        rebuilt = [
            insn(0x10000, "mov", "eax, 1", b"\xb8\x01"),
            insn(0x10002, "ret", "", b"\xc3\x90\x90"),
        ]

        reason = whole_image.fit(record("0x10000", "free", "free_", 5), rebuilt)

        self.assertIsNone(reason)

    def test_fit_rejects_a_short_body(self) -> None:
        rebuilt = [
            insn(0x10000, "mov", "eax, 1", b"\xb8\x01"),
            insn(0x10002, "ret", "", b"\xc3\x90"),
        ]

        reason = whole_image.fit(record("0x10000", "free", "free_", 5), rebuilt)

        self.assertEqual(
            reason, "the rebuilt body holds 4 bytes against the recorded 5"
        )

    def test_fit_rejects_a_long_body(self) -> None:
        rebuilt = [
            insn(0x10000, "mov", "eax, 1", b"\xb8\x01\x00"),
            insn(0x10003, "mov", "ebx, 2", b"\xbb\x02\x00"),
        ]

        reason = whole_image.fit(record("0x10000", "free", "free_", 5), rebuilt)

        self.assertEqual(
            reason, "the rebuilt body holds 6 bytes against the recorded 5"
        )


class AlignmentTestCase(unittest.TestCase):
    def test_aligned_accepts_equal_shapes(self) -> None:
        original = [insn(0x300EC, "mov", "eax, 0", b"\x01" * 6)]
        rebuilt = [insn(0x10000, "mov", "eax, 0", b"\x00" * 6)]
        original_relocations = [
            le.Relocation(
                offset=0x300EE, width=4, kind="internal", object=1, target=0x20
            )
        ]
        rebuilt_relocations = [
            le.Relocation(offset=0x10002, width=4, kind="internal")
        ]

        self.assertTrue(
            whole_image.aligned(
                original, rebuilt, original_relocations, rebuilt_relocations
            )
        )

    def test_aligned_rejects_a_different_instruction_count(self) -> None:
        original = [
            insn(0x300EC, "mov", "eax, 0", b"\x00" * 4),
            insn(0x300F0, "ret", "", b"\xc3\x90\x90\x90"),
        ]
        rebuilt = [insn(0x10000, "mov", "eax, 0", b"\x00" * 4)]

        self.assertFalse(whole_image.aligned(original, rebuilt, [], []))

    def test_aligned_rejects_a_different_slot_offset(self) -> None:
        original = [insn(0x300EC, "mov", "eax, 0", b"\x00" * 8)]
        rebuilt = [insn(0x10000, "mov", "eax, 0", b"\x00" * 8)]
        original_relocations = [
            le.Relocation(
                offset=0x300EE, width=4, kind="internal", object=1, target=0x20
            )
        ]
        rebuilt_relocations = [
            le.Relocation(offset=0x10003, width=4, kind="internal")
        ]

        self.assertFalse(
            whole_image.aligned(
                original, rebuilt, original_relocations, rebuilt_relocations
            )
        )

    def test_aligned_rejects_a_non_internal_slot(self) -> None:
        original = [insn(0x10000, "mov", "eax, 0", b"\x00" * 4)]
        rebuilt = [insn(0x20000, "mov", "eax, 0", b"\x00" * 4)]
        original_relocations = [
            le.Relocation(
                offset=0x10000, width=4, kind="internal", object=1, target=0x20
            )
        ]
        rebuilt_relocations = [
            le.Relocation(offset=0x20000, width=4, kind="import-ordinal", module=1, ordinal=1)
        ]

        self.assertFalse(
            whole_image.aligned(
                original, rebuilt, original_relocations, rebuilt_relocations
            )
        )


class RelocateTestCase(unittest.TestCase):
    def test_relocate_copies_the_original_slot_bytes(self) -> None:
        original = [insn(0x300EC, "mov", "eax, 0", b"\xaa\xbb\x20\x00\x00\x00")]
        rebuilt = [insn(0x10000, "mov", "eax, 0", b"\x00\x00\x00\x00\x00\x00")]
        original_relocations = [
            le.Relocation(
                offset=0x300EE, width=4, kind="internal", object=1, target=0x20
            )
        ]
        rebuilt_relocations = [
            le.Relocation(offset=0x10002, width=4, kind="internal")
        ]

        body, relocated = whole_image.relocate(
            original,
            rebuilt,
            original_relocations,
            rebuilt_relocations,
        )

        self.assertEqual(body, b"\x00\x00\x20\x00\x00\x00")
        self.assertEqual(relocated, 1)

    def test_relocate_counts_each_slot(self) -> None:
        original = [insn(0x300EC, "mov", "eax, 0", b"\x01" * 8)]
        rebuilt = [insn(0x10000, "mov", "eax, 0", b"\x00" * 8)]
        original_relocations = [
            le.Relocation(
                offset=0x300EC, width=4, kind="internal", object=1, target=0x10
            ),
            le.Relocation(
                offset=0x300F0, width=4, kind="internal", object=1, target=0x20
            ),
        ]
        rebuilt_relocations = [
            le.Relocation(offset=0x10000, width=4, kind="internal"),
            le.Relocation(offset=0x10004, width=4, kind="internal"),
        ]

        _, relocated = whole_image.relocate(
            original,
            rebuilt,
            original_relocations,
            rebuilt_relocations,
        )

        self.assertEqual(relocated, 2)

    def test_relocate_returns_none_on_a_different_instruction_count(self) -> None:
        original = [
            insn(0x300EC, "mov", "eax, 0", b"\x00" * 4),
            insn(0x300F0, "ret", "", b"\xc3\x90\x90\x90"),
        ]
        rebuilt = [insn(0x10000, "mov", "eax, 0", b"\x00" * 4)]

        self.assertEqual(
            whole_image.relocate(original, rebuilt, [], []), (None, 0)
        )

    def test_relocate_returns_none_on_a_size_mismatch(self) -> None:
        original = [insn(0x300EC, "mov", "eax, 0", b"\x00" * 5)]
        rebuilt = [insn(0x10000, "mov", "eax, 0", b"\x00" * 4)]

        self.assertEqual(
            whole_image.relocate(original, rebuilt, [], []), (None, 0)
        )

    def test_relocate_returns_none_on_a_slot_count_mismatch(self) -> None:
        original = [insn(0x300EC, "mov", "eax, 0", b"\x00" * 4)]
        rebuilt = [insn(0x10000, "mov", "eax, 0", b"\x00" * 4)]
        original_relocations = [
            le.Relocation(
                offset=0x300EC, width=4, kind="internal", object=1, target=0x20
            )
        ]

        self.assertEqual(
            whole_image.relocate(
                original, rebuilt, original_relocations, []
            ),
            (None, 0),
        )

    def test_relocate_returns_none_on_a_non_internal_slot(self) -> None:
        original = [insn(0x10000, "mov", "eax, 0", b"\x00" * 4)]
        rebuilt = [insn(0x20000, "mov", "eax, 0", b"\x00" * 4)]
        internal = le.Relocation(
            offset=0x10000, width=4, kind="internal", object=1, target=0x20
        )
        imported = le.Relocation(
            offset=0x10000, width=4, kind="import-ordinal", module=1, ordinal=1
        )
        rebuilt_internal = le.Relocation(
            offset=0x20000, width=4, kind="internal"
        )
        rebuilt_imported = le.Relocation(
            offset=0x20000, width=4, kind="import-ordinal", module=1, ordinal=1
        )

        self.assertEqual(
            whole_image.relocate(
                original,
                rebuilt,
                [imported],
                [rebuilt_internal],
            ),
            (None, 0),
        )
        self.assertEqual(
            whole_image.relocate(
                original,
                rebuilt,
                [internal],
                [rebuilt_imported],
            ),
            (None, 0),
        )

    def test_relocate_returns_none_on_a_non_four_byte_slot(self) -> None:
        original = [insn(0x10000, "mov", "eax, 0", b"\x00" * 4)]
        rebuilt = [insn(0x20000, "mov", "eax, 0", b"\x00" * 4)]
        short_original = le.Relocation(
            offset=0x10000, width=2, kind="internal", object=1, target=0x20
        )
        long_original = le.Relocation(
            offset=0x10000, width=4, kind="internal", object=1, target=0x20
        )
        short_rebuilt = le.Relocation(offset=0x20000, width=2, kind="internal")
        long_rebuilt = le.Relocation(offset=0x20000, width=4, kind="internal")

        self.assertEqual(
            whole_image.relocate(
                original,
                rebuilt,
                [short_original],
                [long_rebuilt],
            ),
            (None, 0),
        )
        self.assertEqual(
            whole_image.relocate(
                original,
                rebuilt,
                [long_original],
                [short_rebuilt],
            ),
            (None, 0),
        )

    def test_relocate_returns_none_on_a_different_slot_offset(self) -> None:
        original = [insn(0x300EC, "mov", "eax, 0", b"\x00" * 8)]
        rebuilt = [insn(0x10000, "mov", "eax, 0", b"\x00" * 8)]
        original_relocations = [
            le.Relocation(
                offset=0x300EE, width=4, kind="internal", object=1, target=0x20
            )
        ]
        rebuilt_relocations = [
            le.Relocation(offset=0x10003, width=4, kind="internal")
        ]

        self.assertEqual(
            whole_image.relocate(
                original,
                rebuilt,
                original_relocations,
                rebuilt_relocations,
            ),
            (None, 0),
        )

    def test_relocate_returns_none_on_an_unassigned_slot(self) -> None:
        original = [insn(0x10000, "mov", "eax, 0", b"\x00" * 4)]
        rebuilt = [insn(0x20000, "mov", "eax, 0", b"\x00" * 4)]
        original_relocations = [
            le.Relocation(
                offset=0x10004, width=4, kind="internal", object=1, target=0x20
            )
        ]

        self.assertEqual(
            whole_image.relocate(
                original, rebuilt, original_relocations, []
            ),
            (None, 0),
        )

    def test_relocate_keeps_a_relative_branch(self) -> None:
        original = [insn(0x300EC, "jne", "0x300fe", b"\x75\x10")]
        rebuilt = [insn(0x10000, "jne", "0x10012", b"\x75\x10")]

        body, relocated = whole_image.relocate(original, rebuilt, [], [])

        self.assertEqual(body, b"\x75\x10")
        self.assertEqual(relocated, 0)


class PatchModeTestCase(unittest.TestCase):
    def test_patch_mode_writes_an_aligned_span(self) -> None:
        original = [insn(0x10000, "mov", "eax, 0", b"\x01" * 4)]
        rebuilt = [insn(0x10000, "mov", "eax, 0", b"\x00" * 4)]

        mode = whole_image.patch_mode(
            record("0x10000", "free", "free_", 4), original, rebuilt, [], []
        )

        self.assertEqual(mode, "write")

    def test_patch_mode_reuses_a_strict_padded_body(self) -> None:
        original = [insn(0x10000, "mov", "eax, 0", b"\x01" * 9)]
        rebuilt = [insn(0x20000, "mov", "eax, 0", b"\x00" * 9)]
        original_relocations = [
            le.Relocation(
                offset=0x10000, width=4, kind="internal", object=1, target=0x20
            )
        ]
        rebuilt_relocations = [
            le.Relocation(offset=0x20000, width=4, kind="internal")
        ]

        mode = whole_image.patch_mode(
            record("0x10000", "free", "free_", 12, match="exact-relocations"),
            original,
            rebuilt,
            original_relocations,
            rebuilt_relocations,
        )

        self.assertEqual(mode, "reuse")

    def test_patch_mode_rejects_a_non_strict_padded_body(self) -> None:
        original = [insn(0x10000, "mov", "eax, 0", b"\x01" * 9)]
        rebuilt = [insn(0x20000, "mov", "eax, 0", b"\x00" * 9)]
        original_relocations = [
            le.Relocation(
                offset=0x10000, width=4, kind="internal", object=1, target=0x20
            )
        ]
        rebuilt_relocations = [
            le.Relocation(offset=0x20000, width=4, kind="internal")
        ]

        mode = whole_image.patch_mode(
            record("0x10000", "free", "free_", 12, match="same-shape"),
            original,
            rebuilt,
            original_relocations,
            rebuilt_relocations,
        )

        self.assertIsNone(mode)

    def test_patch_mode_rejects_a_padded_body_that_is_not_aligned(self) -> None:
        original = [insn(0x10000, "mov", "eax, 0", b"\x01" * 9)]
        rebuilt = [insn(0x20000, "mov", "eax, 0", b"\x00" * 9)]
        original_relocations = [
            le.Relocation(
                offset=0x10000, width=4, kind="internal", object=1, target=0x20
            )
        ]
        rebuilt_relocations = [
            le.Relocation(offset=0x20001, width=4, kind="internal")
        ]

        mode = whole_image.patch_mode(
            record("0x10000", "free", "free_", 12, match="exact-relocations"),
            original,
            rebuilt,
            original_relocations,
            rebuilt_relocations,
        )

        self.assertIsNone(mode)

    def test_patch_mode_rejects_a_body_larger_than_the_span(self) -> None:
        original = [insn(0x10000, "mov", "eax, 0", b"\x01" * 9)]
        rebuilt = [insn(0x20000, "mov", "eax, 0", b"\x00" * 9)]

        mode = whole_image.patch_mode(
            record("0x10000", "free", "free_", 8, match="exact-bytes"),
            original,
            rebuilt,
            [],
            [],
        )

        self.assertIsNone(mode)


class FakeObject:
    def __init__(self, index: int, reloc_base: int) -> None:
        self.index = index
        self.reloc_base = reloc_base


class FakeImage:
    def __init__(
        self,
        data: bytes = b"",
        objects: tuple[FakeObject, ...] = (),
        relocations: tuple[le.Relocation, ...] = (),
        file_offset: int = 0,
    ) -> None:
        self.data = data
        self.objects = list(objects)
        self._relocations = list(relocations)
        self._file_offset = file_offset

    def relocations(self) -> list[le.Relocation]:
        return list(self._relocations)

    def object_by_index(self, index: int) -> FakeObject | None:
        for obj in self.objects:
            if obj.index == index:
                return obj
        return None

    def to_file_offset(self, address: int) -> int | None:
        return self._file_offset


class BuildTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.original_path = self.root / "STARTH.EXE"
        self.original_path.write_bytes(bytes(16))
        self.out = self.root / "STARTH-rebuilt.EXE"
        self.work_dir = self.root / "work"
        self.original = FakeImage(bytes(16), objects=(FakeObject(1, 0x10000),))

    def score(self, equal: int = 0, total: int = 16) -> object:
        return whole_image.image_compare.ImageResult(
            exact=False, equal=equal, total=total
        )

    def invoke_build(
        self,
        modules: list[dict],
        records: list[dict],
        loads: list[object],
        verdicts: list[tuple[str, str]] | None = None,
        rebuilt_instructions: list[le.Insn] | None = None,
        original_instructions: list[le.Insn] | None = None,
        score: object | None = None,
        selected: list[str] | None = None,
    ) -> dict:
        if verdicts is None:
            verdicts = [("pass", "")]
        if rebuilt_instructions is None:
            rebuilt_instructions = []
        if original_instructions is None:
            original_instructions = []
        if score is None:
            score = self.score()
        with mock.patch.object(
            whole_image.le, "load", side_effect=loads
        ), mock.patch.object(
            whole_image.run, "build_module", side_effect=verdicts
        ), mock.patch.object(
            whole_image.diff_module, "map_address", return_value=(1, 0)
        ), mock.patch.object(
            whole_image.diff_module, "map_symbols", return_value=[]
        ), mock.patch.object(
            whole_image.diff_module,
            "rebuilt_body",
            return_value=rebuilt_instructions,
        ), mock.patch.object(
            whole_image.diff_module,
            "original_body",
            return_value=original_instructions,
        ), mock.patch.object(
            whole_image.image_compare, "compare_loaded", return_value=score
        ):
            return whole_image.build(
                self.original_path,
                modules,
                records,
                self.work_dir,
                self.out,
                selected,
            )

    def test_build_writes_the_patched_body(self) -> None:
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 4)]
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 1", b"\xaa\xbb\xcc\xdd")
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 1", b"\x01\x02\x03\x04")
            ],
        )

        row = document["modules"][0]
        self.assertEqual(row["mode"], "write")
        self.assertEqual(row["outcome"], "patched")
        self.assertEqual(
            self.out.read_bytes(), b"\xaa\xbb\xcc\xdd" + bytes(12)
        )

    def test_build_reuses_a_padded_body(self) -> None:
        self.original = FakeImage(
            b"\x01\x02\x03\x04" + bytes(12),
            objects=(FakeObject(1, 0x10000),),
        )
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 8, match="exact-bytes")]

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\xaa\xbb\xcc\xdd")
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x01\x02\x03\x04")
            ],
        )

        row = document["modules"][0]
        self.assertFalse(row["fit"])
        self.assertIsNone(row["fit_reason"])
        self.assertEqual(row["relocated"], 0)
        self.assertEqual(row["mode"], "reuse")
        self.assertEqual(row["outcome"], "reused")
        self.assertNotIn("reason", row)
        self.assertEqual(self.out.read_bytes()[:4], b"\x01\x02\x03\x04")

    def test_build_keeps_the_padding_tail(self) -> None:
        self.original = FakeImage(
            b"\x01\x02\x03\x04\x09\x08\x07\x06" + bytes(8),
            objects=(FakeObject(1, 0x10000),),
        )
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 8, match="exact-bytes")]

        self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\xaa\xbb\xcc\xdd")
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x01\x02\x03\x04")
            ],
        )

        self.assertEqual(self.out.read_bytes()[4:8], b"\x09\x08\x07\x06")

    def test_build_reuses_a_body_with_a_far_original_slot(self) -> None:
        self.original = FakeImage(
            b"\x01\x02\x03\x04" + bytes(12),
            objects=(FakeObject(1, 0x10000),),
            relocations=(
                le.Relocation(
                    offset=0x10050,
                    width=4,
                    kind="internal",
                    object=1,
                    target=0x20,
                ),
            ),
        )
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 100, match="exact-bytes")]

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\xaa\xbb\xcc\xdd")
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x01\x02\x03\x04")
            ],
        )

        row = document["modules"][0]
        self.assertEqual(row["mode"], "reuse")
        self.assertEqual(row["outcome"], "reused")

    def test_build_rejects_a_reuse_for_a_non_strict_record(self) -> None:
        self.original = FakeImage(
            b"\x01\x02\x03\x04" + bytes(12),
            objects=(FakeObject(1, 0x10000),),
        )
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 8, match="same-shape")]

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\xaa\xbb\xcc\xdd")
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x01\x02\x03\x04")
            ],
        )

        row = document["modules"][0]
        self.assertEqual(row["outcome"], "skipped")
        self.assertFalse(row["fit"])
        self.assertEqual(
            row["fit_reason"],
            "the rebuilt body holds 4 bytes against the recorded 8",
        )

    def test_build_report_holds_the_reused_count(self) -> None:
        self.original = FakeImage(
            b"\x01\x02\x03\x04" + bytes(12),
            objects=(FakeObject(1, 0x10000),),
        )
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 8, match="exact-bytes")]

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\xaa\xbb\xcc\xdd")
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x01\x02\x03\x04")
            ],
        )

        self.assertEqual(document["patched"], 0)
        self.assertEqual(document["reused"], 1)
        self.assertEqual(document["skipped"], 0)
        self.assertEqual(document["relocated"], 0)

    def test_build_copies_the_original_slot_bytes(self) -> None:
        self.original = FakeImage(
            b"\x01\x02\x20\x00\x00\x00" + bytes(10),
            objects=(FakeObject(1, 0x10000),),
            relocations=(
                le.Relocation(
                    offset=0x10002,
                    width=4,
                    kind="internal",
                    object=1,
                    target=0x20,
                ),
            ),
        )
        rebuilt_image = FakeImage(
            b"",
            objects=(FakeObject(1, 0x10000),),
            relocations=(
                le.Relocation(offset=0x10002, width=4, kind="internal"),
            ),
        )
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 6)]

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 6)
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x01\x02\x20\x00\x00\x00")
            ],
        )

        self.assertEqual(
            self.out.read_bytes()[:6], b"\x00\x00\x20\x00\x00\x00"
        )
        self.assertEqual(document["modules"][0]["mode"], "write")
        self.assertEqual(document["modules"][0]["relocated"], 1)

    def test_build_skips_a_failed_build(self) -> None:
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 4)]

        document = self.invoke_build(
            modules,
            records,
            [self.original, FakeImage(b"")],
            verdicts=[("fail", "boom")],
        )

        row = document["modules"][0]
        self.assertEqual(row["build"], "fail")
        self.assertEqual(row["outcome"], "skipped")
        self.assertEqual(row["reason"], "boom")
        self.assertEqual(document["patched"], 0)
        self.assertEqual(document["skipped"], 1)

    def test_build_keeps_the_original_bytes_on_a_failed_build(self) -> None:
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 4)]

        self.invoke_build(
            modules,
            records,
            [self.original, FakeImage(b"")],
            verdicts=[("fail", "boom")],
        )

        self.assertEqual(self.out.read_bytes(), self.original.data)

    def test_build_skips_a_size_mismatch(self) -> None:
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 5)]
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
        )

        row = document["modules"][0]
        reason = "the rebuilt body holds 4 bytes against the recorded 5"
        self.assertFalse(row["fit"])
        self.assertEqual(row["fit_reason"], reason)
        self.assertEqual(row["reason"], reason)
        self.assertEqual(row["outcome"], "skipped")
        self.assertEqual(row["rebuilt_size"], 4)

    def test_build_skips_a_shape_misalignment(self) -> None:
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 8)]
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4),
                insn(0x10004, "mov", "ebx, 0", b"\x00" * 4),
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 8)
            ],
        )

        row = document["modules"][0]
        self.assertTrue(row["fit"])
        self.assertEqual(
            row["reason"],
            "the rebuilt body does not align with the original body",
        )

    def test_build_selected_restricts_the_modules(self) -> None:
        modules = [
            module("free", ["free.c"], "v2"),
            module("exit", ["exit.c"], "v2"),
        ]
        records = [
            record("0x10000", "free", "free_", 4),
            record("0x20000", "exit", "exit_", 4),
        ]
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
            selected=["free"],
        )

        self.assertEqual(len(document["modules"]), 1)
        self.assertEqual(document["modules"][0]["module"], "free")

    def test_build_skips_a_library_backed_record(self) -> None:
        modules = [module("free", ["free.c"], "v2")]
        backing = record("0x10000", "free", "free_", 4)
        backing["library"] = {"file": "CLIB3R.LIB"}
        records = [backing]

        document = self.invoke_build(
            modules,
            records,
            [self.original, FakeImage(b"")],
        )

        row = document["modules"][0]
        self.assertEqual(row["build"], "skip")
        self.assertEqual(row["reason"], "the original library bytes stay")
        self.assertEqual(row["outcome"], "skipped")
        self.assertEqual(document["patched"], 0)
        self.assertEqual(document["skipped"], 1)
        self.assertEqual(self.out.read_bytes(), self.original.data)

    def test_build_skips_a_per_module_error_and_continues(self) -> None:
        modules = [
            module("free", ["free.c"], "v2"),
            module("exit", ["exit.c"], "v2"),
        ]
        records = [
            record("0x10000", "free", "free_", 4),
            record("0x20000", "exit", "exit_", 4),
        ]
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            verdicts=[OSError("boom"), ("pass", "")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
        )

        skipped = document["modules"][0]
        patched = document["modules"][1]
        self.assertEqual(skipped["outcome"], "skipped")
        self.assertEqual(skipped["reason"], "boom")
        self.assertEqual(patched["outcome"], "patched")
        self.assertEqual(document["patched"], 1)
        self.assertEqual(document["skipped"], 1)

    def test_build_skips_a_non_pass_verdict_with_no_output(self) -> None:
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 4)]

        document = self.invoke_build(
            modules,
            records,
            [self.original, FakeImage(b"")],
            verdicts=[("skip", "")],
        )

        row = document["modules"][0]
        self.assertEqual(row["build"], "skip")
        self.assertEqual(row["reason"], "the v2 engine is missing")
        self.assertEqual(row["outcome"], "skipped")

    def test_build_skips_a_file_offset_miss(self) -> None:
        self.original = FakeImage(
            bytes(16),
            objects=(FakeObject(1, 0x10000),),
            file_offset=None,
        )
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 4)]
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
        )

        row = document["modules"][0]
        self.assertEqual(row["outcome"], "skipped")
        self.assertEqual(
            row["reason"], "the original address is not in a file page"
        )

    def test_build_rejects_a_selection_with_no_item(self) -> None:
        modules = [module("driver", ["driver.c"], "v2")]
        records = [record("0x10000", "driver", None, 4)]

        with self.assertRaises(ValueError):
            self.invoke_build(
                modules,
                records,
                [self.original, FakeImage(b"")],
                selected=["driver"],
            )

    def test_build_report_holds_the_date_and_the_digests(self) -> None:
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 4)]
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
        )

        self.assertEqual(document["date"], date.today().isoformat())
        self.assertEqual(document["original"]["path"], str(self.original_path))
        self.assertEqual(
            document["original"]["sha256"],
            whole_image._sha256(self.original_path),
        )
        self.assertEqual(document["image"]["path"], str(self.out))
        self.assertEqual(
            document["image"]["sha256"],
            hashlib.sha256(self.out.read_bytes()).hexdigest(),
        )

    def test_build_report_holds_the_score(self) -> None:
        modules = [module("free", ["free.c"], "v2")]
        records = [record("0x10000", "free", "free_", 4)]
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
            score=self.score(equal=10, total=16),
        )

        self.assertEqual(
            document["score"], {"equal": 10, "total": 16, "exact": False}
        )

    def test_build_report_holds_the_module_rows(self) -> None:
        modules = [
            module("free", ["free.c"], "v2"),
            module("exit", ["exit.c"], "v2"),
        ]
        records = [
            record("0x10000", "free", "free_", 4),
            record("0x20000", "exit", "exit_", 4),
        ]
        rebuilt_image = FakeImage(b"", objects=(FakeObject(1, 0x10000),))

        document = self.invoke_build(
            modules,
            records,
            [self.original, rebuilt_image, FakeImage(b"")],
            verdicts=[("pass", ""), ("fail", "boom")],
            rebuilt_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
            original_instructions=[
                insn(0x10000, "mov", "eax, 0", b"\x00" * 4)
            ],
        )

        patched = document["modules"][0]
        skipped = document["modules"][1]
        base = {
            "module",
            "address",
            "size",
            "toolchain",
            "sources",
            "build",
            "fit",
            "fit_reason",
            "rebuilt_size",
            "original_size",
            "relocated",
            "mode",
            "outcome",
        }
        self.assertEqual(set(patched), base)
        self.assertNotIn("reason", patched)
        self.assertEqual(set(skipped), base | {"reason"})

    def test_build_rejects_the_original_as_the_output(self) -> None:
        with mock.patch.object(whole_image.le, "load") as load:
            with self.assertRaises(ValueError):
                whole_image.build(
                    self.original_path,
                    [],
                    [],
                    self.work_dir,
                    self.original_path,
                )
            load.assert_not_called()


class Sha256TestCase(unittest.TestCase):
    def test_sha256_raises_for_a_missing_file(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing.bin"

            with self.assertRaises(ValueError):
                whole_image._sha256(missing)


class CommandBuildTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.original = self.root / "STARTH.EXE"
        self.original.write_bytes(b"MZ")
        self.out = self.root / "STARTH-rebuilt.EXE"
        self.report = self.root / "whole-image.json"
        self.modules = [module("free", ["free.c"], "v2")]

    def arguments(self, **overrides: object) -> argparse.Namespace:
        values: dict[str, object] = {
            "original": self.original,
            "out": self.out,
            "report": self.report,
            "module": None,
            "all": False,
        }
        values.update(overrides)
        return argparse.Namespace(**values)

    def patched_row(self) -> dict:
        return {
            "module": "free",
            "address": "0x10000",
            "size": 4,
            "toolchain": "v2",
            "sources": ["free.c"],
            "build": "pass",
            "fit": True,
            "fit_reason": None,
            "rebuilt_size": 4,
            "original_size": 4,
            "relocated": 2,
            "mode": "write",
            "outcome": "patched",
        }

    def reused_row(self) -> dict:
        return {
            "module": "error_report",
            "address": "0x30000",
            "size": 8,
            "toolchain": "v2",
            "sources": ["error_report.c"],
            "build": "pass",
            "fit": False,
            "fit_reason": None,
            "rebuilt_size": 4,
            "original_size": 4,
            "relocated": 0,
            "mode": "reuse",
            "outcome": "reused",
        }

    def skipped_row(self) -> dict:
        return {
            "module": "exit",
            "address": "0x20000",
            "size": 4,
            "toolchain": "v2",
            "sources": ["exit.c"],
            "build": "fail",
            "fit": False,
            "fit_reason": None,
            "rebuilt_size": 0,
            "original_size": 0,
            "relocated": 0,
            "mode": None,
            "outcome": "skipped",
            "reason": "boom",
        }

    def document(self) -> dict:
        return {
            "date": date.today().isoformat(),
            "original": {"path": str(self.original), "sha256": "digest"},
            "image": {"path": str(self.out), "sha256": "image-digest"},
            "modules": [
                self.patched_row(),
                self.reused_row(),
                self.skipped_row(),
            ],
            "patched": 1,
            "reused": 1,
            "skipped": 1,
            "relocated": 2,
            "score": {"equal": 10, "total": 16, "exact": False},
        }

    def invoke(
        self, arguments: argparse.Namespace, document: dict
    ) -> tuple[int, str]:
        standard_out = io.StringIO()
        with mock.patch.object(
            whole_image, "build", return_value=document
        ) as build, mock.patch.object(
            whole_image, "_load_modules", return_value=self.modules
        ), mock.patch.object(
            whole_image, "_load_records", return_value=[]
        ), mock.patch.object(
            whole_image.le, "load", return_value=object()
        ), contextlib.redirect_stdout(standard_out):
            code = whole_image.command_build(arguments)
        self.build = build
        return code, standard_out.getvalue()

    def test_command_build_writes_the_image_and_the_report(self) -> None:
        document = self.document()
        arguments = self.arguments()

        code, _ = self.invoke(arguments, document)

        self.assertEqual(code, 0)
        self.assertEqual(
            json.loads(self.report.read_text()), document
        )
        self.build.assert_called_once()
        self.assertEqual(self.build.call_args.args[4], arguments.out)

    def test_command_build_prints_a_summary(self) -> None:
        arguments = self.arguments()

        _, output = self.invoke(arguments, self.document())

        self.assertIn("free: patched, 2 slots relocated", output)
        self.assertIn(
            "error_report: reused, the original body bytes stay", output
        )
        self.assertIn("exit: skipped, boom", output)
        self.assertIn(
            "1 patched, 1 reused, 1 skipped, 2 slots relocated, "
            "10 of 16 bytes equal",
            output,
        )

    def test_command_build_rejects_a_missing_original(self) -> None:
        missing = self.root / "missing.exe"
        with mock.patch.object(whole_image, "build") as build, mock.patch.object(
            whole_image, "_load_modules", return_value=self.modules
        ), mock.patch.object(
            whole_image, "_load_records", return_value=[]
        ), contextlib.redirect_stderr(io.StringIO()):
            code = whole_image.command_build(
                self.arguments(original=missing)
            )
        self.assertEqual(code, 2)
        build.assert_not_called()

    def test_command_build_rejects_an_unknown_module(self) -> None:
        with mock.patch.object(whole_image, "build") as build, mock.patch.object(
            whole_image, "_load_modules", return_value=self.modules
        ), mock.patch.object(
            whole_image, "_load_records", return_value=[]
        ), mock.patch.object(
            whole_image.le, "load", return_value=object()
        ), contextlib.redirect_stderr(io.StringIO()):
            code = whole_image.command_build(
                self.arguments(module=["missing"])
            )
        self.assertEqual(code, 2)
        build.assert_not_called()

    def test_parse_arguments_selects_all_by_default(self) -> None:
        with mock.patch.object(sys, "argv", ["whole_image.py"]):
            parsed = whole_image.parse_arguments()
        self.assertIsNone(parsed.module)
        self.assertFalse(parsed.all)
        with mock.patch.object(sys, "argv", ["whole_image.py", "--all"]):
            parsed = whole_image.parse_arguments()
        self.assertTrue(parsed.all)

    def test_module_and_all_are_mutually_exclusive(self) -> None:
        argv = ["whole_image.py", "--module", "free", "--all"]
        with mock.patch.object(sys, "argv", argv), contextlib.redirect_stderr(
            io.StringIO()
        ):
            with self.assertRaises(SystemExit) as raised:
                whole_image.parse_arguments()
        self.assertEqual(raised.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
