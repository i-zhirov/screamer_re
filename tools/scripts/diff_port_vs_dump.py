"""Find the first byte where the port diverges from the dumped plaintext.

The plaintext is stitched from the two pixel dumps at the split 0x7FD2C.  The port
expands the compressed member, and the pixel part of the expansion starts at the
offset 68, after the count of 8 bytes and the three records of 60 bytes.  The script
runs the port block by block and reports the first differing byte with the decoder
state that produced it.

    python3 tools/scripts/diff_port_vs_dump.py
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, "tools")

import gfx_container  # noqa: E402

GAME = Path.home() / "Documents/Screamer.app/Contents/Resources/game"
RUN = Path("work/entry/run")
PIXEL_START = 68
PIXEL_FIRST = 0x6BD00
PIXEL_SPLIT = 0x7FD2C
PIXEL_SIZE = 208568


def plaintext() -> bytes:
    first = PIXEL_SPLIT - PIXEL_FIRST
    head = (RUN / "EXP1.BIN").read_bytes()[:first]
    tail = (RUN / "EXP2.BIN").read_bytes()[: PIXEL_SIZE - first]
    return head + tail


def main() -> int:
    plain = plaintext()
    container = gfx_container.parse(GAME / "GFX/GAME.DAT")
    raw = gfx_container.member_bytes(container, container.member("GAMEL.SPR"))
    decoder = gfx_container._Lzss(raw, 1 << 20)
    print(f"plaintext {len(plain)} bytes, compressed {len(raw)} bytes")
    for block in range(5000):
        before, code, shift, bitpos, ring = (
            len(decoder.out), decoder.code, decoder.shift, decoder.bits, decoder.ring_index,
        )
        try:
            more = decoder.block()
        except ValueError as error:
            print(f"block {block}: stopped at output {len(decoder.out)}: {error}")
            return 1
        for index in range(before, len(decoder.out)):
            pixel = index - PIXEL_START
            if 0 <= pixel < len(plain) and decoder.out[index] != plain[pixel]:
                print(
                    f"first divergence: output {index}, pixel {pixel}, "
                    f"port 0x{decoder.out[index]:02x}, plain 0x{plain[pixel]:02x}"
                )
                print(
                    f"  block {block}, code 0x{code:03x}, shift {shift}, "
                    f"bit position {bitpos}, ring index {ring}"
                )
                return 1
        if not more:
            print(f"clean stop at block {block}, output {len(decoder.out)}")
            return 0
    print("no divergence in 5000 blocks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
