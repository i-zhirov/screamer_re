#!/usr/bin/env python3
"""Run the tooling pipeline end to end and check the stages agree.

Stages:

1. syntax gate: compile every tool.
2. inventory: count the game files and check one known digest.
3. CD image: extract the data track and check the ISO volume label.
4. LE parser: report the header entry and its file offset.
5. entry probe: run the game in DOSBox-X and report the run-time entry.
6. debugger session: run a session script that breaks in and reads CS:EIP.
7. cross-check: compare the file view with the run-time view.
8. radare2 cross-check: compare the parser with radare2 when it is on PATH.
9. rebuild: rebuild the smallest named module and diff it against the
   original.

The check pins the reference artifact: the digests and the addresses come from
`docs/reference/inventory.md` and `docs/reference/executables.md`.  The tool exits
with code 0 when every check passes and code 1 when a check fails.

The tool never modifies the shipped artifacts.  It writes only under `work/`.

Usage:
    python3 tools/pipeline_check.py --game-dir DIR [--skip-runtime]
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import le as le_module  # noqa: E402
from entry_probe import run_probe  # noqa: E402

TOOLS = (
    "le.py",
    "inventory.py",
    "extract_cd_image.py",
    "dosboxx_control.py",
    "entry_probe.py",
    "gfx_container.py",
    "export_assets.py",
    "rebuild/diff_module.py",
)

# Reference values from docs/reference/inventory.md and executables.md.
GAME_FILE_COUNT = 56
STARTH_SHA256 = "1246afc95463f96598d5fee764fc99c847b6d3e0bf8ca2268bd362c664bb28a3"
ENTRY_OBJECT_OFFSET = 0x200F8
ENTRY_MODULE_LINEAR = 0x300F8
ENTRY_FILE_OFFSET = 0x3A8F8
ISO_PVD_OFFSET = 0x8000
ISO_VOLUME_ID_OFFSET = 0x8028

# The rebuilt modules: the name, the test program, the linker map, and the diffs.  A
# diff holds the original linear address, the rebuilt symbol, and the instruction count
# (0 takes the count from the original).
REBUILT_MODULES = (
    (
        "free",
        "free.exe",
        "free.map",
        ((0x300EC, "free_", 0),),
    ),
    (
        "exit",
        "exitmod.exe",
        "exit.map",
        (
            (0x300BE, "exit_", 0),
            (0x300D6, "exit_flush_", 6),
        ),
    ),
)
REBUILD_DIR = Path(__file__).parent.parent / "work/rebuild"


class Checks:
    def __init__(self) -> None:
        self.results: list[tuple[str, bool, str]] = []

    def check(self, name: str, ok: bool, detail: str = "") -> bool:
        self.results.append((name, bool(ok), detail))
        print(f"[{'PASS' if ok else 'FAIL'}] {name}" + (f"  ({detail})" if detail else ""))
        return bool(ok)

    @property
    def failures(self) -> list[str]:
        return [name for name, ok, _ in self.results if not ok]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str], timeout: float = 300.0) -> subprocess.CompletedProcess:
    return subprocess.run(command, capture_output=True, text=True, timeout=timeout)


def check_syntax(checks: Checks) -> None:
    result = run([sys.executable, "-m", "py_compile", *[f"tools/{name}" for name in TOOLS]])
    checks.check("stage 1: every tool compiles", result.returncode == 0, result.stderr.strip())


def check_inventory(checks: Checks, game_dir: Path) -> None:
    result = run([sys.executable, "tools/inventory.py", str(game_dir)])
    ok = result.returncode == 0
    count = re.search(r"^Files: (\d+)$", result.stdout, re.MULTILINE)
    checks.check(
        "stage 2: the inventory runs",
        ok and count is not None,
        f"files={count.group(1) if count else '?'}",
    )
    if count:
        checks.check(
            "stage 2: the game file count matches the reference",
            int(count.group(1)) == GAME_FILE_COUNT,
            f"{count.group(1)}",
        )
    digest = sha256(game_dir / "STARTH.EXE")
    checks.check(
        "stage 2: the STARTH.EXE digest matches the reference",
        digest == STARTH_SHA256,
        digest[:16],
    )
    verify_reference_digests(checks, game_dir)


def reference_digests() -> dict[str, str]:
    """Read the path and digest pairs of the reference inventory."""
    table = (Path(__file__).parent.parent / "docs/reference/inventory.md").read_text()
    digests: dict[str, str] = {}
    for line in table.splitlines():
        cells = [cell.strip().strip("`") for cell in line.split("|")]
        if len(cells) < 5:
            continue
        path, _size, digest = cells[1], cells[2], cells[3]
        if not path.upper().endswith(
            (".EXE", ".386", ".DAT", ".CFG", ".ICO", ".TXT", ".PDF", ".RPL", ".TAB", ".GOG", ".CONF")
        ):
            continue
        if len(digest) == 64 and all(c in "0123456789abcdef" for c in digest):
            digests[path] = digest
    return digests


def is_expected_extra(relative: str) -> bool:
    """Tell the game state files and the wrapper files from a regression.

    The game writes the choice file and the replays during play, and the distribution
    wrapper ships its own DOSBox configuration.  Both are expected, and neither is a
    regression.  Any other file outside the reference inventory is a regression.
    """
    name = relative.replace(os.sep, "/").rsplit("/", 1)[-1].upper()
    if name.endswith(".RPL"):
        return True
    if name == "CHOICE.DAT":
        return True
    return name.startswith("DOSBOX") and name.endswith(".CONF")


def verify_reference_digests(checks: Checks, game_dir: Path) -> None:
    """Verify every shipped file, and name the files the game writes itself.

    A shipped file that changes is a regression.  A file that the game writes and the
    distribution does not ship, such as the choice file and the replays, is game state
    and not a regression, so it is reported on its own.
    """
    digests = reference_digests()
    mismatched = []
    for path, digest in sorted(digests.items()):
        target = game_dir / path.replace("/", os.sep)
        if not target.is_file():
            mismatched.append(f"{path} (missing)")
            continue
        if sha256(target) != digest:
            mismatched.append(path)
    checks.check(
        "stage 2: every shipped file digest matches the reference",
        not mismatched,
        ", ".join(mismatched[:3]) if mismatched else f"{len(digests)} files",
    )
    shipped = {path.replace("/", os.sep) for path in digests}
    written = []
    for target in sorted(game_dir.rglob("*")):
        if not target.is_file():
            continue
        relative = str(target.relative_to(game_dir))
        if relative not in shipped:
            written.append(relative)
    checks.check(
        "stage 2: only the game writes its own state files",
        all(is_expected_extra(name) for name in written),
        f"outside the reference: {', '.join(written[:3])}" if written else "none",
    )



def check_cd_image(checks: Checks, game_dir: Path, work_dir: Path) -> None:
    iso = work_dir / "screamer_data_track.iso"
    result = run(
        [sys.executable, "tools/extract_cd_image.py", str(game_dir / "SCREAM.DAT"), str(iso)]
    )
    checks.check("stage 3: the CD extractor runs", result.returncode == 0, result.stdout.strip())
    if not iso.exists():
        checks.check("stage 3: the ISO volume label", False, "no ISO")
        return
    data = iso.read_bytes()
    volume_id = data[ISO_VOLUME_ID_OFFSET : ISO_VOLUME_ID_OFFSET + 32].decode("ascii", "replace")
    checks.check(
        "stage 3: the ISO volume label",
        data[ISO_PVD_OFFSET : ISO_PVD_OFFSET + 6] == b"\x01CD001" and volume_id.startswith("SCREAMER"),
        volume_id.strip(),
    )


def check_le_parser(checks: Checks, game_dir: Path, exe_name: str) -> dict:
    exe = le_module.load(game_dir / exe_name)
    checks.check(
        "stage 4: the entry object offset matches the reference",
        exe.entry_offset == ENTRY_OBJECT_OFFSET,
        f"{exe.entry_offset:#x}",
    )
    checks.check(
        "stage 4: the entry module address matches the reference",
        exe.entry_linear == ENTRY_MODULE_LINEAR,
        f"{exe.entry_linear:#x}",
    )
    entry_file = exe.to_file_offset(exe.entry_linear)
    checks.check(
        "stage 4: the entry maps to the reference file offset",
        entry_file == ENTRY_FILE_OFFSET,
        f"{entry_file:#x}" if entry_file is not None else "unmapped",
    )
    entry_bytes = exe.read(exe.entry_linear, 2) or b""
    checks.check(
        "stage 4: the entry starts with a short jump",
        entry_bytes[:1] == b"\xeb",
        entry_bytes.hex(),
    )
    jump_offset = int.from_bytes(entry_bytes[1:2], "little", signed=True)
    target = exe.entry_linear + 2 + jump_offset
    target_bytes = exe.read(target, 0x30) or b""
    checks.check(
        "stage 4: the jump skips the banner string",
        b"\xfb" in target_bytes[:4],
        target_bytes[:8].hex(),
    )
    checks.check(
        "stage 4: the startup sets the frame pointer",
        b"\x89\xe3" in target_bytes[:0x10],
        target_bytes[:0x10].hex(),
    )
    checks.check(
        "stage 4: the startup calls the DOS version service",
        b"\xb4\x30\xcd\x21" in target_bytes,
        "mov ah,30h / int 21h",
    )
    return {
        "entry_linear": exe.entry_linear,
        "entry_file": entry_file,
        "jump_offset": jump_offset,
        "target": target,
        "target_bytes": target_bytes,
    }


def check_runtime(
    checks: Checks, game_dir: Path, work_root: Path, port: int, timeout: float
) -> dict:
    result = run_probe(
        game_dir,
        count=0x100000,
        port=port,
        timeout=timeout,
        work_root=work_root,
        dosboxx="dosbox-x",
        verbose=False,
    )
    checks.check("stage 5: the entry probe finds the entry", result.get("found", False), str(result.get("note", "")))
    if not result.get("found"):
        return result
    runtime_eip = int(result["eip"], 16)
    following = result.get("following", [])
    checks.check(
        "stage 5: the run-time entry is a short jump",
        "jmp  short" in result.get("instruction", ""),
        result.get("instruction", ""),
    )
    checks.check(
        "stage 5: the startup runs after the jump",
        bool(following) and "sti" in following[0],
        following[0] if following else "no lines",
    )
    checks.check(
        "stage 5: the startup sets the frame pointer",
        any("mov  ebx,esp" in line for line in following[1:3]),
        "mov ebx,esp",
    )
    result["runtime_eip"] = runtime_eip
    return result


def check_cross(
    checks: Checks, parser: dict, runtime: dict, log_path: Path
) -> None:
    if not runtime.get("found"):
        checks.check("stage 7: the file view matches the run-time view", False, "no run-time entry")
        return
    base = runtime["runtime_eip"] - parser["entry_linear"]
    checks.check(
        "stage 7: the module load base is page aligned",
        base > 0 and base % 0x1000 == 0,
        f"{base:#x}",
    )
    log = Path(runtime["log"])
    if not log.exists():
        checks.check("stage 7: the log holds the run-time entry", False, "no log")
        return
    lines = log.read_text(errors="replace").splitlines()
    entry_index = None
    for index, line in enumerate(lines):
        if f":{runtime['eip']} " in line and "jmp  short" in line:
            entry_index = index
            break
    if entry_index is None:
        checks.check("stage 7: the log holds the run-time entry", False, "no entry line")
        return
    checks.check("stage 7: the log holds the run-time entry", True, f"{runtime['eip']}")
    target = runtime["runtime_eip"] + 2 + parser["jump_offset"]
    target_line = None
    for line in lines[entry_index : entry_index + 8]:
        if f":{target:X} " in line:
            target_line = line
            break
    checks.check(
        "stage 7: the run-time jump target matches the file target",
        target_line is not None and "sti" in target_line,
        f"{target:#x}",
    )
    for line in lines[entry_index : entry_index + 20]:
        if "int  21" in line:
            checks.check(
                "stage 7: the run-time startup calls the DOS version service",
                "EAX:00003000" in line,
                "AH=30h",
            )
            break
    else:
        checks.check("stage 7: the run-time startup calls the DOS version service", False, "no int 21h")
    stub = runtime.get("break_address", "")
    checks.check(
        "stage 7: the stub exec break is outside the BIOS",
        bool(stub) and not stub.lower().startswith("f000"),
        stub,
    )


def check_radare2(checks: Checks, game_dir: Path, exe_name: str, timeout: float) -> None:
    """Cross-check the parser against radare2 when it is on PATH."""
    if shutil.which("r2") is None:
        print("radare2 is not on PATH; the static cross-check is skipped")
        return
    target = str(game_dir / exe_name)
    info = run(["r2", "-A", "-q", "-c", "ie", "-c", "iS", "-c", "q", target], timeout=timeout)
    output = re.sub(r"\x1b\[[0-9;]*m", "", info.stdout)
    checks.check(
        "stage 8: radare2 reports the entry point",
        "0x000300f8" in output,
        "0x000300f8",
    )
    checks.check(
        "stage 8: radare2 maps page 0 to the reference addresses",
        "0x0001a800" in output and "0x00010000" in output,
        "paddr 0x1a800, vaddr 0x10000",
    )
    listing = run(
        ["r2", "-A", "-q", "-c", "s 0x10020; pdf", "-c", "q", target], timeout=timeout
    )
    text = re.sub(r"\x1b\[[0-9;]*m", "", listing.stdout)
    checks.check(
        "stage 8: radare2 finds the main function",
        "fcn.00010020" in text,
        "fcn.00010020",
    )
    checks.check(
        "stage 8: radare2 reports the main caller",
        "fcn.000d4324" in text and "0xd4364" in text,
        "call from 0xd4364",
    )


def check_rebuild(checks: Checks, game_dir: Path, timeout: float) -> None:
    """Rebuild the named modules and diff them against the original.

    The modules are `free` at linear 0x300ec, the smallest named function in the
    program map, and `exit` at linear 0x300be with its helper at 0x300d6.  The stage
    needs Docker for the OpenWatcom V2 container, so it skips when Docker is missing.
    """
    if shutil.which("docker") is None:
        print("docker is not on PATH; the module rebuild is skipped")
        return
    for name, executable, map_name, diffs in REBUILT_MODULES:
        result = run(["sh", f"tools/rebuild/build_{name}.sh"], timeout=timeout)
        checks.check(
            f"stage 9: the {name} module rebuild links",
            result.returncode == 0 and executable in result.stdout,
            result.stderr.strip()[-80:],
        )
        if result.returncode != 0:
            continue
        for original_address, symbol, count in diffs:
            command = [
                sys.executable,
                "tools/rebuild/diff_module.py",
                "--original",
                str(game_dir / "STARTH.EXE"),
                "--original-address",
                f"{original_address:#x}",
                "--rebuilt",
                str(REBUILD_DIR / executable),
                "--rebuilt-map",
                str(REBUILD_DIR / map_name),
                "--rebuilt-symbol",
                symbol,
            ]
            if count:
                command += ["--count", str(count)]
            result = run(command, timeout=timeout)
            detail = ""
            lines = result.stdout.strip().splitlines()
            if lines:
                detail = lines[-1]
            checks.check(
                f"stage 9: the {name} module matches the original at "
                f"{original_address:#x}",
                result.returncode == 0 and "instruction sequences match" in result.stdout,
                detail or result.stderr.strip()[-80:],
            )


def check_session_script(checks: Checks, work_root: Path, port: int, timeout: float) -> None:
    script = Path("tools/scripts/example_session.dbg")
    launch = (
        "dosbox-x -nogui -break-start -conf ../probe.conf "
        "-conf game/dosboxScreamer_single.conf"
    )
    result = run(
        [
            sys.executable,
            "tools/dosboxx_control.py",
            "script",
            "--file",
            str(script),
            "--port",
            str(port),
            "--timeout",
            str(timeout),
            "--cwd",
            str(work_root / "run"),
            "--launch",
            launch,
        ]
    )
    checks.check(
        "stage 6: the session script runs",
        result.returncode == 0,
        f"exit={result.returncode}",
    )
    checks.check(
        "stage 6: the session script reads the reset vector",
        "read CS=f000 EIP=fff0" in result.stdout,
        "CS=f000 EIP=fff0",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game-dir", type=Path, required=True)
    parser.add_argument("--exe", default="STARTH.EXE")
    parser.add_argument("--port", type=int, default=9876)
    parser.add_argument("--timeout", type=float, default=300.0)
    parser.add_argument("--work-root", type=Path, default=Path("work/entry"))
    parser.add_argument("--skip-runtime", action="store_true")
    args = parser.parse_args()

    game_dir = args.game_dir.resolve()
    work_root = args.work_root.resolve()
    work_root.mkdir(parents=True, exist_ok=True)

    checks = Checks()
    check_syntax(checks)
    check_inventory(checks, game_dir)
    check_cd_image(checks, game_dir, work_root)
    parser_result = check_le_parser(checks, game_dir, args.exe)
    if args.skip_runtime:
        print("run-time stages skipped")
    else:
        runtime = check_runtime(checks, game_dir, work_root, args.port, args.timeout)
        check_session_script(checks, work_root, args.port, args.timeout)
        check_cross(checks, parser_result, runtime, Path(runtime.get("log", "")))
        check_radare2(checks, game_dir, args.exe, args.timeout)
    check_rebuild(checks, game_dir, args.timeout)

    print()
    print(f"{len(checks.results)} checks, {len(checks.failures)} failures")
    for name in checks.failures:
        print(f"  FAIL: {name}")
    return 1 if checks.failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
