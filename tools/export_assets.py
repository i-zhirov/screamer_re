"""Export the Screamer graphics assets to PNG and build a whiteboard page.

The tool reads the graphics containers (`GFX/GAME.DAT`, `GFX/MENU.DAT`, and
`GFX/GRAF.DAT`), expands the LZW members, and writes PNG files under the
output directory.  It then writes `index.html`, a whiteboard page that shows
every asset with its name, its size, and the evidence note.

The sprite members use the confirmed pixel format: one byte per pixel with the
`MENU.COL` palette of 256 colours.  The font members use four bits per pixel,
two pixels per byte, with the high nibble first.  The PCX members hold their own
palette at the end of the file.

The tool uses the standard library only: `zlib` and `struct` write the PNG files.

Usage:

    python3 tools/export_assets.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game
    python3 tools/export_assets.py --game-dir GAME --out work/assets
"""

from __future__ import annotations

import argparse
import base64
import html
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import gfx_container


def write_png(path: Path, width: int, height: int, rgb: bytes) -> int:
    """Write an 8 bit RGB PNG from a flat buffer of r, g, b triples."""
    if len(rgb) != width * height * 3:
        raise ValueError(
            f"the pixel buffer holds {len(rgb)} bytes, not {width * height * 3}"
        )
    raw = bytearray()
    stride = width * 3
    for y in range(height):
        raw.append(0)
        raw += rgb[y * stride : (y + 1) * stride]

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", zlib.compress(bytes(raw), 6))
        + chunk(b"IEND", b"")
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)
    return len(png)


def scale_palette(palette: bytes, six_bit: bool = True) -> list[tuple[int, int, int]]:
    """Scale a palette of 768 bytes to 8 bit triples.

    The `MENU.COL` palette holds the six bit VGA values, so the tool scales
    them with a shift of two.  The PCX palette at the end of a PCX file already
    holds the eight bit values, so the tool keeps it.
    """
    if len(palette) < 768:
        raise ValueError(f"the palette holds {len(palette)} bytes, not 768")
    out = []
    for i in range(256):
        r, g, b = palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]
        if six_bit:
            out.append(((r << 2) | (r >> 4), (g << 2) | (g >> 4), (b << 2) | (b >> 4)))
        else:
            out.append((r, g, b))
    return out


def scale_pixels(pixels: bytes, width: int, height: int, scale: int) -> tuple[int, int, bytes]:
    """Nearest neighbour scale of an index buffer."""
    if scale <= 1:
        return width, height, pixels
    out = bytearray()
    for y in range(height):
        row = pixels[y * width : (y + 1) * width]
        big = bytearray()
        for value in row:
            big += bytes((value,)) * scale
        for _ in range(scale):
            out += big
    return width * scale, height * scale, bytes(out)


def indexed_to_rgb(pixels: bytes, palette: list[tuple[int, int, int]]) -> bytes:
    """Map the index bytes to the rgb triples."""
    out = bytearray()
    for value in pixels:
        r, g, b = palette[value]
        out += bytes((r, g, b))
    return bytes(out)


def decode_pcx(raw: bytes) -> tuple[int, int, bytes, bytes]:
    """Decode an 8 bit one plane PCX with the palette at the end."""
    if len(raw) < 128 or raw[0] != 0x0A:
        raise ValueError("not a PCX image")
    encoding, bits = raw[2], raw[3]
    xmin, ymin, xmax, ymax = struct.unpack_from("<4H", raw, 4)
    planes = raw[65]
    bpl = struct.unpack_from("<H", raw, 66)[0]
    if bits != 8 or planes != 1:
        raise ValueError(f"the PCX holds {bits} bits and {planes} planes")
    width = xmax - xmin + 1
    height = ymax - ymin + 1
    pixels = bytearray()
    pos = 128
    while len(pixels) < bpl * height and pos < len(raw) - 769:
        value = raw[pos]
        pos += 1
        if encoding == 1 and (value & 0xC0) == 0xC0:
            count = value & 0x3F
            pixels += bytes((raw[pos],)) * count
            pos += 1
        else:
            pixels.append(value)
    # the palette sits at the end of the file behind the 0x0c marker
    palette = raw[-768:]
    return width, height, bytes(pixels[: bpl * height]), palette


