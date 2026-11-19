"""Tests for the first-difference report of the non-strict module records.

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

import first_difference  # noqa: E402
import le  # noqa: E402
import oracle  # noqa: E402
import store  # noqa: E402

CATALOGUE = Path(__file__).resolve().parents[3] / "docs/reference/source-forms.md"


def insn(address: int, mnemonic: str, operands: str, raw: bytes) -> le.Insn:
    return le.Insn(address, len(raw), mnemonic, operands, raw)


def record(
    address: str = "0x10978",
    module: str | None = "error_report",
    symbol: str | None = "error_report_",
    match: str | None = "same-shape",
    size: int | None = 12,
) -> dict:
    identity = {} if match is None else {"match": match}
    item = {
        "address": address,
        "module": module,
        "symbol": symbol,
        "result": {"checks": {"identity": identity}},
    }
    if size is not None:
        item["size"] = size
    return item


def module(name: str) -> dict:
    return {
        "name": name,
        "exe": f"{name}.exe",
        "map": f"{name}.map",
        "toolchain": "v2",
        "sources": [{"file": f"{name}.c", "compiler": "wcc386"}],
    }


def result(
    differences: list[str],
    match: str = "same-shape",
    bytes_equal: int = 0,
    bytes_total: int = 0,
    instructions_equal: int = 0,
    instructions_total: int = 0,
) -> oracle.OracleResult:
    return oracle.OracleResult(
        match=match,
        bytes_equal=bytes_equal,
        bytes_total=bytes_total,
        instructions_equal=instructions_equal,
        instructions_total=instructions_total,
        differences=list(differences),
    )


def row_for_module(
    record: dict, module: dict, original_path: Path, work_dir: Path
) -> dict:
    return {
        "module": module["name"],
        "address": record["address"],
        "symbol": record["symbol"],
        "toolchain": module.get("toolchain", "v2"),
        "sources": module.get("sources", []),
        "build": "pass",
        "match": "same-shape",
        "instructions_equal": 0,
        "instructions_total": 0,
        "bytes_equal": 0,
        "bytes_total": 0,
        "first_difference": None,
        "sequence_index": None,
        "idiom": "other",
        "original": None,
        "rebuilt": None,
        "record_size": record.get("size"),
        "original_size": 0,
        "rebuilt_size": 0,
        "family": "padding",
    }


class IdiomTestCase(unittest.TestCase):
    def test_idioms_hold_the_eleven_names(self) -> None:
        self.assertEqual(
            first_difference.IDIOMS,
            (
                "local slot order",
                "constant operand",
                "instruction encoding",
                "instruction selection",
                "basic-block order",
                "relocation target",
                "register save order",
                "frame size",
                "instruction count",
                "match",
                "other",
            ),
        )

    def test_idiom_of_classifies_a_local_slot_byte(self) -> None:
        original = insn(
            0x1000, "mov", "dword ptr [ebp - 8], eax", b"\x89\x45\xf8"
        )
        rebuilt = insn(
            0x1000, "mov", "dword ptr [ebp - 0xc], eax", b"\x89\x45\xf4"
        )
        self.assertEqual(
            first_difference.idiom_of(
                "byte 2: 0xf8 against 0xf4", original, rebuilt
            ),
            "local slot order",
        )

    def test_idiom_of_requires_two_stack_slots_for_a_local_slot_byte(
        self,
    ) -> None:
        original = insn(
            0x1000, "mov", "dword ptr [ebp - 8], eax", b"\x89\x45\xf8"
        )
        rebuilt = insn(0x1000, "mov", "dword ptr [ebx], eax", b"\x89\x03")
        self.assertEqual(
            first_difference.idiom_of(
                "byte 2: 0xf8 against 0xf4", original, rebuilt
            ),
            "other",
        )

    def test_idiom_of_classifies_a_register_save_order(self) -> None:
        original = insn(0x1000, "push", "ebx", b"\x53")
        rebuilt = insn(0x1000, "push", "edx", b"\x52")
        self.assertEqual(
            first_difference.idiom_of(
                "byte 0: 0x53 against 0x52", original, rebuilt
            ),
            "register save order",
        )

    def test_idiom_of_requires_a_push_pair_for_a_register_save_order(
        self,
    ) -> None:
        original = insn(0x1000, "mov", "al, 1", b"\xb0\x01")
        rebuilt = insn(0x1000, "mov", "bl, 1", b"\xb3\x01")
        self.assertEqual(
            first_difference.idiom_of(
                "byte 0: 0xb0 against 0xb3", original, rebuilt
            ),
            "other",
        )

    def test_idiom_of_classifies_a_frame_size(self) -> None:
        original = insn(0x1000, "sub", "esp, 0xc", b"\x83\xec\x0c")
        rebuilt = insn(0x1000, "sub", "esp, 0x8", b"\x83\xec\x08")
        self.assertEqual(
            first_difference.idiom_of(
                "byte 2: 0x0c against 0x08", original, rebuilt
            ),
            "frame size",
        )

    def test_idiom_of_requires_a_sub_esp_pair_for_a_frame_size(self) -> None:
        original = insn(
            0x1000, "sub", "dword ptr [ebp - 4], 0x10", b"\x83\x6d\xfc\x10"
        )
        rebuilt = insn(
            0x1000, "sub", "dword ptr [ebp - 4], 0x08", b"\x83\x6d\xfc\x08"
        )
        self.assertEqual(
            first_difference.idiom_of(
                "byte 2: 0xfc against 0xfc", original, rebuilt
            ),
            "other",
        )

    def test_idiom_of_classifies_an_instruction_count(self) -> None:
        original = insn(0x1000, "nop", "", b"\x90")
        rebuilt = insn(0x1000, "nop", "", b"\x90")
        self.assertEqual(
            first_difference.idiom_of(
                "instruction count: 3 against 2", original, rebuilt
            ),
            "instruction count",
        )

    def test_idiom_of_classifies_a_mask(self) -> None:
        original = insn(0x1000, "push", "0x7fe8", b"\x68\xe8\x7f")
        rebuilt = insn(0x1000, "push", "0x7fe8", b"\x68\xe8\x7f")
        self.assertEqual(
            first_difference.idiom_of(
                "mask: [1, 2, 3, 4] against []", original, rebuilt
            ),
            "constant operand",
        )

    def test_idiom_of_classifies_a_size(self) -> None:
        original = insn(0x1000, "mov", "ds, word ptr [ebp + 0xc]", b"\x8e\x5d\x0c")
        rebuilt = insn(
            0x1000, "mov", "ds, word ptr [ebp + 0xc]", b"\x66\x8e\x5d\x0c"
        )
        self.assertEqual(
            first_difference.idiom_of("size: 3 against 4", original, rebuilt),
            "instruction encoding",
        )

    def test_idiom_of_classifies_a_mnemonic(self) -> None:
        original = insn(0x1000, "mov", "eax, ebx", b"\x89\xd8")
        rebuilt = insn(0x1000, "lea", "eax, [ebx]", b"\x8d\x03")
        self.assertEqual(
            first_difference.idiom_of(
                "mnemonic: 'mov' against 'lea'", original, rebuilt
            ),
            "instruction selection",
        )

    def test_idiom_of_classifies_a_branch_target(self) -> None:
        original = insn(0x1000, "jne", "0x1010", b"\x75\x0e")
        rebuilt = insn(0x1000, "jne", "0x1014", b"\x75\x12")
        self.assertEqual(
            first_difference.idiom_of(
                "branch target: instruction 3 against 4", original, rebuilt
            ),
            "basic-block order",
        )

    def test_idiom_of_classifies_a_relocation(self) -> None:
        original = insn(0x1000, "push", "0x7fe8", b"\x68\xe8\x7f\x00\x00")
        rebuilt = insn(0x1000, "push", "0x7fe8", b"\x68\xe8\x7f\x00\x00")
        for difference in (
            "relocations: 1 against 0",
            "relocation offset: 1 against 2",
            "relocation width: 4 against 2",
            "relocation target: ('internal',) against ('internal', 'x')",
        ):
            with self.subTest(difference=difference):
                self.assertEqual(
                    first_difference.idiom_of(difference, original, rebuilt),
                    "relocation target",
                )

    def test_idiom_of_classifies_no_difference(self) -> None:
        original = insn(0x1000, "ret", "", b"\xc3")
        rebuilt = insn(0x1000, "ret", "", b"\xc3")
        self.assertEqual(
            first_difference.idiom_of("", original, rebuilt), "match"
        )

    def test_idiom_of_classifies_an_unknown_difference(self) -> None:
        original = insn(0x1000, "mov", "eax, ebx", b"\x89\xd8")
        rebuilt = insn(0x1000, "mov", "eax, ecx", b"\x89\xc8")
        self.assertEqual(
            first_difference.idiom_of(
                "byte 1: 0xd8 against 0xc8", original, rebuilt
            ),
            "other",
        )

    def test_idiom_of_ignores_the_instruction_prefix(self) -> None:
        original = insn(0x1000, "push", "0x7fe8", b"\x68\xe8\x7f")
        rebuilt = insn(0x1000, "push", "0x7fe8", b"\x68\xe8\x7f")
        self.assertEqual(
            first_difference.idiom_of(
                "instruction 23: mask: [1, 2, 3, 4] against []",
                original,
                rebuilt,
            ),
            "constant operand",
        )


class FirstPairTestCase(unittest.TestCase):
    def body(self) -> list[le.Insn]:
        return [
            insn(0x1000, "nop", "", b"\x90"),
            insn(0x1001, "nop", "", b"\x90"),
            insn(0x1002, "ret", "", b"\xc3"),
        ]

    def test_first_pair_returns_the_first_indexed_pair(self) -> None:
        original = self.body()
        rebuilt = self.body()
        difference = result(["instruction 2: byte 0: 0x90 against 0xc3"])
        index, left, right = first_difference.first_pair(
            original, rebuilt, difference
        )
        self.assertEqual(index, 2)
        self.assertIs(left, original[2])
        self.assertIs(right, rebuilt[2])

    def test_first_pair_uses_the_first_difference(self) -> None:
        original = self.body()
        rebuilt = self.body()
        difference = result(
            [
                "instruction 2: byte 0: 0x90 against 0xc3",
                "instruction 0: byte 0: 0x90 against 0x90",
            ]
        )
        index, left, _ = first_difference.first_pair(
            original, rebuilt, difference
        )
        self.assertEqual(index, 2)
        self.assertIs(left, original[2])

    def test_first_pair_is_none_without_a_difference(self) -> None:
        original = self.body()
        self.assertIsNone(
            first_difference.first_pair(original, original, result([]))
        )

    def test_first_pair_is_none_for_a_malformed_difference(self) -> None:
        original = self.body()
        self.assertIsNone(
            first_difference.first_pair(
                original,
                original,
                result(["mask: [1, 2, 3, 4] against []"]),
            )
        )


class ReportRowTestCase(unittest.TestCase):
    def pair(self) -> tuple[list[le.Insn], list[le.Insn]]:
        original = [
            insn(0x10000, "mov", "dword ptr [ebp - 8], eax", b"\x89\x45\xf8"),
            insn(0x10003, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x20000, "mov", "dword ptr [ebp - 0xc], eax", b"\x89\x45\xf4"),
            insn(0x20003, "ret", "", b"\xc3"),
        ]
        return original, rebuilt

    def test_report_row_holds_the_pair_and_the_counts(self) -> None:
        original, rebuilt = self.pair()
        module_recipe = module("error_report")
        difference = "instruction 0: byte 2: 0xf8 against 0xf4"
        row = first_difference.report_row(
            record(),
            module_recipe,
            result(
                [difference],
                bytes_equal=4,
                bytes_total=6,
                instructions_equal=1,
                instructions_total=2,
            ),
            original,
            rebuilt,
        )
        self.assertEqual(row["module"], "error_report")
        self.assertEqual(row["address"], "0x10978")
        self.assertEqual(row["symbol"], "error_report_")
        self.assertEqual(row["toolchain"], "v2")
        self.assertEqual(row["sources"], module_recipe["sources"])
        self.assertEqual(row["build"], "pass")
        self.assertEqual(row["match"], "same-shape")
        self.assertEqual(row["instructions_equal"], 1)
        self.assertEqual(row["instructions_total"], 2)
        self.assertEqual(row["bytes_equal"], 4)
        self.assertEqual(row["bytes_total"], 6)
        self.assertEqual(row["first_difference"], difference)
        self.assertEqual(row["idiom"], "local slot order")
        self.assertEqual(
            row["original"],
            {
                "address": "0x10000",
                "bytes": "8945f8",
                "text": "mov dword ptr [ebp - 8], eax",
            },
        )
        self.assertEqual(
            row["rebuilt"],
            {
                "address": "0x20000",
                "bytes": "8945f4",
                "text": "mov dword ptr [ebp - 0xc], eax",
            },
        )

    def test_report_row_holds_null_fields_without_a_difference(self) -> None:
        original, rebuilt = self.pair()
        row = first_difference.report_row(
            record(),
            module("error_report"),
            result([], match="exact-bytes"),
            original,
            rebuilt,
        )
        self.assertIsNone(row["first_difference"])
        self.assertEqual(row["idiom"], "match")
        self.assertIsNone(row["original"])
        self.assertIsNone(row["rebuilt"])

    def test_report_row_classifies_a_difference_without_a_pair_as_other(
        self,
    ) -> None:
        original, rebuilt = self.pair()
        row = first_difference.report_row(
            record(),
            module("error_report"),
            result(["mask: [1, 2, 3, 4] against []"]),
            original,
            rebuilt,
        )
        self.assertEqual(row["first_difference"], "mask: [1, 2, 3, 4] against []")
        self.assertEqual(row["idiom"], "other")
        self.assertIsNone(row["original"])
        self.assertIsNone(row["rebuilt"])

    def test_report_row_holds_the_sequence_pair(self) -> None:
        original, rebuilt = self.pair()
        row = first_difference.report_row(
            record(),
            module("error_report"),
            result([]),
            original,
            rebuilt,
            sequence=(0, "byte 2: 0xf8 against 0xf4"),
        )
        self.assertEqual(
            row["first_difference"],
            "instruction 0: byte 2: 0xf8 against 0xf4",
        )
        self.assertEqual(row["sequence_index"], 0)
        self.assertEqual(row["idiom"], "local slot order")
        self.assertEqual(
            row["original"],
            {
                "address": "0x10000",
                "bytes": "8945f8",
                "text": "mov dword ptr [ebp - 8], eax",
            },
        )
        self.assertEqual(
            row["rebuilt"],
            {
                "address": "0x20000",
                "bytes": "8945f4",
                "text": "mov dword ptr [ebp - 0xc], eax",
            },
        )

    def test_report_row_holds_the_sequence_count_difference(self) -> None:
        original = [
            insn(0x10000, "nop", "", b"\x90"),
            insn(0x10001, "nop", "", b"\x90"),
            insn(0x10002, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x20000, "nop", "", b"\x90"),
            insn(0x20001, "ret", "", b"\xc3"),
        ]
        row = first_difference.report_row(
            record(),
            module("error_report"),
            result([]),
            original,
            rebuilt,
            sequence=(2, "instruction count: 3 against 2"),
        )
        self.assertEqual(
            row["first_difference"],
            "instruction 2: instruction count: 3 against 2",
        )
        self.assertEqual(row["sequence_index"], 2)
        self.assertEqual(row["idiom"], "instruction count")
        self.assertIsNone(row["original"])
        self.assertIsNone(row["rebuilt"])

    def test_report_row_classifies_a_strict_result_with_no_difference_as_match(
        self,
    ) -> None:
        original, rebuilt = self.pair()
        row = first_difference.report_row(
            record(),
            module("error_report"),
            result([], match="exact-relocations"),
            original,
            rebuilt,
        )
        self.assertIsNone(row["first_difference"])
        self.assertEqual(row["idiom"], "match")
        self.assertIsNone(row["original"])
        self.assertIsNone(row["rebuilt"])


class FamilyTestCase(unittest.TestCase):
    def family(
        self,
        size: int | None,
        original: list[le.Insn],
        rebuilt: list[le.Insn],
    ) -> str:
        return first_difference.body_family(
            record(size=size), original, rebuilt
        )

    def test_family_marks_a_smaller_body(self) -> None:
        original = [
            insn(0x1000, "push", "ebx", b"\x53"),
            insn(0x1001, "mov", "eax, ebx", b"\x89\xd8"),
        ]
        rebuilt = [insn(0x2000, "push", "ebx", b"\x53")]
        self.assertEqual(self.family(12, original, rebuilt), "smaller")

    def test_family_marks_a_larger_body(self) -> None:
        original = [insn(0x1000, "push", "ebx", b"\x53")]
        rebuilt = [
            insn(0x2000, "push", "ebx", b"\x53"),
            insn(0x2001, "mov", "eax, ebx", b"\x89\xd8"),
        ]
        self.assertEqual(self.family(12, original, rebuilt), "larger")

    def test_family_marks_a_padding_body(self) -> None:
        original = [
            insn(0x1000, "push", "ebx", b"\x53"),
            insn(0x1001, "mov", "eax, ebx", b"\x89\xd8"),
        ]
        rebuilt = [
            insn(0x2000, "push", "ebx", b"\x53"),
            insn(0x2001, "mov", "eax, ebx", b"\x89\xd8"),
        ]
        self.assertEqual(self.family(12, original, rebuilt), "padding")

    def test_family_marks_an_aligned_body(self) -> None:
        original = [
            insn(0x1000 + index, "nop", "", b"\x90") for index in range(12)
        ]
        rebuilt = [
            insn(0x2000 + index, "nop", "", b"\x90") for index in range(12)
        ]
        self.assertEqual(self.family(12, original, rebuilt), "aligned")

    def test_family_marks_a_body_at_the_span_as_aligned(self) -> None:
        original = [insn(0x1000, "push", "ebx", b"\x53")]
        rebuilt = [
            insn(0x2000 + index, "nop", "", b"\x90") for index in range(12)
        ]
        self.assertEqual(self.family(12, original, rebuilt), "aligned")

    def test_family_marks_a_missing_size_as_unmeasured(self) -> None:
        body = [insn(0x1000, "push", "ebx", b"\x53")]
        self.assertEqual(self.family(None, body, body), "unmeasured")


class SummaryTestCase(unittest.TestCase):
    def test_summary_counts_the_families_and_the_idioms(self) -> None:
        rows = [
            {
                "family": "smaller",
                "idiom": "register save order",
                "sequence_index": 0,
            },
            {
                "family": "smaller",
                "idiom": "instruction selection",
                "sequence_index": 1,
            },
            {
                "family": "padding",
                "idiom": "register save order",
                "sequence_index": 2,
            },
        ]
        summary = first_difference.summarize(rows)
        self.assertEqual(summary["modules"], 3)
        self.assertEqual(summary["families"], {"padding": 1, "smaller": 2})
        self.assertEqual(
            summary["idioms"],
            {"instruction selection": 1, "register save order": 2},
        )
        self.assertEqual(
            list(summary["families"]), sorted(summary["families"])
        )
        self.assertEqual(list(summary["idioms"]), sorted(summary["idioms"]))

    def test_summary_counts_the_first_index_zero_rows(self) -> None:
        rows = [
            {
                "family": "smaller",
                "idiom": "register save order",
                "sequence_index": 0,
            },
            {
                "family": "larger",
                "idiom": "instruction selection",
                "sequence_index": 0,
            },
            {
                "family": "padding",
                "idiom": "other",
                "sequence_index": 3,
            },
        ]
        summary = first_difference.summarize(rows)
        self.assertEqual(summary["first_index_zero"], 2)

    def test_summary_counts_the_count_differences(self) -> None:
        rows = [
            {
                "family": "smaller",
                "idiom": "instruction count",
                "sequence_index": 1,
            }
        ]
        summary = first_difference.summarize(rows)
        self.assertEqual(summary["count_differences"], 1)

    def test_summary_counts_an_unmeasured_family(self) -> None:
        rows = [
            {
                "build": "fail",
                "family": "unmeasured",
                "idiom": "other",
                "sequence_index": None,
            }
        ]
        summary = first_difference.summarize(rows)
        self.assertEqual(summary["families"], {"unmeasured": 1})

    def test_report_document_holds_the_summary(self) -> None:
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            original = root / "STARTH.EXE"
            original.write_bytes(b"original")
            records = [record("0x1", "alpha", "alpha_", "same-shape")]
            modules = [module("alpha")]
            with mock.patch.object(
                first_difference,
                "diff_record",
                side_effect=row_for_module,
            ):
                document = first_difference.report(
                    records, modules, original
                )
        self.assertEqual(
            document["summary"],
            first_difference.summarize(document["modules"]),
        )


class ReportTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.original = self.root / "STARTH.EXE"
        self.original.write_bytes(b"original")

    def test_report_selects_the_non_strict_module_records(self) -> None:
        records = [
            record("0x1", "alpha", "alpha_", "same-shape"),
            record("0x2", "beta", "beta_", "different"),
            record("0x3", "gamma", "gamma_", "exact-relocations"),
        ]
        modules = [module("alpha"), module("beta"), module("gamma")]
        with mock.patch.object(
            first_difference,
            "diff_record",
            side_effect=row_for_module,
        ):
            document = first_difference.report(records, modules, self.original)
        self.assertEqual(
            [row["module"] for row in document["modules"]], ["alpha", "beta"]
        )

    def test_report_skips_a_record_without_a_result(self) -> None:
        item = record()
        del item["result"]
        self.assert_no_rows([item])

    def test_report_skips_a_record_without_an_identity_entry(self) -> None:
        item = record()
        item["result"]["checks"] = {}
        self.assert_no_rows([item])

    def test_report_skips_a_record_without_an_identity_match(self) -> None:
        self.assert_no_rows([record(match=None)])

    def test_report_skips_a_record_without_a_symbol(self) -> None:
        self.assert_no_rows([record(symbol=None)])

    def test_report_skips_a_record_without_a_module(self) -> None:
        self.assert_no_rows([record(module=None)])

    def test_report_skips_a_library_record(self) -> None:
        item = record()
        item["library"] = {
            "file": "CLIB3R.LIB",
            "symbol": "malloc_",
            "module": "nmalloc",
        }
        self.assert_no_rows([item])

    def assert_no_rows(self, records: list[dict]) -> None:
        modules = [module("error_report")]
        with mock.patch.object(
            first_difference,
            "diff_record",
            side_effect=row_for_module,
        ) as diff:
            document = first_difference.report(records, modules, self.original)
        self.assertEqual(document["modules"], [])
        diff.assert_not_called()

    def test_report_restricts_the_records_to_the_names(self) -> None:
        records = [
            record("0x1", "alpha", "alpha_", "same-shape"),
            record("0x2", "beta", "beta_", "same-shape"),
        ]
        modules = [module("alpha"), module("beta")]
        with mock.patch.object(
            first_difference,
            "diff_record",
            side_effect=row_for_module,
        ):
            document = first_difference.report(
                records, modules, self.original, names=["beta"]
            )
        self.assertEqual(
            [row["module"] for row in document["modules"]], ["beta"]
        )

    def test_report_orders_the_rows_by_module_name(self) -> None:
        names = [
            "pool_init",
            "error_report",
            "dpmi_int31_entry",
            "memory_available",
            "list_create",
            "memory_alloc_small",
        ]
        records = [
            record(f"0x{index + 1:x}", name, f"{name}_", "same-shape")
            for index, name in enumerate(names)
        ]
        modules = [module(name) for name in names]
        with mock.patch.object(
            first_difference,
            "diff_record",
            side_effect=row_for_module,
        ):
            document = first_difference.report(records, modules, self.original)
        self.assertEqual(
            [row["module"] for row in document["modules"]],
            [
                "dpmi_int31_entry",
                "error_report",
                "list_create",
                "memory_alloc_small",
                "memory_available",
                "pool_init",
            ],
        )

    def test_report_document_holds_the_date_and_the_original_digest(self) -> None:
        document = first_difference.report([], [], self.original)
        self.assertEqual(document["date"], date.today().isoformat())
        self.assertEqual(document["original"]["path"], str(self.original))
        self.assertEqual(
            document["original"]["sha256"],
            hashlib.sha256(b"original").hexdigest(),
        )

    def test_sha256_raises_for_a_missing_path(self) -> None:
        missing = self.root / "missing.exe"
        with self.assertRaises(ValueError) as caught:
            first_difference._sha256(missing)
        self.assertIn(str(missing), str(caught.exception))


class DiffRecordTestCase(unittest.TestCase):
    def test_diff_record_holds_the_sequence(self) -> None:
        original_object = mock.Mock()
        original_object.relocations.return_value = []
        rebuilt_object = mock.Mock()
        rebuilt_object.relocations.return_value = []
        rebuilt_object.object_by_index.return_value = mock.Mock(
            reloc_base=0x20000
        )
        original_body = [insn(0x10000, "push", "ebx", b"\x53")]
        rebuilt_body = [insn(0x20000, "push", "edx", b"\x52")]
        comparison = result([], match="different")
        with mock.patch.object(
            first_difference.run, "build_module", return_value=("pass", "")
        ), mock.patch.object(
            first_difference.le,
            "load",
            side_effect=[original_object, rebuilt_object],
        ), mock.patch.object(
            first_difference.diff_module, "map_address", return_value=(1, 0x100)
        ), mock.patch.object(
            first_difference.diff_module, "map_symbols", return_value=[]
        ), mock.patch.object(
            first_difference.diff_module, "next_symbol", return_value=None
        ), mock.patch.object(
            first_difference.diff_module,
            "original_body",
            return_value=original_body,
        ), mock.patch.object(
            first_difference.diff_module,
            "rebuilt_body",
            return_value=rebuilt_body,
        ), mock.patch.object(
            first_difference.diff_module,
            "original_symbol_resolver",
            return_value=None,
        ), mock.patch.object(
            first_difference.diff_module,
            "rebuilt_symbol_resolver",
            return_value=None,
        ), mock.patch.object(
            first_difference.oracle, "compare", return_value=comparison
        ), mock.patch.object(
            first_difference.oracle,
            "first_difference",
            return_value=(0, "byte 0: 0x53 against 0x52"),
        ) as first_difference_mock:
            row = first_difference.diff_record(
                record(),
                module("error_report"),
                Path("STARTH.EXE"),
                Path("work"),
            )
        first_difference_mock.assert_called_once()
        called_original, called_rebuilt = first_difference_mock.call_args.args[:2]
        self.assertIs(called_original, original_body)
        self.assertIs(called_rebuilt, rebuilt_body)
        self.assertEqual(
            row["first_difference"],
            "instruction 0: byte 0: 0x53 against 0x52",
        )
        self.assertEqual(row["sequence_index"], 0)
        self.assertEqual(row["idiom"], "register save order")

    def test_diff_record_marks_a_failed_build(self) -> None:
        with mock.patch.object(
            first_difference.run, "build_module", return_value=("fail", "boom")
        ):
            row = first_difference.diff_record(
                record(),
                module("error_report"),
                Path("STARTH.EXE"),
                Path("work"),
            )
        self.assertEqual(row["build"], "fail")
        self.assertIsNone(row["match"])
        self.assertIsNone(row["instructions_equal"])
        self.assertIsNone(row["instructions_total"])
        self.assertIsNone(row["bytes_equal"])
        self.assertIsNone(row["bytes_total"])
        self.assertIsNone(row["first_difference"])
        self.assertEqual(row["idiom"], "other")
        self.assertIsNone(row["original"])
        self.assertIsNone(row["rebuilt"])

    def test_diff_record_marks_a_skipped_build(self) -> None:
        with mock.patch.object(
            first_difference.run, "build_module", return_value=("skip", "")
        ):
            row = first_difference.diff_record(
                record(),
                module("error_report"),
                Path("STARTH.EXE"),
                Path("work"),
            )
        self.assertEqual(row["build"], "skip")
        self.assertEqual(row["idiom"], "other")


class CommandTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.original = self.root / "STARTH.EXE"
        self.original.write_bytes(b"MZ")
        self.out = self.root / "out.json"
        self.modules = [module("error_report")]

    def arguments(self, **overrides: object) -> argparse.Namespace:
        values: dict[str, object] = {
            "command": "report",
            "module": None,
            "all": True,
            "original": self.original,
            "out": self.out,
        }
        values.update(overrides)
        return argparse.Namespace(**values)

    def pass_row(self) -> dict:
        return {
            "module": "error_report",
            "address": "0x10978",
            "symbol": "error_report_",
            "build": "pass",
            "match": "same-shape",
            "first_difference": "instruction 0: byte 2: 0xf8 against 0xf4",
            "idiom": "local slot order",
        }

    def document(self, row: dict) -> dict:
        return {
            "date": date.today().isoformat(),
            "original": {"path": str(self.original), "sha256": "digest"},
            "summary": {
                "modules": 1,
                "families": {row.get("family", "other"): 1},
                "idioms": {row["idiom"]: 1},
                "first_index_zero": 0,
                "count_differences": 0,
            },
            "modules": [row],
        }

    def test_parse_arguments_accepts_the_report_flags(self) -> None:
        argv = [
            "first_difference.py",
            "report",
            "--module",
            "error_report",
            "--module",
            "pool_init",
            "--out",
            "out.json",
        ]
        with mock.patch.object(sys, "argv", argv):
            parsed = first_difference.parse_arguments()
        self.assertEqual(parsed.command, "report")
        self.assertEqual(parsed.module, ["error_report", "pool_init"])
        self.assertEqual(parsed.out, Path("out.json"))
        with mock.patch.object(
            sys, "argv", ["first_difference.py", "report", "--all"]
        ):
            parsed = first_difference.parse_arguments()
        self.assertTrue(parsed.all)

    def test_command_report_rejects_a_missing_selection(self) -> None:
        with mock.patch.object(
            first_difference, "_load_modules", return_value=self.modules
        ), mock.patch.object(
            first_difference, "_load_records", return_value=[]
        ), mock.patch.object(
            first_difference.le, "load", return_value=object()
        ), contextlib.redirect_stderr(io.StringIO()):
            code = first_difference.command_report(
                self.arguments(module=None, all=False)
            )
        self.assertEqual(code, 2)

    def test_command_report_rejects_a_missing_module(self) -> None:
        with mock.patch.object(
            first_difference, "_load_modules", return_value=self.modules
        ), mock.patch.object(
            first_difference, "_load_records", return_value=[]
        ), mock.patch.object(
            first_difference.le, "load", return_value=object()
        ), contextlib.redirect_stderr(io.StringIO()):
            code = first_difference.command_report(
                self.arguments(module=["missing"], all=False)
            )
        self.assertEqual(code, 2)

    def test_command_report_rejects_a_missing_original(self) -> None:
        missing = self.root / "missing.exe"
        with mock.patch.object(
            first_difference, "_load_modules", return_value=self.modules
        ), mock.patch.object(
            first_difference, "_load_records", return_value=[]
        ), mock.patch.object(
            first_difference, "diff_record"
        ) as diff, contextlib.redirect_stderr(io.StringIO()):
            code = first_difference.command_report(
                self.arguments(module=["error_report"], all=False, original=missing)
            )
        self.assertEqual(code, 2)
        diff.assert_not_called()

    def test_command_report_writes_the_document(self) -> None:
        row = self.pass_row()
        with mock.patch.object(
            first_difference,
            "report",
            return_value=self.document(row),
        ), mock.patch.object(
            first_difference, "_load_modules", return_value=self.modules
        ), mock.patch.object(
            first_difference, "_load_records", return_value=[]
        ), mock.patch.object(
            first_difference.le, "load", return_value=object()
        ), contextlib.redirect_stdout(io.StringIO()):
            code = first_difference.command_report(
                self.arguments(module=["error_report"], all=False)
            )
        self.assertEqual(code, 0)
        written = json.loads(self.out.read_text())
        self.assertEqual(written["modules"], [row])

    def test_command_report_exits_1_for_a_failed_build(self) -> None:
        row = dict(self.pass_row(), build="fail")
        with mock.patch.object(
            first_difference,
            "report",
            return_value=self.document(row),
        ), mock.patch.object(
            first_difference, "_load_modules", return_value=self.modules
        ), mock.patch.object(
            first_difference, "_load_records", return_value=[]
        ), mock.patch.object(
            first_difference.le, "load", return_value=object()
        ), contextlib.redirect_stdout(io.StringIO()):
            code = first_difference.command_report(
                self.arguments(module=["error_report"], all=False)
            )
        self.assertEqual(code, 1)
        self.assertTrue(self.out.is_file())

    def test_command_report_exits_1_for_a_skipped_build(self) -> None:
        row = dict(self.pass_row(), build="skip")
        with mock.patch.object(
            first_difference,
            "report",
            return_value=self.document(row),
        ), mock.patch.object(
            first_difference, "_load_modules", return_value=self.modules
        ), mock.patch.object(
            first_difference, "_load_records", return_value=[]
        ), mock.patch.object(
            first_difference.le, "load", return_value=object()
        ), contextlib.redirect_stdout(io.StringIO()):
            code = first_difference.command_report(
                self.arguments(module=["error_report"], all=False)
            )
        self.assertEqual(code, 1)
        self.assertTrue(self.out.is_file())

    def test_command_report_writes_no_store_record(self) -> None:
        row = self.pass_row()
        with mock.patch.object(
            first_difference,
            "report",
            return_value=self.document(row),
        ), mock.patch.object(
            first_difference, "_load_modules", return_value=self.modules
        ), mock.patch.object(
            first_difference, "_load_records", return_value=[]
        ), mock.patch.object(
            first_difference.le, "load", return_value=object()
        ), mock.patch.object(store, "save_record") as save_record, mock.patch.object(
            store, "save_module"
        ) as save_module, contextlib.redirect_stdout(io.StringIO()):
            code = first_difference.command_report(
                self.arguments(module=["error_report"], all=False)
            )
        self.assertEqual(code, 0)
        save_record.assert_not_called()
        save_module.assert_not_called()


class CatalogueTestCase(unittest.TestCase):
    def test_catalogue_holds_every_idiom_name(self) -> None:
        text = CATALOGUE.read_text()
        self.assertTrue(text)
        for name in first_difference.IDIOMS:
            with self.subTest(name=name):
                self.assertIn(name, text)

    def test_catalogue_holds_the_known_source_forms(self) -> None:
        text = CATALOGUE.read_text()
        headings = (
            "## The registers-before-frame prologue with -od without -of+",
            "## The frame-only epilogue against the conservative mov esp,ebp",
            "## The local slot order",
            "## The frame size",
            "## The volatile parameter local",
            "## The #pragma aux modify exact [regs] register contract",
            "## The -ot body expansion",
            "## The jump-table bias with inc eax",
            "## The short push imm8 against the long push imm32 with a relocation",
            "## The period compiler jump-table placement",
            "## The 8.3 source-name collision",
        )
        for heading in headings:
            with self.subTest(heading=heading):
                self.assertIn(heading, text)


if __name__ == "__main__":
    unittest.main()
