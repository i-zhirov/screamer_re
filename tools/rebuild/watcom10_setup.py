"""Fetch and extract the period Watcom C/C++ 10.0a toolchain.

The original game carries the Watcom 10.0a run-time banner, so the period build
uses the Watcom C/C++ 10.0a toolchain.  The tool downloads the Watcom C/C++
10.0a CD-ROM image from the Internet Archive, verifies its SHA-256 digest, and
extracts the `WATCOM` tree to `work/watcom10/WATCOM`.  It writes only under
`work/` and never modifies the shipped artifacts.  The DOS tools (wcc386, wasm,
wlink) run under DOSBox-X, because they are DOS programs; the period build
harness is `tools/rebuild/harness_dos.py`.

The CD-ROM image is abandonware.  The Internet Archive item is
`watcomcc10.0cd-rom` and the image is `WATCOM_C10A.ISO`, dated 1994.

Usage:
    python3 tools/rebuild/watcom10_setup.py [--iso PATH] [--out-dir DIR]

Exit code 0 on success, 1 on a bad digest or a failed extraction, and 2 on a bad
argument.
"""
from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import sys
import urllib.request
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent
ARCHIVE_DIR = REPOSITORY_ROOT / "work/archive"
ISO_NAME = "WATCOM_C10A.ISO"
ISO_URL = f"https://archive.org/download/watcomcc10.0cd-rom/{ISO_NAME}"
ISO_SHA256 = "f51ed9358e7eaf6a0d292268bbbc15832530489dd8239a98f6938c244f1736fc"
TOOLCHAIN_TREE = "WATCOM"
EXTRACT_TOOL = "bsdtar"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def fetch(iso_path: Path) -> None:
    print(f"downloading {ISO_URL}")
    iso_path.parent.mkdir(parents=True, exist_ok=True)
    urllib.request.urlretrieve(ISO_URL, iso_path)


def extract(iso_path: Path, out_dir: Path) -> None:
    if shutil.which(EXTRACT_TOOL) is None:
        raise ValueError(
            f"the {EXTRACT_TOOL} command is missing; install it to extract the toolchain"
        )
    out_dir.mkdir(parents=True, exist_ok=True)
    completed = subprocess.run(
        [EXTRACT_TOOL, "-x", "-f", str(iso_path), "-C", str(out_dir), TOOLCHAIN_TREE],
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        sys.stderr.write(completed.stderr)
        raise ValueError(f"the extraction of {iso_path} failed")


def command_setup(iso_path: Path, out_dir: Path) -> int:
    if not iso_path.is_file():
        fetch(iso_path)
    actual = sha256(iso_path)
    if actual != ISO_SHA256:
        raise ValueError(
            f"bad digest of {iso_path}: expected {ISO_SHA256}, got {actual}"
        )
    print(f"verified {iso_path}: {actual}")
    toolchain = out_dir / TOOLCHAIN_TREE
    if toolchain.is_dir():
        print(f"the toolchain is already present at {toolchain}")
        return 0
    extract(iso_path, out_dir)
    print(f"extracted the toolchain to {toolchain}")
    return 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Fetch and extract the period Watcom C/C++ 10.0a toolchain."
    )
    parser.add_argument(
        "--iso",
        type=Path,
        default=ARCHIVE_DIR / ISO_NAME,
        help="the path of the CD-ROM image (default: work/archive/WATCOM_C10A.ISO)",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=REPOSITORY_ROOT / "work/watcom10",
        help="the extraction directory (default: work/watcom10)",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        return command_setup(arguments.iso, arguments.out_dir)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
