"""Build one rebuilt module in the pinned container and run one in DOSBox-X.

The tool reads the build and run recipe of a module record, compiles the sources
with the pinned OpenWatcom V2 container, links the executable and the map, and drives
the DOSBox-X debugger through `tools/dosboxx_control.py`.  It writes only under
`work/rebuild/` and never modifies the shipped artifacts.

Usage:
    python3 tools/rebuild/harness.py build --module NAME
    python3 tools/rebuild/harness.py run --module NAME [--show]
    python3 tools/rebuild/harness.py digest --module NAME

Exit code 0 on success, 1 on a compile, a link, or an output difference, 2 on a
missing module record or a missing run recipe, and 3 when Docker or DOSBox-X is
missing, because a missing engine is a skip.
"""
from __future__ import annotations

import argparse
import shutil
import socket
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import store  # noqa: E402

IMAGE = (
    "arlaneenalra/watcom-docker@sha256:"
    "16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"
)
REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent
REBUILD_DIR = Path(__file__).resolve().parent
WORK_DIR = REPOSITORY_ROOT / "work/rebuild"
SHARED_SOURCES = ("chk.asm", "stack.asm", "cstart.c")
DOSBOX = "dosbox-x"
DOCKER = "docker"
DOSBOX_CONTROL = REPOSITORY_ROOT / "tools/dosboxx_control.py"
BASE_FLAGS = {
    "wasm": ("-bt=dos", "-zq", "-zcm=watcom"),
    "wcc386": ("-bt=dos", "-s", "-zq"),
}


def object_name(file: str) -> str:
    return f"{Path(file).stem}.obj"


def compile_line(source: dict) -> str:
    compiler = source["compiler"]
    flags = list(BASE_FLAGS[compiler]) + list(source.get("flags", []))
    output = f"/w/{object_name(source['file'])}"
    return " ".join([compiler, *flags, f"-fo={output}", source["file"]])


def link_line(module: dict) -> str:
    objects = [object_name(source["file"]) for source in module["sources"]]
    objects += ["chk.obj", "cstart.obj", "stack.obj"]
    parts = [
        "wlink",
        "system",
        "dos4g",
        "option",
        "quiet",
        "option",
        "nodefaultlibs",
        "option",
        "start=_cstart_",
    ]
    if "map" in module:
        parts += ["option", f"map=/w/{module['map']}"]
    parts += ["name", module["exe"], "file", ",".join(objects)]
    return " ".join(parts)


def build_script(module: dict) -> str:
    """The shell script of the container: the compiles and the link.

    The shared harness sources come first, then the sources of the record in
    record order, then the link.
    """
    lines = []
    for name in SHARED_SOURCES:
        compiler = "wasm" if name.endswith(".asm") else "wcc386"
        lines.append(compile_line({"file": name, "compiler": compiler}))
    for source in module["sources"]:
        lines.append(compile_line(source))
    lines.append(link_line(module))
    return "\n".join(lines)


def free_port() -> int:
    """A port that is free on 127.0.0.1 at the time of the call."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def run_script_text() -> str:
    return "break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n"


def conf_text(module: dict, port: int) -> str:
    return (
        "[dosbox]\n"
        f"mcp_server={port}\n"
        "memsize=32\n"
        "machine=svga_s3\n"
        "\n"
        "[autoexec]\n"
        f'mount D "{WORK_DIR}"\n'
        "D:\n"
        f"{module['run']['command']}\n"
    )


def missing_lines(expect: list[str], text: str) -> list[str]:
    """The expect lines no output line equals and no output line ends with.

    A suffix match is needed because the first driver marker is embedded in a
    longer print-buffer line.
    """
    lines = text.splitlines()
    return [
        line
        for line in expect
        if not any(out_line == line or out_line.endswith(line) for out_line in lines)
    ]


def command_build(name: str) -> int:
    module = store.load_module(name)
    if shutil.which(DOCKER) is None:
        print(f"skip: the {DOCKER} command is missing; install Docker to build {name}")
        return 3
    WORK_DIR.mkdir(parents=True, exist_ok=True)
    for source in SHARED_SOURCES:
        shutil.copy2(REBUILD_DIR / source, WORK_DIR / source)
    for source in module["sources"]:
        shutil.copy2(REBUILD_DIR / source["file"], WORK_DIR / source["file"])
    completed = subprocess.run(
        [
            DOCKER,
            "run",
            "--rm",
            "-v",
            f"{WORK_DIR}:/w",
            "-w",
            "/w",
            IMAGE,
            "sh",
            "-c",
            "set -e\n" + build_script(module),
        ],
        capture_output=True,
        text=True,
    )
    sys.stdout.write(completed.stdout)
    sys.stderr.write(completed.stderr)
    if completed.returncode != 0:
        print(f"error: the build of {name} failed", file=sys.stderr)
        return 1
    artifacts = [module["exe"]]
    if "map" in module:
        artifacts.append(module["map"])
    print(f"built {name}: " + ", ".join(str(WORK_DIR / a) for a in artifacts))
    return 0


def command_run(name: str, show: bool) -> int:
    module = store.load_module(name)
    recipe = module.get("run")
    if not recipe:
        print(f"error: the module {name} holds no run recipe", file=sys.stderr)
        return 2
    if shutil.which(DOSBOX) is None:
        print(f"skip: the {DOSBOX} command is missing; install DOSBox-X to run {name}")
        return 3
    port = free_port()
    WORK_DIR.mkdir(parents=True, exist_ok=True)
    conf_path = WORK_DIR / f"run_{name}.conf"
    debug_path = WORK_DIR / f"run_{name}.dbg"
    conf_path.write_text(conf_text(module, port))
    debug_path.write_text(run_script_text())
    completed = subprocess.run(
        [
            sys.executable,
            str(DOSBOX_CONTROL),
            "script",
            "--file",
            str(debug_path),
            "--port",
            str(port),
            "--cwd",
            str(WORK_DIR),
            "--timeout",
            str(recipe.get("timeout", 180)),
            "--launch",
            f"{DOSBOX} -nogui -conf run_{name}.conf",
        ],
        cwd=REPOSITORY_ROOT,
    )
    text = ""
    output_path = WORK_DIR / recipe["output"]
    if output_path.is_file():
        data = output_path.read_bytes().replace(b"\x00", b"")
        text = data.decode("utf-8", "replace")
    missing = missing_lines(recipe["expect"], text)
    if completed.returncode != 0 or missing:
        if completed.returncode != 0:
            print(f"error: the run of {name} failed", file=sys.stderr)
        for line in missing:
            print(f"error: the output lacks {line!r}", file=sys.stderr)
        sys.stdout.write(text)
        return 1
    if show:
        sys.stdout.write(text)
    print(f"ran {name}: {len(recipe['expect'])} markers found")
    return 0


def command_digest(name: str) -> int:
    print(store.source_digest(store.load_module(name)))
    return 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build and run one rebuilt module with the pinned engines."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    build_parser = subparsers.add_parser(
        "build", help="build one module in the pinned container"
    )
    build_parser.add_argument("--module", required=True, help="the module record name")
    run_parser = subparsers.add_parser(
        "run", help="run one module under DOSBox-X"
    )
    run_parser.add_argument("--module", required=True, help="the module record name")
    run_parser.add_argument(
        "--show", action="store_true", help="print the captured module output"
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
            return command_build(arguments.module)
        if arguments.command == "run":
            return command_run(arguments.module, arguments.show)
        return command_digest(arguments.module)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
