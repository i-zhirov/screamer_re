#!/usr/bin/env python3
"""Extract the data track of the Screamer CD image into a plain ISO 9660 image.

The GOG distribution ships the CD as a raw BIN file (SCREAM.gog) with a CUE
sheet (SCREAM.DAT).  The data track uses 2352-byte sectors (MODE1/2352), while
ISO 9660 parsers expect 2048-byte sectors.  This tool strips the 16-byte sync
and header plus the 288-byte EDC/ECC tail of every data sector.

Usage:
    python3 tools/extract_cd_image.py <cue-file> <output-iso>
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

TRACK_RE = re.compile(
    r'TRACK\s+(?P<number>\d+)\s+(?P<mode>\S+)(?:\s+(?P<submode>\S+))?',
    re.IGNORECASE,
)
INDEX_RE = re.compile(r'INDEX\s+(?P<index>\d+)\s+(?P<msf>\d+:\d+:\d+)')
FILE_RE = re.compile(r'FILE\s+"(?P<name>[^"]+)"\s+(?P<type>\S+)', re.IGNORECASE)


def msf_to_sectors(msf: str) -> int:
    minutes, seconds, frames = (int(part) for part in msf.split(":"))
    return (minutes * 60 + seconds) * 75 + frames


def parse_cue(cue_path: Path) -> tuple[str, str, int, int]:
    """Return (image name, mode, start sector, end sector) of the first track."""
    image_name = None
    mode = None
    start = None
    next_start = None
    for raw_line in cue_path.read_text(encoding="ascii", errors="replace").splitlines():
        line = raw_line.strip()
        if not line:
            continue
        file_match = FILE_RE.match(line)
        if file_match and image_name is None:
            image_name = file_match.group("name")
            continue
        track_match = TRACK_RE.match(line)
        if track_match:
            if mode is None:
                mode = f"{track_match.group('mode')}/{track_match.group('submode') or ''}".rstrip("/")
            continue
        index_match = INDEX_RE.match(line)
        if index_match and int(index_match.group("index")) == 1:
            sector = msf_to_sectors(index_match.group("msf"))
            if start is None:
                start = sector
            elif next_start is None:
                next_start = sector
                break
    if image_name is None or mode is None or start is None:
        raise SystemExit(f"cannot parse CUE sheet: {cue_path}")
    return image_name, mode, start, next_start


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    cue_path = Path(sys.argv[1]).resolve()
    output_path = Path(sys.argv[2]).resolve()

    image_name, mode, start, end = parse_cue(cue_path)
    image_path = cue_path.parent / image_name
    if not image_path.exists():
        raise SystemExit(f"missing image file: {image_path}")
    if mode != "MODE1/2352":
        raise SystemExit(f"unsupported track mode: {mode}")

    raw = image_path.read_bytes()
    total_sectors = len(raw) // 2352
    if end is None:
        end = total_sectors

    with output_path.open("wb") as out:
        for sector in range(start, end):
            offset = sector * 2352
            out.write(raw[offset + 16 : offset + 16 + 2048])

    print(f"wrote {output_path} ({end - start} sectors, {(end - start) * 2048} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