def export_sprites(
    container, palette, out_dir: Path, notes: list[str], scale: int = 1
) -> list[tuple[str, Path]]:
    """Export the sprite members and return the file list."""
    files = []
    for member in container.members:
        if not member.name.endswith(".SPR"):
            continue
        out = gfx_container.expand(gfx_container.member_bytes(container, member), member.end)
        count, size = struct.unpack_from("<II", out, 0)
        records = [struct.unpack_from("<IIIII", out, 8 + i * 20) for i in range(count)]
        pixels = out[8 + count * 20 : 8 + count * 20 + size]
        # stack the sprites vertically, one under the other
        sheet_w = max(r[2] for r in records) if records else 1
        sheet_h = sum(r[3] for r in records)
        canvas = bytearray(sheet_w * sheet_h)
        y = 0
        for x, yy, w, h, offset in records:
            for row in range(h):
                src = offset + row * w
                dst = (y + row) * sheet_w
                canvas[dst : dst + w] = pixels[src : src + w]
            y += h
        sheet_w, sheet_h, canvas = scale_pixels(bytes(canvas), sheet_w, sheet_h, scale)
        rgb = indexed_to_rgb(canvas, palette)
        path = out_dir / f"{member.name.replace('.', '_')}.png"
        write_png(path, sheet_w, sheet_h, rgb)
        notes.append(
            f"{member.name}: {len(out) } expanded bytes, the count {count}, "
            f"the size {size}, the sheet {sheet_w} by {sheet_h}"
        )
        files.append((member.name, path))
    return files


