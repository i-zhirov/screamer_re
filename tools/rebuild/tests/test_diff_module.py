"""Tests for the module diff helpers: the symbols, the resolvers, and the bodies.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import diff_module  # noqa: E402
import le  # noqa: E402


class StubObject:
    def __init__(self, reloc_base: int) -> None:
        self.reloc_base = reloc_base


class StubImage:
    def __init__(self, objects: dict[int, StubObject]) -> None:
        self._objects = objects

    def object_by_index(self, index: int) -> StubObject | None:
        return self._objects.get(index)


def insn(address: int, mnemonic: str, operands: str, raw: bytes) -> le.Insn:
    return le.Insn(address, len(raw), mnemonic, operands, raw)


def slot(
    offset: int,
    width: int = 4,
    kind: str = "internal",
    **fields: object,
) -> le.Relocation:
    return le.Relocation(offset=offset, width=width, kind=kind, **fields)


def body(count: int) -> list[le.Insn]:
    return [
        insn(0x10000 + index, "nop", "", b"\x90") for index in range(count)
    ]


class DiffModuleTestCase(unittest.TestCase):
    def test_map_symbols_reads_a_linear_address_and_a_symbol(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "free.map"
            path.write_text("0001:00000000  free_\n")
            image = StubImage({1: StubObject(0x10000)})
            self.assertEqual(
                diff_module.map_symbols(path, image), [(0x10000, "free_")]
            )

    def test_map_symbols_skips_a_line_with_a_missing_object(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "free.map"
            path.write_text("0002:00000000  other_\n")
            image = StubImage({1: StubObject(0x10000)})
            self.assertEqual(diff_module.map_symbols(path, image), [])

    def test_symbol_at_returns_the_exact_symbol(self) -> None:
        symbols = [(0x10000, "free_"), (0x10020, "malloc_")]
        self.assertEqual(diff_module.symbol_at(symbols, 0x10000), "free_")
        self.assertEqual(diff_module.symbol_at(symbols, 0x10020), "malloc_")

    def test_symbol_at_returns_none_for_an_interior_address(self) -> None:
        symbols = [(0x10000, "free_"), (0x10020, "malloc_")]
        self.assertIsNone(diff_module.symbol_at(symbols, 0x10018))

    def test_next_symbol_returns_the_nearest_later_symbol(self) -> None:
        symbols = [(0x10000, "free_"), (0x10034, "nfree_")]
        self.assertEqual(diff_module.next_symbol(symbols, 0x10000), 0x10034)

    def test_next_symbol_returns_none_for_the_last_symbol(self) -> None:
        symbols = [(0x10000, "free_"), (0x10034, "nfree_")]
        self.assertIsNone(diff_module.next_symbol(symbols, 0x10034))

    def test_rebuilt_resolver_maps_an_internal_slot_to_the_symbol(self) -> None:
        image = StubImage({1: StubObject(0x10000)})
        resolver = diff_module.rebuilt_symbol_resolver(
            image, [(0x10000, "free_")]
        )
        self.assertEqual(
            resolver(slot(0x1001, object=1, target=0)), "free_"
        )
        self.assertIsNone(resolver(slot(0x1001, object=1, target=0x8)))
        self.assertIsNone(resolver(slot(0x1001, object=9, target=0)))

    def test_original_resolver_maps_a_slot_to_the_record_symbol(self) -> None:
        image = StubImage({1: StubObject(0x10000)})
        resolver = diff_module.original_symbol_resolver(
            image, [{"address": "0x10000", "symbol": "free_"}]
        )
        self.assertEqual(
            resolver(slot(0x1001, object=1, target=0)), "free_"
        )
        self.assertIsNone(resolver(slot(0x1001, object=1, target=0x18)))

    def test_original_resolver_returns_none_for_a_record_without_a_symbol(
        self,
    ) -> None:
        image = StubImage({1: StubObject(0x10000)})
        resolver = diff_module.original_symbol_resolver(
            image, [{"address": "0x10010"}]
        )
        self.assertIsNone(resolver(slot(0x1001, object=1, target=0x10)))

    def test_resolver_returns_none_for_an_imported_slot(self) -> None:
        image = StubImage({1: StubObject(0x10000)})
        resolver = diff_module.rebuilt_symbol_resolver(
            image, [(0x10000, "free_")]
        )
        imported = slot(
            0x1001, width=1, kind="import-ordinal", module=1, ordinal=3
        )
        self.assertIsNone(resolver(imported))

    def test_original_body_uses_the_end_over_the_count(self) -> None:
        image = StubImage({1: StubObject(0x10000)})
        instructions = body(5)
        with mock.patch.object(
            diff_module.le, "function_body", return_value=instructions
        ) as function_body:
            result = diff_module.original_body(image, 0x10000, 0x10100, 3)
        function_body.assert_called_once_with(image, 0x10000, end=0x10100)
        self.assertEqual(result, instructions)

    def test_original_body_uses_the_count_without_an_end(self) -> None:
        image = StubImage({1: StubObject(0x10000)})
        instructions = body(5)
        with mock.patch.object(
            diff_module.le, "function_body", return_value=instructions
        ) as function_body:
            result = diff_module.original_body(image, 0x10000, None, 3)
        function_body.assert_called_once_with(image, 0x10000)
        self.assertEqual(result, instructions[:3])

    def test_rebuilt_body_uses_the_extent_with_an_end(self) -> None:
        image = StubImage({1: StubObject(0x10000)})
        instructions = body(5)
        with mock.patch.object(
            diff_module.le, "function_body", return_value=instructions
        ) as function_body:
            with mock.patch.object(diff_module.le, "decode") as decode:
                result = diff_module.rebuilt_body(image, 0x10000, 0x10100, 3)
        function_body.assert_called_once_with(image, 0x10000)
        decode.assert_not_called()
        self.assertEqual(result, instructions)

    def test_rebuilt_body_uses_the_rebuilt_extent(self) -> None:
        image = StubImage({1: StubObject(0x10000)})
        instructions = body(3)
        with mock.patch.object(
            diff_module.le, "function_body", return_value=instructions
        ) as function_body:
            with mock.patch.object(diff_module.le, "decode") as decode:
                result = diff_module.rebuilt_body(
                    image, 0x10000, 0x10100, 3, extent=0x10010
                )
        function_body.assert_called_once_with(image, 0x10000, end=0x10010)
        decode.assert_not_called()
        self.assertEqual(result, instructions)

    def test_rebuilt_body_uses_the_count_without_an_end(self) -> None:
        image = StubImage({1: StubObject(0x10000)})
        instructions = body(3)
        with mock.patch.object(
            diff_module.le, "decode", return_value=instructions
        ) as decode:
            with mock.patch.object(diff_module.le, "function_body") as function_body:
                result = diff_module.rebuilt_body(image, 0x10000, None, 3)
        decode.assert_called_once_with(image, 0x10000, 3)
        function_body.assert_not_called()
        self.assertEqual(result, instructions)

    def test_original_body_uses_the_default_extent_without_an_end_or_count(
        self,
    ) -> None:
        image = StubImage({1: StubObject(0x10000)})
        instructions = body(5)
        with mock.patch.object(
            diff_module.le, "function_body", return_value=instructions
        ) as function_body:
            result = diff_module.original_body(image, 0x10000, None, 0)
        function_body.assert_called_once_with(image, 0x10000)
        self.assertEqual(result, instructions)

    def test_rebuilt_body_uses_the_default_extent_without_an_end_or_count(
        self,
    ) -> None:
        image = StubImage({1: StubObject(0x10000)})
        instructions = body(5)
        with mock.patch.object(
            diff_module.le, "function_body", return_value=instructions
        ) as function_body:
            result = diff_module.rebuilt_body(image, 0x10000, None, 0)
        function_body.assert_called_once_with(image, 0x10000)
        self.assertEqual(result, instructions)

    def test_parse_arguments_accepts_the_original_end(self) -> None:
        arguments = [
            "diff_module.py",
            "--original",
            "STARTH.EXE",
            "--original-address",
            "0x3009b",
            "--rebuilt",
            "prtfmod.exe",
            "--original-end",
            "0x10100",
        ]
        with mock.patch.object(sys, "argv", arguments):
            parsed = diff_module.parse_arguments()
        self.assertEqual(parsed.original_end, 0x10100)


if __name__ == "__main__":
    unittest.main()
