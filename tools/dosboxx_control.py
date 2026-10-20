#!/usr/bin/env python3
"""Drive the DOSBox-X debugger through its external control socket.

DOSBox-X connects to 127.0.0.1:<port> when the `[dosbox] mcp_server` option
is set.  This tool listens on that port, sends debugger requests, and prints the
responses.  The protocol is line based:

    REQ <id> PING                 -> BEGIN <id> OK / PONG / END <id>
    REQ <id> BREAK                -> break into the debugger
    REQ <id> EXEC <debug command>  -> run one debugger command

The debugger needs a terminal.  Start DOSBox-X under a pseudo terminal and set
TERM, or the debugger refuses to start on macOS.  The `--launch` option of the
`script` command does this.

Usage:
    python3 tools/dosboxx_control.py listen --port 9876
    python3 tools/dosboxx_control.py run --port 9876 \
        --command BREAK --command "EV CS EIP" --command GDT
    python3 tools/dosboxx_control.py script --file FILE --port 9876 \
        --cwd WORKDIR \
        --launch "dosbox-x -nogui -break-start -conf probe.conf -conf game/game.conf"

Session script commands, one per line:

    # comment                     ignored
    break                         break into the debugger
    clear                         clear the captured emulator output
    exec <debug command>           run one debugger command
    expect <text>                  fail when the last response lacks the text
    note <text>                    print a note
    read <register> ...            read registers through the debugger EV command
    sleep <seconds>                wait
    stop                          end the script
    wait <marker> [timeout]        wait for a marker in the emulator output
    key <text>                    write text to the emulator console, \\r for Enter
"""

from __future__ import annotations

import argparse
import codecs
import fcntl
import os
import pty
import re
import shlex
import socket
import struct
import subprocess
import sys
import termios
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path

ANSI_RE = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]|\x1b\([0-9A-Z]|\x1b\[[0-9;]*[A-Za-z]")


@dataclass
class Response:
    request_id: str
    ok: bool
    lines: list[str] = field(default_factory=list)

    @property
    def text(self) -> str:
        return "\n".join(self.lines)


class ControlSession:
    """Server side of the DOSBox-X debugger control protocol."""

    def __init__(self, port: int, timeout: float = 60.0) -> None:
        self.port = port
        self.timeout = timeout
        self._server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._server.bind(("127.0.0.1", port))
        self._server.listen(1)
        self._connection: socket.socket | None = None
        self._buffer = b""
        self._next_id = 0
        self.events: list[str] = []

    def __enter__(self) -> "ControlSession":
        print(f"listening on 127.0.0.1:{self.port}")
        self._server.settimeout(self.timeout)
        self._connection, _ = self._server.accept()
        self._connection.settimeout(self.timeout)
        print("DOSBox-X connected")
        return self

    def __exit__(self, *_exc: object) -> None:
        if self._connection is not None:
            self._connection.close()
        self._server.close()

    def _read_line(self) -> str:
        while b"\n" not in self._buffer:
            chunk = self._connection.recv(65536)
            if not chunk:
                raise ConnectionError("DOSBox-X closed the control connection")
            self._buffer += chunk
        line, _, self._buffer = self._buffer.partition(b"\n")
        return line.decode("utf-8", "replace")

    def request(self, command: str, payload: str = "", timeout: float | None = None) -> Response:
        self._next_id += 1
        request_id = str(self._next_id)
        message = f"REQ {request_id} {command}"
        if payload:
            message += f" {payload}"
        self._connection.sendall((message + "\n").encode())
        if timeout is not None:
            self._connection.settimeout(timeout)
        return self._read_response(request_id)

    def _read_response(self, request_id: str) -> Response:
        while True:
            line = self._read_line()
            if not line.startswith("BEGIN "):
                continue
            _, identifier, status = (line.split(" ", 2) + ["", ""])[:3]
            lines: list[str] = []
            while True:
                inner = self._read_line()
                if inner.startswith("END "):
                    break
                lines.append(inner)
            if identifier == request_id:
                return Response(request_id, status == "OK", lines)
            if identifier == "event":
                self.events.append("\n".join(lines))
                print("[event]")
                for inner in lines:
                    print(f"  {inner}")


class PtyProcess:
    """Run a child process on a pseudo terminal and capture its output."""

    def __init__(self, argv: list[str], cwd: Path, env: dict[str, str] | None = None) -> None:
        self.master, slave = pty.openpty()
        fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 50, 200, 0, 0))
        self.process = subprocess.Popen(
            argv,
            cwd=cwd,
            env=env,
            stdin=slave,
            stdout=slave,
            stderr=slave,
            close_fds=True,
        )
        os.close(slave)
        self._output: list[bytes] = []
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._drain, daemon=True)
        self._thread.start()

    def _drain(self) -> None:
        try:
            while not self._stop.is_set():
                chunk = os.read(self.master, 65536)
                if not chunk:
                    break
                self._output.append(chunk)
        except OSError:
            pass

    def text(self) -> str:
        return ANSI_RE.sub("", b"".join(self._output).decode("utf-8", "replace"))

    def clear(self) -> None:
        self._output.clear()

    def wait_for(self, marker: str, timeout: float) -> bool:
        deadline = time.time() + timeout
        while time.time() < deadline:
            if marker in self.text():
                return True
            if self.process.poll() is not None:
                return False
            time.sleep(0.3)
        return False

    def close(self) -> None:
        self._stop.set()
        self.process.kill()
        time.sleep(0.3)
        try:
            os.close(self.master)
        except OSError:
            pass


