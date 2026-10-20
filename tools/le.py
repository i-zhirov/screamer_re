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
    python3 tools/le.py calls <file> <linear-address> [--max N]
    python3 tools/le.py tree <file> <linear-address> [--depth N]
    python3 tools/le.py funcs <file> [--from ADDR] [--to ADDR]
    python3 tools/le.py near <file> <linear-address>
"""

from __future__ import annotations

import argparse
import ctypes
import ctypes.util
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
            "entry_offset": u32(0x1C),
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
        self.entry_object = header["start_object"]

        for index in range(header["object_count"]):
            offset = self.header_offset + header["object_table_offset"] + index * 24
            virtual_size, reloc_base, flags, page_index, page_count, reserved = struct.unpack_from(
                "<IIIIII", data, offset
            )
            self.objects.append(
                Object(index + 1, virtual_size, reloc_base, flags, page_index, page_count, reserved)
            )

        # The header entry value is an offset inside the start object.  The
        # object-relative offset maps to a page index of the object page table.
        start = self.object_by_index(self.entry_object)
        self.entry_offset = header["entry_offset"]
        self.entry_linear = (start.reloc_base if start else 0) + self.entry_offset

        for obj in self.objects:
            for page_index in range(obj.page_count):
                table_index = obj.page_table_index - 1 + page_index
                page = self._read_page(obj, page_index, table_index)
                self.pages.append(page)

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

    def object_by_index(self, index: int) -> Object | None:
        for obj in self.objects:
            if obj.index == index:
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
    print(f"entry:           object {exe.entry_object} offset {exe.entry_offset:#x} "
          f"(module linear {exe.entry_linear:#x})")
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
    """Load the capstone library from the loader path or a known location."""
    candidates: list[str] = []
    found = ctypes.util.find_library("capstone")
    if found:
        candidates.append(found)
    candidates.extend(CAPSTONE_PATHS)
    for candidate in candidates:
        try:
            return ctypes.CDLL(candidate)
        except OSError:
            continue
    return None


@dataclass
class Insn:
    address: int
    size: int
    mnemonic: str
    operands: str
    raw: bytes

    @property
    def text(self) -> str:
        return f"{self.mnemonic} {self.operands}".strip()


# The Watcom compiler ends a function with one of these instructions.
TERMINATORS = {"ret", "retf", "iret", "iretd", "hlt", "int3"}
MAX_INSTRUCTION_SIZE = 15

_CAPSTONE: tuple[ctypes.CDLL, ctypes.c_void_p] | None = None


def _capstone() -> tuple[ctypes.CDLL, ctypes.c_void_p]:
    global _CAPSTONE
    if _CAPSTONE is not None:
        return _CAPSTONE
    lib = _load_capstone()
    if lib is None:
        raise ValueError("capstone library not found")
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
        raise ValueError("capstone open failed")
    _CAPSTONE = (lib, handle)
    return _CAPSTONE


def decode(exe: LinearExecutable, address: int, max_instructions: int = 64) -> list[Insn]:
    """Decode up to max_instructions from a linear address."""
    lib, handle = _capstone()
    size = min(max_instructions * MAX_INSTRUCTION_SIZE + 16, 1 << 20)
    chunk = exe.read(address, size)
    if chunk is None:
        raise ValueError(f"address {address:#x} is not mapped to a file offset")
    buffer = (ctypes.c_ubyte * len(chunk)).from_buffer_copy(chunk)
    pointer = ctypes.POINTER(_CsInsn)()
    count = lib.cs_disasm(
        handle, buffer, len(chunk), address, max_instructions, ctypes.byref(pointer)
    )
    instructions = [
        Insn(
            address=pointer[index].address,
            size=pointer[index].size,
            mnemonic=pointer[index].mnemonic.decode(),
            operands=pointer[index].op_str.decode(),
            raw=bytes(pointer[index].bytes[: pointer[index].size]),
        )
        for index in range(count)
    ]
    lib.cs_free(pointer, count)
    return instructions


def function_body(
    exe: LinearExecutable, address: int, max_instructions: int = 4000
) -> list[Insn]:
    """Decode a function body from its start until a terminator.

    A leading unconditional jump is followed, because the linker puts a jump over the
    run-time banner string at the entry point.  The sweep then stops at the first
    `ret`, `retf`, `iret`, `hlt`, or `int3`, or after max_instructions.  A function
    that ends with a tail call has no terminator, so the sweep continues into the
    next function.
    """
    body: list[Insn] = []
    current = address
    for _ in range(4):
        instructions = decode(exe, current, 1)
        if not instructions:
            return body
        first = instructions[0]
        operand = first.operands.strip()
        if first.mnemonic == "jmp" and operand.startswith("0x"):
            body.append(first)
            current = int(operand.split()[0], 16)
            continue
        break
    remaining = max_instructions - len(body)
    if remaining <= 0:
        return body
    for insn in decode(exe, current, remaining):
        body.append(insn)
        if insn.mnemonic in TERMINATORS:
            break
    return body


@dataclass
class CallSite:
    address: int
    target: int | None
    text: str


def call_sites(instructions: list[Insn]) -> list[CallSite]:
    """List the call sites of a decoded body.  An indirect call has no target."""
    sites: list[CallSite] = []
    for insn in instructions:
        if insn.mnemonic != "call":
            continue
        operand = insn.operands.strip()
        target = int(operand.split()[0], 16) if operand.startswith("0x") else None
        sites.append(CallSite(insn.address, target, insn.text))
    return sites


def print_calls(exe: LinearExecutable, address: int, max_instructions: int) -> int:
    body = function_body(exe, address, max_instructions)
    print(f"function {address:#x}: {len(body)} instructions")
    for insn in body:
        print(f"{insn.address:#010x}: {insn.raw.hex():<18} {insn.text}")
    sites = call_sites(body)
    print()
    print(f"calls: {len(sites)}")
    for site in sites:
        target = f"{site.target:#010x}" if site.target is not None else "indirect"
        print(f"  {site.address:#010x} -> {target}  {site.text}")
    return 0


def print_call_tree(
    exe: LinearExecutable,
    address: int,
    depth: int,
    max_instructions: int,
    max_nodes: int,
) -> int:
    visited: set[int] = set()
    budget = [max_nodes]

    def walk(node: int, level: int) -> None:
        if budget[0] <= 0:
            return
        budget[0] -= 1
        indent = "  " * level
        body = function_body(exe, node, max_instructions)
        sites = call_sites(body)
        repeats = node in visited
        visited.add(node)
        note = " (seen)" if repeats else ""
        print(f"{indent}{node:#010x}: {len(body)} instructions, {len(sites)} calls{note}")
        if repeats or level >= depth:
            return
        for site in sites:
            if site.target is None:
                print(f"{indent}  indirect call at {site.address:#010x}")
                continue
            print(f"{indent}  call at {site.address:#010x} ->")
            walk(site.target, level + 2)

    walk(address, 0)
    if budget[0] <= 0:
        print(f"node budget {max_nodes} reached")
    return 0


def print_functions(
    exe: LinearExecutable, start: int | None, end: int | None, limit: int
) -> int:
    """List candidate function starts: the Watcom prologue `push ebp; mov ebp,esp`."""
    obj = exe.object_at(exe.entry_linear)
    if obj is None:
        print("the entry object is missing", file=sys.stderr)
        return 1
    low = obj.reloc_base if start is None else start
    high = obj.reloc_base + obj.virtual_size if end is None else end
    data = exe.read(low, high - low)
    if data is None:
        print("the range is not mapped to a file offset", file=sys.stderr)
        return 1
    pattern = b"\x55\x89\xe5"
    found: list[int] = []
    position = data.find(pattern)
    while position != -1 and len(found) < limit:
        found.append(low + position)
        position = data.find(pattern, position + 1)
    print(f"candidate function starts: {len(found)}")
    for candidate in found:
        print(f"  {candidate:#010x}")
    return 0


def print_nearest_function(exe: LinearExecutable, address: int) -> int:
    """Print the candidate function start nearest below an address."""
    obj = exe.object_at(address)
    if obj is None:
        print("the address is unmapped", file=sys.stderr)
        return 1
    low = obj.reloc_base
    data = exe.read(low, address - low + 1)
    if data is None:
        print("the range is not mapped to a file offset", file=sys.stderr)
        return 1
    pattern = b"\x55\x89\xe5"
    position = data.rfind(pattern)
    if position == -1:
        print(f"no prologue below {address:#x}")
        return 1
    start = low + position
    print(f"nearest prologue: {start:#x} ({address - start:#x} bytes below {address:#x})")
    return 0


def disasm(exe: LinearExecutable, linear_address: int, count: int) -> int:
    try:
        instructions = decode(exe, linear_address, count)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    for insn in instructions:
        print(f"{insn.address:#010x}: {insn.raw.hex():<20} {insn.text}")
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

    calls_parser = subparsers.add_parser("calls", help="disassemble a function and list its calls")
    calls_parser.add_argument("file", type=Path)
    calls_parser.add_argument("address", type=lambda value: int(value, 0))
    calls_parser.add_argument("--max", dest="max_instructions", type=int, default=4000)

    tree_parser = subparsers.add_parser("tree", help="follow the call tree of a function")
    tree_parser.add_argument("file", type=Path)
    tree_parser.add_argument("address", type=lambda value: int(value, 0))
    tree_parser.add_argument("--depth", type=int, default=2)
    tree_parser.add_argument("--max", dest="max_instructions", type=int, default=4000)
    tree_parser.add_argument("--max-nodes", type=int, default=64)

    funcs_parser = subparsers.add_parser("funcs", help="list candidate function starts")
    funcs_parser.add_argument("file", type=Path)
    funcs_parser.add_argument("--from", dest="start", type=lambda value: int(value, 0))
    funcs_parser.add_argument("--to", dest="end", type=lambda value: int(value, 0))
    funcs_parser.add_argument("--limit", type=int, default=500)

    near_parser = subparsers.add_parser("near", help="show the nearest function start below")
    near_parser.add_argument("file", type=Path)
    near_parser.add_argument("address", type=lambda value: int(value, 0))

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
    elif args.command == "calls":
        print_map(exe, args.address)
        return print_calls(exe, args.address, args.max_instructions)
    elif args.command == "tree":
        return print_call_tree(
            exe, args.address, args.depth, args.max_instructions, args.max_nodes
        )
    elif args.command == "funcs":
        return print_functions(exe, args.start, args.end, args.limit)
    elif args.command == "near":
        return print_nearest_function(exe, args.address)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
