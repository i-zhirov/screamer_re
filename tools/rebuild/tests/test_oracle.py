"""Tests for the relocation-aware function oracle.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import sys
import unittest
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


class OracleTestCase(unittest.TestCase):
    def compare(
        self,
        original: list[le.Insn],
        rebuilt: list[le.Insn],
        original_relocations: list[le.Relocation] | None = None,
        rebuilt_relocations: list[le.Relocation] | None = None,
    ) -> oracle.OracleResult:
        return oracle.compare(
            original,
            rebuilt,
            original_relocations=original_relocations or [],
            rebuilt_relocations=rebuilt_relocations or [],
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

    def test_direct_branch_relocation_passes_strict(self) -> None:
        original = [
            insn(0x300EE, "jne", "0xd2008", bytes.fromhex("0f85141f0a00"))
        ]
        rebuilt = [
            insn(0x10000, "jne", "0x10024", bytes.fromhex("0f8500000000"))
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
        relocation_only = oracle.VERDICTS[oracle.EXACT_RELOCATIONS]
        shape_only = oracle.VERDICTS[oracle.SAME_SHAPE]
        reordered = oracle.VERDICTS[oracle.SAME_MNEMONICS]
        self.assertTrue(run.identity_accepted(relocation_only, True))
        self.assertFalse(run.identity_accepted(shape_only, True))
        self.assertFalse(run.identity_accepted(reordered, True))
        self.assertTrue(run.identity_accepted(shape_only, False))
        self.assertTrue(run.identity_accepted(reordered, False))


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
