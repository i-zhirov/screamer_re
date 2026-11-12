"""Tests for the function-start helpers, the page reads, and the reachable body.

The reachable body tests script the decoder, so they need no capstone library.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests -p 'test_le_helpers.py'
"""
from __future__ import annotations

import contextlib
import io
import sys
import unittest
from collections.abc import Callable
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import le  # noqa: E402

PROLOGUE_SAVE = b"\x53\x55\x89\xe5\x90\x55\x89\xe5"
MIXED_PROLOGUES = b"\x53\x55\x89\xe5\x90\x53\x55\x8b\xec"
BASE = 0x10000


def page(
    object_index: int = 1,
    page_index: int = 0,
    number: int = 1,
    flags: int = le.P_LEGAL,
    file_offset: int | None = 0x20,
    data_size: int = 0x10,
    virtual_address: int = 0x10000,
    virtual_size: int = 0x10,
) -> le.Page:
    """The page table row the page reads walk."""
    return le.Page(
        object_index,
        page_index,
        number,
        flags,
        file_offset,
        data_size,
        virtual_address,
        virtual_size,
    )


def page_image(data: bytes, pages: list[le.Page]) -> le.LinearExecutable:
    """The minimal image the page reads walk."""
    exe = le.LinearExecutable(Path("image.exe"), data)
    exe.header = {}
    exe.objects = []
    exe.pages = list(pages)
    return exe


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


def insn(address: int, mnemonic: str, operands: str, raw: bytes) -> le.Insn:
    """One decoded instruction for a scripted decode."""
    return le.Insn(address, len(raw), mnemonic, operands, raw)


def scripted_decode(
    blocks: dict[int, list[le.Insn]]
) -> Callable[[le.LinearExecutable, int, int], list[le.Insn]]:
    """A `decode` side effect that returns the scripted block at an address."""

    def decode(
        exe: le.LinearExecutable, address: int, max_instructions: int
    ) -> list[le.Insn]:
        return blocks.get(address, [])[:max_instructions]

    return decode


class FunctionStartsTestCase(unittest.TestCase):
    def test_function_starts_returns_the_prologue_starts(self) -> None:
        image = StubImage(PROLOGUE_SAVE)
        self.assertEqual(le.function_starts(image), [0x10000, 0x10005])

    def test_function_starts_accepts_both_prologue_encodings(self) -> None:
        image = StubImage(MIXED_PROLOGUES)
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

    def test_nearest_function_start_accepts_both_prologue_encodings(self) -> None:
        image = StubImage(MIXED_PROLOGUES)
        self.assertEqual(le.nearest_function_start(image, 0x10008), 0x10005)

    def test_nearest_function_start_returns_none_without_a_prologue(self) -> None:
        image = StubImage(b"\x90" * 8)
        self.assertIsNone(le.nearest_function_start(image, 0x10004))

    def test_nearest_function_start_rejects_an_unmapped_address(self) -> None:
        image = StubImage(b"\x90" * 8, base=0x20000)
        image.entry_linear = 0x20000
        with self.assertRaisesRegex(ValueError, "the address is unmapped"):
            le.nearest_function_start(image, 0x10004)