def export_fonts(
    container, out_dir: Path, notes: list[str], scale: int = 1
) -> list[tuple[str, Path]]:
    """Export the font members as glyph sheets in the confirmed 4 bit format."""
    files = []
    for member in container.members:
        if not member.name.endswith(".FNT"):
            continue
        out = gfx_container.expand(gfx_container.member_bytes(container, member), member.end)
        count = out[0]
        total = count + 1
        descriptors = [struct.unpack_from("<III", out, 1 + i * 12) for i in range(total)]
        base = 1 + total * 12
        blocks = [out[base + i * 0x500 : base + (i + 1) * 0x500] for i in range(total)]
        tail = out[base + total * 0x500 :]
        blocks += [
            tail[i * 0x500 : (i + 1) * 0x500]
            for i in range(len(tail) // 0x500)
        ]
        # a sheet of glyph cells of 32 by 40 pixels, eight cells per row
        per_row = 8
        rows = (len(blocks) + per_row - 1) // per_row
        cell_w, cell_h = 32, 40
        sheet_w, sheet_h = per_row * cell_w, rows * cell_h
        canvas = bytearray(sheet_w * sheet_h)
        for index, block in enumerate(blocks):
            gx, gy = (index % per_row) * cell_w, (index // per_row) * cell_h
            for row in range(cell_h):
                for x in range(cell_w):
                    byte = block[(row * cell_w + x) >> 1]
                    nibble = (byte >> 4) if (x & 1) == 0 else (byte & 0xF)
                    canvas[(gy + row) * sheet_w + gx + x] = nibble * 17
        sheet_w, sheet_h, canvas = scale_pixels(bytes(canvas), sheet_w, sheet_h, scale)
        rgb = bytes()
        for value in canvas:
            rgb += bytes((value, value, value))
        path = out_dir / f"{member.name.replace('.', '_')}_sheet.png"
        write_png(path, sheet_w, sheet_h, rgb)
        notes.append(
            f"{member.name}: {len(out)} expanded bytes, the count {count}, "
            f"{total} header glyphs and {len(blocks) - total} tail glyphs"
        )
        files.append((member.name, path))
    return files


def export_pcx(container, out_dir: Path, notes: list[str]) -> list[tuple[str, Path]]:
    """Export the PCX members, which hold their own palette."""
    files = []
    for member in container.members:
        if not member.name.endswith(".PCX"):
            continue
        raw = gfx_container.member_bytes(container, member)
        try:
            width, height, pixels, palette = decode_pcx(raw)
        except ValueError as error:
            notes.append(f"{member.name}: {error}")
            continue
        rgb = indexed_to_rgb(pixels, scale_palette(palette, six_bit=False))
        path = out_dir / f"{member.name.replace('.', '_')}.png"
        write_png(path, width, height, rgb)
        notes.append(f"{member.name}: {width} by {height} pixels")
        files.append((member.name, path))
    return files


def build_page(
    out_dir: Path,
    sections: list[tuple[str, list[tuple[str, Path]]]],
    notes: list[str],
    embed: bool = False,
) -> Path:
    """Write the whiteboard page with every asset."""
    parts = [
        "<!doctype html>",
        "<html lang=\"en\"><head><meta charset=\"utf-8\">",
        "<title>Screamer assets</title>",
        "<style>",
        "body{font-family:ui-monospace,monospace;background:#14161a;color:#e6e6e6;margin:2rem}",
        "h1{font-size:1.3rem}h2{font-size:1.05rem;margin-top:2rem;color:#9fd0ff}",
        ".grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(220px,1fr));gap:1rem}",
        ".card{background:#1d2026;border:1px solid #2e3440;border-radius:6px;padding:.6rem}",
        ".card img{width:100%;height:auto;image-rendering:pixelated;background:#000}",
        ".name{font-size:.8rem;color:#9fd0ff;word-break:break-all}",
        ".size{font-size:.72rem;color:#8b93a1}",
        "ul{font-size:.8rem;color:#b8c0cc;line-height:1.6}",
        "</style></head><body>",
        "<h1>Screamer (1995) - extracted assets</h1>",
        "<p>Every image is decoded from the shipped containers with the confirmed formats: "
        "the LZW member decoder, the sprite pixel format of one byte per pixel, the font "
        "pixel format of four bits per pixel, and the PCX palette at the end of the file.</p>",
    ]
    for title, files in sections:
        parts.append(f"<h2>{html.escape(title)} ({len(files)})</h2>")
        parts.append("<div class=\"grid\">")
        for name, path in files:
            size = path.stat().st_size
            if embed:
                data = base64.b64encode(path.read_bytes()).decode("ascii")
                src = f"data:image/png;base64,{data}"
            else:
                src = path.name
            parts.append(
                f"<div class=\"card\"><img src=\"{src}\" loading=\"lazy\" "
                f"alt=\"{html.escape(name)}\">"
                f"<div class=\"name\">{html.escape(name)}</div>"
                f"<div class=\"size\">{size} bytes</div></div>"
            )
        parts.append("</div>")
    parts.append("<h2>Notes</h2><ul>")
    for note in notes:
        parts.append(f"<li>{html.escape(note)}</li>")
    parts.append("</ul></body></html>")
    page = out_dir / "index.html"
    page.parent.mkdir(parents=True, exist_ok=True)
    page.write_text("\n".join(parts))
    return page


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game-dir", type=Path, required=True)
    parser.add_argument("--out", type=Path, default=Path("work/assets"))
    parser.add_argument(
        "--scale",
        type=int,
        default=4,
        help="scale the sprite and font images by whole numbers",
    )
    parser.add_argument(
        "--embed",
        action="store_true",
        help="embed the images in the page instead of linking them",
    )
    args = parser.parse_args()

    game = args.game_dir
    menu = gfx_container.parse(game / "GFX/MENU.DAT")
    palette = gfx_container.member_bytes(menu, menu.member("MENU.COL"))
    scaled = scale_palette(palette)
    print(f"the palette MENU.COL: {len(palette)} bytes, 256 colours")

    notes: list[str] = []
    sprite_files: list[tuple[str, Path]] = []
    font_files: list[tuple[str, Path]] = []
    pcx_files: list[tuple[str, Path]] = []

    for name in ("GFX/GAME.DAT", "GFX/GRAF.DAT"):
        container = gfx_container.parse(game / name)
        sprite_files += export_sprites(container, scaled, args.out, notes, args.scale)
        font_files += export_fonts(container, args.out, notes, args.scale)
        pcx_files += export_pcx(container, args.out, notes)

    page = build_page(
        args.out,
        [
            ("Sprite members", sprite_files),
            ("Font members", font_files),
            ("PCX images", pcx_files),
        ],
        notes,
        embed=args.embed,
    )
    print(f"the sprites: {len(sprite_files)}")
    print(f"the fonts:   {len(font_files)}")
    print(f"the PCX:     {len(pcx_files)}")
    print(f"the page:    {page}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
