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


def expand(raw: bytes) -> bytes:
    """Expand the LZSS member data.

    The port follows the decoded structure: a bit buffer of 0x400 bytes, a bit
    position, a shift count that starts at 9 and grows to 12, a mask table with
    `(1 << n) - 1` for the shift counts 9 to 12, a ring buffer of 0x200 bytes,
    the literal code 0x100, and the end code 0x101.  The bit order and the match
    decoding are not confirmed, so the result is marked unconfirmed.
    """
    if len(raw) < 2:
        raise ValueError("the member is too short to expand")
    return raw


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
