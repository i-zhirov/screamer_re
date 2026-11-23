"""Build one rebuilt module in the pinned container and run one in DOSBox-X or FreeDOS.

The tool reads the build and run recipe of a module record, compiles the sources
with the pinned OpenWatcom V2 container, links the executable and the map, and drives
the DOSBox-X debugger through `tools/dosboxx_control.py`.  It writes only under
`work/rebuild/` and never modifies the shipped artifacts.

Usage:
    python3 tools/rebuild/harness.py build --module NAME [--backend dosker]
    python3 tools/rebuild/harness.py run --module NAME [--backend dosker] [--show]
    python3 tools/rebuild/harness.py digest --module NAME

The `dosker` backend builds on the Dosker FreeDOS base with the V2 toolchain
volume.  A `--backend dosker` behavior run also uses the Dosker FreeDOS base,
with the DOS/4G 2.01a extender that `dosker_setup.py` extracts from the pinned
DOSBox-X.  The 2.01a accepts the V2 test stub; the shipped 1.95 and the V2 1.97
reject it with DOS/4GW error 1012.  The default behavior run uses DOSBox-X and
copies the shipped DOS4GW.EXE beside the module EXE.

Exit code 0 on success, 1 on a compile, a link, a Dosker failure, or an output
difference, 2 on a missing module record or a missing run recipe, and 3 when
Docker, DOSBox-X, Dosker, or the toolchain volume is missing.
"""
from __future__ import annotations

import argparse
import shutil
import socket
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import dosker  # noqa: E402
import store  # noqa: E402

IMAGE = (
    "arlaneenalra/watcom-docker@sha256:"
    "16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"
)
REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent
REBUILD_DIR = Path(__file__).resolve().parent
WORK_DIR = REPOSITORY_ROOT / "work/rebuild"
SHARED_SOURCES = store.SHARED_SOURCES
DOSBOX = "dosbox-x"
DOCKER = "docker"
DOSBOX_CONTROL = REPOSITORY_ROOT / "tools/dosboxx_control.py"
DOSKER_IMAGE = "freedos:1.3"
DOSKER_TOOLCHAIN_DRIVE = "D:"
DOSKER_TOOLCHAIN_ROOT = "D:\\WATCOM"
DOSKER_STAGE = REPOSITORY_ROOT / "work/dosker/stage"
EXTENDER = store.DEFAULT_ORIGINAL.parent / "DOS4GW.EXE"
EXTENDER_201 = REPOSITORY_ROOT / "work/dosker/DOS4GW.EXE"
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


def dosker_source_name(index: int, source: dict) -> str:
    """The 8.3 copy name of one module source."""
    suffix = ".ASM" if source["compiler"] == "wasm" else ".C"
    return f"S{index}{suffix}"


def dosker_object_name(index: int) -> str:
    """The 8.3 object name of one module source."""
    return f"O{index}.OBJ"


def dosker_build_batch(module: dict) -> str:
    """The DOS batch that builds one module on the toolchain volume."""
    lines = [
        "@ECHO OFF",
        f"SET WATCOM={DOSKER_TOOLCHAIN_ROOT}",
        "SET DOS4GPATH=%WATCOM%\\BINW",
        "SET INCLUDE=%WATCOM%\\H;%WATCOM%\\H\\SYS",
        "SET PATH=%WATCOM%\\BINW;C:\\DOS",
        "IF NOT EXIST C:\\TMP\\NUL MD C:\\TMP >NUL",
        "SET TMP=C:\\TMP",
        "SET TEMP=C:\\TMP",
        "CD C:\\WORK",
        "WCC386 -bt=dos -s -zq -fo=C:\\WORK\\CSTART.OBJ C:\\WORK\\CSTART.C",
        "WASM -bt=dos -zq -zcm=watcom -fo=C:\\WORK\\CHK.OBJ C:\\WORK\\CHK.ASM",
        "WASM -bt=dos -zq -zcm=watcom -fo=C:\\WORK\\STACK.OBJ C:\\WORK\\STACK.ASM",
    ]
    for index, source in enumerate(module["sources"]):
        compiler = source["compiler"]
        flags = list(BASE_FLAGS[compiler]) + list(source.get("flags", []))
        name = dosker_source_name(index, source)
        obj = dosker_object_name(index)
        lines.append(
            f"{compiler} {' '.join(flags)} -fo=C:\\WORK\\{obj} C:\\WORK\\{name}"
        )
    lines.append("WLINK @C:\\WORK\\LINK.RSP")
    return "\r\n".join(lines) + "\r\n"


def dosker_link_rsp(module: dict) -> str:
    """The wlink response file that links one module on the volume."""
    lines = [
        "system dos4g",
        "option quiet",
        "option nodefaultlibs",
        "option start=_cstart_",
    ]
    if "map" in module:
        lines.append(f"option map=C:\\WORK\\{module['map']}")
    lines.append(f"name C:\\WORK\\{module['exe']}")
    files = [f"C:\\WORK\\{dosker_object_name(i)}" for i in range(len(module["sources"]))]
    files += ["C:\\WORK\\CHK.OBJ", "C:\\WORK\\CSTART.OBJ", "C:\\WORK\\STACK.OBJ"]
    lines.append("file " + ",".join(files))
    return "\r\n".join(lines) + "\r\n"


