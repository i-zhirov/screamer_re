#!/usr/bin/env python3
"""Inspect a DOS/4GW linear executable (LE) without running it.

The tool parses the MZ stub, the LE image header, the object table, and the
object page table.  It reports header metadata, objects, the entry point, and the
mapping from a linear address to a file offset.  An optional disassembly uses the
capstone shared library through ctypes when that library is available.

Usage:
    python3 tools/le.py info <file>
    python3 tools/le.py map <file> <linear-address>
    python3 tools/le.py disasm <file> <linear-address> [--count N]
"""

from __future__ import annotations

import argparse
import ctypes
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

MZ_LFANEW_OFFSET = 0x3C

# LE object flags (radare2 le_specs.h)
O_READABLE = 1 << 0
O_WRITABLE = 1 << 1
O_EXECUTABLE = 1 << 2
O_RESOURCE = 1 << 3
O_BIG_BIT = 1 << 13

# LE page flags
P_LEGAL = 0
P_ITERATED = 1
P_INVALID = 2
P_ZEROED = 3
P_COMPRESSED = 4
P_RANGE = 5

PAGE_FLAG_NAMES = {
    P_LEGAL: "legal",
    P_ITERATED: "iterated",
    P_INVALID: "invalid",
    P_ZEROED: "zeroed",
    P_COMPRESSED: "compressed",
    P_RANGE: "range",
}

CPU_NAMES = {1: "80286", 2: "80386", 3: "80486", 0x20: "i860", 0x40: "MIPS"}
OS_NAMES = {1: "OS/2", 2: "Windows", 3: "DOS 4.x", 4: "Windows 386", 5: "Personality Neutral"}

MODULE_TYPE_MASK = 0x38000
MODULE_TYPE_NAMES = {
    0x00000: "EXE",
    0x08000: "DLL",
    0x10000: "PM DLL",
    0x20000: "PDD",
    0x28000: "VDD",
}


@dataclass
class Object:
    index: int
    virtual_size: int
    reloc_base: int
    flags: int
    page_table_index: int
    page_count: int
    reserved: int

    @property
    def permissions(self) -> str:
        perm = ""
        perm += "r" if self.flags & O_READABLE else "-"
        perm += "w" if self.flags & O_WRITABLE else "-"
        perm += "x" if self.flags & O_EXECUTABLE else "-"
        return perm


@dataclass
class Page:
    object_index: int
    page_index: int
    number: int
    flags: int
    file_offset: int | None
    data_size: int
    virtual_address: int
    virtual_size: int


