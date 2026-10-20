#!/usr/bin/env python3
"""Find the program entry with a DOSBox-X instruction log.

The tool runs the game in DOSBox-X, breaks into the debugger, sets an interrupt
breakpoint on the game stub's DOS4GW exec call (INT 21h AH=4B), and starts a CPU
instruction log.  The log covers the DOS4GW load and the first game instruction.
The tool reads the growing log, finds the entry sequence, stops the emulation, and
reports the entry address.

The tool never modifies the shipped artifacts.  It writes only under `work/`.

Usage:
    python3 tools/entry_probe.py --game-dir DIR [--count 0x100000]
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

# Fallback signature only.  The entry is a short jump over the Watcom run-time
# banner string.  The code after the jump starts with `sti` and `mov ebx,esp`.
CANDIDATES = [
    ("entry code after the jump", "mov  ebx,esp"),
]


def read_cs_eip(session: ControlSession) -> tuple[str, str]:
    """Read the current CS and EIP through the debugger EV command."""
    response = session.request("EXEC", "EV CS EIP", timeout=20)
    values = response.lines[-1].split()
    if len(values) < 2:
        raise ValueError(f"unexpected EV response: {response.lines}")
    return values[0], values[1]


def detect_entry(
    parsed: list[tuple[str, str, str]],
    candidates: list[tuple[str, str]],
    window: int = 6,
) -> dict | None:
    """Find the entry sequence in parsed log lines."""
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
            "mov  ebx,esp" in line for line in following[1:3]
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
    return None


class LogScanner:
    """Read a growing instruction log and find the entry sequence."""

    def __init__(self, candidates: list[tuple[str, str]], window: int = 6) -> None:
        self.candidates = candidates
        self.window = window
        self.parsed: list[tuple[str, str, str]] = []
        self._offset = 0
        self._pending = ""

    @property
    def lines(self) -> int:
        return len(self.parsed)

    def feed(self, path: Path) -> dict | None:
        """Read the new part of the log and return the entry when it is present."""
        if not path.exists():
            return None
        with path.open("rb") as stream:
            stream.seek(self._offset)
            chunk = stream.read()
            self._offset = stream.tell()
        text = self._pending + chunk.decode("utf-8", "replace")
        lines = text.split("\n")
        self._pending = lines.pop()
        for line in lines:
            match = LOG_LINE_RE.search(line)
            if match:
                self.parsed.append((match.group(1), match.group(2), match.group(3)))
        return detect_entry(self.parsed, self.candidates, self.window)


def find_entry(log: str, candidates: list[tuple[str, str]], window: int = 6) -> dict:
    """Find the entry sequence in a complete instruction log."""
    parsed: list[tuple[str, str, str]] = []
    for line in log.splitlines():
        match = LOG_LINE_RE.search(line)
        if match:
            parsed.append((match.group(1), match.group(2), match.group(3)))
    found = detect_entry(parsed, candidates, window)
    if found:
        return found
    return {"found": False, "lines": len(parsed)}


def build_conf(source: Path, target: Path, port: int) -> None:
    text = source.read_text()
    text = text.replace("fullscreen=true", "fullscreen=false")
    text = text.replace("output=opengl", "output=surface")
    text = text.replace("[dosbox]\n", f"[dosbox]\nmcp_server={port}\n", 1)
    target.write_text(text)


def run_probe(
    game_dir: Path,
    count: int,
    port: int,
    timeout: float,
    work_root: Path,
    dosboxx: str,
    verbose: bool = True,
) -> dict:
    """Run the game in DOSBox-X and return the entry result."""
    work_root = work_root.resolve()
    work_root.mkdir(parents=True, exist_ok=True)
    conf = work_root / "probe.conf"
    build_conf(game_dir.resolve() / "dosboxScreamer.conf", conf, port)
    # The conf mounts the relative path "game", so the working directory needs a
    # game copy below it.  A clone keeps the copy fast and space efficient.
    run_dir = work_root / "run"
    if (run_dir / "game").exists():
        shutil.rmtree(run_dir / "game")
    run_dir.mkdir(parents=True, exist_ok=True)
    subprocess.run(["cp", "-cR", str(game_dir.resolve()), str(run_dir / "game")], check=True)

    env = dict(os.environ)
    env["SDL_VIDEODRIVER"] = "dummy"
    env["SDL_AUDIODRIVER"] = "dummy"
    env["TERM"] = "xterm-256color"
    child = PtyProcess(
        [
            dosboxx,
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
        with ControlSession(port, timeout=timeout) as session:
            time.sleep(2.0)
            for command in ('BPINT 21 "4B"', "BPLIST"):
                response = session.request("EXEC", command, timeout=30)
                if verbose:
                    print(f"$ {command} -> ok={response.ok}")
                    for line in response.lines[:5]:
                        print(f"   {line}")
            session.request("EXEC", "RUN", timeout=30)
            at_stub_exec = False
            for _ in range(20):
                if not child.wait_for("I->", timeout):
                    result["note"] = "the debugger did not stop"
                    break
                selector, offset = read_cs_eip(session)
                if verbose:
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
                    if verbose:
                        for line in response.lines[:8]:
                            print(f"   {line}")
                # Remove the breakpoint first, or it triggers again on the same
                # instruction and stops the log.
                session.request("EXEC", "BPDEL *", timeout=30)
                session.request("EXEC", f"LOGS {count:x}", timeout=30)
                scanner = LogScanner(CANDIDATES)
                found = None
                deadline = time.time() + timeout
                while time.time() < deadline:
                    time.sleep(2.0)
                    found = scanner.feed(log_file)
                    if found:
                        break
                    if "LOGCPU.TXT created" in child.text():
                        found = scanner.feed(log_file)
                        break
                # Stop the emulation and read the rest of the log.
                session.request("BREAK", "", timeout=30)
                time.sleep(0.5)
                if found is None:
                    found = scanner.feed(log_file)
                result["lines"] = scanner.lines
                if found:
                    result.update(found)
                else:
                    result.setdefault("note", "the entry is not in the log")
    finally:
        child.close()
    return result


def print_result(result: dict) -> None:
    for key, value in result.items():
        if key == "following":
            print(f"{key}:")
            for line in value:
                print(f"   {line}")
        else:
            print(f"{key}: {value}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game-dir", type=Path, required=True)
    parser.add_argument("--count", type=lambda value: int(value, 0), default=0x100000)
    parser.add_argument("--port", type=int, default=9876)
    parser.add_argument("--timeout", type=float, default=300.0)
    parser.add_argument("--work-root", type=Path, default=Path("work/entry"))
    parser.add_argument("--dosboxx", default="dosbox-x")
    args = parser.parse_args()

    result = run_probe(
        args.game_dir,
        args.count,
        args.port,
        args.timeout,
        args.work_root,
        args.dosboxx,
    )
    print_result(result)
    return 0 if result.get("found") else 1


if __name__ == "__main__":
    raise SystemExit(main())