class PageReadTestCase(unittest.TestCase):
    def test_read_stitches_two_pages(self) -> None:
        data = bytearray(0x60)
        data[0x20:0x30] = b"ABCDEFGHIJKLMNOP"
        data[0x40:0x50] = b"qrstuvwxyz012345"
        pages = [
            page(file_offset=0x20),
            page(
                page_index=1,
                number=2,
                file_offset=0x40,
                virtual_address=0x10010,
            ),
        ]
        image = page_image(bytes(data), pages)
        self.assertEqual(image.read(0x10008, 0x10), b"IJKLMNOPqrstuvwx")

    def test_read_starts_inside_a_page(self) -> None:
        data = bytearray(0x40)
        data[0x20:0x30] = b"ABCDEFGHIJKLMNOP"
        image = page_image(bytes(data), [page()])
        self.assertEqual(image.read(0x10004, 4), b"EFGH")

    def test_read_stops_at_the_page_data_size(self) -> None:
        data = bytearray(0x40)
        data[0x20:0x28] = b"ABCDEFGH"
        data[0x28:0x30] = b"12345678"
        pages = [
            page(file_offset=0x20, data_size=8),
            page(
                page_index=1,
                number=2,
                file_offset=0x28,
                data_size=8,
                virtual_address=0x10010,
            ),
        ]
        image = page_image(bytes(data), pages)
        self.assertEqual(image.read(0x10000, 0x10), b"ABCDEFGH")

    def test_read_zero_fills_a_zeroed_page(self) -> None:
        image = page_image(
            b"", [page(flags=le.P_ZEROED, file_offset=None, data_size=0)]
        )
        self.assertEqual(image.read(0x10004, 0x20), b"\x00" * 12)

    def test_read_continues_past_a_zeroed_page(self) -> None:
        data = bytearray(0x80)
        data[0x20:0x30] = b"ABCDEFGHIJKLMNOP"
        data[0x50:0x60] = b"qrstuvwxyz012345"
        pages = [
            page(file_offset=0x20),
            page(
                page_index=1,
                number=2,
                flags=le.P_ZEROED,
                file_offset=None,
                virtual_address=0x10010,
            ),
            page(
                page_index=2,
                number=3,
                file_offset=0x50,
                virtual_address=0x10020,
            ),
        ]
        image = page_image(bytes(data), pages)
        self.assertEqual(
            image.read(0x10008, 0x20),
            b"IJKLMNOP" + b"\x00" * 0x10 + b"qrstuvwx",
        )

    def test_read_stops_at_a_page_without_a_file_offset(self) -> None:
        data = bytearray(0x40)
        data[0x20:0x30] = b"ABCDEFGHIJKLMNOP"
        pages = [
            page(file_offset=0x20),
            page(
                page_index=1,
                number=2,
                flags=le.P_COMPRESSED,
                file_offset=None,
                virtual_address=0x10010,
            ),
        ]
        image = page_image(bytes(data), pages)
        self.assertEqual(image.read(0x10008, 0x10), b"IJKLMNOP")

    def test_read_returns_none_when_the_start_page_has_no_file_offset(self) -> None:
        image = page_image(
            b"", [page(flags=le.P_COMPRESSED, file_offset=None)]
        )
        self.assertIsNone(image.read(0x10000, 4))

    def test_read_stops_at_a_gap_between_pages(self) -> None:
        data = bytearray(0x40)
        data[0x20:0x28] = b"ABCDEFGH"
        data[0x30:0x38] = b"qrstuvwx"
        pages = [
            page(file_offset=0x20, data_size=8, virtual_size=8),
            page(
                page_index=1,
                number=2,
                file_offset=0x30,
                data_size=8,
                virtual_size=8,
                virtual_address=0x10010,
            ),
        ]
        image = page_image(bytes(data), pages)
        self.assertEqual(image.read(0x10000, 0x20), b"ABCDEFGH")

    def test_read_returns_none_when_the_start_is_unmapped(self) -> None:
        image = page_image(b"", [page()])
        self.assertIsNone(image.read(0x20000, 4))

    def test_read_accepts_a_zero_length(self) -> None:
        image = page_image(b"", [page()])
        self.assertEqual(image.read(0x10000, 0), b"")


class ReachableCallTargetsTestCase(unittest.TestCase):
    def test_reachable_call_targets_walks_the_direct_calls(self) -> None:
        image = StubImage(b"")
        sites = {
            0x100: [le.CallSite(0x101, 0x200, "call 0x200")],
            0x200: [
                le.CallSite(0x201, 0x300, "call 0x300"),
                le.CallSite(0x202, None, "call eax"),
            ],
        }
        with mock.patch.object(
            le,
            "function_body",
            side_effect=lambda exe, address, max_instructions=4000: [address],
        ), mock.patch.object(
            le, "call_sites", side_effect=lambda body: sites.get(body[0], [])
        ):
            self.assertEqual(
                le.reachable_call_targets(image, [0x100]), [0x200, 0x300]
            )


