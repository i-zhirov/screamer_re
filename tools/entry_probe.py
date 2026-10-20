#!/usr/bin/env python3
"""Resolve the LE entry alignment with a DOSBox-X instruction log.

The tool runs the game in DOSBox-X, breaks into the debugger, sets an interrupt
breakpoint on the game stub's DOS4GW exec call (INT 21h AH=4B), and starts a CPU
instruction log.  The log covers the DOS4GW load and the first game instruction.
The tool then finds the first game instruction in the log and reports its address.

The tool never modifies the shipped artifacts.  It writes only under `work/`.

Usage:
    python3 tools/entry_probe.py --game-dir DIR [--count 0x40000]
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from dosboxx_control import ControlSession, PtyProcess  # noqa: E402

LOG_LINE_RE = re.compile(
    r"^\s*(?:[\d.]+\s+)?([0-9A-F]{4}):([0-9A-F]{1,8})\s+(.*?)\s+EAX:([0-9A-F]{8})"
)


def read_cs_eip(session: ControlSession) -> tuple[str, str]:
    """Read the current CS and EIP through the debugger EV command."""
    response = session.request("EXEC", "EV CS EIP", timeout=20)
    values = response.lines[-1].split()
    if len(values) < 2:
        raise ValueError(f"unexpected EV response: {response.lines}")
    return values[0], values[1]


def find_entry(log: str, candidates: list[tuple[str, str]], window: int = 6) -> dict:
    """Find the first entry sequence in a LOGS instruction log.

    The entry is a short jump over the Watcom run-time banner string.  The code
    after the jump starts with `sti`, `and esp,-4`, `mov ebx,esp`.
    """
    lines = log.splitlines()
    parsed: list[tuple[str, str, str]] = []
    for line in lines:
        match = LOG_LINE_RE.search(line)
        if not match:
            continue
        parsed.append((match.group(1), match.group(2), match.group(3)))
    for index, (selector, eip, body) in enumerate(parsed):
        if "jmp  short" not in body:
            continue
        following: list[str] = []
        for step in range(index + 1, len(parsed)):
            next_selector, next_eip, next_body = parsed[step]
            if (next_selector, next_eip) == (selector, eip):
                # The debugger logs the current instruction twice.
                continue
            following.append(next_body[:60])
            if len(following) == window:
                break
        if len(following) >= 3 and "sti" in following[0] and any(
            "mov  ebx,esp" in line for line in following[1:4]
        ):
            return {
                "found": True,
                "candidate": "entry jump over the Watcom banner",
                "selector": selector,
                "eip": eip,
                "instruction": body[:70],
                "following": following,
            }
    for index, (selector, eip, body) in enumerate(parsed):
        for name, signature in candidates:
            if signature in body:
                following = [
                    parsed[index + step][2][:60]
                    for step in range(1, window)
                    if index + step < len(parsed)
                ]
                return {
                    "found": True,
                    "candidate": name,
                    "selector": selector,
                    "eip": eip,
                    "instruction": body[:70],
                    "following": following,
                }
    return {"found": False, "lines": len(parsed)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game-dir", type=Path, required=True)
    parser.add_argument("--count", type=lambda value: int(value, 0), default=0x40000)
    parser.add_argument("--port", type=int, default=9876)
    parser.add_argument("--timeout", type=float, default=180.0)
    parser.add_argument("--work-root", type=Path, default=Path("work/entry"))
    parser.add_argument("--dosboxx", default="dosbox-x")
    args = parser.parse_args()

    work_root = args.work_root.resolve()
    work_root.mkdir(parents=True, exist_ok=True)
    conf = work_root / "probe.conf"
    build_conf(args.game_dir.resolve() / "dosboxScreamer.conf", conf, args.port)
    # The conf mounts the relative path "game", so the working directory needs a
    # game copy below it.  A clone keeps the copy fast and space efficient.
    run_dir = work_root / "run"
    if (run_dir / "game").exists():
        shutil.rmtree(run_dir / "game")
    run_dir.mkdir(parents=True, exist_ok=True)
    subprocess.run(["cp", "-cR", str(args.game_dir.resolve()), str(run_dir / "game")], check=True)

    env = dict(os.environ)
    env["SDL_VIDEODRIVER"] = "dummy"
    env["SDL_AUDIODRIVER"] = "dummy"
    env["TERM"] = "xterm-256color"
    child = PtyProcess(
        [
            args.dosboxx,
            "-nogui",
            "-break-start",
            "-conf",
            str(conf),
            "-conf",
            "game/dosboxScreamer_single.conf",
        ],
        run_dir,
        env,
    )
    log_file = run_dir / "LOGCPU.TXT"
    if log_file.exists():
        log_file.unlink()

    result: dict[str, object] = {"log": str(log_file)}
    try:
        with ControlSession(args.port, timeout=args.timeout) as session:
            time.sleep(2.0)
            for command in ('BPINT 21 "4B"', "BPLIST"):
                response = session.request("EXEC", command, timeout=30)
                print(f"$ {command} -> ok={response.ok}")
                for line in response.lines[:5]:
                    print(f"   {line}")
            session.request("EXEC", "RUN", timeout=30)
            at_stub_exec = False
            for _ in range(20):
                if not child.wait_for("I->", args.timeout):
                    result["note"] = "the debugger did not stop"
                    break
                selector, offset = read_cs_eip(session)
                print(f"break at {selector}:{offset}")
                if selector.lower() == "f000":
                    # The BIOS installs its interrupt stubs through INT 21h.
                    child.clear()
                    session.request("EXEC", "RUN", timeout=30)
                    continue
                at_stub_exec = True
                result["break_address"] = f"{selector}:{offset}"
                break
            if not at_stub_exec:
                result.setdefault("note", "no break at the game stub exec")
            else:
                for command in ("EV CS EIP", "CPU"):
                    response = session.request("EXEC", command, timeout=30)
                    for line in response.lines[:8]:
                        print(f"   {line}")
                # Remove the breakpoint first, or it triggers again on the same
                # instruction and stops the log.
                session.request("EXEC", "BPDEL *", timeout=30)
                session.request("EXEC", f"LOGS {args.count:x}", timeout=30)
                if not child.wait_for("LOGCPU.TXT created", args.timeout):
                    result["note"] = "the log did not finish; the log holds the instructions so far"
                time.sleep(1.0)
    finally:
        child.close()

    if not log_file.exists():
        result["note"] = result.get("note", "no LOGCPU.TXT")
        print(result)
        return 1

    candidates = [
        # Text signatures of the entry sequence, used as a fallback.
        ("entry jump over the Watcom banner", "jmp  short"),
        ("entry code after the jump", "mov  ebx,esp"),
    ]
    found = find_entry(log_file.read_text(errors="replace"), candidates)
    result.update(found)
    for key, value in result.items():
        if key == "following":
            print(f"{key}:")
            for line in value:
                print(f"   {line}")
        else:
            print(f"{key}: {value}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