@dataclass
class LinearExecutable:
    path: Path
    data: bytes
    header_offset: int = 0
    header: dict = field(default_factory=dict)
    objects: list[Object] = field(default_factory=list)
    pages: list[Page] = field(default_factory=list)
    entry_linear: int = 0
    entry_object: int = 0
    entry_offset: int = 0
    module_name: str | None = None
    symbols: list[tuple[str, int]] = field(default_factory=list)

    @property
    def is_le(self) -> bool:
        return self.header.get("signature") == b"LE"

    @property
    def page_size(self) -> int:
        return self.header.get("page_size", 0)

    def parse(self) -> "LinearExecutable":
        data = self.data
        if data[:2] != b"MZ":
            raise ValueError("not an MZ image")
        self.header_offset = struct.unpack_from("<I", data, MZ_LFANEW_OFFSET)[0]
        if data[self.header_offset : self.header_offset + 2] not in (b"LE", b"LX"):
            raise ValueError("not an LE/LX image")
        if data[self.header_offset : self.header_offset + 2] != b"LE":
            raise ValueError("unsupported image format: the tool handles LE images only")
        u32 = lambda offset: struct.unpack_from("<I", data, self.header_offset + offset)[0]
        u16 = lambda offset: struct.unpack_from("<H", data, self.header_offset + offset)[0]
        self.header = {
            "signature": data[self.header_offset : self.header_offset + 2],
            "byte_order": data[self.header_offset + 2],
            "word_order": data[self.header_offset + 3],
            "format_level": u32(0x04),
            "cpu": u16(0x08),
            "os": u16(0x0A),
            "module_version": u32(0x0C),
            "module_flags": u32(0x10),
            "page_count": u32(0x14),
            "start_object": u32(0x18),
            "entry_linear": u32(0x1C),
            "stack_object": u32(0x20),
            "stack_pointer": u32(0x24),
            "page_size": u32(0x28),
            "last_page_size": u32(0x2C),
            "fixup_size": u32(0x30),
            "object_table_offset": u32(0x40),
            "object_count": u32(0x44),
            "object_page_table_offset": u32(0x48),
            "resource_table_offset": u32(0x50),
            "resident_name_table_offset": u32(0x58),
            "entry_table_offset": u32(0x5C),
            "import_module_table_offset": u32(0x70),
            "import_module_count": u32(0x74),
            "import_procedure_table_offset": u32(0x78),
            "data_pages_offset": u32(0x80),
            "preload_pages": u32(0x84),
            "non_resident_name_table_offset": u32(0x88),
            "non_resident_name_size": u32(0x8C),
        }

        header = self.header
        self.entry_linear = header["entry_linear"]
        self.entry_object = header["start_object"]

        for index in range(header["object_count"]):
            offset = self.header_offset + header["object_table_offset"] + index * 24
            virtual_size, reloc_base, flags, page_index, page_count, reserved = struct.unpack_from(
                "<IIIIII", data, offset
            )
            self.objects.append(
                Object(index + 1, virtual_size, reloc_base, flags, page_index, page_count, reserved)
            )

        for obj in self.objects:
            for page_index in range(obj.page_count):
                table_index = obj.page_table_index - 1 + page_index
                page = self._read_page(obj, page_index, table_index)
                self.pages.append(page)

        entry_object = self.object_at(self.entry_linear)
        if entry_object is not None:
            self.entry_offset = self.entry_linear - entry_object.reloc_base
        else:
            self.entry_offset = self.entry_linear

        self.module_name = self._read_resident_name(header["resident_name_table_offset"])
        self.symbols = self._read_symbols()
        return self

    def _read_page(self, obj: Object, page_index: int, table_index: int) -> Page:
        header = self.header
        page_size = header["page_size"]
        if self.is_le:
            # LE page table entry: 4 bytes big-endian; low byte = flags,
            # remaining bits = 1-based page number in the file.
            offset = self.header_offset + header["object_page_table_offset"] + table_index * 4
            raw = struct.unpack_from(">I", self.data, offset)[0]
            flags = raw & 0xFF
            number = raw >> 8
            data_size = header["last_page_size"] if number == header["page_count"] and header["last_page_size"] else page_size
        else:
            offset = self.header_offset + header["object_page_table_offset"] + table_index * 8
            number, size, flags = struct.unpack_from("<IHH", self.data, offset)
            data_size = size
        file_offset = None
        if flags in (P_LEGAL, P_ITERATED):
            file_offset = header["data_pages_offset"] + (number - 1) * page_size
        return Page(
            object_index=obj.index,
            page_index=page_index,
            number=number,
            flags=flags,
            file_offset=file_offset,
            data_size=data_size,
            virtual_address=obj.reloc_base + page_index * page_size,
            virtual_size=min(page_size, obj.virtual_size - page_index * page_size),
        )

    def _read_resident_name(self, table_offset: int) -> str | None:
        if not table_offset:
            return None
        offset = self.header_offset + table_offset
        length = self.data[offset] & 0x7F
        if not length:
            return None
        return self.data[offset + 1 : offset + 1 + length].decode("ascii", "replace")

    def _read_symbols(self) -> list[tuple[str, int]]:
        """Read resident and non-resident name tables as (name, ordinal) pairs."""
        header = self.header
        symbols: list[tuple[str, int]] = []
        for table_offset, end in (
            (header["resident_name_table_offset"], header["entry_table_offset"]),
            (
                header["non_resident_name_table_offset"],
                header["non_resident_name_table_offset"] + header["non_resident_name_size"],
            ),
        ):
            if not table_offset:
                continue
            offset = self.header_offset + table_offset
            limit = self.header_offset + end
            while offset < limit:
                length = self.data[offset] & 0x7F
                if not length:
                    break
                name = self.data[offset + 1 : offset + 1 + length].decode("ascii", "replace")
                ordinal = struct.unpack_from("<H", self.data, offset + 1 + length)[0]
                symbols.append((name, ordinal))
                offset += 1 + length + 2
        return symbols

    def object_at(self, linear_address: int) -> Object | None:
        for obj in self.objects:
            if obj.reloc_base <= linear_address < obj.reloc_base + obj.virtual_size:
                return obj
        return None

    def page_at(self, linear_address: int) -> Page | None:
        for page in self.pages:
            if page.virtual_address <= linear_address < page.virtual_address + page.virtual_size:
                return page
        return None

    def to_file_offset(self, linear_address: int) -> int | None:
        page = self.page_at(linear_address)
        if page is None or page.file_offset is None:
            return None
        return page.file_offset + (linear_address - page.virtual_address)

    def read(self, linear_address: int, length: int) -> bytes | None:
        offset = self.to_file_offset(linear_address)
        if offset is None:
            return None
        return self.data[offset : offset + length]


def load(path: Path) -> LinearExecutable:
    return LinearExecutable(path, path.read_bytes()).parse()