class ReachableBodyTestCase(unittest.TestCase):
    def test_function_body_follows_a_conditional_jump_target(self) -> None:
        blocks = {
            BASE: [
                insn(BASE, "jne", "0x10010", b"\x75\x0e"),
                insn(BASE + 2, "mov", "eax, ebx", b"\x89\xd8"),
                insn(BASE + 4, "ret", "", b"\xc3"),
            ],
            BASE + 0x10: [
                insn(BASE + 0x10, "mov", "ecx, edx", b"\x89\xd1"),
                insn(BASE + 0x12, "ret", "", b"\xc3"),
            ],
        }
        image = StubImage(b"\x90" * 0x20)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10002, 0x10004, 0x10010, 0x10012],
        )

    def test_function_body_follows_a_backward_loop_target(self) -> None:
        blocks = {
            BASE: [
                insn(BASE, "mov", "eax, 0", b"\xb8\x00\x00\x00\x00"),
                insn(BASE + 5, "add", "eax, 1", b"\x83\xc0\x01"),
                insn(BASE + 8, "jb", "0x10000", b"\x72\xf6"),
                insn(BASE + 0xA, "jne", "0x10010", b"\x75\x04"),
                insn(BASE + 0xC, "ret", "", b"\xc3"),
            ],
            BASE + 0x10: [insn(BASE + 0x10, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x20)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10005, 0x10008, 0x1000A, 0x1000C, 0x10010],
        )

    def test_function_body_does_not_follow_a_call(self) -> None:
        blocks = {
            BASE: [
                insn(BASE, "call", "0x10010", b"\xe8\x0b\x00\x00\x00"),
                insn(BASE + 5, "jne", "0x10020", b"\x75\x19"),
                insn(BASE + 7, "ret", "", b"\xc3"),
            ],
            BASE + 0x10: [insn(BASE + 0x10, "ret", "", b"\xc3")],
            BASE + 0x20: [insn(BASE + 0x20, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x30)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10005, 0x10007, 0x10020],
        )

    def test_function_body_stops_at_a_terminator(self) -> None:
        blocks = {
            BASE: [
                insn(BASE, "nop", "", b"\x90"),
                insn(BASE + 1, "jne", "0x10010", b"\x75\x0d"),
                insn(BASE + 3, "ret", "", b"\xc3"),
                insn(BASE + 4, "nop", "", b"\x90"),
            ],
            BASE + 0x10: [insn(BASE + 0x10, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x20)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10001, 0x10003, 0x10010],
        )

    def test_function_body_stops_after_an_unconditional_jump(self) -> None:
        blocks = {
            BASE: [
                insn(BASE, "je", "0x10010", b"\x74\x0e"),
                insn(BASE + 2, "jmp", "0x10010", b"\xeb\x0c"),
                insn(BASE + 4, "nop", "", b"\x90"),
            ],
            BASE + 0x10: [insn(BASE + 0x10, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x20)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body], [0x10000, 0x10002, 0x10010]
        )

    def test_function_body_keeps_a_leading_jump(self) -> None:
        blocks = {
            BASE: [insn(BASE, "jmp", "0x10010", b"\xe9\x0b\x00\x00\x00")],
            BASE + 0x10: [
                insn(BASE + 0x10, "nop", "", b"\x90"),
                insn(BASE + 0x11, "jne", "0x10020", b"\x75\x0d"),
                insn(BASE + 0x13, "ret", "", b"\xc3"),
            ],
            BASE + 0x20: [insn(BASE + 0x20, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x30)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10010, 0x10011, 0x10013, 0x10020],
        )

    def test_function_body_ignores_a_target_outside_the_end(self) -> None:
        blocks = {
            BASE: [
                insn(BASE, "je", "0x10008", b"\x74\x06"),
                insn(BASE + 2, "jne", "0x10010", b"\x75\x0c"),
                insn(BASE + 4, "ret", "", b"\xc3"),
            ],
            BASE + 0x08: [insn(BASE + 0x08, "ret", "", b"\xc3")],
            BASE + 0x10: [insn(BASE + 0x10, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x20)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE, 4000, BASE + 0x10)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10002, 0x10004, 0x10008],
        )

    def test_function_body_uses_the_next_start_as_the_default_end(self) -> None:
        blocks = {
            BASE: [
                insn(BASE, "je", "0x10008", b"\x74\x06"),
                insn(BASE + 2, "jne", "0x10010", b"\x75\x0c"),
                insn(BASE + 4, "ret", "", b"\xc3"),
            ],
            BASE + 0x08: [insn(BASE + 0x08, "ret", "", b"\xc3")],
            BASE + 0x10: [insn(BASE + 0x10, "ret", "", b"\xc3")],
        }
        data = b"\x00" * 0x10 + b"\x55\x89\xe5" + b"\x00" * 0x0D
        image = StubImage(data)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10002, 0x10004, 0x10008],
        )

    def test_function_body_skips_the_current_register_save_prologue(self) -> None:
        blocks = {
            BASE: [
                insn(BASE, "push", "ebx", b"\x53"),
                insn(BASE + 1, "push", "ebp", b"\x55"),
                insn(BASE + 2, "mov", "ebp, esp", b"\x89\xe5"),
                insn(BASE + 4, "jne", "0x10008", b"\x75\x02"),
                insn(BASE + 6, "jne", "0x10010", b"\x75\x08"),
            ],
            BASE + 0x08: [insn(BASE + 0x08, "ret", "", b"\xc3")],
            BASE + 0x10: [insn(BASE + 0x10, "ret", "", b"\xc3")],
        }
        data = (
            b"\x53\x55\x89\xe5"
            + b"\x00" * 0x0C
            + b"\x55\x89\xe5"
            + b"\x00" * 0x0D
        )
        image = StubImage(data)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10001, 0x10002, 0x10004, 0x10006, 0x10008],
        )

    def test_function_body_stops_at_the_object_end(self) -> None:
        blocks = {
            BASE: [
                insn(BASE, "jne", "0x10008", b"\x75\x06"),
                insn(BASE + 2, "je", "0x10010", b"\x74\x0c"),
                insn(BASE + 4, "ret", "", b"\xc3"),
            ],
            BASE + 0x08: [insn(BASE + 0x08, "ret", "", b"\xc3")],
            BASE + 0x10: [insn(BASE + 0x10, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x10)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10002, 0x10004, 0x10008],
        )

    def test_function_body_honors_the_instruction_cap(self) -> None:
        blocks = {
            BASE: [
                insn(BASE, "jne", "0x10010", b"\x75\x0e"),
                insn(BASE + 2, "nop", "", b"\x90"),
                insn(BASE + 3, "ret", "", b"\xc3"),
            ],
            BASE + 0x10: [
                insn(BASE + 0x10, "nop", "", b"\x90"),
                insn(BASE + 0x11, "ret", "", b"\xc3"),
            ],
        }
        image = StubImage(b"\x90" * 0x20)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE, 4)
        self.assertEqual(len(body), 4)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10002, 0x10003, 0x10010],
        )

    def test_function_body_keeps_the_leading_jump_target_outside_the_bound(
        self,
    ) -> None:
        blocks = {
            BASE: [insn(BASE, "jmp", "0x10020", b"\xe9\x1b\x00\x00\x00")],
            BASE + 0x20: [insn(BASE + 0x20, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x30)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE, 4000, BASE + 0x10)
        self.assertEqual(
            [item.address for item in body], [0x10000, 0x10020]
        )

    def test_function_body_caps_the_leading_jumps(self) -> None:
        blocks = {
            BASE: [insn(BASE, "jmp", "0x10010", b"\xe9\x0b\x00\x00\x00")],
            BASE + 0x10: [
                insn(BASE + 0x10, "jmp", "0x10020", b"\xe9\x0b\x00\x00\x00")
            ],
            BASE + 0x20: [
                insn(BASE + 0x20, "jmp", "0x10030", b"\xe9\x0b\x00\x00\x00")
            ],
            BASE + 0x30: [insn(BASE + 0x30, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x40)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE, 2)
        self.assertEqual(len(body), 2)
        self.assertEqual(
            [item.address for item in body], [0x10000, 0x10010]
        )

    def test_function_body_removes_a_duplicate_leading_jump(self) -> None:
        blocks = {BASE: [insn(BASE, "jmp", "0x10000", b"\xeb\xfe")]}
        image = StubImage(b"\x90" * 0x10)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual([item.address for item in body], [0x10000])

    def test_function_body_follows_a_compiler_jump_table(self) -> None:
        table = b"\x20\x00\x00\x00\x30\x00\x00\x00\x40\x00\x00\x00"
        blocks = {
            BASE: [
                insn(BASE, "cmp", "eax, 2", b"\x83\xf8\x02"),
                insn(BASE + 3, "ja", "0x10040", b"\x77\x38"),
                insn(
                    BASE + 5,
                    "jmp",
                    "dword ptr cs:[eax*4 + 0x100]",
                    b"\x2e\xff\x24\x85\x00\x01\x00\x00",
                ),
            ],
            BASE + 0x20: [insn(BASE + 0x20, "ret", "", b"\xc3")],
            BASE + 0x30: [insn(BASE + 0x30, "ret", "", b"\xc3")],
            BASE + 0x40: [insn(BASE + 0x40, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x100 + table)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10003, 0x10005, 0x10020, 0x10030, 0x10040],
        )

    def test_function_body_follows_a_pre_scaled_jump_table(self) -> None:
        table = b"\x20\x00\x00\x00\x30\x00\x00\x00\x40\x00\x00\x00"
        blocks = {
            BASE: [
                insn(BASE, "cmp", "eax, 2", b"\x83\xf8\x02"),
                insn(BASE + 3, "ja", "0x10040", b"\x77\x38"),
                insn(BASE + 5, "shl", "eax, 2", b"\xc1\xe0\x02"),
                insn(
                    BASE + 8,
                    "jmp",
                    "dword ptr cs:[eax + 0x100]",
                    b"\x2e\xff\x20",
                ),
            ],
            BASE + 0x20: [insn(BASE + 0x20, "ret", "", b"\xc3")],
            BASE + 0x30: [insn(BASE + 0x30, "ret", "", b"\xc3")],
            BASE + 0x40: [insn(BASE + 0x40, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x100 + table)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10003, 0x10005, 0x10008, 0x10020, 0x10030, 0x10040],
        )

    def test_function_body_jump_table_skips_a_target_outside_the_bound(
        self,
    ) -> None:
        table = b"\x20\x00\x00\x00\x30\x00\x00\x00\x40\x00\x00\x00"
        blocks = {
            BASE: [
                insn(BASE, "cmp", "eax, 2", b"\x83\xf8\x02"),
                insn(BASE + 3, "ja", "0x10040", b"\x77\x38"),
                insn(
                    BASE + 5,
                    "jmp",
                    "dword ptr cs:[eax*4 + 0x100]",
                    b"\x2e\xff\x24\x85\x00\x01\x00\x00",
                ),
            ],
            BASE + 0x20: [insn(BASE + 0x20, "ret", "", b"\xc3")],
            BASE + 0x30: [insn(BASE + 0x30, "ret", "", b"\xc3")],
            BASE + 0x40: [insn(BASE + 0x40, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x100 + table)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE, 4000, BASE + 0x30)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10003, 0x10005, 0x10020],
        )

    def test_function_body_jump_table_uses_the_cmp_bound(self) -> None:
        table = (
            b"\x20\x00\x00\x00\x30\x00\x00\x00"
            b"\x40\x00\x00\x00\x50\x00\x00\x00"
        )
        blocks = {
            BASE: [
                insn(BASE, "cmp", "eax, 2", b"\x83\xf8\x02"),
                insn(BASE + 3, "ja", "0x10040", b"\x77\x38"),
                insn(
                    BASE + 5,
                    "jmp",
                    "dword ptr cs:[eax*4 + 0x100]",
                    b"\x2e\xff\x24\x85\x00\x01\x00\x00",
                ),
            ],
            BASE + 0x20: [insn(BASE + 0x20, "ret", "", b"\xc3")],
            BASE + 0x30: [insn(BASE + 0x30, "ret", "", b"\xc3")],
            BASE + 0x40: [insn(BASE + 0x40, "ret", "", b"\xc3")],
            BASE + 0x50: [insn(BASE + 0x50, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x100 + table)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE, 4000, BASE + 0x60)
        self.assertEqual(
            [item.address for item in body],
            [0x10000, 0x10003, 0x10005, 0x10020, 0x10030, 0x10040],
        )

    def test_function_body_stops_at_an_indirect_jump_without_a_table(
        self,
    ) -> None:
        blocks = {
            BASE: [
                insn(
                    BASE,
                    "jmp",
                    "dword ptr cs:[eax*4 + 0x100]",
                    b"\x2e\xff\x24\x85\x00\x01\x00\x00",
                ),
            ],
            BASE + 0x10: [insn(BASE + 0x10, "ret", "", b"\xc3")],
        }
        image = StubImage(b"\x90" * 0x20)
        with mock.patch.object(le, "decode", side_effect=scripted_decode(blocks)):
            body = le.function_body(image, BASE)
        self.assertEqual([item.address for item in body], [0x10000])


class JumpTargetTestCase(unittest.TestCase):
    def test_jump_target_returns_a_direct_target(self) -> None:
        instruction = insn(BASE, "jne", "0x10010", b"\x75\x0e")
        self.assertEqual(le._jump_target(instruction), 0x10010)

    def test_jump_target_returns_none_for_a_call(self) -> None:
        instruction = insn(BASE, "call", "0x10010", b"\xe8\x0b\x00\x00\x00")
        self.assertIsNone(le._jump_target(instruction))

    def test_jump_target_returns_none_for_an_indirect_operand(self) -> None:
        instruction = insn(
            BASE,
            "jmp",
            "dword ptr cs:[eax*4 + 0x100]",
            b"\x2e\xff\x24\x85\x00\x01\x00\x00",
        )
        self.assertIsNone(le._jump_target(instruction))


class NextFunctionStartTestCase(unittest.TestCase):
    def test_next_function_start_returns_the_nearest_later_prologue(self) -> None:
        data = b"\x55\x89\xe5" + b"\x00" * 0x0D + b"\x55\x89\xe5"
        image = StubImage(data)
        self.assertEqual(le._next_function_start(image, 0x10005), 0x10010)

    def test_next_function_start_raises_for_an_unmapped_address(self) -> None:
        image = StubImage(b"\x90" * 8, objects=False)
        with self.assertRaisesRegex(ValueError, "the address is unmapped"):
            le._next_function_start(image, 0x10005)

    def test_next_function_start_raises_for_an_unmapped_range(self) -> None:
        image = UnreadableImage(b"\x90" * 8)
        with self.assertRaisesRegex(
            ValueError, "the range is not mapped to a file offset"
        ):
            le._next_function_start(image, 0x10005)


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

    def test_targets_output_lists_the_targets(self) -> None:
        image = StubImage(b"")
        with mock.patch.object(
            le, "reachable_call_targets", return_value=[0x200, 0x300]
        ):
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                self.assertEqual(le.print_targets(image, [0x100], 4000), 0)
        self.assertEqual(
            output.getvalue(),
            "call targets: 2\n"
            "  0x00000200\n"
            "  0x00000300\n",
        )


if __name__ == "__main__":
    unittest.main()
