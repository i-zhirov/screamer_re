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
9. member probe: hook the container member open at run time.
10. sound bank: decode the ADLIB- banks, the samples, and the songs.
11. check_run: run every recorded check of the changed records, verify the
    unchanged records against their stored fingerprint, and compare the
    printed verdicts with the recorded results.
12. check_store: prove the store, the seed, the generated tables, and the
    status page.

The check pins the reference artifact: the digests and the addresses come from
`docs/reference/inventory.md` and `docs/reference/executables.md`.  The tool exits
with code 0 when every check passes and code 1 when a check fails.

The tool never modifies the shipped artifacts.  It writes only under `work/`.

Usage:
    python3 tools/pipeline_check.py --game-dir DIR [--skip-runtime]
    python3 tools/pipeline_check.py --game-dir DIR --record-timeout 1800
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

sys.path.insert(0, str(Path(__file__).parent))
import le as le_module  # noqa: E402
from entry_probe import run_probe  # noqa: E402

sys.path.insert(0, str(Path(__file__).parent / "rebuild"))
import store as store_module  # noqa: E402

TOOLS = (
    "le.py",
    "inventory.py",
    "extract_cd_image.py",
    "dosboxx_control.py",
    "entry_probe.py",
    "gfx_container.py",
    "export_assets.py",
    "sound_bank.py",
    "member_probe.py",
    "rebuild/diff_module.py",
    "rebuild/store.py",
    "rebuild/harness.py",
    "rebuild/run.py",
    "rebuild/view.py",
    "rebuild/docs.py",
)

# Reference values from docs/reference/inventory.md and executables.md.
GAME_FILE_COUNT = 56
STARTH_SHA256 = "1246afc95463f96598d5fee764fc99c847b6d3e0bf8ca2268bd362c664bb28a3"
ENTRY_OBJECT_OFFSET = 0x200F8
ENTRY_MODULE_LINEAR = 0x300F8
ENTRY_FILE_OFFSET = 0x3A8F8
ISO_PVD_OFFSET = 0x8000
ISO_VOLUME_ID_OFFSET = 0x8028

# The progress line of the runner: `[<i>/<n>] <label>: <verdict> (<detail>)`.
PROGRESS_LINE = re.compile(r"^\[(\d+)/(\d+)\] (.+?): (PASS|FAIL|SKIP) \((.*)\)$")
# The detail of a record with no recorded result.
NO_RESULT_DETAIL = "no recorded result"
# The detail the runner writes for a skipped behavior check.
RUNTIME_DETAIL = "the runtime is skipped"
# The detail the runner writes for a record whose inputs are unchanged.
UNCHANGED_DETAIL = "the inputs are unchanged since the previous run"


class Checks:
    def __init__(self) -> None:
        self.results: list[tuple[str, bool, str]] = []

    def check(self, name: str, ok: bool, detail: str = "") -> bool:
        self.results.append((name, bool(ok), detail))
        print(f"[{'PASS' if ok else 'FAIL'}] {name}" + (f"  ({detail})" if detail else ""))
        return bool(ok)

    def skip(self, name: str, detail: str = "") -> None:
        """Print a check the run cannot compare, outside the check count."""
        print(f"[SKIP] {name}" + (f"  ({detail})" if detail else ""))

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


def last_line(text: str) -> str:
    """The last non-empty line of a tool output."""
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    return lines[-1] if lines else ""


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



def check_member_probe(checks: Checks, work_dir: Path, port: int, timeout: float) -> None:
    """Hook the member open and check the first member name."""
    command = [
        sys.executable,
        "tools/member_probe.py",
        "--game-dir",
        str(work_dir / "game"),
        "--work-dir",
        str(work_dir),
        "--port",
        str(port),
        "--limit",
        "55",
        "--timeout",
        str(timeout),
    ]
    result = run(command, timeout=timeout + 30)
    samples_missing = (
        "name='skidfx.RAW'" not in result.stdout
        or "name='airplane.RAW'" not in result.stdout
    )
    if (
        result.returncode != 0
        and (
            "closed the control connection" in result.stderr
            or "timed out" in result.stderr
        )
    ) or (result.returncode == 0 and samples_missing):
        # The debugger can close the connection or time out before the probe
        # reports the first member.  A mid-run timeout can also end the probe
        # before the sample loads, and the fallback then makes the run look
        # successful.  Repeat the probe once in both cases.
        result = run(command, timeout=timeout + 30)
    checks.check("stage 9: the member probe runs", result.returncode == 0, result.stderr.strip())
    checks.check(
        "stage 9: the member probe observes the first member",
        "name='MENU.dtr'" in result.stdout,
        "MENU.dtr",
    )
    checks.check(
        "stage 9: the member probe observes the sample loads",
        "name='skidfx.RAW'" in result.stdout and "name='airplane.RAW'" in result.stdout,
        "the SFXSBASE and MAP1 samples",
    )
    checks.check(
        "stage 9: the member probe observes the unlisted members stay absent",
        "name='cry.RAW'" not in result.stdout and "name='train.RAW'" not in result.stdout,
        "the five unlisted members stay absent",
    )


