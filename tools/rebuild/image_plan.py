"""Plan, build, and score the whole rebuilt STARTH.EXE image.

The tool owns the whole-image link plan, the best-effort whole-image build on
the Dosker V2 toolchain volume, and the coverage report.  The link plan names
the module link order and the address ranges that no module rebuilds.  The
coverage report counts the linked code bytes, the code gap, the data gap, and
the unmodeled ranges against the shipped image.  A gap is a plan gap, not a
build failure, so the tool reports it and does not claim a pass.

Usage:
    python3 tools/rebuild/image_plan.py seed [--original PATH] [--check]
    python3 tools/rebuild/image_plan.py build [--check] [--dosker PATH]
        [--data-root PATH] [--toolchain-volume NAME] [--timeout SECONDS]
    python3 tools/rebuild/image_plan.py coverage [--original PATH]
        [--rebuilt PATH] [--plan PATH] [--out PATH]

Exit code 0 on success and on a plan or an image that is complete, 1 on a
coverage gap, a link failure, a missing plan, or a missing image, 2 on a bad
argument or a broken plan, and 3 when Dosker or the toolchain volume is missing.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import dosker  # noqa: E402
import harness  # noqa: E402
import image_compare  # noqa: E402
import le  # noqa: E402
import store  # noqa: E402

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent
REBUILD_DIR = Path(__file__).resolve().parent
WORK_DIR = REPOSITORY_ROOT / "work/rebuild"
PLAN_PATH = REBUILD_DIR / "link.json"
STAGE_DIR = REPOSITORY_ROOT / "work/dosker/image-stage"
COVERAGE_PATH = WORK_DIR / "image_coverage.json"
IMAGE_EXE = "STARTH.EXE"
IMAGE_MAP = "STARTH.MAP"
CONTAINER = "screamer-image"
BUILD_LOG = "BUILD.LOG"
STEP_TIMEOUT = 1800
LINK_OPTIONS = (
    "system dos4g",
    "option quiet",
    "option nodefaultlibs",
    "option start=_cstart_",
)
RUNTIME_LIBS = ("D:\\WATCOM\\LIB386\\DOS\\CLIB3R.LIB",)
RUNTIME_MODULES = (
    "memory_runtime",
    "startup_runtime",
    "input_runtime",
    "sound_runtime",
    "game_runtime",
    "physics_runtime",
    "misc_runtime",
    "render_runtime",
    "files_runtime",
    "data_runtime",
)
SHARED_OBJECTS = ("CHK.OBJ", "CSTART.OBJ", "STACK.OBJ")
SHARED_FILES = ("CHK.ASM", "CSTART.C", "STACK.ASM")


class LinkPlanMissing(ValueError):
    """The link plan file is not on the host."""


def _read_json(path: Path) -> object:
    try:
        text = Path(path).read_text()
    except FileNotFoundError:
        raise LinkPlanMissing(f"{path}: the link plan is missing") from None
    except OSError as error:
        raise ValueError(f"{path}: {error}") from error
    try:
        return json.loads(text)
    except json.JSONDecodeError as error:
        raise ValueError(f"{path}: broken JSON: {error}") from error


def load_plan(path: Path = PLAN_PATH) -> dict:
    """Read and validate one link plan."""
    plan = _read_json(path)
    if not isinstance(plan, dict):
        raise ValueError(f"{path}: the link plan is not a JSON object")
    if plan.get("version") != 1:
        raise ValueError(
            f"{path}: the link plan version {plan.get('version')!r} is not 1"
        )
    order = plan.get("order")
    if not isinstance(order, list) or not order:
        raise ValueError(f"{path}: the link order is not a non-empty list")
    if not all(isinstance(name, str) and name for name in order):
        raise ValueError(f"{path}: every link order entry must be a name")
    if len(set(order)) != len(order):
        raise ValueError(f"{path}: the link order repeats a module name")
    for key in ("exe", "map"):
        value = plan.get(key)
        if not isinstance(value, str) or not store.is_dos_83(value):
            raise ValueError(f"{path}: the {key} {value!r} is not an 8.3 name")
    unmodeled = plan.get("unmodeled", [])
    if not isinstance(unmodeled, list):
        raise ValueError(f"{path}: the unmodeled field is not a list")
    image_compare.plan_unmodeled(plan)
    for key in ("link_options", "runtime_libs"):
        value = plan.get(key, [])
        if not isinstance(value, list) or not all(
            isinstance(entry, str) for entry in value
        ):
            raise ValueError(f"{path}: the {key} field is not a string list")
    return plan


def _write_plan(path: Path, plan: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(plan, indent=2) + "\n")


def derive_order(records: list[dict]) -> list[str]:
    """Order the modules by the lowest record address they hold."""
    first: dict[str, int] = {}
    for record in records:
        name = record.get("module")
        if not name:
            continue
        address = int(record["address"], 16)
        first[name] = min(first.get(name, address), address)
    return [name for name, _ in sorted(first.items(), key=lambda item: (item[1], item[0]))]


def merge_intervals(intervals: list[tuple[int, int]]) -> list[tuple[int, int]]:
    merged: list[tuple[int, int]] = []
    for start, end in sorted(intervals):
        if end <= start:
            continue
        if merged and start <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(merged[-1][1], end))
        else:
            merged.append((start, end))
    return merged


def record_extents(records: list[dict], limit: int) -> list[tuple[int, int]]:
    """The covered ranges of the module records, clipped to the code object."""
    ordered = sorted(
        ((int(record["address"], 16), record) for record in records),
        key=lambda pair: pair[0],
    )
    extents: list[tuple[int, int]] = []
    for index, (address, record) in enumerate(ordered):
        size = record.get("size")
        if isinstance(size, int) and size > 0:
            end = address + size
        elif index + 1 < len(ordered):
            end = ordered[index + 1][0]
        else:
            end = limit
        end = min(end, limit)
        if end > address:
            extents.append((address, end))
    return merge_intervals(extents)


def complement(
    start: int, end: int, covered: list[tuple[int, int]]
) -> list[tuple[int, int]]:
    """The ranges of a span that no covered range reaches."""
    clipped = merge_intervals(
        [(max(start, left), min(end, right)) for left, right in covered]
    )
    gaps: list[tuple[int, int]] = []
    cursor = start
    for left, right in clipped:
        if left > cursor:
            gaps.append((cursor, left))
        cursor = max(cursor, right)
    if cursor < end:
        gaps.append((cursor, end))
    return gaps


def seed_unmodeled(
    objects: list[le.Object], extents: list[tuple[int, int]]
) -> list[dict]:
    """The ranges that no module rebuilds: the code gaps and the data objects."""
    ranges: list[dict] = []
    for obj in objects:
        start = obj.reloc_base
        end = obj.reloc_base + obj.virtual_size
        if end <= start:
            continue
        if obj.flags & le.O_EXECUTABLE:
            for left, right in complement(start, end, extents):
                ranges.append(
                    {"start": f"0x{left:x}", "end": f"0x{right:x}", "kind": "code"}
                )
        else:
            ranges.append(
                {"start": f"0x{start:x}", "end": f"0x{end:x}", "kind": "data"}
            )
    ranges.sort(key=lambda entry: (int(entry["start"], 16), int(entry["end"], 16)))
    return ranges


def module_records() -> list[dict]:
    """Every record that names a module recipe."""
    records = []
    for address in store.candidate_addresses():
        record = store.load_record(address)
        name = record.get("module")
        if not name:
            continue
        try:
            store.load_module(name)
        except ValueError:
            continue
        records.append(record)
    return records


def seed_plan(original: Path) -> dict:
    """Derive the link plan from the shipped objects and the record extents."""
    image = le.load(original)
    records = module_records()
    executable = [obj for obj in image.objects if obj.flags & le.O_EXECUTABLE]
    limit = max(
        (obj.reloc_base + obj.virtual_size for obj in executable), default=0
    )
    order = derive_order(records)
    order += [name for name in RUNTIME_MODULES if name not in order]
    return {
        "version": 1,
        "toolchain": dosker.TOOLCHAIN_VOLUME,
        "engine": "qemu",
        "exe": IMAGE_EXE,
        "map": IMAGE_MAP,
        "link_options": list(LINK_OPTIONS),
        "runtime_libs": list(RUNTIME_LIBS),
        "order": order,
        "unmodeled": seed_unmodeled(image.objects, record_extents(records, limit)),
    }


def harness_source(module: str, file: str) -> bool:
    """True when the source is the stand-alone test harness of a module."""
    return Path(file).stem == f"{module}_main"


def implementation_sources(plan: dict) -> list[dict]:
    """The non-harness sources of the link order, each file once."""
    seen: set[str] = set()
    sources: list[dict] = []
    for name in plan["order"]:
        module = store.load_module(name)
        for source in module["sources"]:
            file = source["file"]
            if harness_source(name, file) or file in seen:
                continue
            seen.add(file)
            sources.append(source)
    return sources


def staged_source_name(index: int, source: dict) -> str:
    """The 8.3 staging name of one implementation source."""
    suffix = ".ASM" if source["compiler"] == "wasm" else ".C"
    return f"S{index:03X}{suffix}"


def staged_object_name(index: int) -> str:
    """The 8.3 object name of one implementation source."""
    return f"S{index:03X}.OBJ"


def image_name(name: str) -> str:
    """The 8.3 guest name of the output executable or map."""
    path = Path(name)
    return path.stem.upper()[:8] + path.suffix.upper()


def staged_names(plan: dict, sources: list[dict]) -> list[str]:
    names = [image_name(plan["exe"]), image_name(plan["map"])]
    names += [staged_source_name(index, source) for index, source in enumerate(sources)]
    names += [staged_object_name(index) for index in range(len(sources))]
    names += list(SHARED_FILES) + list(SHARED_OBJECTS)
    names += ["BUILD.BAT", "LINK.RSP"]
    return names


def validate_staged_names(plan: dict, sources: list[dict]) -> None:
    for name in staged_names(plan, sources):
        if not store.is_dos_83(name):
            raise ValueError(f"the staged name {name!r} does not fit 8.3")


def build_batch(plan: dict, sources: list[dict]) -> str:
    """The DOS batch that compiles every implementation and links the image.

    Every command appends to `BUILD.LOG`.  The agent returns the whole batch
    output as one control line and does not interpret `>` on the exec command
    line, so the batch itself must keep the output short.  The tool copies the
    log back when the batch fails.
    """
    log = f" >> C:\\WORK\\{BUILD_LOG}"
    lines = [
        "@ECHO OFF",
        f"SET WATCOM={harness.DOSKER_TOOLCHAIN_ROOT}",
        "SET DOS4GPATH=%WATCOM%\\BINW",
        "SET INCLUDE=%WATCOM%\\H;%WATCOM%\\H\\SYS",
        "SET PATH=%WATCOM%\\BINW;C:\\DOS",
        "IF NOT EXIST C:\\TMP\\NUL MD C:\\TMP >NUL",
        "SET TMP=C:\\TMP",
        "SET TEMP=C:\\TMP",
        "CD C:\\WORK",
        f"ECHO BUILD > C:\\WORK\\{BUILD_LOG}",
        "WCC386 -bt=dos -s -zq -fo=C:\\WORK\\CSTART.OBJ C:\\WORK\\CSTART.C" + log,
        "WASM -bt=dos -zq -zcm=watcom -fo=C:\\WORK\\CHK.OBJ C:\\WORK\\CHK.ASM" + log,
        "WASM -bt=dos -zq -zcm=watcom -fo=C:\\WORK\\STACK.OBJ C:\\WORK\\STACK.ASM" + log,
    ]
    for index, source in enumerate(sources):
        flags = list(harness.BASE_FLAGS[source["compiler"]]) + list(
            source.get("flags", [])
        )
        lines.append(
            f"{source['compiler']} {' '.join(flags)} "
            f"-fo=C:\\WORK\\{staged_object_name(index)} "
            f"C:\\WORK\\{staged_source_name(index, source)}" + log
        )
    lines.append("WLINK @C:\\WORK\\LINK.RSP" + log)
    return "\r\n".join(lines) + "\r\n"


def link_response(plan: dict, sources: list[dict]) -> str:
    """The wlink response file that links the whole image in plan order."""
    lines = list(plan["link_options"])
    lines.append(f"option map=C:\\WORK\\{image_name(plan['map'])}")
    lines.append(f"name C:\\WORK\\{image_name(plan['exe'])}")
    files = [
        f"C:\\WORK\\{staged_object_name(index)}"
        for index in range(len(sources))
    ]
    files += [f"C:\\WORK\\{name}" for name in SHARED_OBJECTS]
    files += list(plan.get("runtime_libs", []))
    lines.append("file " + ",".join(files))
    return "\r\n".join(lines) + "\r\n"


def build_command() -> str:
    """The guest command that runs the batch.

    The batch redirects every command to `BUILD.LOG` itself, because the agent
    does not interpret `>` on the exec command line and returns the batch
    output as one control line.
    """
    return "C:\\WORK\\BUILD.BAT"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def print_build_log(client: dosker.DoskerClient) -> None:
    """Copy the guest build log back and print it, when the guest wrote one."""
    target = WORK_DIR / BUILD_LOG
    try:
        client.copy_out(CONTAINER, f"C:\\WORK\\{BUILD_LOG}", target)
    except dosker.DoskerError:
        return
    print(f"--- {target} ---")
    print(target.read_text(errors="replace").rstrip())


def command_seed(arguments: argparse.Namespace) -> int:
    plan = seed_plan(arguments.original)
    if arguments.check:
        try:
            current = load_plan(arguments.plan)
        except LinkPlanMissing:
            print(
                f"error: the link plan {arguments.plan} is missing; "
                "run image_plan.py seed",
                file=sys.stderr,
            )
            return 1
        if current != plan:
            print(
                f"error: the link plan {arguments.plan} differs from the seed",
                file=sys.stderr,
            )
            return 1
        print(
            f"the link plan is current: {len(plan['order'])} modules, "
            f"{len(plan['unmodeled'])} unmodeled ranges"
        )
        return 0
    _write_plan(arguments.plan, plan)
    print(
        f"wrote {arguments.plan}: {len(plan['order'])} modules, "
        f"{len(plan['unmodeled'])} unmodeled ranges"
    )
    return 0


def command_build(arguments: argparse.Namespace) -> int:
    plan = load_plan(arguments.plan)
    sources = implementation_sources(plan)
    validate_staged_names(plan, sources)
    command = build_command()
    if len(command) > 127:
        raise ValueError(f"the guest command {command!r} exceeds 127 characters")
    if arguments.check:
        print(
            f"the plan links {len(plan['order'])} modules as "
            f"{len(sources)} unique sources"
        )
        print("the staging names pass the 8.3 rule")
        print(f"the guest command {command} fits 127 characters")
        return 0
    client = dosker.DoskerClient(arguments.dosker, arguments.data_root, WORK_DIR)
    client.check_version()
    volume = arguments.toolchain_volume or plan.get("toolchain") or dosker.TOOLCHAIN_VOLUME
    if not client.volume_exists(volume):
        raise dosker.DoskerMissing(
            f"the toolchain volume {volume} is missing; "
            "run tools/rebuild/dosker_setup.py"
        )
    staging = Path(arguments.stage)
    shutil.rmtree(staging, ignore_errors=True)
    staging.mkdir(parents=True)
    for name in harness.SHARED_SOURCES:
        shutil.copy2(REBUILD_DIR / name, staging / name.upper())
    for index, source in enumerate(sources):
        shutil.copy2(
            REBUILD_DIR / source["file"],
            staging / staged_source_name(index, source),
        )
    (staging / "BUILD.BAT").write_bytes(build_batch(plan, sources).encode())
    (staging / "LINK.RSP").write_bytes(link_response(plan, sources).encode())
    WORK_DIR.mkdir(parents=True, exist_ok=True)
    try:
        client.start(
            dosker.DEBUG_IMAGE,
            CONTAINER,
            volumes=[dosker.Volume(volume, harness.DOSKER_TOOLCHAIN_DRIVE)],
            engine=plan.get("engine") or None,
        )
        client.exec(CONTAINER, "MD C:\\WORK", timeout=10)
        for host in sorted(staging.iterdir()):
            client.copy_in(CONTAINER, host, f"C:\\WORK\\{host.name}")
        result = client.exec(
            CONTAINER,
            command,
            workdir="C:\\WORK",
            timeout=arguments.timeout,
        )
        if result.exit_code != 0:
            sys.stdout.write(result.output)
            print_build_log(client)
            print("error: the whole-image link failed", file=sys.stderr)
            return 1
        artifacts = [
            (image_name(plan["exe"]), plan["exe"]),
            (image_name(plan["map"]), plan["map"]),
        ]
        try:
            for guest, host in artifacts:
                client.copy_out(CONTAINER, f"C:\\WORK\\{guest}", WORK_DIR / host)
        except dosker.DoskerError:
            print_build_log(client)
            raise
    finally:
        client.remove(CONTAINER)
    for _, host in artifacts:
        target = WORK_DIR / host
        print(f"built {target}: sha256 {sha256(target)}")
    return 0


def command_coverage(arguments: argparse.Namespace) -> int:
    plan = load_plan(arguments.plan)
    if not Path(arguments.rebuilt).is_file():
        print(
            f"error: the rebuilt image {arguments.rebuilt} is missing",
            file=sys.stderr,
        )
        return 1
    report = image_compare.coverage_images(
        arguments.original,
        arguments.rebuilt,
        image_compare.plan_unmodeled(plan),
    )
    out = Path(arguments.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report.as_dict(), indent=2) + "\n")
    image_compare.print_coverage(report)
    print(f"wrote {out}")
    return 0 if report.verdict == "identical" else 1


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)
    seed_parser = subparsers.add_parser(
        "seed", help="derive the link plan from the records and the image"
    )
    seed_parser.add_argument(
        "--original", type=Path, default=store.DEFAULT_ORIGINAL
    )
    seed_parser.add_argument("--plan", type=Path, default=PLAN_PATH)
    seed_parser.add_argument(
        "--check", action="store_true", help="compare the plan with the seed"
    )
    build_parser = subparsers.add_parser(
        "build", help="compile the link order and link the whole image"
    )
    build_parser.add_argument("--plan", type=Path, default=PLAN_PATH)
    build_parser.add_argument(
        "--check", action="store_true", help="validate the plan and the command"
    )
    build_parser.add_argument("--dosker", default="dosker")
    build_parser.add_argument(
        "--data-root",
        type=Path,
        default=REPOSITORY_ROOT / "work/dosker/root",
    )
    build_parser.add_argument("--toolchain-volume", default=None)
    build_parser.add_argument("--stage", type=Path, default=STAGE_DIR)
    build_parser.add_argument("--timeout", type=int, default=STEP_TIMEOUT)
    coverage_parser = subparsers.add_parser(
        "coverage", help="print and write the whole-image coverage report"
    )
    coverage_parser.add_argument(
        "--original", type=Path, default=store.DEFAULT_ORIGINAL
    )
    coverage_parser.add_argument("--plan", type=Path, default=PLAN_PATH)
    coverage_parser.add_argument(
        "--rebuilt", type=Path, default=WORK_DIR / IMAGE_EXE
    )
    coverage_parser.add_argument("--out", type=Path, default=COVERAGE_PATH)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "seed":
            return command_seed(arguments)
        if arguments.command == "build":
            return command_build(arguments)
        return command_coverage(arguments)
    except LinkPlanMissing as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    except dosker.DoskerMissing as error:
        print(f"skip: {error}")
        return 3
    except dosker.DoskerError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    except (ValueError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
