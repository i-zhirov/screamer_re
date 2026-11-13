"""Tests for the OMF library index, the matcher, and the object verification.

The tests build small OMF libraries in a temporary directory.  They need no
toolchain and no game directory.  The capstone-dependent check skips when the
library is absent.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests -p "test_library.py"
"""
from __future__ import annotations

import hashlib
import json
import sys
import tempfile
import unittest
from datetime import date
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import le  # noqa: E402
import library as lib  # noqa: E402
import store  # noqa: E402

PAGE_LENGTH = 16


def index_field(value: int) -> bytes:
    """Encode an OMF indexed reference."""
    if value < 0x80:
        return bytes([value])
    return bytes([0x80 | (value >> 8), value & 0xFF])


def omf(command: int, payload: bytes) -> bytes:
    """Build one OMF record with a checksum."""
    header = bytes([command]) + (len(payload) + 1).to_bytes(2, "little")
    record = bytearray(header)
    record.extend(payload)
    record.append((-sum(record)) & 0xFF)
    return bytes(record)


def header_page(
    page_length: int, dictionary_offset: int, dictionary_blocks: int
) -> bytes:
    """Build the library header with a dictionary block count."""
    return (
        bytes([lib.LIBRARY_HEADER_RECORD])
        + (page_length - lib.OMF_HEADER).to_bytes(2, "little")
        + dictionary_offset.to_bytes(4, "little")
        + dictionary_blocks.to_bytes(2, "little")
    )


def theader(name: str) -> bytes:
    return omf(lib.THEADR, bytes([len(name)]) + name.encode("ascii"))


def lnames(names: list[str]) -> bytes:
    payload = bytearray()
    for name in names:
        payload.append(len(name))
        payload.extend(name.encode("ascii"))
    return omf(lib.LNAMES[0], bytes(payload))


def segdef(
    index: int,
    name_index: int,
    size: int,
    class_index: int = 1,
    record_type: int = 0x99,
    size_width: int = 4,
) -> bytes:
    del index
    payload = (
        bytes([0x49])
        + size.to_bytes(size_width, "little")
        + index_field(name_index)
        + index_field(class_index)
        + index_field(1)
    )
    return omf(record_type, payload)


def pubdef(
    segment: int,
    entries: list[tuple[str, int]],
    record_type: int = 0x91,
    offset_width: int | None = None,
    frame: int = 0,
) -> bytes:
    payload = bytearray(index_field(0))
    payload.extend(index_field(segment))
    if segment == 0:
        payload.extend(frame.to_bytes(2, "little"))
    width = lib.PUBDEF[record_type] if offset_width is None else offset_width
    for name, offset in entries:
        payload.append(len(name))
        payload.extend(name.encode("ascii"))
        payload.extend(offset.to_bytes(width, "little"))
        payload.extend(index_field(0))
    return omf(record_type, bytes(payload))


def ledata(
    segment: int,
    offset: int,
    data: bytes,
    record_type: int = 0xA1,
    offset_width: int | None = None,
) -> bytes:
    width = lib.LEDATA[record_type] if offset_width is None else offset_width
    payload = index_field(segment) + offset.to_bytes(width, "little") + data
    return omf(record_type, payload)


def iterated_block(repeat: int, data: bytes, repeat_width: int = 2) -> bytes:
    """Build one single-block iterated payload."""
    return (
        repeat.to_bytes(repeat_width, "little")
        + (0).to_bytes(2, "little")
        + bytes([len(data)])
        + data
    )


def fixupp(
    location: int,
    location_offset: int,
    target: str = "internal",
    p_bit: int = 0,
    thread: int | None = None,
) -> bytes:
    methods = {"internal": 0, "import": 2, "entry": 3}
    method = methods[target]
    locat = 0x8000 | (location << 10) | (location_offset & 0x3FF)
    payload = bytearray()
    if thread is not None:
        payload.append((method << 2) | (thread & 0x03))
        if method in (0, 1, 2):
            payload.extend(index_field(1))
        fixdat = 0x50 | (p_bit << 2) | 0x08 | (thread & 0x03)
    else:
        fixdat = 0x50 | (p_bit << 2) | method
    payload.extend(locat.to_bytes(2, "big"))
    payload.append(fixdat)
    if thread is None and method in (0, 1, 2):
        payload.extend(index_field(1))
    if not p_bit:
        payload.extend(bytes(4))
    return omf(lib.FIXUPP[1], bytes(payload))


def modend() -> bytes:
    return omf(lib.MODEND[0], b"\x00")


def library(
    modules: list[bytes], page_length: int, dictionary: bytes
) -> bytes:
    data = bytearray(header_page(page_length, 0, 0))
    data.extend(bytes(page_length - len(data)))
    position = page_length
    for module in modules:
        if len(data) < position:
            data.extend(bytes(position - len(data)))
        data.extend(module)
        length = len(data)
        position = (
            (length + page_length - 1) // page_length
        ) * page_length
    dictionary_offset = position if dictionary else 0
    if dictionary:
        data.extend(bytes(dictionary_offset - len(data)))
        data.extend(dictionary)
    dictionary_blocks = (
        (len(dictionary) + lib.DICTIONARY_BLOCK_SIZE - 1)
        // lib.DICTIONARY_BLOCK_SIZE
    )
    header = header_page(page_length, dictionary_offset, dictionary_blocks)
    data[: len(header)] = header
    return bytes(data)


def module_with_public(
    name: str,
    symbol: str,
    data: bytes,
    size: int = 0x20,
    fixups: tuple[tuple[int, int, str], ...] = (),
) -> bytes:
    """Build one module with one public symbol and optional fixups."""
    module = bytearray()
    module.extend(theader(name))
    module.extend(lnames(["", "CODE", "_TEXT"]))
    module.extend(segdef(1, 3, size, class_index=2))
    module.extend(pubdef(1, [(symbol, 0)]))
    module.extend(ledata(1, 0, data))
    for location, offset, target in fixups:
        module.extend(fixupp(location, offset, target=target, p_bit=1))
    module.extend(modend())
    return bytes(module)


def code_insn(
    address: int, mnemonic: str, operands: str, raw: bytes
) -> le.Insn:
    return le.Insn(
        address=address,
        size=len(raw),
        mnemonic=mnemonic,
        operands=operands,
        raw=raw,
    )


class FakeOriginal:
    """A fake LE image with a byte slice and a relocation list."""

    def __init__(
        self,
        data: bytes = b"",
        base: int = 0,
        relocations: list[le.Relocation] | None = None,
        path: Path | None = None,
    ) -> None:
        self.data = data
        self.base = base
        self._relocations = list(relocations or [])
        self.path = path
        self.reads: list[tuple[int, int]] = []

    def read(self, address: int, length: int) -> bytes | None:
        self.reads.append((address, length))
        offset = address - self.base
        if offset < 0:
            return None
        return self.data[offset : offset + length]

    def relocations(self) -> list[le.Relocation]:
        return list(self._relocations)


class LibraryFixture:
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def write_library(self, data: bytes, name: str = "TEST.LIB") -> Path:
        path = self.root / name
        path.write_bytes(data)
        return path

    def simple_module(
        self,
        name: str = "module",
        size: int = 0x20,
        data: bytes = b"",
        public: str | None = None,
    ) -> bytes:
        module = bytearray()
        module.extend(theader(name))
        module.extend(lnames(["", "CODE", "_TEXT"]))
        module.extend(segdef(1, 3, size, class_index=2))
        if public is not None:
            module.extend(pubdef(1, [(public, 0)]))
        if data:
            module.extend(ledata(1, 0, data))
        module.extend(modend())
        return bytes(module)