def print_info(exe: LinearExecutable) -> None:
    header = exe.header
    print(f"file:            {exe.path}")
    print(f"format:          {header['signature'].decode()} (LE image header at {exe.header_offset:#x})")
    print(f"cpu/os:          {CPU_NAMES.get(header['cpu'], header['cpu'])} / {OS_NAMES.get(header['os'], header['os'])}")
    module_type = MODULE_TYPE_NAMES.get(header["module_flags"] & MODULE_TYPE_MASK, "unknown")
    print(f"module flags:    {header['module_flags']:#010x} ({module_type})")
    print(f"module name:     {exe.module_name}")
    print(f"pages:           {header['page_count']} x {header['page_size']} bytes (last {header['last_page_size']})")
    print(f"data pages at:   {header['data_pages_offset']:#x}")
    print(f"entry:           linear {exe.entry_linear:#x} (object {exe.entry_object}, offset {exe.entry_offset:#x})")
    entry_file = exe.to_file_offset(exe.entry_linear)
    print(f"entry file off:  {entry_file:#x}" if entry_file is not None else "entry file off:  unmapped")
    print(f"stack:           object {header['stack_object']}, esp {header['stack_pointer']:#x}")
    print(f"import modules:  {header['import_module_count']}")
    print()
    print("objects:")
    for obj in exe.objects:
        print(
            f"  obj.{obj.index}: base {obj.reloc_base:#010x} size {obj.virtual_size:#010x} "
            f"flags {obj.flags:#010x} [{obj.permissions}] pages {obj.page_count} "
            f"(table index {obj.page_table_index})"
        )
    if exe.symbols:
        print()
        print("name table:")
        for name, ordinal in exe.symbols:
            print(f"  {name}: ordinal {ordinal}")


def print_map(exe: LinearExecutable, linear_address: int) -> None:
    obj = exe.object_at(linear_address)
    page = exe.page_at(linear_address)
    file_offset = exe.to_file_offset(linear_address)
    print(f"linear address: {linear_address:#x}")
    print(f"object:         {f'obj.{obj.index} (+{linear_address - obj.reloc_base:#x})' if obj else 'unmapped'}")
    if page:
        print(f"page:           obj.{page.object_index} page {page.page_index} (file page {page.number}, {PAGE_FLAG_NAMES.get(page.flags, page.flags)})")
    print(f"file offset:    {file_offset:#x}" if file_offset is not None else "file offset:    unmapped")


CAPSTONE_PATHS = (
    "/opt/homebrew/lib/libcapstone.5.dylib",
    "/opt/homebrew/lib/libcapstone.dylib",
    "/usr/local/lib/libcapstone.5.dylib",
    "/usr/local/lib/libcapstone.dylib",
)


class _CsInsn(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint),
        ("address", ctypes.c_uint64),
        ("size", ctypes.c_uint16),
        ("bytes", ctypes.c_ubyte * 24),
        ("mnemonic", ctypes.c_char * 32),
        ("op_str", ctypes.c_char * 160),
        ("detail", ctypes.c_void_p),
    ]


def _load_capstone() -> ctypes.CDLL | None:
    for path in CAPSTONE_PATHS:
        if Path(path).exists():
            return ctypes.CDLL(path)
    return None


def disasm(exe: LinearExecutable, linear_address: int, count: int) -> int:
    lib = _load_capstone()
    if lib is None:
        print("capstone library not found", file=sys.stderr)
        return 1
    lib.cs_open.restype = ctypes.c_int
    lib.cs_open.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_void_p)]
    lib.cs_disasm.restype = ctypes.c_size_t
    lib.cs_disasm.argtypes = [
        ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_ubyte),
        ctypes.c_size_t,
        ctypes.c_uint64,
        ctypes.c_size_t,
        ctypes.POINTER(ctypes.POINTER(_CsInsn)),
    ]
    lib.cs_free.argtypes = [ctypes.POINTER(_CsInsn), ctypes.c_size_t]
    handle = ctypes.c_void_p()
    if lib.cs_open(3, 4, ctypes.byref(handle)) != 0:  # CS_ARCH_X86, CS_MODE_32
        print("capstone open failed", file=sys.stderr)
        return 1
    chunk = exe.read(linear_address, 512)
    if chunk is None:
        print("address is not mapped to a file offset", file=sys.stderr)
        return 1
    buf = (ctypes.c_ubyte * len(chunk)).from_buffer_copy(chunk)
    insn_pointer = ctypes.POINTER(_CsInsn)()
    decoded = lib.cs_disasm(
        handle, buf, len(chunk), linear_address, count, ctypes.byref(insn_pointer)
    )
    for index in range(decoded):
        insn = insn_pointer[index]
        raw = bytes(insn.bytes[: insn.size]).hex()
        print(
            f"{insn.address:#010x}: {raw:<20} {insn.mnemonic.decode()} {insn.op_str.decode()}"
        )
    lib.cs_free(insn_pointer, decoded)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)

    info_parser = subparsers.add_parser("info", help="show header and object metadata")
    info_parser.add_argument("file", type=Path)

    map_parser = subparsers.add_parser("map", help="map a linear address to a file offset")
    map_parser.add_argument("file", type=Path)
    map_parser.add_argument("address", type=lambda value: int(value, 0))

    disasm_parser = subparsers.add_parser("disasm", help="disassemble at a linear address")
    disasm_parser.add_argument("file", type=Path)
    disasm_parser.add_argument("address", type=lambda value: int(value, 0))
    disasm_parser.add_argument("--count", type=int, default=16)

    args = parser.parse_args()
    if args.command is None:
        parser.print_help()
        return 2

    try:
        exe = load(args.file)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    if args.command == "info":
        print_info(exe)
    elif args.command == "map":
        print_map(exe, args.address)
    elif args.command == "disasm":
        print_map(exe, args.address)
        return disasm(exe, args.address, args.count)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
