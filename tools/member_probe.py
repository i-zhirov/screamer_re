#!/usr/bin/env python3
"""Hook the container member open at run time and report every member name.

The protected-mode and linear breakpoints of the DOSBox-X debugger watch a memory
change, so they never fire at the member open.  The `BP` breakpoint is a real-mode
breakpoint, so it never fires at the protected-mode code either.  This tool uses a
software breakpoint instead: it patches a copy of the game executable with the INT 3
opcode 0xCC at the file offset of the member open `FUN_0004C08D`, sets the `BPINT 3`
breakpoint, runs the game, and reads the member name from the stack at every stop.

The member open sits at the module address 0x4C08D.  The tool recognizes the stop by
the patched bytes `CC 57 55 89 E5` at the stop address, so the module base does not
matter.  The member name sits at the `EBX` pointer.

The debugger consumes the INT 3 opcode, so the guest resumes at the next byte and the
patched `push esi` is skipped.  The Watcom frame then restores the stack pointer to a
frame that is four bytes too high, and the return address comes from the wrong stack
slot.  The tool repairs the frame: it decrements the stack pointer by four and writes
the `ESI` value at the new top, which is exactly what the skipped `push esi` does.
The member open then runs with a correct frame, and the opcode stays in place, so the
next member open fires the same breakpoint.

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
STACK_DUMP_BASE = 0x2F4000
STACK_DUMP_SIZE = 0x2000
REGISTERS = ("EAX", "EBX", "ECX", "EDX", "ESI", "EDI", "EBP", "ESP", "EIP")


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
    original = bytes(data[MEMBER_OPEN_FILE : MEMBER_OPEN_FILE + 5])
    print(f"member open bytes: {original.hex(' ')}")
    data[MEMBER_OPEN_FILE] = 0xCC
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
    started = time.monotonic()
    try:
        with ControlSession(port, timeout=timeout) as session:
            time.sleep(2.0)
            session.request("EXEC", "BPINT 3")
            session.request("EXEC", "RUN")
            deadline = time.time() + timeout
            while time.time() < deadline and len(names) < limit:
                try:
                    session.request("BREAK", timeout=10.0)
                    response = session.request("EXEC", "EV " + " ".join(REGISTERS), timeout=10.0)
                    values = response.lines[-1].split() if response.lines else []
                    if len(values) < len(REGISTERS):
                        session.request("EXEC", "RUN", timeout=10.0)
                        time.sleep(1)
                        continue
                    registers = dict(zip(REGISTERS, (int(value, 16) for value in values)))
                    eip = registers["EIP"]
                    probe = f"P{len(names):02d}.BIN"
                    session.request(
                        "EXEC", f"MEMDUMPBIN 0150:{eip:X} 5 {probe}", timeout=10.0
                    )
                    path = work_dir / probe
                    head = path.read_bytes()[:5] if path.exists() else b""
                    if head != MEMBER_OPEN_PATCHED:
                        session.request("EXEC", "RUN", timeout=10.0)
                        time.sleep(1)
                        continue
                    dump = f"N{len(names):02d}.BIN"
                    session.request(
                        "EXEC",
                        f"MEMDUMPBIN 0150:{STACK_DUMP_BASE:X} {STACK_DUMP_SIZE:X} {dump}",
                        timeout=10.0,
                    )
                    name = ""
                    offset = registers["EBX"] - STACK_DUMP_BASE
                    stack = work_dir / dump
                    if stack.exists() and 0 <= offset < STACK_DUMP_SIZE:
                        raw = stack.read_bytes()[offset : offset + 32]
                        name = raw.split(b"\x00")[0].decode("latin1", "replace")
                    names.append(name)
                    print(
                        f"member open {len(names) - 1}: t={time.monotonic() - started:6.1f}s "
                        f"EIP={eip:X} EBX={registers['EBX']:X} name={name!r}"
                    )
                    # Repair the skipped push esi: the frame needs the ESI on the stack.
                    esp = registers["ESP"] - 4
                    session.request("EXEC", f"SR ESP {esp:X}", timeout=10.0)
                    value = registers["ESI"]
                    session.request(
                        "EXEC",
                        f"SMV {esp:X} " + " ".join(f"{byte:02X}" for byte in value.to_bytes(4, "little")),
                        timeout=10.0,
                    )
                    session.request("EXEC", "RUN", timeout=10.0)
                    time.sleep(0.05)
                except ConnectionError:
                    print(
                        "the game closed the control connection; "
                        f"the capture ends after {len(names)} member opens"
                    )
                    break
    finally:
        child.close()
        data[MEMBER_OPEN_FILE] = original[0]
        exe.write_bytes(bytes(data))
        print("game copy restored")
    return names


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game-dir", type=Path, default=Path("work/entry/run/game"))
    parser.add_argument("--work-dir", type=Path, default=Path("work/entry/run"))
    parser.add_argument("--port", type=int, default=9876)
    parser.add_argument("--limit", type=int, default=200)
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
