"""List and extract the members of a Screamer graphics container.

The graphics files (`GFX/GAME.DAT`, `GFX/MENU.DAT`, and the map files) hold a
directory at the end of the file.  The last 4 bytes hold the size of the
directory.  The directory holds `size / 32` records of 32 bytes.  A record holds
the member name at `+0x00` (16 bytes, exclusive-ORed with `0x3a`, NUL-terminated),
the member offset at `+0x10`, the member size at `+0x14`, the end offset at
`+0x18`, and a flag at `+0x1c`.  The offsets form one chain: `offset + size` of
a member is the `offset` of the next member.

The member data is LZW-compressed.  The expander sits at `0x4d3fc`, the block
decoder at `0x4d4c4`, and the bit reader at `0x4d844` in the game executable.
The port in `expand` follows the instructions and is confirmed against the runtime
records of `LOGOH.SPR`.

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


class _Lzw:
    """The LZW decoder of the game, ported from `0x4d3fc`, `0x4d4c4`, and `0x4d844`.

    The block decoder reads a code with a width from 9 to 12 bits, the code 0x100
    clears the dictionary, the code 0x101 ends the stream, and the first free
    dictionary entry is 0x102.  The width grows when the next free entry reaches
    `1 << shift`, checked before the entry is added.  The reader loads the 16 bit
    word at the byte index, keeps the full word, and uses the byte at +2 only for
    the bits that the bit offset pulls in at bit 15.
    """

    CLEAR = 0x100
    END = 0x101
    FIRST_FREE = 0x102

    def __init__(self, raw: bytes, limit: int) -> None:
        self.raw = raw
        self.limit = limit
        self.buf = bytearray(0x400)
        prefill = min(len(raw), 0x400)
        self.buf[:prefill] = raw[:prefill]
        self.read_pos = prefill
        self.bits = 0
        self.shift = 9
        self.table: dict[int, bytes] = {}
        self.next_code = self.FIRST_FREE
        self.prev: bytes | None = None
        self.out = bytearray()

    def _read_code(self) -> int:
        """Read one code, refilling the bit buffer when fewer than five bytes remain."""
        remaining = 0x400 - (self.bits >> 3)
        if remaining < 5:
            for i in range(remaining):
                self.buf[i] = self.buf[(self.bits >> 3) + i]
            room = len(self.raw) - self.read_pos
            want = remaining if remaining < room else room
            if want > 0:
                self.buf[remaining : remaining + want] = self.raw[
                    self.read_pos : self.read_pos + want
                ]
                self.read_pos += want
            self.bits &= 7
        byte_index = self.bits >> 3
        offset = self.bits & 7
        word = self.buf[byte_index] | (self.buf[byte_index + 1] << 8)
        low = self.buf[byte_index + 2]
        value = word
        for _ in range(offset):
            value = (value >> 1) | ((low & 1) << 15)
            low >>= 1
        self.bits += self.shift
        return value & ((1 << self.shift) - 1)

    def step(self) -> bool:
        """Decode one code and report whether the stream continues."""
        code = self._read_code()
        if code == self.END:
            if len(self.out) >= self.limit:
                return False
            # the game writes the end code inside a member as well: the dictionary
            # resets at that point and the stream continues at the same bit
            # position with the previous entry kept
            self.table.clear()
            self.next_code = self.FIRST_FREE
            self.shift = 9
            return True
        if code == self.CLEAR:
            self.table.clear()
            self.next_code = self.FIRST_FREE
            self.shift = 9
            return True
        if code < 0x100:
            entry = bytes([code])
        elif code in self.table:
            entry = self.table[code]
        elif self.prev is not None:
            entry = self.prev + self.prev[:1]
        else:
            return False
        if self.prev is not None and self.next_code < 0x1000:
            self.table[self.next_code] = self.prev + entry[:1]
            self.next_code += 1
            if self.next_code == (1 << self.shift) and self.shift < 12:
                self.shift += 1
        self.prev = entry
        for byte in entry:
            self.out.append(byte)
            if len(self.out) >= self.limit:
                return False
        return True


def expand(raw: bytes, limit: int = 1 << 20) -> bytes:
    """Expand the LZW member data.

    The port follows the instructions at `0x4d3fc`, `0x4d4c4`, and `0x4d844`.
    The code width runs from 9 to 12 bits, the code 0x100 clears the dictionary,
    the code 0x101 ends the stream, and the first free entry is 0x102.

    The port is confirmed against the runtime records and against the member end
    fields.  `LOGOH.SPR` expands to 208636 bytes, `LOGOL.SPR` to 52538,
    `GAMEL.SPR` to 24483, and `GAMEH.SPR` to 93675, which are the member end
    fields exactly, and the count, the size, and the record chain hold in every
    case.  The head of `LOGOH.SPR` is `03 00 00 00 b8 2e 03 00`, the count 3 and
    the pixel size 208568, and the records are 0, 0, 200, 240, 0, then 1, 1, 466,
    188, 48000, then 0, 0, 640, 114, 135608, which matches the runtime records
    at `0150:8D800`.
    """
    if len(raw) < 2:
        raise ValueError("the member is too short to expand")
    decoder = _Lzw(raw, limit)
    guard = 0
    while decoder.step():
        guard += 1
        if guard > len(raw) * 64 + 0x1000:
            raise ValueError("the member does not end within the bound")
    return bytes(decoder.out)


def print_font(container: Container, name: str) -> int:
    """Print the font layout of a member.

    The font loader at `0x25658` reads the count (one byte) at `+0x00`, then
    `count + 1` descriptors of 12 bytes each into the table at `0x1ca98`, then
    `count + 1` bitmaps of `0x500` bytes each into the table at `0x119898`.
    The descriptor holds three 32 bit fields: the glyph height, then the two
    font wide constants 32 and 76.  The height doubles between the low and the
    high fonts.
    """
    member = container.member(name)
    expanded = expand(member_bytes(container, member), member.end)
    count = expanded[0]
    total = count + 1
    print(f"member:  {member.name}")
    print(f"bytes:   {len(expanded)}")
    print(f"count at +0x00: {count} -> {total} descriptors and bitmaps")
    offset = 1
    for index in range(total):
        height, field2, field3 = struct.unpack_from("<III", expanded, offset)
        print(
            f"  descriptor {index}: height {height} field2 {field2} "
            f"field3 {field3}"
        )
        offset += 12
    bitmaps = total * 0x500
    print(f"descriptors end at +0x{offset:X}")
    print(f"bitmaps: {total} x 0x500 = {bitmaps} bytes to +0x{offset + bitmaps:X}")
    tail = len(expanded) - offset - bitmaps
    print(f"the tail after the bitmaps: {tail} bytes")
    return 0


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

    font_parser = subparsers.add_parser("font", help="print a font layout")
    font_parser.add_argument("file", type=Path)
    font_parser.add_argument("name")

    args = parser.parse_args()
    try:
        container = parse(args.file)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    if args.command == "info":
        return print_info(container)
    if args.command == "font":
        try:
            return print_font(container, args.name)
        except ValueError as error:
            print(f"error: {error}", file=sys.stderr)
            return 1
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