class LibraryTestCase(LibraryFixture, unittest.TestCase):
    def test_index_reads_the_library_header(self) -> None:
        dictionary = bytes(range(256)) * 2
        path = self.write_library(
            library([self.simple_module()], PAGE_LENGTH, dictionary)
        )
        data = path.read_bytes()
        self.assertEqual(data[0], 0xF0)
        self.assertEqual(int.from_bytes(data[7:9], "little"), 1)
        document = lib.read_library(path)
        self.assertEqual(document["page_length"], PAGE_LENGTH)
        self.assertEqual(document["dictionary"]["size"], 512)
        self.assertGreater(document["dictionary"]["offset"], 0)
        self.assertEqual(
            document["dictionary"]["sha256"],
            "sha256:" + hashlib.sha256(dictionary).hexdigest(),
        )

    def test_index_scales_the_dictionary_by_512_byte_blocks(self) -> None:
        dictionary = bytes(range(256)) * 4
        path = self.write_library(
            library([self.simple_module()], PAGE_LENGTH, dictionary)
        )
        data = path.read_bytes()
        self.assertEqual(int.from_bytes(data[7:9], "little"), 2)
        document = lib.read_library(path)
        self.assertEqual(document["dictionary"]["size"], 1024)
        self.assertEqual(
            document["dictionary"]["sha256"],
            "sha256:" + hashlib.sha256(dictionary).hexdigest(),
        )
        self.assertNotEqual(
            document["dictionary"]["sha256"],
            "sha256:" + hashlib.sha256(dictionary[:512]).hexdigest(),
        )

    def test_index_reads_a_module_name(self) -> None:
        path = self.write_library(
            library([self.simple_module("amodf386")], PAGE_LENGTH, b"")
        )
        document = lib.read_library(path)
        self.assertEqual(document["modules"][0]["name"], "amodf386")

    def test_index_reads_a_segment(self) -> None:
        path = self.write_library(
            library([self.simple_module("module", size=0x20)], PAGE_LENGTH, b"")
        )
        segment = lib.read_library(path)["modules"][0]["segments"][0]
        self.assertEqual(segment["index"], 1)
        self.assertEqual(segment["name"], "_TEXT")
        self.assertEqual(segment["class"], "CODE")
        self.assertEqual(segment["size"], 0x20)

    def test_index_reads_a_public_symbol(self) -> None:
        path = self.write_library(
            library([self.simple_module(public="_foo")], PAGE_LENGTH, b"")
        )
        public = lib.read_library(path)["modules"][0]["publics"][0]
        self.assertEqual(public["name"], "_foo")
        self.assertEqual(public["segment"], 1)
        self.assertEqual(public["offset"], 0)
        self.assertEqual(public["size"], 0x20)

    def test_index_reads_the_16_bit_public_offset(self) -> None:
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + pubdef(1, [("_bar", 0x10)], record_type=0x90)
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        public = lib.read_library(path)["modules"][0]["publics"][0]
        self.assertEqual(public["offset"], 0x10)

    def test_index_concatenates_the_ledata_records(self) -> None:
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 8, class_index=2)
            + ledata(1, 0, b"AB")
            + ledata(1, 2, b"CD")
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        segments = lib.read_library(path)["modules"][0]["segments"]
        self.assertEqual(len(segments), 1)
        self.assertEqual(segments[0]["data"], b"ABCD" + bytes(4))

    def test_index_expands_an_iterated_data_record(self) -> None:
        nested = (
            (2).to_bytes(4, "little")
            + (2).to_bytes(2, "little")
            + iterated_block(1, b"A", repeat_width=4)
            + iterated_block(2, b"B", repeat_width=4)
        )
        payload = index_field(1) + (0).to_bytes(4, "little") + nested
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + omf(0xA3, payload)
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        segment = lib.read_library(path)["modules"][0]["segments"][0]
        self.assertEqual(segment["data"][:6], b"ABBABB")

    def test_index_reads_the_pharlap_32_bit_fields(self) -> None:
        module = (
            theader("pharlap")
            + omf(0x88, bytes([0x00, 0xAA]))
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2, record_type=0x98)
            + pubdef(1, [("_phar", 0x10)], record_type=0x90, offset_width=4)
            + ledata(1, 0x08, b"AB", record_type=0xA0, offset_width=4)
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        document = lib.read_library(path)["modules"][0]
        self.assertEqual(document["segments"][0]["size"], 0x20)
        self.assertEqual(document["publics"][0]["offset"], 0x10)
        self.assertEqual(document["segments"][0]["data"][0x08:0x0A], b"AB")

    def test_index_skips_the_pharlap_4_byte_fixup_displacement(self) -> None:
        locat = 0x8000 | (9 << 10) | 4
        payload = (
            locat.to_bytes(2, "big")
            + bytes([0x50])
            + index_field(1)
            + b"\x00\x00\x80\x00"
        )
        module = (
            theader("pharlap")
            + omf(0x88, bytes([0x00, 0xAA]))
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2, record_type=0x98)
            + ledata(1, 0, bytes(8), record_type=0xA0, offset_width=4)
            + omf(0x9C, payload)
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        fixup = lib.read_library(path)["modules"][0]["fixups"][0]
        self.assertEqual(fixup["offset"], 4)
        self.assertEqual(fixup["width"], 4)

    def test_index_reads_a_fixup_slot(self) -> None:
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + ledata(1, 0, bytes(8))
            + fixupp(9, 4, target="internal", p_bit=1)
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        fixup = lib.read_library(path)["modules"][0]["fixups"][0]
        self.assertEqual(fixup["offset"], 4)
        self.assertEqual(fixup["width"], lib.FIXUP_WIDTHS[9])
        self.assertEqual(fixup["location"], 9)

    def test_index_reads_a_thread_subrecord(self) -> None:
        thread = bytes([0x08]) + index_field(1)
        locat = 0x8000 | (9 << 10) | 4
        payload = (
            thread
            + locat.to_bytes(2, "big")
            + bytes([0x50 | (1 << 2) | 0x08])
        )
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + ledata(1, 0, bytes(8))
            + omf(lib.FIXUPP[1], payload)
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        fixup = lib.read_library(path)["modules"][0]["fixups"][0]
        self.assertEqual(fixup["offset"], 4)
        self.assertEqual(fixup["target"], "import")

    def test_index_reads_an_internal_fixup(self) -> None:
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + ledata(1, 0, bytes(8))
            + fixupp(1, 2, target="internal", p_bit=1)
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        fixup = lib.read_library(path)["modules"][0]["fixups"][0]
        self.assertEqual(fixup["target"], "internal")

    def test_index_reads_an_import_fixup(self) -> None:
        thread = bytes([0x08]) + index_field(1)
        locat = 0x8000 | (9 << 10) | 4
        thread_fixup = omf(
            lib.FIXUPP[1],
            thread + locat.to_bytes(2, "big") + bytes([0x50 | (1 << 2) | 0x08]),
        )
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + ledata(1, 0, bytes(8))
            + fixupp(9, 0, target="import", p_bit=1)
            + thread_fixup
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        fixups = lib.read_library(path)["modules"][0]["fixups"]
        self.assertEqual(
            [fixup["target"] for fixup in fixups], ["import", "import"]
        )

    def test_index_rejects_an_unknown_fixup_location(self) -> None:
        module = (
            theader("badmod")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + ledata(1, 0, bytes(8))
            + fixupp(8, 4, target="internal", p_bit=1)
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        with self.assertRaises(ValueError) as caught:
            lib.read_library(path)
        message = str(caught.exception)
        self.assertIn(str(path), message)
        self.assertIn("badmod", message)
        self.assertIn("0x4", message)

    def test_index_skips_the_dictionary(self) -> None:
        path = self.write_library(
            library(
                [self.simple_module()],
                PAGE_LENGTH,
                b"\x80\x00\x00trailing",
            )
        )
        document = lib.read_library(path)
        self.assertEqual(len(document["modules"]), 1)
        self.assertGreater(document["dictionary"]["offset"], 0)

    def test_index_sizes_a_public_at_the_next_symbol(self) -> None:
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + pubdef(1, [("_first", 0), ("_second", 0x10)])
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        publics = lib.read_library(path)["modules"][0]["publics"]
        self.assertEqual([public["size"] for public in publics], [0x10, 0x10])

    def test_index_sizes_the_last_public_at_the_segment_end(self) -> None:
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + pubdef(1, [("_last", 0x18)])
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        public = lib.read_library(path)["modules"][0]["publics"][0]
        self.assertEqual(public["size"], 8)

    def test_index_drops_an_absolute_public_from_the_entries(self) -> None:
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + pubdef(1, [("_foo", 0)])
            + pubdef(0, [("_fltused_", 0)])
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        index = lib.build_index([path])
        self.assertEqual(
            [entry["symbol"] for entry in index["entries"]], ["_foo"]
        )
        self.assertEqual(index["libraries"][0]["symbol_count"], 1)

    def test_index_rejects_a_bad_library_header(self) -> None:
        path = self.write_library(b"\x00" + bytes(15))
        with self.assertRaises(ValueError):
            lib.read_library(path)

    def test_index_rejects_a_truncated_record(self) -> None:
        data = library([self.simple_module()], PAGE_LENGTH, b"")
        path = self.write_library(data[:-1])
        with self.assertRaises(ValueError):
            lib.read_library(path)

    def test_index_rejects_a_malformed_iterated_block(self) -> None:
        module = (
            theader("module")
            + omf(0xA3, bytes(6))
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        with self.assertRaises(ValueError):
            lib.read_library(path)

    def test_index_is_stable(self) -> None:
        module = (
            theader("module")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + pubdef(1, [("_foo", 0)])
            + ledata(1, 0, bytes(4))
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        first = lib.build_index([path])
        second = lib.build_index([path])
        self.assertEqual(
            json.dumps(first, sort_keys=True),
            json.dumps(second, sort_keys=True),
        )
        self.assertEqual(first["entries"], second["entries"])

    def test_index_uses_the_common_directory_of_the_libraries(self) -> None:
        parent = self.root / "LIB386"
        nested = parent / "DOS"
        nested.mkdir(parents=True)
        module_data = library([self.simple_module()], PAGE_LENGTH, b"")
        clib = nested / "CLIB3R.LIB"
        emu = nested / "EMU387.LIB"
        math = parent / "MATH387R.LIB"
        for path in (clib, emu, math):
            path.write_bytes(module_data)
        index = lib.build_index([clib, emu, math])
        self.assertEqual(index["directory"], str(parent))

    def test_load_index_reads_the_document(self) -> None:
        path = self.write_library(library([self.simple_module()], PAGE_LENGTH, b""))
        document = lib.build_index([path])
        index_path = self.root / "index.json"
        index_path.write_text(json.dumps(document))
        loaded = lib.load_index(index_path)
        self.assertEqual(
            set(loaded), {"directory", "libraries", "entries"}
        )

    def test_load_index_rejects_a_missing_file(self) -> None:
        path = self.root / "missing.json"
        with self.assertRaises(ValueError) as caught:
            lib.load_index(path)
        self.assertIn(str(path), str(caught.exception))

    def test_load_index_rejects_a_missing_entry_key(self) -> None:
        entry = {
            "file": "TEST.LIB",
            "sha256": "sha256:" + "0" * 64,
            "module": "module",
            "module_offset": 0x10,
            "symbol": "_foo",
            "segment": 1,
            "offset": 0,
            "size": 4,
            "prefix_sha256": "sha256:" + "0" * 64,
            "fixups": [],
        }
        document = {"directory": ".", "libraries": [], "entries": [entry]}
        path = self.root / "index.json"
        path.write_text(json.dumps(document))
        with self.assertRaises(ValueError):
            lib.load_index(path)


class LibraryMatchTestCase(LibraryFixture, unittest.TestCase):
    def build(self, modules: list[bytes], name: str = "TEST.LIB"):
        path = self.write_library(library(modules, PAGE_LENGTH, b""), name)
        return path, lib.read_library(path), lib.build_index([path])

    def verify_inputs(
        self,
        data: bytes = bytes(0x20),
        fixups: tuple[tuple[int, int, str], ...] = (),
        record: dict | None = None,
        size: int = 0x20,
    ) -> tuple[dict, dict, dict]:
        path = self.write_library(
            library(
                [
                    module_with_public(
                        "mod", "func", data, size=size, fixups=fixups
                    )
                ],
                PAGE_LENGTH,
                b"",
            )
        )
        document = lib.read_library(path)
        index = lib.build_index([path])
        field = lib.library_field_from_entry(index["entries"][0])
        if record is None:
            record = {"address": "0x1000", "size": 0x20}
        return record, document, field

    def test_candidates_prefer_the_record_symbol(self) -> None:
        other = module_with_public("other", "_other", bytes(0x20))
        exact = module_with_public("exact", "exact_symbol", bytes(0x20))
        _, _, index = self.build([other, exact])
        record = {
            "address": "0x1000",
            "size": 0x20,
            "symbol": "exact_symbol",
            "name": "exact",
        }
        result = lib.candidates(index, record, FakeOriginal(base=0x1000))
        self.assertEqual(result[0]["symbol"], "exact_symbol")

    def test_candidates_add_the_watcom_underscore(self) -> None:
        module = module_with_public("nmalloc", "malloc_", bytes(0x20))
        _, _, index = self.build([module])
        record = {"address": "0x1000", "size": 0x20, "name": "malloc"}
        result = lib.candidates(index, record, FakeOriginal(base=0x1000))
        self.assertEqual(
            [entry["symbol"] for entry in result], ["malloc_"]
        )

    def test_candidates_fall_back_to_the_prefix_digest(self) -> None:
        data = b"\x55\x89\xe5" + bytes([0x11, 0x22, 0x33, 0x44]) + bytes(0x19)
        module = module_with_public(
            "mod", "hidden", data, fixups=((9, 3, "import"),)
        )
        _, _, index = self.build([module])
        original_data = (
            b"\x55\x89\xe5" + bytes([0xAA, 0xBB, 0xCC, 0xDD]) + bytes(0x19)
        )
        original = FakeOriginal(
            original_data,
            base=0x1000,
            relocations=[
                le.Relocation(
                    offset=0x1003,
                    width=4,
                    kind="import-name",
                    module=1,
                    name_offset=4,
                )
            ],
        )
        record = {"address": "0x1000", "size": 0x20}
        result = lib.candidates(index, record, original)
        self.assertEqual([entry["symbol"] for entry in result], ["hidden"])
        self.assertIn((0x1000, lib.PREFIX_BYTES), original.reads)

    def test_candidates_zero_the_object_fixups_in_the_prefix(self) -> None:
        data = b"\x53\x31\xdb\xe8" + bytes(4) + b"\x5b\xc3" + bytes(0x16)
        module = module_with_public(
            "mod", "hidden", data, fixups=((9, 4, "internal"),)
        )
        _, _, index = self.build([module])
        original_data = (
            b"\x53\x31\xdb\xe8" + b"\xdd\xff\xff\xff" + b"\x5b\xc3" + bytes(0x16)
        )
        original = FakeOriginal(original_data, base=0x1000)
        record = {"address": "0x1000", "size": 0x20}
        result = lib.candidates(index, record, original)
        self.assertEqual([entry["symbol"] for entry in result], ["hidden"])

    def test_candidates_reject_a_prefix_difference_outside_the_fixups(self) -> None:
        data = b"\x53\x31\xdb\xe8" + bytes(4) + b"\x5b\xc3" + bytes(0x16)
        module = module_with_public(
            "mod", "hidden", data, fixups=((9, 4, "internal"),)
        )
        _, _, index = self.build([module])
        original_data = (
            b"\x53\x31\xdc\xe8" + b"\xdd\xff\xff\xff" + b"\x5b\xc3" + bytes(0x16)
        )
        original = FakeOriginal(original_data, base=0x1000)
        record = {"address": "0x1000", "size": 0x20}
        result = lib.candidates(index, record, original)
        self.assertEqual(result, [])

    def test_candidates_skip_a_prefix_covered_by_fixups(self) -> None:
        data = bytes(0x20)
        module = module_with_public(
            "mod",
            "hidden",
            data,
            fixups=(
                (9, 0, "internal"),
                (9, 4, "internal"),
                (9, 8, "internal"),
                (9, 12, "internal"),
            ),
        )
        _, _, index = self.build([module])
        original = FakeOriginal(bytes(0x20), base=0x1000)
        record = {"address": "0x1000", "size": 0x20}
        self.assertEqual(lib.candidates(index, record, original), [])

    def test_candidates_are_unique_and_ordered(self) -> None:
        first = module_with_public("amod", "foo_", bytes(0x20))
        second = module_with_public("bmod", "foo_", bytes(0x20))
        _, _, index = self.build([first, second])
        record = {
            "address": "0x1000",
            "size": 0x20,
            "symbol": "foo_",
            "name": "foo",
        }
        result = lib.candidates(index, record, FakeOriginal(base=0x1000))
        matches = [entry for entry in result if entry["symbol"] == "foo_"]
        self.assertEqual([entry["module"] for entry in matches], ["amod", "bmod"])
        keys = [
            (entry["file"], entry["module"], entry["symbol"])
            for entry in result
        ]
        self.assertEqual(len(keys), len(set(keys)))

    def test_candidates_keep_the_group_order(self) -> None:
        prefix = module_with_public("pmod", "pref", bytes([0x90]) * 0x20)
        name = module_with_public("nmod", "bar", bytes([0xCC]) * 0x20)
        underscore = module_with_public("umod", "bar_", bytes([0xCC]) * 0x20)
        exact = module_with_public("emod", "exact", bytes([0xCC]) * 0x20)
        _, _, index = self.build([prefix, name, underscore, exact])
        record = {
            "address": "0x1000",
            "size": 0x20,
            "symbol": "exact",
            "name": "bar",
        }
        original = FakeOriginal(bytes([0x90]) * 0x20, base=0x1000)
        result = lib.candidates(index, record, original)
        self.assertEqual(
            [entry["symbol"] for entry in result],
            ["exact", "bar_", "bar", "pref"],
        )

    def test_library_field_from_entry_keeps_the_five_keys(self) -> None:
        path, _, index = self.build(
            [module_with_public("mod", "func", bytes(0x20))]
        )
        entry = index["entries"][0]
        field = lib.library_field_from_entry(entry)
        self.assertEqual(
            set(field),
            {"file", "sha256", "module", "symbol", "object_sha256"},
        )
        self.assertEqual(field["sha256"], index["libraries"][0]["sha256"])
        self.assertEqual(field["object_sha256"], entry["object_sha256"])

    def test_resolve_entry_rebuilds_the_public_extent(self) -> None:
        module = (
            theader("two")
            + lnames(["", "CODE", "_TEXT"])
            + segdef(1, 3, 0x20, class_index=2)
            + pubdef(1, [("_first", 0), ("_second", 0x10)])
            + ledata(1, 0, bytes(0x20))
            + modend()
        )
        path = self.write_library(library([module], PAGE_LENGTH, b""))
        document = lib.read_library(path)
        index = lib.build_index([path])
        field = lib.library_field_from_entry(index["entries"][0])
        entry = lib.resolve_entry(document, field)
        self.assertEqual(entry["module"], "two")
        self.assertEqual(entry["segment"], 1)
        self.assertEqual(entry["offset"], 0)
        self.assertEqual(entry["size"], 0x10)

    def test_resolve_entry_rebuilds_the_fixups(self) -> None:
        _, document, field = self.verify_inputs(
            fixups=((9, 4, "import"),)
        )
        entry = lib.resolve_entry(document, field)
        self.assertEqual(
            entry["fixups"],
            [
                {
                    "offset": 4,
                    "width": 4,
                    "location": 9,
                    "target": "import",
                }
            ],
        )

    def test_resolve_entry_rejects_a_missing_symbol(self) -> None:
        _, document, field = self.verify_inputs()
        missing = dict(field)
        missing["symbol"] = "_missing"
        with self.assertRaises(ValueError) as caught:
            lib.resolve_entry(document, missing)
        message = str(caught.exception)
        self.assertIn("mod", message)
        self.assertIn("_missing", message)

    def test_object_sha256_zeroes_the_fixup_slots(self) -> None:
        data = bytes(range(0x20))
        path, document, index = self.build(
            [module_with_public("mod", "func", data, fixups=((9, 4, "internal"),))]
        )
        expected = bytearray(data)
        expected[4:8] = bytes(4)
        digest = "sha256:" + hashlib.sha256(bytes(expected)).hexdigest()
        self.assertEqual(index["entries"][0]["object_sha256"], digest)
        field = lib.library_field_from_entry(index["entries"][0])
        self.assertEqual(lib.resolve_entry(document, field)["object_sha256"], digest)

    def test_prefix_sha256_zeroes_the_prefix_fixups(self) -> None:
        data = bytes(range(0x20))
        path, document, index = self.build(
            [module_with_public("mod", "func", data, fixups=((9, 4, "internal"),))]
        )
        expected = bytearray(data[: lib.PREFIX_BYTES])
        expected[4:8] = bytes(4)
        digest = "sha256:" + hashlib.sha256(bytes(expected)).hexdigest()
        self.assertEqual(index["entries"][0]["prefix_sha256"], digest)
        field = lib.library_field_from_entry(index["entries"][0])
        self.assertEqual(lib.resolve_entry(document, field)["prefix_sha256"], digest)

    def test_source_digest_is_the_canonical_five_key_object(self) -> None:
        field = {
            "file": "TEST.LIB",
            "sha256": "sha256:" + "1" * 64,
            "module": "mod",
            "symbol": "func",
            "object_sha256": "sha256:" + "2" * 64,
        }
        canonical = json.dumps(
            field, sort_keys=True, separators=(",", ":")
        ).encode("utf-8")
        expected = "sha256:" + hashlib.sha256(canonical).hexdigest()
        self.assertEqual(lib.source_digest(field), expected)

    def test_verify_entry_accepts_the_record_field_shape(self) -> None:
        record, document, field = self.verify_inputs()
        body = [code_insn(0x1000, "push", "ebp", b"\x55")]
        original = FakeOriginal()
        with mock.patch.object(le, "decode_bytes", return_value=body), mock.patch.object(
            lib, "object_body", return_value=body
        ):
            result = lib.verify_entry(record, original, document, field)
        self.assertEqual(result["match"], "exact-bytes")

    def test_verify_entry_accepts_the_index_field_shape(self) -> None:
        record, document, _ = self.verify_inputs()
        path = self.root / "TEST.LIB"
        index = lib.build_index([path])
        field = index["entries"][0]
        body = [code_insn(0x1000, "push", "ebp", b"\x55")]
        original = FakeOriginal()
        with mock.patch.object(le, "decode_bytes", return_value=body), mock.patch.object(
            lib, "object_body", return_value=body
        ):
            result = lib.verify_entry(record, original, document, field)
        self.assertEqual(result["match"], "exact-bytes")
        self.assertEqual(result["verdict"], "pass")

    def test_verify_entry_rejects_a_file_digest_mismatch(self) -> None:
        record, document, field = self.verify_inputs()
        changed = dict(field)
        changed["sha256"] = "sha256:" + "0" * 64
        with self.assertRaises(ValueError):
            lib.verify_entry(record, FakeOriginal(), document, changed)

    def test_verify_entry_rejects_an_object_digest_mismatch(self) -> None:
        record, document, field = self.verify_inputs()
        changed = dict(field)
        changed["object_sha256"] = "sha256:" + "0" * 64
        with self.assertRaises(ValueError):
            lib.verify_entry(record, FakeOriginal(), document, changed)

    def test_format_verify_prints_the_masked_counts(self) -> None:
        record, document, field = self.verify_inputs()
        body = [code_insn(0x1000, "push", "ebp", b"\x55")]
        original = FakeOriginal()
        with mock.patch.object(le, "decode_bytes", return_value=body), mock.patch.object(
            lib, "object_body", return_value=body
        ):
            line = lib.format_verify(record, original, document, field)
        self.assertIn("0x1000", line)
        self.assertIn("pass", line)
        self.assertIn("original_masked=0", line)
        self.assertIn("object_masked=0", line)

    def test_format_verify_does_not_change_the_check(self) -> None:
        record, document, field = self.verify_inputs()
        body = [code_insn(0x1000, "push", "ebp", b"\x55")]
        original = FakeOriginal()
        with mock.patch.object(le, "decode_bytes", return_value=body), mock.patch.object(
            lib, "object_body", return_value=body
        ):
            check = lib.verify_entry(record, original, document, field)
            lib.format_verify(record, original, document, field)
        self.assertEqual(
            set(check),
            {
                "verdict",
                "detail",
                "match",
                "bytes_equal",
                "bytes_total",
                "instructions_equal",
                "instructions_total",
            },
        )

    def test_verify_reports_exact_relocations(self) -> None:
        data = b"\xe8" + bytes([0x11, 0x22, 0x33, 0x44]) + bytes(0x1B)
        record, document, field = self.verify_inputs(
            data=data, fixups=((9, 1, "import"),)
        )
        original_body = [
            code_insn(0x1000, "call", "0x44332211", b"\xe8\x11\x22\x33\x44")
        ]
        object_body = [
            code_insn(0x1000, "call", "0x88776655", b"\xe8\x55\x66\x77\x88")
        ]
        original = FakeOriginal(
            relocations=[
                le.Relocation(
                    offset=0x1001,
                    width=4,
                    kind="import-ordinal",
                    module=1,
                    ordinal=2,
                )
            ]
        )
        with mock.patch.object(
            le, "decode_bytes", return_value=original_body
        ), mock.patch.object(lib, "object_body", return_value=object_body):
            result = lib.verify_entry(record, original, document, field)
        self.assertEqual(result["match"], "exact-relocations")
        self.assertEqual(result["verdict"], "pass")

    def test_verify_reports_exact_bytes(self) -> None:
        record, document, field = self.verify_inputs()
        body = [code_insn(0x1000, "push", "ebp", b"\x55")]
        with mock.patch.object(le, "decode_bytes", return_value=body), mock.patch.object(
            lib, "object_body", return_value=body
        ):
            result = lib.verify_entry(
                record, FakeOriginal(), document, field
            )
        self.assertEqual(result["match"], "exact-bytes")
        self.assertEqual(result["verdict"], "pass")

    def test_verify_rejects_an_empty_extent(self) -> None:
        record, document, field = self.verify_inputs(
            data=b"", size=0, record={"address": "0x1000", "size": 0}
        )
        with mock.patch.object(
            le, "function_body", return_value=[]
        ), mock.patch.object(lib, "object_body", return_value=[]):
            result = lib.verify_entry(
                record, FakeOriginal(), document, field
            )
        self.assertEqual(result["match"], "different")
        self.assertEqual(result["verdict"], "fail")

    def test_verify_rejects_a_non_relocation_difference(self) -> None:
        record, document, field = self.verify_inputs(
            data=b"\xb8\x01\x00\x00\x00" + bytes(0x1B)
        )
        original_body = [
            code_insn(0x1000, "mov", "eax, 1", b"\xb8\x01\x00\x00\x00")
        ]
        object_body = [
            code_insn(0x1000, "mov", "eax, 2", b"\xb8\x02\x00\x00\x00")
        ]
        with mock.patch.object(
            le, "decode_bytes", return_value=original_body
        ), mock.patch.object(lib, "object_body", return_value=object_body):
            result = lib.verify_entry(
                record, FakeOriginal(), document, field
            )
        self.assertNotIn(result["match"], lib.STRICT_MATCHES)
        self.assertEqual(result["verdict"], "fail")

    def test_verify_rejects_a_shape_match(self) -> None:
        record, document, field = self.verify_inputs()
        original_body = [
            code_insn(0x1000, "add", "eax, 1", b"\x83\xc0\x01")
        ]
        object_body = [
            code_insn(0x1000, "add", "eax, 0x100", b"\x81\xc0\x00\x01\x00\x00")
        ]
        with mock.patch.object(
            le, "decode_bytes", return_value=original_body
        ), mock.patch.object(lib, "object_body", return_value=object_body):
            result = lib.verify_entry(
                record, FakeOriginal(), document, field
            )
        self.assertEqual(result["match"], "same-shape")
        self.assertEqual(result["verdict"], "fail")

    def test_verify_masks_the_object_fixup_slots(self) -> None:
        data = b"\xb8" + bytes([0x11, 0x22, 0x33, 0x44]) + bytes(0x1B)
        record, document, field = self.verify_inputs(
            data=data, fixups=((9, 1, "internal"),)
        )
        original_body = [
            code_insn(0x1000, "mov", "eax, 0x44332211", b"\xb8\x11\x22\x33\x44")
        ]
        object_body = [
            code_insn(0x1000, "mov", "eax, 0x88776655", b"\xb8\x55\x66\x77\x88")
        ]
        original = FakeOriginal(
            relocations=[
                le.Relocation(
                    offset=0x1001, width=4, kind="internal", object=1, target=0
                )
            ]
        )
        with mock.patch.object(
            le, "decode_bytes", return_value=original_body
        ), mock.patch.object(lib, "object_body", return_value=object_body):
            result = lib.verify_entry(record, original, document, field)
        self.assertEqual(result["match"], "exact-relocations")
        self.assertEqual(result["verdict"], "pass")

    def test_verify_accepts_an_object_only_fixup(self) -> None:
        data = b"\xe8" + bytes([0x11, 0x22, 0x33, 0x44]) + bytes(0x1B)
        record, document, field = self.verify_inputs(
            data=data, fixups=((9, 1, "internal"),)
        )
        original_body = [
            code_insn(0x1000, "call", "0x44332211", b"\xe8\x11\x22\x33\x44")
        ]
        object_body = [
            code_insn(0x1000, "call", "0x88776655", b"\xe8\x55\x66\x77\x88")
        ]
        with mock.patch.object(
            le, "decode_bytes", return_value=original_body
        ), mock.patch.object(lib, "object_body", return_value=object_body):
            result = lib.verify_entry(
                record, FakeOriginal(), document, field
            )
        self.assertEqual(result["match"], "exact-relocations")
        self.assertEqual(result["verdict"], "pass")

    def test_verify_rejects_an_original_only_slot(self) -> None:
        record, document, field = self.verify_inputs()
        body = [
            code_insn(0x1000, "mov", "eax, 0x44332211", b"\xb8\x11\x22\x33\x44")
        ]
        original = FakeOriginal(
            relocations=[
                le.Relocation(
                    offset=0x1001, width=4, kind="internal", object=1, target=0
                )
            ]
        )
        with mock.patch.object(
            le, "decode_bytes", return_value=body
        ), mock.patch.object(lib, "object_body", return_value=list(body)):
            result = lib.verify_entry(record, original, document, field)
        self.assertEqual(result["match"], "different")
        self.assertEqual(result["verdict"], "fail")

    def test_verify_rejects_a_slot_width_mismatch(self) -> None:
        data = b"\xb8" + bytes([0x11, 0x22]) + bytes(0x1D)
        record, document, field = self.verify_inputs(
            data=data, fixups=((2, 1, "internal"),)
        )
        body = [
            code_insn(0x1000, "mov", "eax, 0x44332211", b"\xb8\x11\x22\x33\x44")
        ]
        original = FakeOriginal(
            relocations=[
                le.Relocation(
                    offset=0x1001, width=4, kind="internal", object=1, target=0
                )
            ]
        )
        with mock.patch.object(
            le, "decode_bytes", return_value=body
        ), mock.patch.object(lib, "object_body", return_value=list(body)):
            result = lib.verify_entry(record, original, document, field)
        self.assertEqual(result["match"], "different")
        self.assertEqual(result["verdict"], "fail")

    def test_verify_accepts_an_import_slot_with_a_different_target(self) -> None:
        data = b"\xe8" + bytes([0x11, 0x22, 0x33, 0x44]) + bytes(0x1B)
        record, document, field = self.verify_inputs(
            data=data, fixups=((9, 1, "import"),)
        )
        original_body = [
            code_insn(0x1000, "call", "0x44332211", b"\xe8\x11\x22\x33\x44")
        ]
        object_body = [
            code_insn(0x1000, "call", "0x88776655", b"\xe8\x55\x66\x77\x88")
        ]
        original = FakeOriginal(
            relocations=[
                le.Relocation(
                    offset=0x1001,
                    width=4,
                    kind="import-ordinal",
                    module=1,
                    ordinal=7,
                )
            ]
        )
        with mock.patch.object(
            le, "decode_bytes", return_value=original_body
        ), mock.patch.object(lib, "object_body", return_value=object_body):
            result = lib.verify_entry(record, original, document, field)
        self.assertEqual(result["match"], "exact-relocations")
        self.assertEqual(result["verdict"], "pass")

    def test_normalize_imports_masks_an_ordinal(self) -> None:
        slot = le.Relocation(
            offset=0x1001,
            width=4,
            kind="import-ordinal",
            module=2,
            ordinal=3,
        )
        result = lib.normalize_imports([slot])
        self.assertEqual(result[0].kind, "import")
        self.assertEqual(result[0].offset, 0x1001)
        self.assertEqual(result[0].width, 4)
        self.assertIsNone(result[0].module)
        self.assertIsNone(result[0].ordinal)

    def test_normalize_imports_masks_a_name(self) -> None:
        slot = le.Relocation(
            offset=0x1002,
            width=2,
            kind="import-name",
            module=1,
            name_offset=9,
        )
        result = lib.normalize_imports([slot])
        self.assertEqual(result[0].kind, "import")
        self.assertEqual(result[0].offset, 0x1002)
        self.assertEqual(result[0].width, 2)
        self.assertIsNone(result[0].module)
        self.assertIsNone(result[0].name_offset)

    def test_normalize_imports_keeps_an_internal_slot(self) -> None:
        slot = le.Relocation(
            offset=0x1003,
            width=2,
            kind="internal",
            object=1,
            target=4,
        )
        result = lib.normalize_imports([slot])
        self.assertEqual(result[0].kind, "internal")
        self.assertEqual(result[0].offset, 0x1003)
        self.assertEqual(result[0].width, 2)

    def test_verify_counts_the_masked_bytes(self) -> None:
        data = b"\xe8\x01\x02\x03\x04" + b"\xb8\x11\x22\x33\x44" + bytes(0x16)
        record, document, field = self.verify_inputs(
            data=data, fixups=((9, 6, "internal"),)
        )
        original_body = [
            code_insn(0x1000, "call", "0x04030201", b"\xe8\x01\x02\x03\x04"),
            code_insn(0x1005, "mov", "eax, 0x44332211", b"\xb8\x11\x22\x33\x44"),
        ]
        object_body = [
            code_insn(0x1000, "call", "0x08070605", b"\xe8\x05\x06\x07\x08"),
            code_insn(0x1005, "mov", "eax, 0x88776655", b"\xb8\x55\x66\x77\x88"),
        ]
        original = FakeOriginal(
            relocations=[
                le.Relocation(
                    offset=0x1006, width=4, kind="internal", object=1, target=0
                )
            ]
        )
        with mock.patch.object(
            le, "decode_bytes", return_value=original_body
        ), mock.patch.object(lib, "object_body", return_value=object_body):
            line = lib.format_verify(record, original, document, field)
        self.assertIn("original_masked=8", line)
        self.assertIn("object_masked=8", line)

    def test_verify_decodes_the_original_instruction_count(self) -> None:
        record, document, field = self.verify_inputs()
        original_body = [
            code_insn(0x1000, "push", "ebp", b"\x55"),
            code_insn(0x1001, "ret", "", b"\xc3"),
        ]
        object_body = list(original_body)
        with mock.patch.object(
            le, "decode_bytes", return_value=original_body
        ), mock.patch.object(
            lib, "object_body", return_value=object_body
        ) as decode:
            lib.verify_entry(record, FakeOriginal(), document, field)
        self.assertEqual(decode.call_args.args[3], len(original_body))

    @unittest.skipUnless(
        le.capstone_available(), "capstone is not available"
    )
    def test_object_body_is_linear_for_a_jump_table(self) -> None:
        data = (
            b"\xff\x24\x85\x00\x10\x00\x00"
            + (0x1000).to_bytes(4, "little")
            + (0x1006).to_bytes(4, "little")
            + bytes(0x12)
        )
        path, document, index = self.build(
            [module_with_public("mod", "func", data, size=len(data))]
        )
        field = lib.library_field_from_entry(index["entries"][0])
        entry = lib.resolve_entry(document, field)
        result = lib.object_body(entry, document, 0x1000, 3)
        self.assertEqual(result[0].mnemonic, "jmp")
        self.assertEqual(result[1].address, 0x1007)
        self.assertEqual(result[1].mnemonic, "add")
        self.assertEqual(result[1].raw, data[7:9])

    def test_verify_uses_the_entry_size_as_the_end(self) -> None:
        record, document, field = self.verify_inputs(
            data=bytes(0x30),
            record={"address": "0x1000", "size": 0x40},
            size=0x30,
        )
        body = [code_insn(0x1000, "push", "ebp", b"\x55")]
        original = FakeOriginal(bytes(0x30), base=0x1000)
        with mock.patch.object(
            le, "decode_bytes", return_value=body
        ) as decode, mock.patch.object(
            lib, "object_body", return_value=body
        ):
            lib.verify_entry(record, original, document, field)
        self.assertEqual(decode.call_args.args[0], bytes(0x30))
        self.assertEqual(decode.call_args.args[1], 0x1000)
        self.assertEqual(decode.call_args.kwargs["end"], 0x1030)

    @unittest.skipUnless(
        le.capstone_available(), "capstone is not available"
    )
    def test_verify_compares_the_byte_after_a_ret(self) -> None:
        data = b"\xc3\xb8\x01\x00\x00\x00" + bytes(0x1A)
        record, document, field = self.verify_inputs(data=data)
        original = FakeOriginal(
            b"\xc3\xb8\x02\x00\x00\x00" + bytes(0x1A), base=0x1000
        )
        result = lib.verify_entry(record, original, document, field)
        self.assertNotIn(result["match"], lib.STRICT_MATCHES)
        self.assertEqual(result["verdict"], "fail")
        self.assertGreater(result["instructions_total"], 1)

    def test_verify_returns_none_without_a_candidate(self) -> None:
        module = module_with_public(
            "mod", "unrelated", bytes([0x11]) * 0x20
        )
        _, _, index = self.build([module])
        record = {"address": "0x1000", "size": 0x20}
        original = FakeOriginal(bytes([0x00]) * 0x20, base=0x1000)
        self.assertIsNone(
            lib.verify(record, original, index, self.root)
        )

    def test_verify_prefers_the_first_strict_pass(self) -> None:
        first = module_with_public(
            "amod",
            "foo_",
            b"\xe8" + bytes([0x11, 0x22, 0x33, 0x44]) + bytes(0x1B),
            fixups=((9, 1, "import"),),
        )
        second = module_with_public(
            "bmod",
            "foo_",
            b"\xe8" + bytes([0x11, 0x22, 0x33, 0x44]) + bytes(0x1B),
            fixups=((9, 1, "import"),),
        )
        _, _, index = self.build([first, second])
        record = {"address": "0x1000", "size": 0x20, "name": "foo"}
        original_body = [
            code_insn(0x1000, "call", "0x44332211", b"\xe8\x11\x22\x33\x44")
        ]
        first_body = list(original_body)
        second_body = [
            code_insn(0x1000, "call", "0x88776655", b"\xe8\x55\x66\x77\x88")
        ]
        original = FakeOriginal(
            relocations=[
                le.Relocation(
                    offset=0x1001,
                    width=4,
                    kind="import-ordinal",
                    module=1,
                    ordinal=2,
                )
            ]
        )
        with mock.patch.object(
            le, "decode_bytes", return_value=original_body
        ), mock.patch.object(
            lib, "object_body", side_effect=[first_body, second_body]
        ):
            result = lib.verify_candidate(record, original, index, self.root)
        self.assertEqual(result[0]["module"], "amod")
        self.assertEqual(result[1]["match"], "exact-bytes")

    def test_verify_returns_the_first_candidate_when_none_passes(self) -> None:
        first = module_with_public("amod", "foo_", bytes(0x20))
        second = module_with_public("bmod", "foo_", bytes(0x20))
        _, _, index = self.build([first, second])
        record = {"address": "0x1000", "size": 0x20, "name": "foo"}
        original_body = [code_insn(0x1000, "push", "ebp", b"\x55")]
        first_body = [code_insn(0x1000, "push", "eax", b"\x50")]
        second_body = [code_insn(0x1000, "push", "ebx", b"\x53")]
        with mock.patch.object(
            le, "decode_bytes", return_value=original_body
        ), mock.patch.object(
            lib, "object_body", side_effect=[first_body, second_body]
        ):
            result = lib.verify_candidate(
                record, FakeOriginal(), index, self.root
            )
        self.assertEqual(result[0]["module"], "amod")
        self.assertNotIn(result[1]["match"], lib.STRICT_MATCHES)
        self.assertEqual(result[1]["verdict"], "fail")


class ApplyTestCase(LibraryFixture, unittest.TestCase):
    """The apply pass writes one strict library result into a record."""

    def setUp(self) -> None:
        super().setUp()
        self.saved = {
            name: getattr(store, name)
            for name in ("FUNCTIONS_DIR", "REBUILD_DIR", "WORK_DIR", "TOOLS_DIR")
        }
        store.FUNCTIONS_DIR = self.root / "functions"
        store.REBUILD_DIR = self.root / "rebuild"
        store.WORK_DIR = self.root / "work"
        store.TOOLS_DIR = self.root / "tools"
        store.FUNCTIONS_DIR.mkdir(parents=True)
        store.REBUILD_DIR.mkdir(parents=True)
        store.WORK_DIR.mkdir(parents=True)
        self.addCleanup(self.restore_store)
        self.original = self.root / "STARTH.EXE"
        self.original.write_bytes(b"ORIGINAL")
        self.entry = {
            "file": "TEST.LIB",
            "sha256": "sha256:" + "a" * 64,
            "module": "mod",
            "symbol": "malloc_",
            "object_sha256": "sha256:" + "b" * 64,
        }
        self.check = {
            "verdict": "pass",
            "detail": "the modules are byte-identical",
            "match": "exact-bytes",
            "bytes_equal": 9,
            "bytes_total": 9,
            "instructions_equal": 3,
            "instructions_total": 3,
        }
        self.image = FakeOriginal(path=self.original)

    def restore_store(self) -> None:
        for name, value in self.saved.items():
            setattr(store, name, value)

    def record(self, address: str = "0xd1f25", **overrides: object) -> dict:
        base: dict = {
            "address": address,
            "status": "unknown",
            "checks": [],
            "evidence": [],
            "notes": [],
            "provenance": "library",
        }
        base.update(overrides)
        return base

    def apply(
        self, record: dict, found: tuple[dict, dict] | None = None
    ) -> str:
        if found is None:
            found = (self.entry, self.check)
        with mock.patch.object(
            lib, "verify_candidate", return_value=found
        ):
            return lib.apply_record(record, self.image, {"entries": []}, self.root)

    def stored(self, address: str = "0xd1f25") -> dict:
        return store.load_record(int(address, 16))

    def expected_fingerprint(self, record: dict) -> str:
        return store.compute_fingerprint(
            record,
            None,
            library=record["library"],
            original=self.original,
            mode=lib.library_mode(),
        )

    def test_apply_writes_the_identity_check(self) -> None:
        record = self.record()
        self.assertEqual(self.apply(record), "written")
        stored = self.stored()
        self.assertEqual(set(stored["result"]["checks"]), {"identity"})
        self.assertEqual(
            stored["result"]["checks"]["identity"]["match"], "exact-bytes"
        )
        self.assertEqual(stored["checks"], ["identity"])

    def test_apply_fingerprint_covers_the_checks_and_the_library_field(self) -> None:
        self.apply(self.record())
        stored = self.stored()
        base = self.expected_fingerprint(stored)
        changed_checks = dict(stored)
        changed_checks["checks"] = ["identity", "behavior"]
        self.assertNotEqual(
            base, self.expected_fingerprint(changed_checks)
        )
        changed_library = dict(stored)
        changed_library["library"] = dict(stored["library"])
        changed_library["library"]["symbol"] = "other_"
        self.assertNotEqual(
            base, self.expected_fingerprint(changed_library)
        )
        renamed = dict(stored)
        renamed["name"] = "other"
        self.assertEqual(base, self.expected_fingerprint(renamed))

    def test_apply_uses_the_library_mode(self) -> None:
        self.apply(self.record())
        self.assertEqual(
            self.stored()["result"]["mode"], store.run_mode(True, False)
        )

    def test_apply_sets_the_verified_status(self) -> None:
        self.apply(self.record())
        self.assertEqual(self.stored()["status"], "verified")

    def test_apply_names_an_unnamed_record(self) -> None:
        self.apply(self.record())
        self.assertEqual(self.stored()["name"], "malloc")

    def test_apply_keeps_the_module_symbol(self) -> None:
        record = self.record(
            name="malloc", module="malloc_rt", symbol="malloc_rt_"
        )
        self.apply(record)
        self.assertEqual(self.stored()["symbol"], "malloc_rt_")

    def test_apply_writes_the_library_field(self) -> None:
        self.apply(self.record())
        self.assertEqual(
            self.stored()["library"],
            lib.library_field_from_entry(self.entry),
        )

    def test_apply_writes_the_fingerprint(self) -> None:
        self.apply(self.record())
        stored = self.stored()
        self.assertEqual(
            stored["result"]["fingerprint"], self.expected_fingerprint(stored)
        )

    def test_apply_writes_the_source_digest(self) -> None:
        self.apply(self.record())
        stored = self.stored()
        digest = stored["result"]["source_digest"]
        self.assertRegex(digest, r"^sha256:[0-9a-f]{64}$")
        self.assertEqual(digest, lib.source_digest(stored["library"]))

    def test_apply_appends_the_command_evidence(self) -> None:
        record = self.record()
        self.apply(record)
        stored = self.stored()
        command = {
            "kind": "command",
            "text": "python3 tools/rebuild/library.py verify --address 0xd1f25",
        }
        commands = [
            item for item in stored["evidence"] if item.get("kind") == "command"
        ]
        self.assertEqual(commands, [command])
        self.apply(stored)
        self.assertEqual(
            [
                item.get("text")
                for item in self.stored()["evidence"]
                if item.get("kind") == "command"
            ],
            [command["text"]],
        )

    def test_apply_is_idempotent(self) -> None:
        record = self.record()
        self.apply(record)
        first = store.record_path(0xD1F25).read_text()
        self.apply(self.stored())
        self.assertEqual(store.record_path(0xD1F25).read_text(), first)

    def test_apply_leaves_a_different_record_unchanged(self) -> None:
        record = self.record()
        store.save_record(record)
        before = store.record_path(0xD1F25).read_text()
        failing = dict(self.check)
        failing["verdict"] = "fail"
        failing["match"] = "different"
        self.assertEqual(self.apply(record, (self.entry, failing)), "different")
        self.assertEqual(store.record_path(0xD1F25).read_text(), before)

    def test_apply_skips_a_second_pass_on_a_later_date(self) -> None:
        class LaterDate:
            @classmethod
            def today(cls) -> date:
                return date(2026, 12, 31)

        record = self.record()
        self.apply(record)
        before = store.record_path(0xD1F25).read_text()
        with mock.patch.object(lib, "date", LaterDate):
            outcome = self.apply(self.stored())
        self.assertEqual(outcome, "skipped")
        self.assertEqual(store.record_path(0xD1F25).read_text(), before)

    def test_apply_checks_a_stored_field_without_a_candidate(self) -> None:
        record = self.record(library=dict(self.entry))
        store.save_record(record)
        document = {"file": "TEST.LIB"}
        with mock.patch.object(
            lib, "verify_candidate", return_value=None
        ), mock.patch.object(
            lib, "_library_document", return_value=document
        ), mock.patch.object(
            lib, "verify_entry", return_value=dict(self.check)
        ):
            outcome = lib.apply_record(
                record, self.image, {"entries": []}, self.root
            )
        self.assertEqual(outcome, "skipped")
        failing = dict(self.check)
        failing["verdict"] = "fail"
        failing["match"] = "different"
        with mock.patch.object(
            lib, "verify_candidate", return_value=None
        ), mock.patch.object(
            lib, "_library_document", return_value=document
        ), mock.patch.object(
            lib, "verify_entry", return_value=failing
        ):
            outcome = lib.apply_record(
                record, self.image, {"entries": []}, self.root
            )
        self.assertEqual(outcome, "different")

    def test_apply_reports_the_outcomes(self) -> None:
        records = [
            self.record("0xd1f25"),
            self.record("0xd1f30"),
            self.record("0xd1f3b"),
        ]
        failing = dict(self.check)
        failing["verdict"] = "fail"
        failing["match"] = "different"
        with mock.patch.object(
            lib,
            "verify_candidate",
            side_effect=[(self.entry, self.check), None, (self.entry, failing)],
        ):
            counts = lib.apply(
                records, self.image, {"entries": []}, self.root, None
            )
        self.assertEqual(
            counts, {"written": 1, "skipped": 1, "different": 1}
        )


class ManifestTestCase(unittest.TestCase):
    """The link manifest lists the library records and their digests."""

    def field(self, **overrides: str) -> dict:
        base = {
            "file": "CLIB3R.LIB",
            "sha256": "sha256:" + "a" * 64,
            "module": "nmalloc",
            "symbol": "malloc_",
            "object_sha256": "sha256:" + "b" * 64,
        }
        base.update(overrides)
        return base

    def index(self, *libraries: tuple[str, str]) -> dict:
        return {
            "directory": ".",
            "libraries": [
                {"file": file, "sha256": digest}
                for file, digest in libraries
            ],
            "entries": [],
        }

    def record(self, address: str = "0xd1f25", **overrides: object) -> dict:
        base: dict = {
            "address": address,
            "status": "verified",
            "library": self.field(),
        }
        base.update(overrides)
        return base

    def matching_index(self, record: dict) -> dict:
        return self.index((record["library"]["file"], record["library"]["sha256"]))

    def check(self, records: list[dict], document: dict) -> list[str]:
        return lib.check_manifest(
            records, document, Path("tools/rebuild/library.json")
        )

    def test_manifest_lists_every_library_record(self) -> None:
        first = self.record("0x300")
        second = self.record("0x100")
        plain = {"address": "0x200", "status": "rebuilt"}
        document = lib.manifest_document(
            [first, second, plain], self.matching_index(first)
        )
        self.assertEqual(list(document["entries"]), ["0x100", "0x300"])
        self.assertEqual(document["entries"]["0x100"], self.field())
        self.assertEqual(len(document["entries"]), 2)

    def test_manifest_lists_the_libraries(self) -> None:
        record = self.record()
        index_digest = "sha256:" + "d" * 64
        index = self.index(
            ("MATH387R.LIB", "sha256:" + "c" * 64),
            ("CLIB3R.LIB", index_digest),
        )
        document = lib.manifest_document([record], index)
        self.assertEqual(
            list(document["libraries"]), ["CLIB3R.LIB", "MATH387R.LIB"]
        )
        self.assertEqual(
            document["libraries"]["CLIB3R.LIB"], index_digest
        )
        self.assertNotEqual(
            document["libraries"]["CLIB3R.LIB"],
            record["library"]["sha256"],
        )
        self.assertEqual(
            document["libraries"]["MATH387R.LIB"], "sha256:" + "c" * 64
        )

    def test_manifest_is_stable(self) -> None:
        records = [self.record("0x300"), self.record("0x100")]
        index = self.matching_index(records[0])
        first = lib.manifest_document(records, index)
        second = lib.manifest_document(records, index)
        self.assertEqual(
            json.dumps(first, sort_keys=True),
            json.dumps(second, sort_keys=True),
        )

    def test_load_manifest_rejects_a_non_object_entry(self) -> None:
        for value in (None, "text", 5, [1, 2]):
            with self.subTest(value=value):
                temporary = tempfile.TemporaryDirectory()
                self.addCleanup(temporary.cleanup)
                path = Path(temporary.name) / "library.json"
                path.write_text(
                    json.dumps(
                        {"libraries": {}, "entries": {"0xdead": value}}
                    )
                )
                with self.assertRaises(ValueError) as caught:
                    lib._load_manifest(path)
                message = str(caught.exception)
                self.assertIn(str(path), message)
                self.assertIn("0xdead", message)

    def test_check_manifest_accepts_a_matching_document(self) -> None:
        record = self.record()
        document = lib.manifest_document(
            [record], self.matching_index(record)
        )
        self.assertEqual(self.check([record], document), [])

    def test_check_manifest_reports_a_missing_entry(self) -> None:
        record = self.record()
        document = lib.manifest_document(
            [], self.matching_index(record)
        )
        messages = self.check([record], document)
        self.assertEqual(len(messages), 1)
        self.assertIn(record["address"], messages[0])

    def test_check_manifest_reports_a_stale_entry(self) -> None:
        record = self.record()
        document = lib.manifest_document(
            [record], self.matching_index(record)
        )
        document["entries"][record["address"]]["object_sha256"] = (
            "sha256:" + "f" * 64
        )
        messages = self.check([record], document)
        self.assertEqual(len(messages), 1)
        self.assertIn(record["address"], messages[0])

    def test_check_manifest_suppresses_the_digest_message_for_a_stale_sha256(
        self,
    ) -> None:
        record = self.record()
        document = lib.manifest_document(
            [record], self.matching_index(record)
        )
        document["entries"][record["address"]]["sha256"] = (
            "sha256:" + "f" * 64
        )
        messages = self.check([record], document)
        self.assertEqual(len(messages), 1)
        self.assertIn(record["address"], messages[0])
        self.assertNotIn("digests do not agree", messages[0])

    def test_check_manifest_reports_an_entry_without_a_record(self) -> None:
        record = self.record()
        document = lib.manifest_document(
            [record], self.matching_index(record)
        )
        document["entries"]["0xdead"] = self.field(symbol="orphan_")
        messages = self.check([record], document)
        self.assertEqual(len(messages), 1)
        self.assertIn("0xdead", messages[0])

    def test_check_manifest_reports_a_missing_library_digest(self) -> None:
        record = self.record()
        document = lib.manifest_document(
            [record], self.matching_index(record)
        )
        document["libraries"].pop(record["library"]["file"])
        messages = self.check([record], document)
        self.assertEqual(len(messages), 1)
        self.assertIn(record["address"], messages[0])

    def test_check_manifest_reports_a_wrong_library_digest(self) -> None:
        record = self.record()
        document = lib.manifest_document(
            [record], self.matching_index(record)
        )
        document["libraries"][record["library"]["file"]] = (
            "sha256:" + "f" * 64
        )
        messages = self.check([record], document)
        self.assertEqual(len(messages), 1)
        self.assertIn(record["address"], messages[0])

    def test_check_manifest_accepts_a_reused_object(self) -> None:
        first = self.record("0xd1f25")
        second = self.record("0xd1f30")
        document = lib.manifest_document(
            [first, second], self.matching_index(first)
        )
        self.assertEqual(self.check([first, second], document), [])

    def test_check_manifest_reports_a_duplicate_object(self) -> None:
        first = self.record("0xd1f25")
        second = self.record(
            "0xd1f30", library=self.field(object_sha256="sha256:" + "e" * 64)
        )
        document = lib.manifest_document(
            [first, second], self.matching_index(first)
        )
        messages = self.check([first, second], document)
        self.assertEqual(len(messages), 1)
        self.assertIn(first["library"]["symbol"], messages[0])

    def test_check_manifest_reports_a_record_that_is_not_verified(self) -> None:
        record = self.record(status="rebuilt")
        document = lib.manifest_document(
            [record], self.matching_index(record)
        )
        messages = self.check([record], document)
        self.assertEqual(len(messages), 1)
        self.assertIn(record["address"], messages[0])

    def test_check_manifest_ignores_a_record_without_a_library_field(self) -> None:
        record = {"address": "0x100", "status": "verified"}
        document = {"libraries": {}, "entries": {}}
        self.assertEqual(self.check([record], document), [])


if __name__ == "__main__":
    unittest.main()
