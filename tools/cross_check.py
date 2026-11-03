#!/usr/bin/env python3
"""Compare the identity facts of an LE image across three tools.

The tool reads the entry, the page-0 mapping, the main function, and the main
caller with `le.py` in process, radare2 as a subprocess, and the Ghidra wrapper
as a subprocess.  It prints four fact lines in a fixed order and exits 0 when
every fact agrees, 1 when a fact differs or a tool fails, and 2 when an input
or a tool is unusable.  The raw tool output stays under
`<work-root>/cross-check/`.

Usage:
    python3 tools/cross_check.py --original PATH --main ADDR --call-site ADDR
        [--work-root work] [--r2 r2] [--ghidra tools/ghidra/identity.sh]
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import le  # noqa: E402

FACT_ORDER = ("entry", "page0", "main", "caller")
FACT_FIELDS = {
    "entry": ("entry",),
    "page0": ("page0_module", "page0_file"),
    "main": ("main",),
    "caller": ("caller", "call_site", "call_target"),
}
TOOLS = ("le", "r2", "ghidra")
GHIDRA_KEYS = (
    "entry",
    "page0_module",
    "page0_file",
    "main",
    "caller",
    "call_site",
    "call_target",
)
PAGE0_MODULE = 0x10000
R2_TIMEOUT_SECONDS = 300.0
GHIDRA_TIMEOUT_SECONDS = 1800.0

ANSI_ESCAPE = re.compile(r"\x1b\[[0-9;]*m")
CALL_XREF = re.compile(
    r"CALL XREF from fcn\.([0-9a-fA-F]+) @ (0x[0-9a-fA-F]+)"
)
PDF_FUNCTION = re.compile(r"fcn\.([0-9a-fA-F]+)\s*\(\s*\)")
GHIDRA_PREFIX = "INFO  LeIdentity.java> "
CROSS_CHECK_COMMAND = "nix develop .#cross-check"


@dataclass
class IdentityFact:
    name: str
    values: dict[str, dict[str, int | None]]
    status: str


def strip_ansi(text: str) -> str:
    """Remove the ANSI color escapes from a tool output."""
    return ANSI_ESCAPE.sub("", text)


def parse_r2(text: str, call_site: int) -> dict[str, dict[str, int | None]]:
    """Read the four facts from the `ie`, `iS`, and `pdf` radare2 output."""
    text = strip_ansi(text)
    entry = None
    page0_file = None
    page0_module = None
    main = None
    caller = None
    caller_site = None
    caller_target = None
    for line in text.splitlines():
        tokens = line.split()
        if len(tokens) >= 2 and tokens[-1] == "program":
            try:
                entry = int(tokens[1], 16)
            except ValueError:
                entry = None
            continue
        if len(tokens) >= 4 and tokens[0].isdigit():
            try:
                paddr = int(tokens[1], 16)
                vaddr = int(tokens[3], 16)
            except ValueError:
                continue
            if vaddr == PAGE0_MODULE:
                page0_module = vaddr
                page0_file = paddr
    function = PDF_FUNCTION.search(text)
    if function is not None:
        main = int(function.group(1), 16)
    matches = [
        match
        for match in CALL_XREF.finditer(text)
        if int(match.group(2), 16) == call_site
    ]
    if len(matches) == 1:
        caller = int(matches[0].group(1), 16)
        caller_site = int(matches[0].group(2), 16)
        caller_target = main
    return {
        "entry": {"entry": entry},
        "page0": {"page0_module": page0_module, "page0_file": page0_file},
        "main": {"main": main},
        "caller": {
            "caller": caller,
            "call_site": caller_site,
            "call_target": caller_target,
        },
    }


def parse_ghidra(text: str) -> dict[str, int | None]:
    """Read the seven identity keys from the Ghidra wrapper output."""
    values: dict[str, int | None] = {key: None for key in GHIDRA_KEYS}
    for line in strip_ansi(text).splitlines():
        line = line.strip()
        if line.startswith(GHIDRA_PREFIX):
            line = line[len(GHIDRA_PREFIX) :]
        for key in GHIDRA_KEYS:
            marker = f"{key}="
            if not line.startswith(marker):
                continue
            raw = line[len(marker) :].strip()
            if raw and raw != "unknown":
                try:
                    values[key] = int(raw, 16)
                except ValueError:
                    values[key] = None
            break
    return values


def le_facts(
    exe: le.LinearExecutable, main: int, call_site: int
) -> dict[str, dict[str, int | None]]:
    """Read the four facts from the image with the LE parser."""
    try:
        starts = le.function_starts(exe)
    except ValueError:
        starts = []
    below = [start for start in starts if start <= main]
    main_value = max(below) if below else None
    page = exe.page_at(PAGE0_MODULE)
    page0_module = PAGE0_MODULE if page is not None else None
    page0_file = exe.to_file_offset(PAGE0_MODULE) if page is not None else None
    try:
        caller = le.nearest_function_start(exe, call_site)
    except ValueError:
        caller = None
    call_target = None
    try:
        instructions = le.decode(exe, call_site, 1)
    except ValueError:
        instructions = []
    if instructions:
        instruction = instructions[0]
        operand = instruction.operands.strip()
        if instruction.mnemonic == "call" and operand.startswith("0x"):
            call_target = int(operand.split()[0], 16)
    return {
        "entry": {"entry": exe.entry_linear},
        "page0": {"page0_module": page0_module, "page0_file": page0_file},
        "main": {"main": main_value},
        "caller": {
            "caller": caller,
            "call_site": call_site,
            "call_target": call_target,
        },
    }


def _ghidra_view(
    fields: dict[str, int | None]
) -> dict[str, dict[str, int | None]]:
    """Group the flat Ghidra keys into the four fact maps."""
    return {
        "entry": {"entry": fields.get("entry")},
        "page0": {
            "page0_module": fields.get("page0_module"),
            "page0_file": fields.get("page0_file"),
        },
        "main": {"main": fields.get("main")},
        "caller": {
            "caller": fields.get("caller"),
            "call_site": fields.get("call_site"),
            "call_target": fields.get("call_target"),
        },
    }


def compare_facts(
    le_view: dict[str, dict[str, int | None]],
    r2_view: dict[str, dict[str, int | None]],
    ghidra_view: dict[str, dict[str, int | None]],
) -> list[IdentityFact]:
    """Build the four facts and mark each one agree or differ."""
    views = {"le": le_view, "r2": r2_view, "ghidra": ghidra_view}
    facts: list[IdentityFact] = []
    for name in FACT_ORDER:
        values = {
            tool: dict(views[tool].get(name, {})) for tool in TOOLS
        }
        agree = True
        for field in FACT_FIELDS[name]:
            field_values = [values[tool].get(field) for tool in TOOLS]
            if any(value is None for value in field_values):
                agree = False
                break
            if len(set(field_values)) != 1:
                agree = False
                break
        facts.append(IdentityFact(name, values, "agree" if agree else "differ"))
    return facts


def _hex(value: int | None) -> str:
    return "unknown" if value is None else f"{value:#x}"


def _fact_value(name: str, fields: dict[str, int | None]) -> str:
    if name == "entry":
        return _hex(fields.get("entry"))
    if name == "page0":
        return f"{_hex(fields.get('page0_file'))}->{_hex(fields.get('page0_module'))}"
    if name == "main":
        return _hex(fields.get("main"))
    return (
        f"{_hex(fields.get('call_site'))}@{_hex(fields.get('caller'))}"
        f"->{_hex(fields.get('call_target'))}"
    )


def render_fact(fact: IdentityFact) -> str:
    """Render one fact line in the fixed output grammar."""
    parts = [
        f"{tool}={_fact_value(fact.name, fact.values.get(tool, {}))}"
        for tool in TOOLS
    ]
    return f"{fact.name}: {fact.status} " + " ".join(parts)


def run_radare2(original: Path, r2: str, main: int) -> str:
    """Run radare2 and return the ANSI-stripped output."""
    command = [
        r2,
        "-A",
        "-q",
        "-c",
        "ie",
        "-c",
        "iS",
        "-c",
        f"s {main:#x}; pdf",
        "-c",
        "q",
        str(original),
    ]
    result = subprocess.run(
        command, capture_output=True, text=True, timeout=R2_TIMEOUT_SECONDS
    )
    if result.returncode != 0:
        raise subprocess.CalledProcessError(
            result.returncode, command, result.stdout, result.stderr
        )
    return strip_ansi(result.stdout)


def run_ghidra(
    original: Path,
    ghidra: Path,
    project_dir: Path,
    log_path: Path,
    main: int,
    call_site: int,
) -> str:
    """Run the Ghidra wrapper and return its identity output."""
    command = [
        "sh",
        str(ghidra),
        "--exe",
        str(original),
        "--project-dir",
        str(project_dir),
        "--log",
        str(log_path),
        "--main",
        f"{main:#x}",
        "--call-site",
        f"{call_site:#x}",
    ]
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            timeout=GHIDRA_TIMEOUT_SECONDS,
        )
    except (subprocess.TimeoutExpired, OSError):
        shutil.rmtree(project_dir, ignore_errors=True)
        raise
    if result.returncode != 0:
        shutil.rmtree(project_dir, ignore_errors=True)
        raise subprocess.CalledProcessError(
            result.returncode, command, result.stdout, result.stderr
        )
    return result.stdout


def _resolve_tool(value: str) -> str | None:
    """Resolve a tool from an explicit path or from `PATH`."""
    if os.sep in value or (os.altsep is not None and os.altsep in value):
        return value if Path(value).is_file() else None
    return shutil.which(value)


def _text(value: object) -> str:
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", "replace")
    return str(value)


def _run_tool(function, *arguments) -> tuple[str, str, bool]:
    try:
        return function(*arguments), "", True
    except subprocess.TimeoutExpired as error:
        return _text(error.stdout), _text(error.stderr), False
    except subprocess.SubprocessError as error:
        return (
            _text(getattr(error, "stdout", "")),
            _text(getattr(error, "stderr", "")),
            False,
        )
    except OSError as error:
        return "", _text(error), False


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--main", type=lambda value: int(value, 0), required=True)
    parser.add_argument(
        "--call-site", type=lambda value: int(value, 0), required=True
    )
    parser.add_argument("--work-root", type=Path, default=Path("work"))
    parser.add_argument("--r2", default="r2")
    parser.add_argument("--ghidra", default="tools/ghidra/identity.sh")
    args = parser.parse_args(argv)

    try:
        exe = le.load(args.original)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    if exe.page_at(args.main) is None:
        print(
            f"error: the main address {args.main:#x} is not mapped",
            file=sys.stderr,
        )
        return 2
    if exe.page_at(args.call_site) is None:
        print(
            f"error: the call site {args.call_site:#x} is not mapped",
            file=sys.stderr,
        )
        return 2
    if not le.capstone_available():
        print(
            "error: the capstone library is missing; run the cross-check in: "
            f"{CROSS_CHECK_COMMAND}",
            file=sys.stderr,
        )
        return 2
    r2 = _resolve_tool(args.r2)
    if r2 is None:
        print(
            "error: radare2 is missing; run the cross-check in: "
            f"{CROSS_CHECK_COMMAND}",
            file=sys.stderr,
        )
        return 2
    ghidra = Path(args.ghidra)
    if not ghidra.is_file():
        print(
            f"error: the Ghidra wrapper {ghidra} is missing; run the "
            f"cross-check in: {CROSS_CHECK_COMMAND}",
            file=sys.stderr,
        )
        return 2

    work_dir = args.work_root / "cross-check"
    project_dir = work_dir / "ghidra" / "proj"
    work_dir.mkdir(parents=True, exist_ok=True)

    le_view = le_facts(exe, args.main, args.call_site)
    r2_text, r2_err, r2_ok = _run_tool(
        run_radare2, args.original, r2, args.main
    )
    (work_dir / "r2.txt").write_text(r2_text)
    (work_dir / "r2.err").write_text(r2_err)
    ghidra_text, ghidra_err, ghidra_ok = _run_tool(
        run_ghidra,
        args.original,
        ghidra,
        project_dir,
        work_dir / "ghidra.log",
        args.main,
        args.call_site,
    )
    (work_dir / "ghidra.txt").write_text(ghidra_text)
    (work_dir / "ghidra.err").write_text(ghidra_err)

    r2_view = (
        parse_r2(r2_text, args.call_site)
        if r2_ok
        else parse_r2("", args.call_site)
    )
    ghidra_view = _ghidra_view(
        parse_ghidra(ghidra_text) if ghidra_ok else parse_ghidra("")
    )
    facts = compare_facts(le_view, r2_view, ghidra_view)
    for fact in facts:
        print(render_fact(fact))
    return 0 if all(fact.status == "agree" for fact in facts) else 1


if __name__ == "__main__":
    raise SystemExit(main())