@dataclass
class ScriptStep:
    line_number: int
    command: str
    arguments: str


def parse_script(path: Path) -> list[ScriptStep]:
    steps: list[ScriptStep] = []
    for number, raw in enumerate(path.read_text().splitlines(), start=1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        command, _, arguments = line.partition(" ")
        steps.append(ScriptStep(number, command.lower(), arguments.strip()))
    return steps


class ScriptRunner:
    """Run a debugger script against a control session."""

    def __init__(
        self,
        session: ControlSession,
        child: PtyProcess | None = None,
        timeout: float = 60.0,
    ) -> None:
        self.session = session
        self.child = child
        self.timeout = timeout
        self.failures = 0
        self.last_response: Response | None = None
        self.stopped = False

    def _fail(self, step: ScriptStep, message: str) -> None:
        self.failures += 1
        print(f"line {step.line_number}: FAIL: {message}")

    def run(self, steps: list[ScriptStep]) -> int:
        for step in steps:
            handler = getattr(self, f"_step_{step.command}", None)
            if handler is None:
                self._fail(step, f"unknown command '{step.command}'")
                continue
            handler(step)
            if self.stopped:
                break
        return self.failures

    def _step_break(self, step: ScriptStep) -> bool:
        response = self.session.request("BREAK", "", timeout=self.timeout)
        print(f"break -> ok={response.ok}")
        if not response.ok:
            self._fail(step, "the break request failed")
        return response.ok

    def _step_clear(self, step: ScriptStep) -> bool:
        if self.child is None:
            self._fail(step, "clear needs --launch")
            return False
        self.child.clear()
        return True

    def _step_exec(self, step: ScriptStep) -> bool:
        if not step.arguments:
            self._fail(step, "exec needs a debugger command")
            return False
        response = self.session.request("EXEC", step.arguments, timeout=self.timeout)
        self.last_response = response
        print(f"exec {step.arguments} -> ok={response.ok}")
        for line in response.lines:
            print(f"  {line}")
        if not response.ok:
            self._fail(step, "the debugger command failed")
        return response.ok

    def _step_expect(self, step: ScriptStep) -> bool:
        if self.last_response is None:
            self._fail(step, "expect needs a preceding exec")
            return False
        if step.arguments not in self.last_response.text:
            self._fail(step, f"the response lacks '{step.arguments}'")
            return False
        return True

    def _step_note(self, step: ScriptStep) -> bool:
        print(f"note: {step.arguments}")
        return True

    def _step_read(self, step: ScriptStep) -> bool:
        names = step.arguments.split()
        if not names:
            self._fail(step, "read needs one or more register names")
            return False
        response = self.session.request("EXEC", f"EV {' '.join(names)}", timeout=self.timeout)
        self.last_response = response
        if not response.ok or not response.lines:
            self._fail(step, "the register read failed")
            return False
        values = response.lines[-1].split()
        if len(values) != len(names):
            self._fail(step, f"expected {len(names)} values, got {len(values)}")
            return False
        print("read " + " ".join(f"{name}={value}" for name, value in zip(names, values)))
        return True

    def _step_sleep(self, step: ScriptStep) -> bool:
        try:
            seconds = float(step.arguments)
        except ValueError:
            self._fail(step, f"sleep needs a number, got '{step.arguments}'")
            return False
        time.sleep(seconds)
        return True

    def _step_key(self, step: ScriptStep) -> bool:
        if self.child is None:
            self._fail(step, "key needs --launch")
            return False
        text = codecs.decode(step.arguments, "unicode_escape")
        data = text.encode("latin-1", "replace")
        if not data:
            self._fail(step, "key needs text")
            return False
        os.write(self.child.master, data)
        print(f"key {data!r} -> sent")
        return True

    def _step_stop(self, step: ScriptStep) -> bool:
        print("stop")
        self.stopped = True
        return True

    def _step_wait(self, step: ScriptStep) -> bool:
        if self.child is None:
            self._fail(step, "wait needs --launch")
            return False
        parts = step.arguments.split()
        if not parts:
            self._fail(step, "wait needs a marker")
            return False
        marker = parts[0]
        timeout = float(parts[1]) if len(parts) > 1 else self.timeout
        if not self.child.wait_for(marker, timeout):
            self._fail(step, f"the marker '{marker}' did not appear")
            return False
        print(f"wait {marker} -> found")
        return True


def launch_argv(launch: str) -> list[str]:
    return shlex.split(launch)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)

    listen_parser = subparsers.add_parser("listen", help="print every response")
    listen_parser.add_argument("--port", type=int, required=True)
    listen_parser.add_argument("--count", type=int, default=0, help="stop after N responses")

    run_parser = subparsers.add_parser("run", help="send a fixed command list")
    run_parser.add_argument("--port", type=int, required=True)
    run_parser.add_argument("--command", action="append", default=[], help="debugger command")

    script_parser = subparsers.add_parser("script", help="run a session script")
    script_parser.add_argument("--file", type=Path, required=True)
    script_parser.add_argument("--port", type=int, default=9876)
    script_parser.add_argument("--timeout", type=float, default=60.0)
    script_parser.add_argument("--cwd", type=Path, default=Path.cwd())
    script_parser.add_argument(
        "--launch",
        default=None,
        help="command line of the DOSBox-X child process; omit to attach to a running one",
    )

    args = parser.parse_args()
    if args.command == "script":
        return run_script(args)
    with ControlSession(args.port) as session:
        if args.command == "listen":
            count = 0
            while args.count == 0 or count < args.count:
                line = session._read_line()
                print(line)
                count += 1
        else:
            for debug_command in args.command:
                head, _, payload = debug_command.partition(" ")
                response = session.request(head.upper(), payload)
                print(f"$ {debug_command}")
                print(f"  ok={response.ok}")
                for line in response.lines:
                    print(f"  {line}")
    return 0


def run_script(args: argparse.Namespace) -> int:
    steps = parse_script(args.file)
    print(f"{len(steps)} script steps from {args.file}")
    env = dict(os.environ)
    # The DOSBox-X debugger draws with ncurses and needs a usable terminal.
    env["TERM"] = "xterm-256color"
    child: PtyProcess | None = None
    try:
        if args.launch:
            env["SDL_VIDEODRIVER"] = "dummy"
            env["SDL_AUDIODRIVER"] = "dummy"
            print(f"launch: {args.launch}")
            child = PtyProcess(launch_argv(args.launch), args.cwd, env)
        with ControlSession(args.port, timeout=args.timeout) as session:
            if child is not None:
                time.sleep(2.0)
            runner = ScriptRunner(session, child, args.timeout)
            failures = runner.run(steps)
            print(f"{len(steps)} steps, {failures} failures")
            return 1 if failures else 0
    finally:
        if child is not None:
            child.close()


if __name__ == "__main__":
    raise SystemExit(main())