def build_dosker(module: dict, arguments: argparse.Namespace) -> int:
    """Build one module in a Dosker container with the V2 toolchain volume."""
    name = module["name"]
    client = dosker.DoskerClient(
        arguments.dosker, arguments.data_root, WORK_DIR
    )
    client.check_version()
    if not client.volume_exists(arguments.toolchain_volume):
        raise dosker.DoskerMissing(
            f"the toolchain volume {arguments.toolchain_volume} is missing; "
            "run tools/rebuild/dosker_setup.py"
        )
    staging = DOSKER_STAGE / name
    shutil.rmtree(staging, ignore_errors=True)
    staging.mkdir(parents=True)
    for source in SHARED_SOURCES:
        shutil.copy2(REBUILD_DIR / source, staging / source.upper())
    for index, source in enumerate(module["sources"]):
        shutil.copy2(
            REBUILD_DIR / source["file"],
            staging / dosker_source_name(index, source),
        )
    (staging / "BUILD.BAT").write_bytes(
        dosker_build_batch(module).encode()
    )
    (staging / "LINK.RSP").write_bytes(dosker_link_rsp(module).encode())
    container = f"screamer-build-{name[:24].replace('_', '-')}"
    WORK_DIR.mkdir(parents=True, exist_ok=True)
    try:
        client.start(
            DOSKER_IMAGE,
            container,
            volumes=[dosker.Volume(arguments.toolchain_volume, DOSKER_TOOLCHAIN_DRIVE)],
        )
        client.exec(container, "MD C:\\WORK", timeout=10)
        files = [staging / source.upper() for source in SHARED_SOURCES]
        files.append(staging / "BUILD.BAT")
        files.append(staging / "LINK.RSP")
        files += [
            staging / dosker_source_name(index, source)
            for index, source in enumerate(module["sources"])
        ]
        for host in files:
            client.copy_in(container, host, f"C:\\WORK\\{host.name}")
        result = client.exec(
            container, "C:\\WORK\\BUILD.BAT", workdir="C:\\WORK", timeout=600
        )
        if result.exit_code != 0:
            sys.stdout.write(result.output)
            print(f"error: the build of {name} failed", file=sys.stderr)
            return 1
        artifacts = [module["exe"]]
        if "map" in module:
            artifacts.append(module["map"])
        for artifact in artifacts:
            client.copy_out(
                container, f"C:\\WORK\\{artifact.upper()}", WORK_DIR / artifact
            )
    finally:
        client.remove(container)
    print(f"built {name}: " + ", ".join(str(WORK_DIR / a) for a in artifacts))
    return 0


def ensure_extender() -> None:
    """Put the shipped DOS/4GW extender beside the built module EXE."""
    target = WORK_DIR / "DOS4GW.EXE"
    if target.is_file():
        return
    if not EXTENDER.is_file():
        raise ValueError(f"the shipped extender {EXTENDER} is missing")
    WORK_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(EXTENDER, target)


def command_build(name: str, arguments: argparse.Namespace) -> int:
    module = store.load_module(name)
    if arguments.backend == "dosker":
        return build_dosker(module, arguments)
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


def run_dosker(
    name: str,
    module: dict,
    recipe: dict,
    show: bool,
    arguments: argparse.Namespace,
) -> int:
    """Run one module in the Dosker FreeDOS base with the DOS/4G 2.01a."""
    if not EXTENDER_201.is_file():
        raise dosker.DoskerMissing(
            f"the DOS/4G 2.01a extender {EXTENDER_201} is missing; "
            "run tools/rebuild/dosker_setup.py"
        )
    exe = WORK_DIR / module["exe"]
    if not exe.is_file():
        built = command_build(name, arguments)
        if built != 0:
            return built
    client = dosker.DoskerClient(arguments.dosker, arguments.data_root, WORK_DIR)
    client.check_version()
    container = f"screamer-run-{name[:20].replace('_', '-')}"
    result = None
    try:
        client.start(DOSKER_IMAGE, container)
        client.exec(container, "MD C:\\RUN", timeout=10)
        client.copy_in(container, exe, f"C:\\RUN\\{module['exe'].upper()}")
        client.copy_in(container, EXTENDER_201, "C:\\RUN\\DOS4GW.EXE")
        result = client.exec(
            container,
            module["exe"],
            workdir="C:\\RUN",
            timeout=recipe.get("timeout", 180),
        )
    finally:
        client.remove(container)
    text = result.output if result is not None else ""
    output_path = WORK_DIR / recipe["output"]
    output_path.write_text(text)
    missing = missing_lines(recipe["expect"], text)
    if result is None or result.timed_out or missing:
        if result is None:
            print(f"error: the run of {name} produced no result", file=sys.stderr)
        elif result.timed_out:
            print(f"error: the run of {name} timed out", file=sys.stderr)
        for line in missing:
            print(f"error: the output lacks {line!r}", file=sys.stderr)
        sys.stdout.write(text)
        return 1
    if show:
        sys.stdout.write(text)
    print(
        f"ran {name}: {len(recipe['expect'])} markers found, "
        f"exit {result.exit_code}"
    )
    return 0


def command_run(name: str, show: bool, arguments: argparse.Namespace) -> int:
    module = store.load_module(name)
    recipe = module.get("run")
    if not recipe:
        print(f"error: the module {name} holds no run recipe", file=sys.stderr)
        return 2
    if arguments.backend == "dosker":
        return run_dosker(name, module, recipe, show, arguments)
    ensure_extender()
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
    for command_parser in (build_parser, run_parser):
        command_parser.add_argument(
            "--backend", choices=("legacy", "dosker"), default="legacy"
        )
        command_parser.add_argument("--dosker", default="dosker")
        command_parser.add_argument(
            "--data-root",
            type=Path,
            default=REPOSITORY_ROOT / "work/dosker/root",
        )
        command_parser.add_argument(
            "--toolchain-volume", default=dosker.TOOLCHAIN_VOLUME
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
            return command_build(arguments.module, arguments)
        if arguments.command == "run":
            return command_run(arguments.module, arguments.show, arguments)
        return command_digest(arguments.module)
    except dosker.DoskerMissing as error:
        print(f"skip: {error}")
        return 3
    except dosker.DoskerError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
