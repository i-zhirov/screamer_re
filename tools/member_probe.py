#!/usr/bin/env python3
"""Hook the container member open at run time and report the member names.

The protected-mode and linear breakpoints of the DOSBox-X debugger watch a memory
change, so they never fire at the member open.  The `BP` breakpoint is a real-mode
breakpoint, so it never fires at the protected-mode code either.  This tool uses a
software breakpoint instead: it patches a copy of the game executable with the INT 3
opcode 0xCC at the file offset of the member open `FUN_0004C08D`, sets the `BPINT 3`
breakpoint, runs the game, and reads the member name from the stack at every stop.

The member open sits at the module address 0x4C08D, and the sample loader at
0x1F2E0 calls it at the module address 0x1F466.  The tool patches both offsets
in the game copy, so the breakpoint fires at the member open and at its call site.
At each stop the tool reads the five bytes at the stop address through a memory dump
and recognizes the stop by the patched bytes:

- the member open bytes are `CC 57 55 89 E5`,
- the call site bytes are `CC 22 CC 02 00`.

The tool then reads the member name from the stack at the `EBX` pointer and
continues.

Limit: the debugger intercepts the INT 3 opcode before the guest exception handler
runs, so the guest resumes with the patched instruction skipped.  The member open
survives because its Watcom frame restores the stack pointer, but the game leaves the
debugger after the first member open while the breakpoint stays armed.  The tool
therefore observes one member open per run.

Usage:
    python3 tools/member_probe.py [--game-dir DIR] [--limit N]
"""

from __future__ import annotations

import argparse
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from dosboxx_control import ControlSession, PtyProcess  # noqa: E402

MEMBER_OPEN_FILE = 0x5688D
MEMBER_OPEN_PATCHED = bytes((0xCC, 0x57, 0x55, 0x89, 0xE5))
CALL_SITE_FILE = 0x29C66
CALL_SITE_PATCHED = bytes((0xCC, 0x22, 0xCC, 0x02, 0x00))
STACK_DUMP_BASE = 0x2F4000
STACK_DUMP_SIZE = 0x2000


def run_probe(
    game_dir: Path,
    work_dir: Path,
    port: int,
    limit: int,
    timeout: float,
    dosboxx: str,
) -> list[str]:
    """Patch the game copy, run the game, and return the observed member names."""
    exe = game_dir / "STARTH.EXE"
    data = bytearray(exe.read_bytes())
    originals = {
        MEMBER_OPEN_FILE: bytes(data[MEMBER_OPEN_FILE : MEMBER_OPEN_FILE + 5]),
        CALL_SITE_FILE: bytes(data[CALL_SITE_FILE : CALL_SITE_FILE + 5]),
    }
    print(f"member open bytes: {originals[MEMBER_OPEN_FILE].hex(' ')}")
    print(f"call site bytes:   {originals[CALL_SITE_FILE].hex(' ')}")
    data[MEMBER_OPEN_FILE] = 0xCC
    data[CALL_SITE_FILE] = 0xCC
    exe.write_bytes(bytes(data))

    env = dict(os.environ)
    env["TERM"] = "xterm-256color"
    env["SDL_VIDEODRIVER"] = "dummy"
    env["SDL_AUDIODRIVER"] = "dummy"
    child = PtyProcess(
        [
            dosboxx, "-nogui", "-break-start",
            "-conf", "../probe.conf",
            "-conf", "game/dosboxScreamer_single.conf",
        ],
        work_dir,
        env,
    )
    names: list[str] = []
    crashed = False
    try:
        with ControlSession(port, timeout=timeout) as session:
            time.sleep(2.0)
            session.request("EXEC", "BPINT 3")
            session.request("EXEC", "RUN")
            deadline = time.time() + timeout
            hits = 0
            while time.time() < deadline and hits < limit:
                try:
                    session.request("BREAK", timeout=10.0)
                    response = session.request("EXEC", "EV EAX EBX EIP", timeout=10.0)
                    values = response.lines[-1].split() if response.lines else []
                    if len(values) < 3:
                        session.request("EXEC", "RUN", timeout=10.0)
                        time.sleep(1)
                        continue
                    ebx, eip = values[1].lower(), values[2].lower()
                    address = int(eip, 16)
                    probe = f"P{hits:02d}.BIN"
                    session.request(
                        "EXEC", f"MEMDUMPBIN 0150:{address:X} 5 {probe}", timeout=10.0
                    )
                    path = work_dir / probe
                    head = path.read_bytes()[:5] if path.exists() else b""
                    if head not in (MEMBER_OPEN_PATCHED, CALL_SITE_PATCHED):
                        session.request("EXEC", "RUN", timeout=10.0)
                        time.sleep(1)
                        continue
                    dump = f"N{hits:02d}.BIN"
                    session.request(
                        "EXEC",
                        f"MEMDUMPBIN 0150:{STACK_DUMP_BASE:X} {STACK_DUMP_SIZE:X} {dump}",
                        timeout=10.0,
                    )
                    name = ""
                    offset = int(ebx, 16) - STACK_DUMP_BASE
                    stack = work_dir / dump
                    if stack.exists() and 0 <= offset < STACK_DUMP_SIZE:
                        raw = stack.read_bytes()[offset : offset + 32]
                        name = raw.split(b"\x00")[0].decode("latin1", "replace")
                    names.append(name)
                    print(f"member open {hits}: EIP={eip} EBX={ebx} name={name!r}")
                    hits += 1
                    session.request("EXEC", "RUN", timeout=10.0)
                    time.sleep(0.2)
                except (ConnectionError, TimeoutError):
                    print("the game left the debugger after the member open")
                    crashed = True
                    break
    finally:
        child.close()
        data[MEMBER_OPEN_FILE] = originals[MEMBER_OPEN_FILE][0]
        data[CALL_SITE_FILE] = originals[CALL_SITE_FILE][0]
        exe.write_bytes(bytes(data))
        print("game copy restored")
    if crashed:
        print("limit: the debugger stopped the game at the member open")
    return names


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game-dir", type=Path, default=Path("work/entry/run/game"))
    parser.add_argument("--work-dir", type=Path, default=Path("work/entry/run"))
    parser.add_argument("--port", type=int, default=9876)
    parser.add_argument("--limit", type=int, default=60)
    parser.add_argument("--timeout", type=float, default=300.0)
    parser.add_argument("--dosboxx", default="dosbox-x")
    args = parser.parse_args()

    try:
        names = run_probe(
            args.game_dir, args.work_dir, args.port, args.limit, args.timeout, args.dosboxx
        )
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    print(f"observed {len(names)} member opens")
    for index, name in enumerate(names):
        print(f"  {index:2d} {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
