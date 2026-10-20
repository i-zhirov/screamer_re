"""List and extract the members of a Screamer graphics container.

The graphics files (`GFX/GAME.DAT`, `GFX/MENU.DAT`, and the map files) hold a
directory at the end of the file.  The last 4 bytes hold the size of the
directory.  The directory holds `size / 32` records of 32 bytes.  A record holds
the member name at `+0x00` (16 bytes, exclusive-ORed with `0x3a`, NUL-terminated),
the member offset at `+0x10`, the member size at `+0x14`, the end offset at
`+0x18`, and a flag at `+0x1c`.  The offsets form one chain: `offset + size` of
a member is the `offset` of the next member.

The member data is LZSS-compressed.  The expander sits at `0x4d3fc` and the block
decoder at `0x4d4c4` in the game executable; the port in `expand` follows the
decoded structure but does not yet reproduce the member bytes, so the tool reports
the raw member and marks the expansion as not confirmed.

Usage:

    python3 tools/gfx_container.py info GFX/GAME.DAT
    python3 tools/gfx_container.py member GFX/GAME.DAT GAMEL.SPR --out work/gamel.spr
    python3 tools/gfx_container.py head GFX/GAME.DAT GAMEL.SPR
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

RECORD_SIZE = 32
NAME_SIZE = 16
XOR_KEY = 0x3A
TRAILER_SIZE = 4


class Member:
    """One member of a graphics container."""

    def __init__(
        self,
        name: str,
        offset: int,
        size: int,
        end: int,
        flag: int,
    ) -> None:
        self.name = name
        self.offset = offset
        self.size = size
        self.end = end
        self.flag = flag

    @property
    def data_range(self) -> range:
        return range(self.offset, self.offset + self.size)


class Container:
    """A parsed graphics container."""

    def __init__(self, path: Path, data: bytes, members: list[Member]) -> None:
        self.path = path
        self.data = data
        self.members = members

    def member(self, name: str) -> Member:
        wanted = name.upper()
        for member in self.members:
            if member.name.upper() == wanted:
                raise_error = None
                return member
        raise ValueError(f"member {name!r} is not in {self.path}")


def parse(path: Path) -> Container:
    """Parse a graphics container and return its members."""
    data = path.read_bytes()
    if len(data) < TRAILER_SIZE:
        raise ValueError(f"{path} is too short for a container")
    directory_size = struct.unpack_from("<I", data, len(data) - TRAILER_SIZE)[0]
    if directory_size == 0 or directory_size % RECORD_SIZE != 0:
        raise ValueError(
            f"{path}: the directory size {directory_size} is not a multiple of "
            f"{RECORD_SIZE}"
        )
    directory_start = len(data) - TRAILER_SIZE - directory_size
    if directory_start < 0:
        raise ValueError(f"{path}: the directory runs before the file start")
    members: list[Member] = []
    for index in range(directory_size // RECORD_SIZE):
        record = data[
            directory_start + index * RECORD_SIZE :
            directory_start + (index + 1) * RECORD_SIZE
        ]
        raw_name = bytes(byte ^ XOR_KEY for byte in record[:NAME_SIZE])
        name = raw_name.split(b"\0")[0].decode("ascii", "replace")
        offset, size, end, flag = struct.unpack_from("<IIII", record, 0x10)
        if not name:
            continue
        if offset + size > len(data):
            raise ValueError(
                f"{path}: member {name!r} ends at {offset + size}, past the "
                f"file size {len(data)}"
            )
        members.append(Member(name, offset, size, end, flag))
    if not members:
        raise ValueError(f"{path} holds no members")
    return Container(path, data, members)


def member_bytes(container: Container, member: Member) -> bytes:
    """Return the raw member bytes."""
    return container.data[member.offset : member.offset + member.size]


MASK = 0x1FF


class _Lzss:
    """The LZSS decoder of the game, ported from `0x4d3fc`, `0x4d4c4`, and `0x4d844`."""

    def __init__(self, raw: bytes, limit: int) -> None:
        self.raw = raw
        self.limit = limit
        self.buf = bytearray(0x400)
        self.buf[0] = raw[0]
        self.read_pos = 1
        self.bits = 0
        self.shift = 9
        self.threshold = 0x200
        self.index = 0x102
        self.code = 0
        self.value = 0
        self.code2 = 0
        self.ring = [0] * 0x200
        self.ring_index = 0
        self.cur = 0
        self.cur2 = 0
        self.state = 0
        self.table_ushort = [0] * 0x2000
        self.table_byte = [0] * 0x2000
        self.out = bytearray()
        self.out_count = 0

    def _read_bits(self) -> int:
        remaining = 0x400 - (self.bits >> 3)
        if remaining < 5:
            for i in range(remaining):
                self.buf[i] = self.buf[(self.bits >> 3) + i]
            if self.read_pos < len(self.raw):
                self.buf[remaining] = self.raw[self.read_pos]
                self.read_pos += 1
            self.bits &= 7
        old = self.bits
        self.bits += self.shift
        low = self.buf[old >> 3]
        high = self.buf[(old >> 3) + 1]
        return ((low | (high << 8)) >> (old & 7)) & MASK

    def block(self) -> bool:
        state = self.state
        self.state = 0
        if state != 2:
            code = self._read_bits()
            if code == 0x101:
                return False
            if code == 0x100:
                self.shift = 9
                self.threshold = 0x200
                self.index = 0x102
                value = self._read_bits()
                self.code = value
                self.value = value
                self.cur2 = value & 0xFF
                self.cur = value & 0xFF
                self.out.append(value & 0xFF)
                self.out_count += 1
                if self.out_count != self.limit:
                    return True
                self.state = 1
                return False
            self.code = code
            self.code2 = code
            if self.index <= code:
                self.code = self.value
                entry = (self.code >> 8) | self.cur
                self.ring[self.ring_index] = entry
                self.ring_index += 1
            while 0xFF < self.code:
                entry = (self.code >> 8) | self.table_byte[self.code]
                self.ring[self.ring_index] = entry
                self.ring_index += 1
                self.code = self.table_ushort[self.code]
            self.cur = self.code & 0xFF
            self.cur2 = self.cur
            self.ring[self.ring_index] = self.code
            self.ring_index += 1
        while True:
            if self.ring_index == 0:
                if self.index >= len(self.table_byte) - 1:
                    return False
                self.table_byte[self.index] = self.cur2
                self.index += 1
                self.table_ushort[self.index] = self.value
                self.value = self.code2
                if self.threshold <= self.index and self.shift != 0xC:
                    self.shift += 1
                    self.threshold *= 2
                return True
            self.ring_index -= 1
            self.out.append(self.ring[self.ring_index] & 0xFF)
            self.out_count += 1
            if self.out_count == self.limit:
                return False
            if self.ring_index == 0:
                if self.index >= len(self.table_byte) - 1:
                    return False
                self.table_byte[self.index] = self.cur2
                self.index += 1
                self.table_ushort[self.index] = self.value
                self.value = self.code2
                if self.threshold <= self.index and self.shift != 0xC:
                    self.shift += 1
                    self.threshold *= 2
                return True


def expand(raw: bytes, limit: int = 1 << 20) -> bytes:
    """Expand the LZSS member data.

    The port follows the decoded structure: a bit buffer of 0x400 bytes with the
    unread bytes at the front, a bit position, a shift count that starts at 9 and
    grows to 12, a ring buffer of 0x200 entries, the literal code 0x100, and the end
    code 0x101.  The mask hypothesis is `table[shift] = 0x1ff` for every shift, so
    the code is always 9 significant bits and the shift count controls only the bit
    position advance.

    The port runs to completion but the output head is zero, so the hypothesis is not
    confirmed.  The ring and table semantics and the output loop source stay open.
    """
    if len(raw) < 2:
        raise ValueError("the member is too short to expand")
    decoder = _Lzss(raw, limit)
    guard = 0
    while decoder.block():
        guard += 1
        if guard > len(raw) * 64 + 0x1000:
            break
    return bytes(decoder.out)


def print_info(container: Container) -> int:
    """Print the members of a container."""
    print(f"file:    {container.path}")
    print(f"bytes:   {len(container.data)}")
    print(f"members: {len(container.members)}")
    print(f"{'name':<16} {'offset':>10} {'size':>10} {'end':>10} flag")
    for member in container.members:
        print(
            f"{member.name:<16} {member.offset:>10} {member.size:>10} "
            f"{member.end:>10} {member.flag}"
        )
    return 0


def print_head(container: Container, name: str) -> int:
    """Print the head of a member and test the sprite and font structures."""
    member = container.member(name)
    raw = member_bytes(container, member)
    print(f"member: {member.name}")
    print(f"offset: {member.offset}")
    print(f"size:   {member.size}")
    print(f"head:   {raw[:16].hex(' ')}")
    try:
        expanded = expand(raw)
        print(f"expand: {len(expanded)} bytes")
        print(f"head:   {expanded[:16].hex(' ')}")
        if len(expanded) >= 8:
            count, size = struct.unpack_from("<II", expanded, 0)
            print(f"count at +0x00: {count}")
            print(f"size at +0x04:  {size}")
            print(f"8 + count * 0x14 + size = {8 + count * 0x14 + size}")
    except ValueError as error:
        print(f"expand: {error}")
    if len(raw) >= 8:
        count, size = struct.unpack_from("<II", raw, 0)
        expected = 8 + count * 0x14 + size
        print(f"count at +0x00: {count}")
        print(f"size at +0x04:  {size}")
        print(f"8 + count * 0x14 + size = {expected}")
        if expected == len(raw):
            print("the sprite structure matches the member size")
        else:
            print("the member does not match the plain sprite structure")
    return 0


def write_member(container: Container, name: str, out: Path) -> int:
    """Write the raw bytes of a member to a file."""
    member = container.member(name)
    data = member_bytes(container, member)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(data)
    print(f"wrote {out} ({len(data)} bytes, the raw member)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)

    info_parser = subparsers.add_parser("info", help="list the members")
    info_parser.add_argument("file", type=Path)

    member_parser = subparsers.add_parser("member", help="write a raw member")
    member_parser.add_argument("file", type=Path)
    member_parser.add_argument("name")
    member_parser.add_argument("--out", type=Path)

    head_parser = subparsers.add_parser("head", help="test the member head")
    head_parser.add_argument("file", type=Path)
    head_parser.add_argument("name")

    args = parser.parse_args()
    try:
        container = parse(args.file)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    if args.command == "info":
        return print_info(container)
    if args.command == "member":
        if args.out is None:
            print("error: member needs --out", file=sys.stderr)
            return 1
        try:
            return write_member(container, args.name, args.out)
        except ValueError as error:
            print(f"error: {error}", file=sys.stderr)
            return 1
    try:
        return print_head(container, args.name)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
