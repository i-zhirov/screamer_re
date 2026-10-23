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
COMPRESSED_BIT = 0x2


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


def member_read(container: Container, member: Member) -> bytes:
    """Read one member: expand it when the flag bit 1 is set, else return raw.

    The open at `0x4c128` sets the flag at `file + 0x10` from bit 1 of the member
    flags, and the read at `0x4c4e5` runs the LZW expander at `0x4d3fc` when that
    flag is non-zero and the raw read at `0xc607b` when it is zero.  The `end` field
    of the member is the expanded size.
    """
    raw = member_bytes(container, member)
    if member.flag & COMPRESSED_BIT:
        return expand(raw, member.end)
    return raw


def dump_members(container: Container, out_dir: Path) -> int:
    """Write every member and check the expanded size against the end field."""
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f"file:    {container.path}")
    print(f"members: {len(container.members)}")
    failures = 0
    for member in container.members:
        data = member_read(container, member)
        path = out_dir / member.name
        path.write_bytes(data)
        if not member.flag & COMPRESSED_BIT:
            state = "raw"
        elif len(data) == member.end:
            state = "expanded, the end field matches"
        else:
            state = f"FAIL, the end field is {member.end}"
            failures += 1
        print(f"{member.name:<16} flag {member.flag} {len(data):>8} bytes  {state}")
    if failures:
        raise ValueError(f"{failures} members do not reach the end field")
    return 0


class _Lzw:
    """The LZW decoder of the member data.

    The stream is a plain LSB-first code stream: each code starts at the next bit,
    the low bit first, and the width runs from 9 to 12 bits.  The code 0x100
    clears the dictionary, the code 0x101 ends the stream, and the first free
    dictionary entry is 0x102.  The width grows when the next free entry reaches
    `1 << shift`, checked after the entry is added.

    The first port emulated the 0x400 byte buffer of the game reader at `0x4d844`
    and diverged at the first buffer refill, which made the logo sprites show the
    first rows and noise after them.  The plain bit stream is confirmed: the
    `LOGOH.SPR` sprite 0 renders the Virgin Interactive Entertainment logo.
    """

    CLEAR = 0x100
    END = 0x101
    FIRST_FREE = 0x102

    def __init__(self, raw: bytes, limit: int) -> None:
        self.raw = raw
        self.limit = limit
        self.bitpos = 0
        self.shift = 9
        self.table: dict[int, bytes] = {}
        self.next_code = self.FIRST_FREE
        self.prev: bytes | None = None
        self.out = bytearray()
        self.codes = 0
        self.stop_reason = "running"

    def _read_code(self) -> int | None:
        """Read one code from the bit stream, or None at the end of the stream."""
        shift = self.shift
        if self.bitpos + shift > len(self.raw) * 8:
            return None
        value = 0
        for i in range(shift):
            position = self.bitpos + i
            bit = (self.raw[position >> 3] >> (position & 7)) & 1
            value |= bit << i
        self.bitpos += shift
        return value

    def step(self) -> bool:
        """Decode one code and report whether the stream continues.

        The reason for the stop is recorded in `stop_reason`: `end-code` for the
        code `0x101`, `invalid-code` for a code that is not in the table with no
        previous entry, `out-of-bits` for the end of the bit stream, and `limit`
        for the output limit.
        """
        code = self._read_code()
        if code is None:
            self.stop_reason = "out-of-bits"
            return False
        self.codes += 1
        if code == self.END:
            self.stop_reason = "end-code"
            return False
        if code == self.CLEAR:
            self.table.clear()
            self.next_code = self.FIRST_FREE
            self.shift = 9
            self.prev = None
            return True
        if code < 0x100:
            entry = bytes([code])
        elif code in self.table:
            entry = self.table[code]
        elif self.prev is not None:
            entry = self.prev + self.prev[:1]
        else:
            self.stop_reason = "invalid-code"
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
                self.stop_reason = "limit"
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


def parse_font(out: bytes) -> tuple[list[tuple[int, int, int]], list[list[tuple[int, int]]], bytes]:
    """Parse a font member into the descriptors, the glyph tables, and the tail.

    The font loader at `0x25658` reads the count (one byte) at `+0x00`, then
    `count + 1` descriptors of 12 bytes each: the glyph height, the first
    character code, and the character count.  Then it reads one table per
    descriptor of `(width, offset)` pairs, a four byte tail size, and the tail
    with the packed four bit glyphs at those offsets.
    """
    count = out[0]
    total = count + 1
    descriptors = [struct.unpack_from("<III", out, 1 + i * 12) for i in range(total)]
    position = 1 + total * 12
    tables = []
    for _height, _first, number in descriptors:
        tables.append(
            [struct.unpack_from("<II", out, position + i * 8) for i in range(number)]
        )
        position += number * 8
    size = struct.unpack_from("<I", out, position)[0]
    return descriptors, tables, out[position + 4 : position + 4 + size]


def glyph_pixels(tail: bytes, offset: int, width: int, height: int) -> bytes:
    """Return one glyph as one byte per pixel with the four bit values.

    The glyph holds `height` rows of `width` pixels, two pixels per byte with the
    high nibble first.  The value 0 is a transparent pixel.
    """
    bytes_per_row = (width + 1) // 2
    pixels = bytearray(width * height)
    for row in range(height):
        for column in range(width):
            byte = tail[offset + row * bytes_per_row + (column >> 1)]
            pixels[row * width + column] = (byte >> 4) if (column & 1) == 0 else (byte & 0xF)
    return bytes(pixels)


