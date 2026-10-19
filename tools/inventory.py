#!/usr/bin/env python3
"""Generate a Markdown inventory of a directory with sizes and SHA-256 digests.

Usage:
    python3 tools/inventory.py <directory> [--title TITLE]

The tool prints a Markdown table sorted by path.  The table lists the file
size, the SHA-256 digest, and a coarse role for each file.
"""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

ROLES = {
    ".exe": "DOS executable",
    ".386": "DOS driver",
    ".dat": "game data",
    ".gog": "CD image (raw BIN)",
    ".cfg": "configuration",
    ".conf": "DOSBox configuration",
    ".tab": "game table",
    ".pdf": "document",
    ".txt": "text",
    ".ico": "icon",
    ".info": "GOG metadata",
    ".dylib": "macOS library",
    ".zip": "archive",
    ".sh": "shell script",
    ".apple": "AppleScript",
}


def role_of(path: Path) -> str:
    role = ROLES.get(path.suffix.lower())
    if role:
        return role
    if path.suffix == "":
        return "file (no extension)"
    return path.suffix.lower().lstrip(".")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("directory", type=Path)
    parser.add_argument("--title", default=None)
    args = parser.parse_args()

    root: Path = args.directory
    title = args.title or root.name
    files = sorted(path for path in root.rglob("*") if path.is_file())

    print(f"# Inventory: {title}")
    print()
    print(f"Root: `{root}`")
    print(f"Files: {len(files)}")
    print(f"Total size: {sum(path.stat().st_size for path in files)} bytes")
    print()
    print("| Path | Size | SHA-256 | Role |")
    print("| --- | ---: | --- | --- |")
    for path in files:
        relative = path.relative_to(root)
        size = path.stat().st_size
        print(f"| `{relative}` | {size} | `{sha256(path)}` | {role_of(path)} |")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
