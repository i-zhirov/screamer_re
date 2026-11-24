"""Build one rebuilt module with the period Watcom C/C++ 10.0a toolchain.

The original game carries the Watcom 10.0a run-time banner, so the period build
uses the Watcom C/C++ 10.0a toolchain.  The DOS tools of that toolchain (wcc386,
wasm, wlink) run under DOSBox-X, because they are DOS programs.  This tool reads
the module record, compiles the sources with the period compiler, links the
executable and the map with the period linker, and writes them under
`work/rebuild/`.  The link uses the standard run-time library (the default
libs), because that is the flow that produces a runnable DOS/4G executable; the
module symbol comes from the linker map, so `tools/rebuild/diff_module.py` reads
it as usual.

The toolchain runs alongside the pinned OpenWatcom V2 container of
`tools/rebuild/harness.py`; it does not replace it.

Usage:
    python3 tools/rebuild/harness_dos.py build --module NAME [--timeout SECONDS]
    python3 tools/rebuild/harness_dos.py digest --module NAME

Exit code 0 on success, 2 on a compile or a link failure or a missing module
record, and 3 when DOSBox-X or the period toolchain is missing, because a
missing engine is a skip.
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import store  # noqa: E402

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent
REBUILD_DIR = Path(__file__).resolve().parent
WORK_DIR = REPOSITORY_ROOT / "work/rebuild"
TOOLCHAIN_DIR = REPOSITORY_ROOT / "work/watcom10"
DOSBOX = "dosbox-x"
CONF_NAME = "dos_build.conf"
RSP_NAME = "dos_link.rsp"
LOG_NAME = "dos_build.log"
MAP_NAME = "M.MAP"
WCC_FLAGS = ("-bt=dos", "-s", "-zq")
WASM_FLAGS = ("-bt=dos", "-zq")


class ToolchainLayout:
    """The mount layout of a Watcom toolchain tree."""

    def __init__(self, root: Path, bin: str, include: str, watcom: str) -> None:
        self.root = root
        self.bin = bin
        self.include = include
        self.watcom = watcom


def toolchain_layout(root: Path) -> ToolchainLayout:
    """Detects the 10.0a layout or the DOS-hosted V2 layout."""
    if (root / "WATCOM/BINB/WCC386.EXE").is_file():
        return ToolchainLayout(root, "WATCOM/BINB", "WATCOM/H", "WATCOM")
    if (root / "binw/wcc386.exe").is_file():
        return ToolchainLayout(root, "binw", "h", "")
    raise ValueError(f"the toolchain {root} has no known layout")


def object_name(index: int) -> str:
    return f"O{index}.OBJ"


def source_name(index: int, source: dict) -> str:
    """The DOS copy name of a source, safe for the 8.3 name limit.

    The DOS tools truncate a long name to eight characters.  A module source and its
    test harness, for example `shutdown.c` and `shutdown_main.c`, then share the name
    `SHUTDOWN.C`, and the compiler reads the first file twice.  The index prefix keeps
    the copy names unique.
    """
    suffix = ".ASM" if source["compiler"] == "wasm" else ".C"
    return f"S{index}{suffix}"


def compile_line(source: dict, index: int, layout: ToolchainLayout) -> str:
    compiler = source["compiler"]
    flags = list(WCC_FLAGS if compiler == "wcc386" else WASM_FLAGS)
    if compiler == "wcc386":
        flags.append(f"-i=C:\\{layout.include}")
    flags += list(source.get("period_flags", source.get("flags", [])))
    return " ".join([compiler, *flags, f"-fo=D:\\{object_name(index)}", source_name(index, source)])


def link_rsp(module: dict, count: int) -> str:
    """The wlink response file: the standard run-time, one `file` per object.

    The wlink 10.0a `file` directive takes one object, so the objects are
    repeated.  The map name is short, because the linker truncates long names;
    the harness copies the map to the record name afterwards.
    """
    lines = ["system dos4g", "option quiet"]
    if "map" in module:
        lines.append(f"option map=D:\\{MAP_NAME}")
    lines.append(f"name {module['exe']}")
    lines += [f"file {object_name(index)}" for index in range(count)]
    return "\n".join(lines) + "\n"


def conf_text(module: dict, count: int, layout: ToolchainLayout) -> str:
    lines = [
        "[dosbox]",
        "machine=svga_s3",
        "memsize=32",
        "",
        "[dos]",
        "lfn = true",
        "",
        "[autoexec]",
        f"mount C {layout.root}",
        f"mount D {WORK_DIR}",
        "C:",
        f"set PATH=C:\\{layout.bin.upper()}",
        f"set WATCOM=C:\\{layout.watcom}" if layout.watcom else "set WATCOM=C:\\",
        f"set INCLUDE=C:\\{layout.include.upper()}",
        f"set DOS4GPATH=C:\\{layout.bin.upper()}",
        "D:",
    ]
    for index, source in enumerate(module["sources"]):
        lines.append(f"{compile_line(source, index, layout)} >> D:\\{LOG_NAME}")
    lines.append(f"C:\\{layout.bin.upper()}\\WLINK @D:\\{RSP_NAME} >> D:\\{LOG_NAME}")
    lines.append("exit")
    return "\n".join(lines) + "\n"


def wait_for_artifacts(module: dict, timeout: int) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if (WORK_DIR / module["exe"]).is_file():
            if "map" not in module or (WORK_DIR / MAP_NAME).is_file():
                time.sleep(2)
                return True
        time.sleep(2)
    return False


def command_build(name: str, timeout: int, toolchain: Path | None = None) -> int:
    module = store.load_module(name)
    if shutil.which(DOSBOX) is None:
        print(f"skip: the {DOSBOX} command is missing; install DOSBox-X to build {name}")
        return 3
    root = toolchain if toolchain is not None else TOOLCHAIN_DIR
    try:
        layout = toolchain_layout(root)
    except ValueError:
        print(
            "skip: the period toolchain is missing; run"
            " tools/rebuild/watcom10_setup.py to install it",
            file=sys.stderr,
        )
        return 3
    WORK_DIR.mkdir(parents=True, exist_ok=True)
    for stale in WORK_DIR.glob("O*.OBJ"):
        stale.unlink()
    for index, source in enumerate(module["sources"]):
        shutil.copy2(
            REBUILD_DIR / source["file"], WORK_DIR / source_name(index, source)
        )
    (WORK_DIR / RSP_NAME).write_text(link_rsp(module, len(module["sources"])))
    (WORK_DIR / CONF_NAME).write_text(conf_text(module, len(module["sources"]), layout))
    log_path = WORK_DIR / LOG_NAME
    if log_path.is_file():
        log_path.unlink()
    process = subprocess.Popen(
        [DOSBOX, "-nogui", "-conf", CONF_NAME],
        cwd=WORK_DIR,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    try:
        if not wait_for_artifacts(module, timeout):
            text = ""
            if log_path.is_file():
                text = log_path.read_text(errors="replace")
            if process.poll() is not None:
                raise ValueError(f"the build of {name} failed\n{text}")
            raise ValueError(f"the build of {name} timed out\n{text}")
    finally:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
    if "map" in module:
        target = WORK_DIR / module["map"]
        (WORK_DIR / MAP_NAME).replace(target)
    artifacts = [module["exe"]]
    if "map" in module:
        artifacts.append(module["map"])
    print(f"built {name}: " + ", ".join(str(WORK_DIR / a) for a in artifacts))
    return 0


def command_digest(name: str) -> int:
    print(store.source_digest(store.load_module(name)))
    return 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build one rebuilt module with the period toolchain."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    build_parser = subparsers.add_parser(
        "build", help="build one module with the period toolchain"
    )
    build_parser.add_argument("--module", required=True, help="the module record name")
    build_parser.add_argument(
        "--timeout",
        type=int,
        default=600,
        help="the build timeout in seconds (default: 600)",
    )
    build_parser.add_argument(
        "--toolchain",
        type=Path,
        default=None,
        help="the toolchain tree (default: work/watcom10)",
    )
    digest_parser = subparsers.add_parser(
        "digest", help="print the source digest of one module"
    )
    digest_parser.add_argument("--module", required=True, help="the module record name")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "build":
            return command_build(
                arguments.module, arguments.timeout, arguments.toolchain
            )
        return command_digest(arguments.module)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