def print_font(container: Container, name: str) -> int:
    """Print the font layout of a member.

    The loader at `0x25658` reads the count byte, the descriptors, the glyph
    tables, the tail size, and the tail.  Each descriptor is the glyph height,
    the first character code, and the character count.  Each table entry is the
    glyph width in pixels and the offset of the glyph in the tail.  The glyph
    holds the height rows of the width packed four bits per pixel.
    """
    member = container.member(name)
    expanded = expand(member_bytes(container, member), member.end)
    descriptors, tables, tail = parse_font(expanded)
    print(f"member:  {member.name}")
    print(f"bytes:   {len(expanded)}")
    print(f"count at +0x00: {expanded[0]} -> {len(descriptors)} fonts")
    position = 1 + len(descriptors) * 12
    for index, (height, first, number) in enumerate(descriptors):
        widths = [width for width, _offset in tables[index]]
        print(
            f"  font {index}: height {height}, the characters {first}..{first + number - 1}, "
            f"{number} glyphs, the widths {min(widths)}..{max(widths)} px"
        )
        position += number * 8
    print(f"the tables end at +0x{position:X}")
    print(f"the tail: {len(tail)} bytes at +0x{position + 4:X}")
    return 0


def print_render(container: Container, name: str, glyph: int) -> int:
    """Render one glyph of a font member as characters.

    The glyph index selects the entry of the first font table.  The entry is the
    glyph width and the offset of the glyph in the tail, and the descriptor
    gives the glyph height.  The glyph holds the height rows of the width packed
    four bits per pixel with the high nibble first, so a zero nibble is a
    transparent pixel.

    The draw routine at `0x25316` reads one byte, takes the high nibble with
    `and al, 0xf0` and `shr al, 4`, writes it, takes the low nibble with
    `and bl, 0xf`, writes it at `+1`, and skips the zero nibble, so the format
    is four bits per pixel with two pixels per byte and the high nibble first.
    """
    member = container.member(name)
    expanded = expand(member_bytes(container, member), member.end)
    descriptors, tables, tail = parse_font(expanded)
    if not tables:
        raise ValueError(f"{member.name} holds no font")
    height, first, number = descriptors[0]
    if not 0 <= glyph < number:
        raise ValueError(f"the glyph {glyph} is outside 0 to {number - 1}")
    width, offset = tables[0][glyph]
    print(f"member: {member.name}")
    print(f"font 0: height {height}, the characters {first}..{first + number - 1}")
    print(f"glyph:  {glyph} -> the character {first + glyph}, width {width} px")
    pixels = glyph_pixels(tail, offset, width, height)
    for row in range(height):
        line = "".join(
            "." if pixels[row * width + column] == 0 else "#"
            for column in range(width)
        )
        print(f"  {line}")
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


def print_lzw(path: Path, limit: int = 1 << 20) -> int:
    """Expand a plain LZW file and print the text.

    The `DATA` files (`MAP*.DAT`, `GEN.DAT`, `LOCAL.DAT`) hold one LZW stream
    with no container around it.  The reader is the same one the graphics members
    use.  The expanded bytes are text: the `MAP*.DAT` files hold the `TRACK_DATA`
    and the `OPPONENTS` lists, `GEN.DAT` holds the `CARS_DATA` list, and
    `LOCAL.DAT` holds the credits text.
    """
    raw = path.read_bytes()
    decoder = _Lzw(raw, limit)
    guard = 0
    while decoder.step():
        guard += 1
        if guard > len(raw) * 64 + 0x1000:
            decoder.stop_reason = "guard"
            break
    out = bytes(decoder.out)
    print(f"file:     {path}")
    print(f"raw:      {len(raw)} bytes")
    print(f"expanded: {len(out)} bytes")
    print(f"codes:    {decoder.codes}")
    print(f"stop:     {decoder.stop_reason}")
    text = out.decode("ascii", "replace")
    sys.stdout.write(text)
    if not text.endswith("\n"):
        sys.stdout.write("\n")
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

    render_parser = subparsers.add_parser("render", help="render a glyph as characters")
    render_parser.add_argument("file", type=Path)
    render_parser.add_argument("name")
    render_parser.add_argument("glyph", type=int)

    lzw_parser = subparsers.add_parser("lzw", help="expand a plain LZW data file")
    lzw_parser.add_argument("file", type=Path)

    dump_parser = subparsers.add_parser("dump", help="write every member and check the end size")
    dump_parser.add_argument("file", type=Path)
    dump_parser.add_argument("--out", type=Path, required=True)

    args = parser.parse_args()
    if args.command == "lzw":
        try:
            return print_lzw(args.file)
        except (OSError, ValueError) as error:
            print(f"error: {error}", file=sys.stderr)
            return 1
    try:
        container = parse(args.file)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    if args.command == "dump":
        try:
            return dump_members(container, args.out)
        except ValueError as error:
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
    if args.command == "render":
        try:
            return print_render(container, args.name, args.glyph)
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
