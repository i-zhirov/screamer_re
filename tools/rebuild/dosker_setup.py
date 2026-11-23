"""Prepare the Dosker environments for the rebuild tools.

The setup checks the pinned Dosker version, ensures the FreeDOS base image,
creates the two 256 MiB toolchain volumes, copies the DOS-hosted OpenWatcom V2
tree and the period Watcom 10.0a tree into them, extracts the DOS/4G 2.01a
extender from the pinned DOSBox-X binary, and builds the `screamer-run:1` run
image with the extender.  The V2 toolchain is 121 MiB, so it
cannot live in the 32 MiB FreeDOS image.  The 2.01a extender is the one the
FreeDOS base accepts; the shipped 1.95 and the V2 1.97 reject the V2 test stub
with DOS/4GW error 1012.  The setup records the digests under
`work/dosker/setup.json`.

Usage:
    python3 tools/rebuild/dosker_setup.py --toolchain PATH [--toolchain10 PATH]
    python3 tools/rebuild/dosker_setup.py --check

The tool takes every path from the command line.  It uses `mcopy` and `mdir`
from mtools for the FAT volume.  It enumerates every copied name with the DOS
8.3 rule before the copy and fails with the offending path.  The image and
volume names are flat, because Dosker 0.1.3 rejects a name that holds a slash.
The DOS/4G 2.01a extender is embedded in DOSBox-X 2026.10.01; its offset and
digest are pinned to that build.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import dosker  # noqa: E402

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent
DEFAULT_DATA_ROOT = REPOSITORY_ROOT / "work/dosker/root"
SETUP_RECORD = REPOSITORY_ROOT / "work/dosker/setup.json"
VOLUME_OFFSET = 1048576
VOLUME_NAME = dosker.TOOLCHAIN_VOLUME
TOOLCHAIN_FILE = "BINW/WCC386.EXE"
TOOLCHAIN10_VOLUME = "screamer-watcom10-tools"
TOOLCHAIN10_FILE = "BINB/WCC386.EXE"
TOOLCHAIN10_DIR = REPOSITORY_ROOT / "work/watcom10/WATCOM"
NAME_PATTERN = re.compile(
    r"^[A-Z0-9_^$~!#%&@(){}'`+-]{1,8}(\.[A-Z0-9_^$~!#%&@(){}'`+-]{1,3})?$"
)
DOSBOX_X_SHA256 = "6934abe66fe55d71487ded37e097416de65981250c902a9cca2bfd1284e98c96"
EXTENDER_OFFSET = 0x7CFC67
EXTENDER_SIZE = 0x56584
EXTENDER_SHA256 = "87cb2fb20a83adde247e08b2dd98c3001392e7f5cca1faa17277ccd75186b919"
EXTENDER_PATH = REPOSITORY_ROOT / "work/dosker/DOS4GW.EXE"
RUN_IMAGE = "screamer-run:1"
RUN_CONTEXT = REPOSITORY_ROOT / "work/dosker/run-context"


def file_digest(path: Path) -> str:
    """The SHA-256 digest of one file, streamed."""
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1048576), b""):
            digest.update(chunk)
    return digest.hexdigest()


def extract_extender(
    dosbox_x: Path,
    out: Path,
    binary_digest: str = DOSBOX_X_SHA256,
    extender_digest: str = EXTENDER_SHA256,
) -> str:
    """Extract the embedded DOS/4G 2.01a extender from the pinned DOSBox-X."""
    if not dosbox_x.is_file():
        raise ValueError(f"the DOSBox-X binary {dosbox_x} is missing")
    actual = file_digest(dosbox_x)
    if binary_digest and actual != binary_digest:
        raise ValueError(
            f"the DOSBox-X binary {dosbox_x} has digest {actual}; the pinned "
            f"build is {binary_digest}; the DOS/4G 2.01a offset depends on "
            "the build"
        )
    with dosbox_x.open("rb") as handle:
        handle.seek(EXTENDER_OFFSET)
        data = handle.read(EXTENDER_SIZE)
    if len(data) != EXTENDER_SIZE or data[:2] != b"MZ":
        raise ValueError(
            f"the DOSBox-X binary {dosbox_x} holds no DOS/4G 2.01a at "
            f"0x{EXTENDER_OFFSET:x}"
        )
    actual_extender = hashlib.sha256(data).hexdigest()
    if extender_digest and actual_extender != extender_digest:
        raise ValueError(
            f"the DOS/4G 2.01a at 0x{EXTENDER_OFFSET:x} has digest "
            f"{actual_extender}; the pinned extender is {extender_digest}"
        )
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(data)
    return actual_extender


def check_names(root: Path) -> list[str]:
    """Return the paths under root whose names are not DOS 8.3 names."""
    violations: list[str] = []
    for path in root.rglob("*"):
        if not NAME_PATTERN.fullmatch(path.name.upper()):
            violations.append(str(path.relative_to(root)))
    return violations


def mtools_available() -> bool:
    """True when `mcopy` and `mdir` are on the host."""
    return shutil.which("mcopy") is not None and shutil.which("mdir") is not None


def volume_has_toolchain(image: Path, compiler: str = TOOLCHAIN_FILE) -> bool:
    """True when the volume image holds the named compiler."""
    completed = subprocess.run(
        ["mdir", "-i", f"{image}@@{VOLUME_OFFSET}", f"::WATCOM/{compiler}"],
        capture_output=True,
        text=True,
        env={**os.environ, "MTOOLS_SKIP_CHECK": "1"},
    )
    return completed.returncode == 0


def install_toolchain(image: Path, toolchain: Path) -> None:
    """Copy the V2 or 10.0a tree into the volume image."""
    violations = check_names(toolchain)
    if violations:
        raise ValueError(
            f"the toolchain {toolchain} holds names that are not 8.3: "
            + ", ".join(violations[:5])
        )
    env = {**os.environ, "MTOOLS_SKIP_CHECK": "1"}
    subprocess.run(
        ["mmd", "-i", f"{image}@@{VOLUME_OFFSET}", "::WATCOM"],
        capture_output=True,
        env=env,
        check=False,
    )
    completed = subprocess.run(
        [
            "mcopy",
            "-s",
            "-m",
            "-i",
            f"{image}@@{VOLUME_OFFSET}",
            *[str(entry) for entry in toolchain.iterdir()],
            "::WATCOM",
        ],
        capture_output=True,
        text=True,
        env=env,
    )
    if completed.returncode != 0:
        raise ValueError(
            "cannot copy the toolchain into the volume: "
            + (completed.stderr.strip() or "no message")
        )


def ensure_base(client: dosker.DoskerClient, agent: Path | None, freedos: Path | None) -> None:
    """Ensure the FreeDOS base image, with the given inputs."""
    if client.image_exists(dosker.DEBUG_IMAGE):
        return
    if agent is None or freedos is None:
        raise ValueError(
            "the freedos:1.3 base is missing; pass --agent and --freedos "
            "to build it"
        )
    os.environ["DOSKER_AGENT_EXE"] = str(agent)
    os.environ["DOSKER_FREEDOS_SOURCE"] = str(freedos)
    context = REPOSITORY_ROOT / "work/dosker/base-context"
    context.mkdir(parents=True, exist_ok=True)
    (context / "Doskerfile").write_text(f"FROM {dosker.DEBUG_IMAGE}\n")
    client.build_image(dosker.DEBUG_IMAGE, context)


def ensure_run_image(client: dosker.DoskerClient, extender: Path) -> None:
    """Build the screamer-run:1 image with the DOS/4G 2.01a extender."""
    if client.image_exists(RUN_IMAGE):
        return
    context = RUN_CONTEXT
    (context / "extender").mkdir(parents=True, exist_ok=True)
    shutil.copy2(extender, context / "extender" / "DOS4GW.EXE")
    (context / "Doskerfile").write_text(
        f"FROM {dosker.DEBUG_IMAGE}\n"
        "COPY extender/ C:/\n"
        "RUN DIR C:\\DOS4GW.EXE\n"
    )
    client.build_image(RUN_IMAGE, context)


def prepare(arguments: argparse.Namespace) -> int:
    if not mtools_available():
        raise ValueError("mcopy and mdir from mtools are required")
    client = dosker.DoskerClient(
        arguments.dosker, arguments.data_root, REPOSITORY_ROOT / "work/rebuild"
    )
    client.check_version()
    ensure_base(client, arguments.agent, arguments.freedos)
    client.volume_create(VOLUME_NAME)
    image = client.volume_image(VOLUME_NAME)
    if not image.is_file():
        raise ValueError(f"the volume image {image} is missing")
    if arguments.toolchain is None:
        raise ValueError("pass --toolchain to install the V2 toolchain")
    if not (arguments.toolchain / "BINW" / "WCC386.EXE").is_file():
        raise ValueError(
            f"the toolchain {arguments.toolchain} holds no BINW/WCC386.EXE"
        )
    if not volume_has_toolchain(image):
        print("setup: copying the V2 toolchain into the volume")
        install_toolchain(image, arguments.toolchain)
    if not volume_has_toolchain(image):
        raise ValueError("the V2 toolchain is not on the volume after the copy")
    client.volume_create(TOOLCHAIN10_VOLUME)
    image10 = client.volume_image(TOOLCHAIN10_VOLUME)
    if not image10.is_file():
        raise ValueError(f"the 10.0a volume image {image10} is missing")
    if arguments.toolchain10 is None:
        raise ValueError("pass --toolchain10 to install the Watcom 10.0a tree")
    if not (arguments.toolchain10 / TOOLCHAIN10_FILE).is_file():
        raise ValueError(
            f"the 10.0a toolchain {arguments.toolchain10} holds no "
            f"{TOOLCHAIN10_FILE}"
        )
    if not volume_has_toolchain(image10, TOOLCHAIN10_FILE):
        print("setup: copying the Watcom 10.0a toolchain into the volume")
        install_toolchain(image10, arguments.toolchain10)
    if not volume_has_toolchain(image10, TOOLCHAIN10_FILE):
        raise ValueError(
            "the Watcom 10.0a toolchain is not on the volume after the copy"
        )
    dosbox_x = arguments.dosbox_x or shutil.which("dosbox-x")
    if dosbox_x is None:
        raise ValueError(
            "pass --dosbox-x, or install dosbox-x, to extract the DOS/4G 2.01a"
        )
    extender = arguments.extender
    extender_digest = extract_extender(Path(dosbox_x), extender)
    ensure_run_image(client, extender)
    record = {
        "dosker": client.version(),
        "data_root": str(client.data_root),
        "base": dosker.DEBUG_IMAGE,
        "base_digest": client.image_digest(dosker.DEBUG_IMAGE),
        "volume": VOLUME_NAME,
        "volume_image": str(image),
        "volume_digest": file_digest(image),
        "toolchain": str(arguments.toolchain),
        "volume10": TOOLCHAIN10_VOLUME,
        "volume10_image": str(image10),
        "volume10_digest": file_digest(image10),
        "toolchain10": str(arguments.toolchain10),
        "dosbox_x": str(dosbox_x),
        "extender": str(extender),
        "extender_digest": extender_digest,
        "run_image": RUN_IMAGE,
        "run_image_digest": client.image_digest(RUN_IMAGE),
    }
    SETUP_RECORD.parent.mkdir(parents=True, exist_ok=True)
    SETUP_RECORD.write_text(json.dumps(record, indent=1) + "\n")
    print(f"setup: dosker {record['dosker']}")
    print(f"setup: base {record['base']} {record['base_digest']}")
    print(
        f"setup: volume {VOLUME_NAME} with the V2 toolchain "
        f"{record['volume_digest']}"
    )
    print(
        f"setup: volume {TOOLCHAIN10_VOLUME} with the Watcom 10.0a toolchain "
        f"{record['volume10_digest']}"
    )
    print(f"setup: DOS/4G 2.01a extender {extender} {extender_digest}")
    print(f"setup: run image {RUN_IMAGE} {record['run_image_digest']}")
    print(f"setup: record {SETUP_RECORD}")
    return 0


def check(arguments: argparse.Namespace) -> int:
    client = dosker.DoskerClient(
        arguments.dosker, arguments.data_root, REPOSITORY_ROOT / "work/rebuild"
    )
    version = client.check_version()
    base = client.image_exists(dosker.DEBUG_IMAGE)
    volume = client.volume_exists(VOLUME_NAME)
    image = client.volume_image(VOLUME_NAME)
    toolchain = volume and image.is_file() and mtools_available() and volume_has_toolchain(image)
    volume10 = client.volume_exists(TOOLCHAIN10_VOLUME)
    image10 = client.volume_image(TOOLCHAIN10_VOLUME)
    toolchain10 = (
        volume10
        and image10.is_file()
        and mtools_available()
        and volume_has_toolchain(image10, TOOLCHAIN10_FILE)
    )
    run_image = client.image_exists(RUN_IMAGE)
    extender = arguments.extender
    extender_present = extender.is_file() and file_digest(extender) == EXTENDER_SHA256
    print(f"check: dosker {version}")
    print(f"check: base {dosker.DEBUG_IMAGE}: {'present' if base else 'missing'}")
    print(f"check: volume {VOLUME_NAME}: {'present' if volume else 'missing'}")
    print(f"check: toolchain V2: {'present' if toolchain else 'missing'}")
    print(f"check: volume {TOOLCHAIN10_VOLUME}: {'present' if volume10 else 'missing'}")
    print(f"check: toolchain 10.0a: {'present' if toolchain10 else 'missing'}")
    print(f"check: run image {RUN_IMAGE}: {'present' if run_image else 'missing'}")
    print(f"check: DOS/4G 2.01a extender: {'present' if extender_present else 'missing'}")
    return 0 if base and toolchain and toolchain10 and run_image and extender_present else 1


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Prepare the Dosker V2 and Watcom 10.0a build environments."
    )
    parser.add_argument("--dosker", default="dosker")
    parser.add_argument("--data-root", type=Path, default=DEFAULT_DATA_ROOT)
    parser.add_argument(
        "--toolchain",
        type=Path,
        help="the extracted DOS-hosted OpenWatcom V2 tree",
    )
    parser.add_argument(
        "--toolchain10",
        type=Path,
        default=TOOLCHAIN10_DIR,
        help="the extracted Watcom 10.0a WATCOM tree",
    )
    parser.add_argument(
        "--agent", type=Path, help="DOSKER.EXE for a missing base image"
    )
    parser.add_argument(
        "--freedos", type=Path, help="the FreeDOS boot floppy for a missing base"
    )
    parser.add_argument(
        "--dosbox-x",
        type=Path,
        help="the pinned DOSBox-X binary with the DOS/4G 2.01a extender",
    )
    parser.add_argument(
        "--extender",
        type=Path,
        default=EXTENDER_PATH,
        help="the output path of the DOS/4G 2.01a extender",
    )
    parser.add_argument(
        "--check", action="store_true", help="verify the setup and build nothing"
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        return check(arguments) if arguments.check else prepare(arguments)
    except dosker.DoskerMissing as error:
        print(f"skip: {error}")
        return 3
    except dosker.DoskerError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
