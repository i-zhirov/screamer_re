"""Tests for the relocation-aware function oracle.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import sys
import unittest
from collections.abc import Callable
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import le  # noqa: E402
import oracle  # noqa: E402
import run  # noqa: E402


def insn(
    address: int, mnemonic: str, operands: str, raw: bytes
) -> le.Insn:
    return le.Insn(address, len(raw), mnemonic, operands, raw)


def slot(
    offset: int,
    width: int = 4,
    kind: str = "internal",
    **fields: object,
) -> le.Relocation:
    return le.Relocation(offset=offset, width=width, kind=kind, **fields)


def symbols(
    table: dict[tuple[int | None, int | None], str],
) -> Callable[[le.Relocation], str | None]:
    def resolve(slot: le.Relocation) -> str | None:
        return table.get((slot.object, slot.target))

    return resolve


class OracleTestCase(unittest.TestCase):
    def compare(
        self,
        original: list[le.Insn],
        rebuilt: list[le.Insn],
        original_relocations: list[le.Relocation] | None = None,
        rebuilt_relocations: list[le.Relocation] | None = None,
        original_symbols: Callable[[le.Relocation], str | None] | None = None,
        rebuilt_symbols: Callable[[le.Relocation], str | None] | None = None,
    ) -> oracle.OracleResult:
        return oracle.compare(
            original,
            rebuilt,
            original_relocations=original_relocations or [],
            rebuilt_relocations=rebuilt_relocations or [],
            original_symbols=original_symbols,
            rebuilt_symbols=rebuilt_symbols,
        )

    def test_exact_bytes(self) -> None:
        body = [insn(0x1000, "ret", "", b"\xc3")]
        result = self.compare(body, list(body))
        self.assertEqual(result.match, oracle.EXACT_BYTES)
        self.assertTrue(result.passes_strict)

    def test_relocation_only_push_passes_strict(self) -> None:
        original = [insn(0x1000, "push", "0x2fd2", bytes.fromhex("68d22f0000"))]
        rebuilt = [insn(0x2000, "push", "0x370", bytes.fromhex("6870030000"))]
        result = self.compare(
            original,
            rebuilt,
            [slot(0x1001, object=1, target=0x2FD2)],
            [slot(0x2001, object=1, target=0x370)],
        )
        self.assertEqual(result.match, oracle.EXACT_RELOCATIONS)
        self.assertTrue(result.passes_strict)
        self.assertEqual(result.relocations_equal, 1)
        self.assertEqual(result.relocations_total, 1)

    def test_outside_function_branch_target_stays_masked(self) -> None:
        original = [
            insn(0x300EE, "jne", "0xd2008", bytes.fromhex("0f85141f0a00"))
        ]
        rebuilt = [
            insn(0x10000, "jne", "0x10024", bytes.fromhex("0f8500000000"))
        ]
        result = self.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.EXACT_RELOCATIONS)
        self.assertTrue(result.passes_strict)

    def test_intra_function_branch_target_matches_by_index(self) -> None:
        original = [
            insn(0x1000, "jne", "0x1008", bytes.fromhex("0f8502000000")),
            insn(0x1008, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x2000, "jne", "0x2006", bytes.fromhex("0f8500000000")),
            insn(0x2006, "ret", "", b"\xc3"),
        ]
        result = self.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.EXACT_RELOCATIONS)
        self.assertTrue(result.passes_strict)

    def test_intra_function_branch_target_mismatch_fails_strict(self) -> None:
        original = [
            insn(0x1000, "jne", "0x1006", bytes.fromhex("0f8500000000")),
            insn(0x1006, "nop", "", b"\x90"),
            insn(0x1007, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x2000, "jne", "0x2007", bytes.fromhex("0f8501000000")),
            insn(0x2006, "nop", "", b"\x90"),
            insn(0x2007, "ret", "", b"\xc3"),
        ]
        result = self.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.SAME_SHAPE)
        self.assertFalse(result.passes_strict)

    def test_intra_function_branch_target_into_the_middle_is_a_difference(
        self,
    ) -> None:
        original = [
            insn(0x1000, "jne", "0x1006", bytes.fromhex("0f8500000000")),
            insn(0x1006, "mov", "eax, 1", bytes.fromhex("b801000000")),
            insn(0x100B, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x2000, "jne", "0x2008", bytes.fromhex("0f8502000000")),
            insn(0x2006, "mov", "eax, 1", bytes.fromhex("b801000000")),
            insn(0x200B, "ret", "", b"\xc3"),
        ]
        result = self.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.SAME_SHAPE)
        self.assertFalse(result.passes_strict)

    def test_branch_target_inside_only_one_side_stays_masked(self) -> None:
        original = [
            insn(0x1000, "jne", "0x1006", bytes.fromhex("0f8500000000")),
            insn(0x1006, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x2000, "jne", "0x9000", bytes.fromhex("0f85fa6f0000")),
            insn(0x2006, "ret", "", b"\xc3"),
        ]
        result = self.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.EXACT_RELOCATIONS)
        self.assertTrue(result.passes_strict)

    def test_intra_function_call_target_stays_masked(self) -> None:
        original = [
            insn(0x1000, "call", "0x1006", bytes.fromhex("e801000000")),
            insn(0x1005, "nop", "", b"\x90"),
            insn(0x1006, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x2000, "call", "0x2005", bytes.fromhex("e800000000")),
            insn(0x2005, "nop", "", b"\x90"),
            insn(0x2006, "ret", "", b"\xc3"),
        ]
        result = self.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.EXACT_RELOCATIONS)
        self.assertTrue(result.passes_strict)

    def test_reordered_instructions_fail_strict(self) -> None:
        original = [
            insn(0x1000, "mov", "eax, 1", bytes.fromhex("b801000000")),
            insn(0x1005, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x2000, "ret", "", b"\xc3"),
            insn(0x2001, "mov", "eax, 1", bytes.fromhex("b801000000")),
        ]
        result = self.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.SAME_MNEMONICS)
        self.assertFalse(result.passes_strict)
        self.assertTrue(result.passes_default)

    def test_reordered_same_mnemonic_fails_strict(self) -> None:
        original = [
            insn(0x1000, "mov", "eax, 1", bytes.fromhex("b801000000")),
            insn(0x1005, "mov", "ebx, 2", bytes.fromhex("bb02000000")),
        ]
        rebuilt = [
            insn(0x2000, "mov", "ebx, 2", bytes.fromhex("bb02000000")),
            insn(0x2005, "mov", "eax, 1", bytes.fromhex("b801000000")),
        ]
        result = self.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.SAME_SHAPE)
        self.assertFalse(result.passes_strict)

    def test_non_relocation_immediate_fails_strict(self) -> None:
        original = [insn(0x1000, "add", "esp, 0x10", bytes.fromhex("83c410"))]
        rebuilt = [insn(0x2000, "add", "esp, 0x20", bytes.fromhex("83c420"))]
        result = self.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.SAME_SHAPE)
        self.assertFalse(result.passes_strict)
        self.assertTrue(result.passes_default)

    def test_different_instruction_count(self) -> None:
        original = [insn(0x1000, "ret", "", b"\xc3")]
        rebuilt = [
            insn(0x2000, "ret", "", b"\xc3"),
            insn(0x2001, "nop", "", b"\x90"),
        ]
        result = self.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.DIFFERENT)
        self.assertFalse(result.passes_strict)

    def test_internal_symbol_identity_matches(self) -> None:
        original = [insn(0x1000, "push", "0x2fd2", bytes.fromhex("68d22f0000"))]
        rebuilt = [insn(0x2000, "push", "0x370", bytes.fromhex("6870030000"))]
        result = self.compare(
            original,
            rebuilt,
            [slot(0x1001, object=1, target=0x2FD2)],
            [slot(0x2001, object=2, target=0x370)],
            original_symbols=symbols({(1, 0x2FD2): "free_"}),
            rebuilt_symbols=symbols({(2, 0x370): "free_"}),
        )
        self.assertEqual(result.match, oracle.EXACT_RELOCATIONS)
        self.assertTrue(result.passes_strict)

    def test_internal_symbol_identity_mismatch_fails_strict(self) -> None:
        original = [insn(0x1000, "push", "0x2fd2", bytes.fromhex("68d22f0000"))]
        rebuilt = [insn(0x2000, "push", "0x370", bytes.fromhex("6870030000"))]
        result = self.compare(
            original,
            rebuilt,
            [slot(0x1001, object=1, target=0x2FD2)],
            [slot(0x2001, object=2, target=0x370)],
            original_symbols=symbols({(1, 0x2FD2): "free_"}),
            rebuilt_symbols=symbols({(2, 0x370): "malloc_"}),
        )
        self.assertEqual(result.match, oracle.SAME_SHAPE)
        self.assertFalse(result.passes_strict)

    def test_internal_symbol_identity_compares_the_addend(self) -> None:
        original = [insn(0x1000, "push", "0x2fd2", bytes.fromhex("68d22f0000"))]
        rebuilt = [insn(0x2000, "push", "0x370", bytes.fromhex("6870030000"))]
        result = self.compare(
            original,
            rebuilt,
            [slot(0x1001, object=1, target=0x2FD2, addend=0)],
            [slot(0x2001, object=2, target=0x370, addend=4)],
            original_symbols=symbols({(1, 0x2FD2): "free_"}),
            rebuilt_symbols=symbols({(2, 0x370): "free_"}),
        )
        self.assertEqual(result.match, oracle.SAME_SHAPE)
        self.assertFalse(result.passes_strict)

    def test_internal_symbol_identity_treats_a_missing_addend_as_zero(self) -> None:
        original = [insn(0x1000, "push", "0x2fd2", bytes.fromhex("68d22f0000"))]
        rebuilt = [insn(0x2000, "push", "0x370", bytes.fromhex("6870030000"))]
        result = self.compare(
            original,
            rebuilt,
            [slot(0x1001, object=1, target=0x2FD2)],
            [slot(0x2001, object=2, target=0x370, addend=0)],
            original_symbols=symbols({(1, 0x2FD2): "free_"}),
            rebuilt_symbols=symbols({(2, 0x370): "free_"}),
        )
        self.assertEqual(result.match, oracle.EXACT_RELOCATIONS)
        self.assertTrue(result.passes_strict)

    def test_internal_symbol_identity_falls_back_to_the_kind(self) -> None:
        original = [insn(0x1000, "push", "0x2fd2", bytes.fromhex("68d22f0000"))]
        rebuilt = [insn(0x2000, "push", "0x370", bytes.fromhex("6870030000"))]
        result = self.compare(
            original,
            rebuilt,
            [slot(0x1001, object=1, target=0x2FD2)],
            [slot(0x2001, object=2, target=0x370)],
            original_symbols=symbols({(1, 0x2FD2): "free_"}),
            rebuilt_symbols=symbols({}),
        )
        self.assertEqual(result.match, oracle.EXACT_RELOCATIONS)
        self.assertTrue(result.passes_strict)

    def test_slot_key_holds_the_symbol_and_the_addend(self) -> None:
        internal = slot(0x1001, object=1, target=0x2FD2)
        resolver = symbols({(1, 0x2FD2): "free_"})
        self.assertEqual(
            oracle.slot_key(internal, resolver), ("internal", "free_", 0)
        )
        self.assertEqual(oracle.slot_key(internal), ("internal",))

    def test_slot_key_keeps_an_import_key_with_a_resolver(self) -> None:
        imported = slot(
            0x1001, width=1, kind="import-ordinal", module=1, ordinal=3
        )
        self.assertEqual(
            oracle.slot_key(imported, symbols({})),
            ("import-ordinal", 1, 3),
        )

    def test_imported_target_identity(self) -> None:
        original = [insn(0x1000, "push", "0x10", bytes.fromhex("6a10"))]
        rebuilt = [insn(0x2000, "push", "0x20", bytes.fromhex("6a20"))]
        result = self.compare(
            original,
            rebuilt,
            [slot(0x1001, width=1, kind="import-ordinal",
                  module=1, ordinal=3)],
            [slot(0x2001, width=1, kind="import-ordinal",
                  module=1, ordinal=4)],
        )
        self.assertEqual(result.match, oracle.SAME_SHAPE)
        self.assertFalse(result.passes_strict)

    def test_strict_gate_accepts_a_relocation_only_verdict(self) -> None:
        self.assertTrue(run.identity_accepted(oracle.EXACT_RELOCATIONS, True))
        self.assertFalse(run.identity_accepted(oracle.SAME_SHAPE, True))
        self.assertFalse(run.identity_accepted(oracle.SAME_MNEMONICS, True))
        self.assertTrue(run.identity_accepted(oracle.SAME_SHAPE, False))
        self.assertTrue(run.identity_accepted(oracle.SAME_MNEMONICS, False))


class FirstDifferenceTestCase(unittest.TestCase):
    def test_first_difference_reports_the_first_indexed_pair(self) -> None:
        original = [insn(0x1000, "push", "ebx", b"\x53")]
        rebuilt = [insn(0x2000, "push", "edx", b"\x52")]
        self.assertEqual(
            oracle.first_difference(original, rebuilt),
            (0, "byte 0: 0x53 against 0x52"),
        )

    def test_first_difference_reports_a_later_pair(self) -> None:
        original = [
            insn(0x1000, "nop", "", b"\x90"),
            insn(0x1001, "nop", "", b"\x90"),
            insn(0x1002, "mov", "eax, ebx", b"\x89\xd8"),
        ]
        rebuilt = [
            insn(0x2000, "nop", "", b"\x90"),
            insn(0x2001, "nop", "", b"\x90"),
            insn(0x2002, "lea", "eax, [ebx]", b"\x8d\x03"),
        ]
        self.assertEqual(
            oracle.first_difference(original, rebuilt),
            (2, "mnemonic: 'mov' against 'lea'"),
        )

    def test_first_difference_reports_a_count_mismatch_after_an_equal_prefix(
        self,
    ) -> None:
        original = [
            insn(0x1000, "nop", "", b"\x90"),
            insn(0x1001, "nop", "", b"\x90"),
            insn(0x1002, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x2000, "nop", "", b"\x90"),
            insn(0x2001, "nop", "", b"\x90"),
        ]
        self.assertEqual(
            oracle.first_difference(original, rebuilt),
            (2, "instruction count: 3 against 2"),
        )

    def test_first_difference_is_none_for_equal_bodies(self) -> None:
        body = [
            insn(0x1000, "push", "ebx", b"\x53"),
            insn(0x1001, "ret", "", b"\xc3"),
        ]
        self.assertIsNone(oracle.first_difference(body, list(body)))

    def test_first_difference_masks_a_relocation(self) -> None:
        original = [
            insn(0x1000, "push", "0x2fd2", bytes.fromhex("68d22f0000"))
        ]
        rebuilt = [insn(0x2000, "push", "0x370", bytes.fromhex("6870030000"))]
        self.assertIsNone(
            oracle.first_difference(
                original,
                rebuilt,
                original_relocations=[slot(0x1001, object=1, target=0x2FD2)],
                rebuilt_relocations=[slot(0x2001, object=1, target=0x370)],
            )
        )

    def test_first_difference_masks_an_intra_function_branch(self) -> None:
        original = [
            insn(0x1000, "jne", "0x1006", bytes.fromhex("0f8500000000")),
            insn(0x1006, "nop", "", b"\x90"),
            insn(0x1007, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x2000, "jne", "0x2007", bytes.fromhex("0f8501000000")),
            insn(0x2006, "nop", "", b"\x90"),
            insn(0x2007, "ret", "", b"\xc3"),
        ]
        sequence = oracle.first_difference(original, rebuilt)
        self.assertIsNotNone(sequence)
        index, text = sequence
        self.assertEqual(index, 0)
        self.assertTrue(text.startswith("branch target:"))

    def test_compare_keeps_the_length_only_verdict(self) -> None:
        original = [
            insn(0x1000, "nop", "", b"\x90"),
            insn(0x1001, "nop", "", b"\x90"),
            insn(0x1002, "ret", "", b"\xc3"),
        ]
        rebuilt = [
            insn(0x2000, "nop", "", b"\x90"),
            insn(0x2001, "nop", "", b"\x90"),
        ]
        result = oracle.compare(original, rebuilt)
        self.assertEqual(result.match, oracle.DIFFERENT)
        self.assertEqual(result.differences, [])


class FixupRecordTestCase(unittest.TestCase):
    def test_internal_offset_record(self) -> None:
        data = bytes.fromhex("07000001013412")
        relocations = le.fixup_records(data, 0, len(data), 0x10000)
        self.assertEqual(len(relocations), 1)
        relocation = relocations[0]
        self.assertEqual(relocation.offset, 0x10100)
        self.assertEqual(relocation.width, 4)
        self.assertEqual(relocation.kind, "internal")
        self.assertEqual(relocation.object, 1)
        self.assertEqual(relocation.target, 0x1234)

    def test_additive_32_bit_record(self) -> None:
        data = bytes.fromhex("07340001010000000078563412")
        relocations = le.fixup_records(data, 0, len(data), 0)
        self.assertEqual(len(relocations), 1)
        self.assertEqual(relocations[0].width, 4)
        self.assertEqual(relocations[0].target, 0)
        self.assertEqual(relocations[0].addend, 0x12345678)

    def test_source_list_record(self) -> None:
        data = bytes.fromhex("27000201341210002000")
        relocations = le.fixup_records(data, 0, len(data), 0x10000)
        self.assertEqual(
            [relocation.offset for relocation in relocations],
            [0x10010, 0x10020],
        )
        self.assertTrue(all(relocation.width == 4 for relocation in relocations))

    def test_imported_ordinal_record(self) -> None:
        data = bytes.fromhex("070140000210 00".replace(" ", ""))
        relocations = le.fixup_records(data, 0, len(data), 0)
        self.assertEqual(len(relocations), 1)
        relocation = relocations[0]
        self.assertEqual(relocation.kind, "import-ordinal")
        self.assertEqual(relocation.module, 2)
        self.assertEqual(relocation.ordinal, 0x10)


if __name__ == "__main__":
    unittest.main()