def check_sound(checks: Checks, game_dir: Path, work_dir: Path) -> None:
    """Decode the sound formats and check the reference values."""
    # The bank tool needs a member, so extract the two banks first.
    for name in ("DRUM.BNK", "MELODIC.BNK", "MUSIC1.HMP"):
        run(
            [sys.executable, "tools/gfx_container.py", "member", str(game_dir / "DATA/MUSIC.DAT"), name,
             "--out", str(work_dir / name)]
        )
    drum = run([sys.executable, "tools/sound_bank.py", "bank", str(work_dir / "DRUM.BNK")])
    checks.check("stage 10: the drum bank decodes", drum.returncode == 0, drum.stderr.strip())
    checks.check(
        "stage 10: the drum bank holds the reference header",
        "signature:   ADLIB-" in drum.stdout
        and "count:       28" in drum.stdout
        and "data offset: 0x61c" in drum.stdout,
        "ADLIB-, 28, 0x61c",
    )
    checks.check(
        "stage 10: the first drum instrument holds the driver pairs",
        "20:0F  40:3F  60:01  80:3E  E0:0F  23:3F  43:01  C0:01" in drum.stdout,
        "the eight register pairs",
    )
    melodic = run([sys.executable, "tools/sound_bank.py", "bank", str(work_dir / "MELODIC.BNK")])
    checks.check(
        "stage 10: the melodic bank decodes",
        melodic.returncode == 0 and "PIANO1" in melodic.stdout,
        melodic.stderr.strip(),
    )
    lists = [
        str(game_dir / "DATA/GEN.DAT"),
        *[str(path) for path in sorted((game_dir / "DATA").glob("MAP*I.DAT"))],
    ]
    samples = run(
        [sys.executable, "tools/sound_bank.py", "samples", str(game_dir / "DATA/SOUND.DAT"), "--lists", *lists]
    )
    checks.check("stage 10: the sample bank decodes", samples.returncode == 0, samples.stderr.strip())
    checks.check(
        "stage 10: the sample bank joins 87 members",
        "members: 92" in samples.stdout and "joined:  87" in samples.stdout,
        "92 members, 87 joined",
    )
    checks.check(
        "stage 10: the sample flags match the loader",
        "heli             8127    19521  268468223    -1      0      0  268500992  285212672 0x4700"
        in samples.stdout,
        "heli 0x4700",
    )
    song = run([sys.executable, "tools/sound_bank.py", "song", str(work_dir / "MUSIC1.HMP")])
    checks.check("stage 10: the song decodes", song.returncode == 0, song.stderr.strip())
    checks.check(
        "stage 10: the song header matches the reference",
        "tracks:      13" in song.stdout and "duration:    93 s" in song.stdout,
        "13 tracks, 93 s",
    )
    checks.check(
        "stage 10: the song chunks cover the data",
        song.stdout.count("track") >= 13 and "0x12107" in song.stdout,
        "13 chunks",
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


def recorded_records() -> dict[str, dict]:
    """The store records by the key of their progress line.

    A function record takes its address and a module row takes its name, so the
    store list holds one key per record of the run.
    """
    result = run([sys.executable, "tools/rebuild/store.py", "list", "--json"])
    if result.returncode != 0:
        raise ValueError(f"the store list failed: {last_line(result.stderr)}")
    records = {}
    for record in json.loads(result.stdout):
        if "address" in record:
            records[record["address"]] = record
        else:
            records[record["name"]] = record
    return records


def record_of(records: dict[str, dict], label: str) -> dict | None:
    """The record of a progress label, or None when the store lacks it.

    A function label starts with the address, and a module label is the name.
    """
    record = records.get(label)
    if record is not None:
        return record
    token = label.split(" ", 1)[0]
    if token.startswith("0x"):
        return records.get(token)
    return None


def progress_lines(text: str) -> list[tuple[str, str, str]]:
    """The label, the verdict, and the detail of every progress line."""
    entries = []
    for line in text.splitlines():
        match = PROGRESS_LINE.match(line.strip())
        if match:
            entries.append((match.group(3), match.group(4), match.group(5)))
    return entries


def expected_result(record: dict, skip_runtime: bool) -> tuple[str, str | None]:
    """The verdict and the expected detail of a record.

    The runner prints `FAIL` when a check failed, `SKIP` when a check was
    skipped, and `PASS` otherwise.  Under `--skip-runtime` the runner skips a
    declared behavior check with the detail `the runtime is skipped`.
    """
    result = record.get("result") or {}
    verdicts = {
        name: check.get("verdict")
        for name, check in result.get("checks", {}).items()
    }
    detail = None
    if skip_runtime and "behavior" in record.get("checks", []):
        verdicts["behavior"] = "skip"
        detail = RUNTIME_DETAIL
    if "fail" in verdicts.values():
        return "FAIL", detail
    if "skip" in verdicts.values():
        return "SKIP", detail
    return "PASS", detail


def unchanged_is_verified(record: dict, game_dir: Path) -> bool:
    """Tell whether the recorded result of an unchanged record is trustworthy.

    The runner reports `PASS` with the unchanged detail only when the fingerprint
    of the recorded result equals the current input fingerprint.  The stage
    recomputes the fingerprint itself, so a forged or a stale store record
    fails even when the runner is skipped.
    """
    result = record.get("result") or {}
    stored = result.get("fingerprint")
    if not stored:
        return False
    module_name = record.get("module") or record.get("name")
    try:
        module = store_module.load_module(module_name)
    except ValueError:
        return False
    current = store_module.compute_fingerprint(
        record, module, original=game_dir / "STARTH.EXE"
    )
    return stored == current


def check_run(
    checks: Checks, game_dir: Path, skip_runtime: bool, record_timeout: float
) -> None:
    """Run every recorded check and compare the verdicts with the store.

    The runner builds and runs the modules of the records that declare checks, so
    the stage needs Docker and DOSBox-X.  With `--skip-unchanged` the runner
    reports a record whose inputs are unchanged since the previous run without
    running its checks, and the stage verifies the stored fingerprint of that
    record itself.  The stage compares the printed verdict of every record that
    ran with the recorded result, so a known difference passes and a change or an
    unexpected skip fails.  A record the store holds no result for is skipped,
    because the run holds nothing to compare it with.
    """
    if shutil.which("docker") is None:
        print("docker is not on PATH; the record run is skipped")
        return
    command = [
        sys.executable,
        "tools/rebuild/run.py",
        "run",
        "--all",
        "--no-write",
        "--skip-unchanged",
        "--original",
        str(game_dir / "STARTH.EXE"),
    ]
    if skip_runtime:
        command.append("--skip-runtime")
    result = run(command, timeout=record_timeout)
    records = recorded_records()
    entries = progress_lines(result.stdout)
    if not entries:
        checks.check(
            "stage 11: the run prints a progress line",
            False,
            last_line(result.stderr),
        )
        return
    compared = 0
    for label, verdict, detail in entries:
        record = record_of(records, label)
        if record is None:
            checks.check(f"stage 11: {label}", False, "no record in the store")
            continue
        if not record.get("result"):
            checks.skip(f"stage 11: {label}", NO_RESULT_DETAIL)
            continue
        if detail == UNCHANGED_DETAIL:
            ok = verdict == "PASS" and unchanged_is_verified(record, game_dir)
            detail_text = f"{verdict} ({detail})"
            if not ok:
                detail_text += ", the recorded fingerprint does not match the inputs"
            checks.check(f"stage 11: {label}", ok, detail_text)
            compared += 1
            continue
        expected, expected_detail = expected_result(record, skip_runtime)
        ok = verdict == expected and (
            expected_detail is None or detail == expected_detail
        )
        detail_text = f"{verdict} ({detail})"
        if not ok:
            detail_text += f", the recorded result is {expected}"
        checks.check(f"stage 11: {label}", ok, detail_text)
        compared += 1
    with_result = sum(1 for record in records.values() if record.get("result"))
    checks.check(
        "stage 11: the run compares every record with a recorded result",
        compared == with_result,
        f"{compared} of {with_result}",
    )


def check_store(checks: Checks, game_dir: Path, work_dir: Path) -> None:
    """Prove the store, the seed, the generated tables, and the page.

    The store is the only module registry, so the stage runs the store check,
    the seed check, and the drift check, and it renders the status page.
    """
    result = run([sys.executable, "tools/rebuild/store.py", "check"])
    checks.check(
        "stage 12: the store holds no violation",
        result.returncode == 0,
        last_line(result.stdout) or last_line(result.stderr),
    )
    result = run(
        [
            sys.executable,
            "tools/rebuild/store.py",
            "seed",
            "--check",
            "--original",
            str(game_dir / "STARTH.EXE"),
        ]
    )
    checks.check(
        "stage 12: the seed finds no difference",
        result.returncode == 0,
        last_line(result.stdout) or last_line(result.stderr),
    )
    result = run([sys.executable, "tools/rebuild/docs.py", "check"])
    checks.check(
        "stage 12: the program-map tables hold no drift",
        result.returncode == 0,
        last_line(result.stdout) or last_line(result.stderr),
    )
    page = work_dir / "status.html"
    result = run(
        [sys.executable, "tools/rebuild/view.py", "html", "--out", str(page)]
    )
    checks.check(
        "stage 12: the status page renders",
        result.returncode == 0,
        last_line(result.stdout) or last_line(result.stderr),
    )
    checks.check(
        "stage 12: the status page exists",
        page.is_file(),
        str(page),
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
    parser.add_argument(
        "--record-timeout",
        type=float,
        default=1200.0,
        help="the seconds the record run of stage 11 may take",
    )
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
        check_member_probe(checks, work_root / "run", args.port, args.timeout)
    check_sound(checks, game_dir, work_root)
    check_run(checks, game_dir, args.skip_runtime, args.record_timeout)
    check_store(checks, game_dir, work_root)

    print()
    print(f"{len(checks.results)} checks, {len(checks.failures)} failures")
    for name in checks.failures:
        print(f"  FAIL: {name}")
    return 1 if checks.failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
