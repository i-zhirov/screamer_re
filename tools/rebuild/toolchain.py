"""Record and apply the compiler toolchain of the rebuild module recipes.

A module recipe holds an optional `toolchain` value.  The value is `v2` for the
pinned OpenWatcom V2 harness and `watcom10` for the period Watcom 10.0a
harness.  An absent value means `v2`, so the migration writes the default into
every recipe that has no field.

The matrix tries a set of compiler flags for each selected toolchain.  It
builds every candidate, runs the strict identity check, and ranks the measured
results.  A measured module records its best measured toolchain and flags.  A
module with no identity record stays unmeasured and keeps its recipe.

Usage:
    python3 tools/rebuild/toolchain.py migrate [--check]
    python3 tools/rebuild/toolchain.py matrix (--module NAME | --all)
        [--toolchain NAME] [--manifest PATH] [--only-missing]
        [--resume PATH] [--original PATH] [--timeout SECONDS]
        [--dry-run] [--out PATH]
    python3 tools/rebuild/toolchain.py report
    python3 tools/rebuild/toolchain.py manifest [--report PATH] [--out PATH]
    python3 tools/rebuild/toolchain.py check-matrix [--manifest PATH]

Exit code 0 on success, 1 when `migrate --check` finds a module without a
toolchain or `check-matrix` finds a difference, 2 on a bad input, and 3 when
every matrix candidate skips because the engine is missing.  A missing or a
broken manifest makes `check-matrix` exit 1 with a message that names the
path.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import struct
import sys
from collections.abc import Callable
from datetime import date
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import le  # noqa: E402
import run  # noqa: E402
import store  # noqa: E402

REPOSITORY_ROOT = store.REPOSITORY_ROOT
MATRIX_DOCUMENT = store.WORK_DIR / "toolchain-matrix.json"
MANIFEST_DOCUMENT = store.REBUILD_DIR / "toolchain.json"
CANDIDATE_DIR = store.WORK_DIR / "toolchain"
DEFAULT_FLAG_SETS = (
    ("-of+", ("-of+",)),
    ("-od", ("-od",)),
    ("-of+ -oi", ("-of+", "-oi")),
    ("-of+ -ot", ("-of+", "-ot")),
    ("empty", ()),
)
MATCH_RANK = {
    "exact-bytes": 0,
    "exact-relocations": 1,
    "same-shape": 2,
    "same-mnemonics": 3,
    "different": 4,
}
BUILD_FAIL_RANK = 5
ROW_KEYS = (
    "order",
    "toolchain",
    "label",
    "sources",
    "build",
    "verdict",
    "match",
    "strict",
    "detail",
    "bytes_equal",
    "bytes_total",
    "instructions_equal",
    "instructions_total",
)
REPORT_COUNTS = (
    "modules",
    "strict",
    "exact-bytes",
    "exact-relocations",
    "shape",
    "different",
    "no-result",
)
MATCH_BUCKET = {
    "exact-bytes": "exact-bytes",
    "exact-relocations": "exact-relocations",
    "same-shape": "shape",
    "same-mnemonics": "shape",
    "different": "different",
}
UNMEASURED_DETAIL = "no measured result"


def migrate_modules(modules: list[dict], check: bool = False) -> int:
    """Write the default toolchain into every module without the field.

    Returns the count of modules without the field.  With `check`, it writes
    nothing and returns the same count.
    """
    count = 0
    for module in modules:
        if "toolchain" in module:
            continue
        count += 1
        if check:
            continue
        module["toolchain"] = store.TOOLCHAIN
        store.save_module(module)
    return count


def candidate_sources(
    module: dict, flags: tuple[str, ...] | None
) -> list[dict]:
    """Return one candidate source item per source of the module.

    A `None` flag tuple keeps the source flags.  A flag tuple replaces the
    flags of every `wcc386` source.  A `wasm` source keeps its flags and its
    compiler.
    """
    sources = []
    for source in module.get("sources", []):
        if flags is not None and source.get("compiler") == "wcc386":
            source_flags = list(flags)
        else:
            source_flags = list(source.get("flags", []))
        sources.append(
            {
                "file": source["file"],
                "compiler": source["compiler"],
                "flags": source_flags,
            }
        )
    return sources


def _per_source(module: dict, flags: tuple[str, ...] | None) -> list[tuple]:
    return [
        (source["file"], tuple(source["flags"]))
        for source in candidate_sources(module, flags)
    ]


def candidate_sets(module: dict) -> list[tuple[str, tuple[str, ...] | None]]:
    """Return the flag sets to try, the existing set first.

    The first occurrence of a per-source `(file, flags)` list stays, so a
    module whose source already uses a default set does not try it twice.
    """
    seen = [_per_source(module, None)]
    sets: list[tuple[str, tuple[str, ...] | None]] = [("existing", None)]
    for label, flags in DEFAULT_FLAG_SETS:
        key = _per_source(module, flags)
        if key in seen:
            continue
        seen.append(key)
        sets.append((label, flags))
    return sets


def candidate_descriptors(module: dict, toolchain: str) -> list[dict]:
    """Return one descriptor per candidate set for one toolchain."""
    return [
        {
            "order": index,
            "toolchain": toolchain,
            "label": label,
            "sources": candidate_sources(module, flags),
        }
        for index, (label, flags) in enumerate(candidate_sets(module))
    ]


def write_recipe(module: dict, descriptor: dict, path: Path) -> None:
    """Write a candidate recipe from one descriptor."""
    recipe: dict = {
        "name": module["name"],
        "exe": module["exe"],
    }
    if "map" in module:
        recipe["map"] = module["map"]
    if "run" in module:
        recipe["run"] = module["run"]
    sources = []
    for item in descriptor["sources"]:
        source = {"file": item["file"], "compiler": item["compiler"]}
        flags = list(item.get("flags", []))
        if flags:
            source["flags"] = flags
        sources.append(source)
    recipe["sources"] = sources
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(recipe, indent=2, sort_keys=True) + "\n")


def identity_record(records: list[dict], module_name: str) -> dict | None:
    """The lowest-address record that names the module and checks identity."""
    candidates = [
        record
        for record in records
        if record.get("module") == module_name
        and "identity" in record.get("checks", [])
        and record.get("symbol")
    ]
    if not candidates:
        return None
    return min(candidates, key=lambda record: int(record["address"], 16))


def is_strict(row: dict) -> bool:
    """Tell whether the row match passes the strict identity gate."""
    return row.get("match") in run.STRICT_MATCHES


def _difference(row: dict, equal: str, total: str) -> int:
    left = row.get(equal)
    right = row.get(total)
    if left is None or right is None:
        return 0
    return right - left


def rank(row: dict) -> tuple[int, int, int, int]:
    """The rank key of a row: match, differing bytes, instructions, order.

    A failed or skipped build ranks after every identity result.  A missing
    count of a pair counts as zero differing units.
    """
    order = row.get("order", 0)
    if row.get("build") in ("fail", "skip"):
        return (BUILD_FAIL_RANK, 0, 0, order)
    match_rank = MATCH_RANK.get(row.get("match"), BUILD_FAIL_RANK)
    return (
        match_rank,
        _difference(row, "bytes_equal", "bytes_total"),
        _difference(row, "instructions_equal", "instructions_total"),
        order,
    )


def winner(rows: list[dict]) -> dict | None:
    """The best measured row, or None when no row has a measured match."""
    measured = [
        row
        for row in rows
        if row.get("build") == "pass" and row.get("match") is not None
    ]
    if not measured:
        return None
    return min(measured, key=rank)


def apply_winner(module: dict, winner: dict) -> dict:
    """Return a copy of the module with the winner toolchain and flags."""
    result = copy.deepcopy(module)
    result["toolchain"] = winner["toolchain"]
    flags_by_file = {
        item["file"]: item.get("flags", [])
        for item in winner.get("sources", [])
    }
    for source in result.get("sources", []):
        source.pop("period_flags", None)
        if source["file"] not in flags_by_file:
            continue
        flags = flags_by_file[source["file"]]
        if flags:
            source["flags"] = list(flags)
        else:
            source.pop("flags", None)
    return result


def clear_artifacts(module: dict) -> None:
    """Delete the build outputs a previous candidate may have left behind."""
    run.clear_artifacts(module)
    (store.WORK_DIR / "dos_build.log").unlink(missing_ok=True)


def _row(
    descriptor: dict,
    build: str,
    verdict: str = "",
    match: str | None = None,
    detail: str = "",
    counts: dict | None = None,
) -> dict:
    counts = counts or {}
    return {
        "order": descriptor["order"],
        "toolchain": descriptor["toolchain"],
        "label": descriptor["label"],
        "sources": descriptor["sources"],
        "build": build,
        "verdict": verdict,
        "match": match,
        "strict": match in run.STRICT_MATCHES,
        "detail": detail,
        "bytes_equal": counts.get("bytes_equal"),
        "bytes_total": counts.get("bytes_total"),
        "instructions_equal": counts.get("instructions_equal"),
        "instructions_total": counts.get("instructions_total"),
    }


def _normalize_row(row: dict) -> dict:
    return {key: row.get(key) for key in ROW_KEYS}


def _last_line(text: str) -> str:
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    return lines[-1] if lines else ""


def evaluate(
    module: dict,
    record: dict,
    descriptor: dict,
    original: Path,
    timeout: int | None = None,
) -> dict:
    """Build one candidate and return its report row."""
    clear_artifacts(module)
    CANDIDATE_DIR.mkdir(parents=True, exist_ok=True)
    recipe = CANDIDATE_DIR / f"{module['name']}-{descriptor['order']:02d}.json"
    write_recipe(module, descriptor, recipe)
    build, output = run.build_module(
        module, descriptor["toolchain"], recipe, timeout=timeout
    )
    if build != "pass":
        detail = _last_line(output) or run.BUILD_DETAIL.get(build, "")
        return _row(descriptor, build, verdict=build, detail=detail)
    verdict, check = run.run_identity(record, module, original, strict=True)
    return _row(
        descriptor,
        build,
        verdict=verdict,
        match=check.get("match"),
        detail=check.get("detail", ""),
        counts=check,
    )


def _descriptors(module: dict, toolchains: tuple[str, ...]) -> list[dict]:
    selected = [name for name in store.TOOLCHAINS if name in toolchains]
    descriptors: list[dict] = []
    for toolchain in selected:
        for descriptor in candidate_descriptors(module, toolchain):
            descriptors.append(dict(descriptor, order=len(descriptors)))
    return descriptors


def _module_item(module: dict, record: dict | None) -> dict:
    return {
        "name": module["name"],
        "measured": False,
        "record": record["address"] if record is not None else None,
        "skipped": False,
        "current": {
            "toolchain": store.module_toolchain(module),
            "sources": candidate_sources(module, None),
        },
        "winner": None,
        "rows": [],
    }


def _manifest_rows(manifest: dict | None) -> list[dict]:
    if not manifest:
        return []
    return manifest.get("modules", [])


def _row_for(manifest: dict | None, name: str) -> dict | None:
    for row in _manifest_rows(manifest):
        if row.get("name") == name:
            return row
    return None


def _recipe_sources(sources: list[dict]) -> list[dict]:
    return [
        {
            "file": source["file"],
            "compiler": source.get("compiler"),
            "flags": list(source.get("flags") or []),
        }
        for source in sources
    ]


def _row_matches_module(row: dict, module: dict) -> bool:
    if row.get("toolchain", store.TOOLCHAIN) != store.module_toolchain(module):
        return False
    return _recipe_sources(row.get("sources", [])) == _recipe_sources(
        module.get("sources", [])
    )


def matrix_module(
    module: dict,
    record: dict | None,
    original: Path,
    toolchains: tuple[str, ...] = store.TOOLCHAINS,
    timeout: int | None = None,
    only_missing: bool = False,
    base_manifest: dict | None = None,
    resume_manifest: dict | None = None,
    evaluate: Callable[..., dict] = evaluate,
) -> dict:
    """Measure the selected candidates of one module and return its item."""
    item = _module_item(module, record)
    if record is None:
        item["reason"] = "no identity record"
        return item
    name = module["name"]
    if only_missing:
        resume_row = _row_for(resume_manifest, name)
        if (
            resume_row is not None
            and _row_matches_module(resume_row, module)
            and resume_row.get("measured")
        ):
            item["skipped"] = True
            item["measured"] = True
            item["winner"] = _normalize_row(resume_row)
            item["reason"] = "already measured"
            return item
    rows: list[dict] = []
    base_row = _row_for(base_manifest, name)
    if base_row is not None and _row_matches_module(base_row, module):
        rows.append(_normalize_row(base_row))
        if base_row.get("strict"):
            item["rows"] = rows
            item["winner"] = winner(rows)
            item["measured"] = item["winner"] is not None
            if item["winner"] is None:
                item["reason"] = "no measured result"
            return item
    selected = [toolchain for toolchain in store.TOOLCHAINS if toolchain in toolchains]
    descriptors = _descriptors(module, toolchains)

    def measure(descriptor: dict) -> dict:
        clear_artifacts(module)
        return evaluate(
            module, record, descriptor, original, timeout=timeout
        )

    for descriptor in descriptors:
        if descriptor["toolchain"] != "v2":
            continue
        row = measure(descriptor)
        rows.append(row)
        if row.get("match") == "exact-bytes":
            item["winner"] = winner(rows)
            item["rows"] = rows
            item["measured"] = item["winner"] is not None
            if item["winner"] is None:
                item["reason"] = "no measured result"
            return item
    v2_strict = any(row.get("strict") for row in rows)
    if not v2_strict and "watcom10" in selected:
        for descriptor in descriptors:
            if descriptor["toolchain"] != "watcom10":
                continue
            row = measure(descriptor)
            rows.append(row)
            if row.get("match") == "exact-bytes":
                break
    item["rows"] = rows
    item["winner"] = winner(rows)
    item["measured"] = item["winner"] is not None
    if item["winner"] is None:
        item["reason"] = "no measured result"
    return item


def _sha256(path: Path) -> str | None:
    if not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65536), b""):
            digest.update(block)
    return digest.hexdigest()


def _report(toolchains: tuple[str, ...], original: Path) -> dict:
    return {
        "date": date.today().isoformat(),
        "original": {
            "path": str(original),
            "sha256": _sha256(original),
        },
        "toolchains": list(toolchains),
        "flag_sets": [label for label, _ in DEFAULT_FLAG_SETS],
        "modules": [],
    }


def matrix(
    modules: list[dict],
    records: list[dict],
    original: Path,
    module_name: str | None = None,
    toolchains: tuple[str, ...] = store.TOOLCHAINS,
    timeout: int | None = None,
    only_missing: bool = False,
    base_manifest: dict | None = None,
    resume_manifest: dict | None = None,
    on_module: Callable[[dict], None] | None = None,
    evaluate: Callable[..., dict] = evaluate,
) -> dict:
    """Measure the selected modules and return the report document."""
    report = _report(toolchains, original)
    selected = [
        module
        for module in modules
        if module_name is None or module.get("name") == module_name
    ]
    for module in selected:
        record = identity_record(records, module["name"])
        item = matrix_module(
            module,
            record,
            original,
            toolchains=toolchains,
            timeout=timeout,
            only_missing=only_missing,
            base_manifest=base_manifest,
            resume_manifest=resume_manifest,
            evaluate=evaluate,
        )
        report["modules"].append(item)
        if on_module is not None:
            on_module(item)
    return report


def _write_report(path: Path, report: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    temporary.replace(path)


def _read_manifest(path: Path | None) -> dict | None:
    if path is None or not path.is_file():
        return None
    try:
        return json.loads(path.read_text())
    except json.JSONDecodeError as error:
        raise ValueError(f"{path}: broken JSON: {error}") from error


def _ordered_toolchains(
    toolchains: list[str] | None,
) -> tuple[str, ...]:
    if not toolchains:
        return store.TOOLCHAINS
    return tuple(name for name in store.TOOLCHAINS if name in toolchains)


def _print_summary(report: dict, dry_run: bool) -> None:
    for item in report["modules"]:
        winner_row = item.get("winner")
        if winner_row is None:
            print(f"{item['name']}: {item.get('reason', 'no winner')}")
            continue
        prefix = "dry-run " if dry_run else ""
        print(
            f"{item['name']}: {prefix}winner {winner_row['toolchain']}"
            f" {winner_row['label']} {winner_row['match']}"
        )
    modules = report["modules"]
    measured = sum(1 for item in modules if item.get("measured"))
    strict = sum(
        1
        for item in modules
        if item.get("winner") is not None and item["winner"].get("strict")
    )
    print(
        f"{len(modules)} modules: {measured} measured,"
        f" {len(modules) - measured} unmeasured, {strict} strict"
    )


def command_matrix(arguments: argparse.Namespace) -> int:
    modules = [
        store.load_module_file(path)
        for path in sorted(store.MODULES_DIR.glob("*.json"))
    ]
    if arguments.module is not None:
        modules = [
            module
            for module in modules
            if module.get("name") == arguments.module
        ]
        if not modules:
            raise ValueError(f"the module {arguments.module} is missing")
    if not arguments.module and not arguments.all:
        raise ValueError("select --module NAME or --all")
    original = arguments.original
    if not original.is_file():
        raise ValueError(f"the original image {original} is missing")
    try:
        le.load(original)
    except (ValueError, struct.error) as error:
        raise ValueError(
            f"the original image {original} does not parse: {error}"
        ) from error
    for module in modules:
        if (
            not module.get("name")
            or not module.get("exe")
            or not module.get("sources")
        ):
            raise ValueError(
                f"the module {module.get('name')!r} lacks name, exe, or sources"
            )
    records = [
        store.load_record(address) for address in store.candidate_addresses()
    ]
    toolchains = _ordered_toolchains(arguments.toolchain)
    base_manifest = _read_manifest(arguments.manifest)
    resume_manifest = None
    if arguments.only_missing:
        resume_manifest = _read_manifest(arguments.resume)
    out = arguments.out
    report = _report(toolchains, original)

    def on_module(item: dict) -> None:
        report["modules"].append(item)
        _write_report(out, report)
        if arguments.dry_run:
            return
        winner_row = item.get("winner")
        if winner_row is None:
            return
        module = next(
            (
                module
                for module in modules
                if module.get("name") == item["name"]
            ),
            None,
        )
        if module is None:
            return
        store.save_module(apply_winner(module, winner_row))

    result = matrix(
        modules,
        records,
        original,
        toolchains=toolchains,
        timeout=arguments.timeout,
        only_missing=arguments.only_missing,
        base_manifest=base_manifest,
        resume_manifest=resume_manifest,
        on_module=on_module,
    )
    if not report["modules"]:
        report["modules"] = list(result.get("modules", []))
        _write_report(out, report)
    _print_summary(report, arguments.dry_run)
    rows = [
        row
        for item in report["modules"]
        for row in item.get("rows", [])
    ]
    if rows and all(row.get("build") == "skip" for row in rows):
        return 3
    return 0


def _load_modules() -> list[dict]:
    return [
        store.load_module_file(path)
        for path in sorted(store.MODULES_DIR.glob("*.json"))
    ]


def _load_records() -> list[dict]:
    return [store.load_record(address) for address in store.candidate_addresses()]


def _counts() -> dict:
    return {key: 0 for key in REPORT_COUNTS}


def _identity_result(
    module: dict, record: dict | None
) -> tuple[str, str | None]:
    if record is None:
        return store.module_toolchain(module), None
    result = record.get("result") or {}
    mode = result.get("mode") or {}
    toolchain = mode.get("toolchain") or store.module_toolchain(module)
    check = (result.get("checks") or {}).get("identity") or {}
    return toolchain, check.get("match")


def report_document(modules: list[dict], records: list[dict]) -> dict:
    """Group the modules by the toolchain of their identity record.

    A module with no identity record counts in `no-result` under the toolchain
    of its recipe.  The shape bucket holds the two shape matches.
    """
    rows: dict[str, dict] = {
        toolchain: {"toolchain": toolchain, **_counts()}
        for toolchain in store.TOOLCHAINS
    }
    total = _counts()
    for module in modules:
        record = identity_record(records, module["name"])
        toolchain, match = _identity_result(module, record)
        if toolchain not in rows:
            rows[toolchain] = {"toolchain": toolchain, **_counts()}
        bucket = MATCH_BUCKET.get(match, "no-result")
        rows[toolchain]["modules"] += 1
        rows[toolchain][bucket] += 1
        total["modules"] += 1
        total[bucket] += 1
    for row in rows.values():
        row["strict"] = row["exact-bytes"] + row["exact-relocations"]
    total["strict"] = total["exact-bytes"] + total["exact-relocations"]
    return {"toolchains": list(rows.values()), "total": total}


def format_report(document: dict) -> str:
    """Format the strict-delta report as a fixed table."""
    lines = [
        "toolchain modules strict exact-bytes exact-relocations shape "
        "different no-result"
    ]
    for item in document["toolchains"]:
        values = [item["toolchain"], *(item[key] for key in REPORT_COUNTS)]
        lines.append(" ".join(str(value) for value in values))
    total = ["total", *(document["total"][key] for key in REPORT_COUNTS)]
    lines.append(" ".join(str(value) for value in total))
    return "\n".join(lines)


def command_report(arguments: argparse.Namespace) -> int:
    modules = _load_modules()
    records = _load_records()
    print(format_report(report_document(modules, records)))
    return 0


def _manifest_measured_row(item: dict) -> dict:
    row = item["winner"]
    return {
        "name": item["name"],
        "order": row["order"],
        "toolchain": row["toolchain"],
        "label": row["label"],
        "sources": row["sources"],
        "measured": True,
        "build": row["build"],
        "verdict": row["verdict"],
        "match": row["match"],
        "strict": row["strict"],
        "detail": row["detail"],
        "bytes_equal": row.get("bytes_equal"),
        "bytes_total": row.get("bytes_total"),
        "instructions_equal": row.get("instructions_equal"),
        "instructions_total": row.get("instructions_total"),
    }


def _manifest_unmeasured_row(item: dict) -> dict:
    current = item["current"]
    return {
        "name": item["name"],
        "order": -1,
        "toolchain": current["toolchain"],
        "label": "",
        "sources": current["sources"],
        "measured": False,
        "build": "skip",
        "verdict": "skip",
        "match": None,
        "strict": False,
        "detail": UNMEASURED_DETAIL,
        "bytes_equal": None,
        "bytes_total": None,
        "instructions_equal": None,
        "instructions_total": None,
    }


def manifest_document(
    report: dict, report_path: Path, report_digest: str
) -> dict:
    """Build the committed manifest from the full matrix report.

    A measured module takes the winner recipe.  A module with an identity
    record and no measured result takes the current recipe and the unmeasured
    shape.  A module with no identity record has no row.
    """
    rows = []
    for item in report.get("modules", []):
        if item.get("record") is None:
            continue
        if item.get("measured") and item.get("winner") is not None:
            rows.append(_manifest_measured_row(item))
        else:
            rows.append(_manifest_unmeasured_row(item))
    return {
        "date": date.today().isoformat(),
        "report": {"path": str(report_path), "sha256": report_digest},
        "modules": rows,
    }


def _read_document(path: Path) -> dict:
    """Read a JSON object from an explicit path."""
    try:
        text = path.read_text()
    except FileNotFoundError:
        raise ValueError(f"{path}: the file is missing") from None
    except OSError as error:
        raise ValueError(f"{path}: {error}") from error
    try:
        document = json.loads(text)
    except json.JSONDecodeError as error:
        raise ValueError(f"{path}: broken JSON: {error}") from error
    if not isinstance(document, dict):
        raise ValueError(f"{path}: the document is not a JSON object")
    return document


def command_manifest(arguments: argparse.Namespace) -> int:
    report_path = arguments.report
    report = _read_document(report_path)
    document = manifest_document(report, report_path, _sha256(report_path))
    _write_report(arguments.out, document)
    print(f"{len(document['modules'])} rows")
    return 0


def _source_messages(row: dict, module: dict, path: Path) -> list[str]:
    messages: list[str] = []
    name = module["name"]
    module_sources = {
        source["file"]: source for source in module.get("sources", [])
    }
    for source in row.get("sources", []):
        file = source.get("file")
        module_source = module_sources.get(file)
        if module_source is None:
            continue
        if source.get("compiler") != module_source.get("compiler"):
            messages.append(
                f"{path}: the module {name} source {file} has the compiler "
                f"{module_source.get('compiler')} and the manifest holds "
                f"{source.get('compiler')}"
            )
        row_flags = list(source.get("flags") or [])
        module_flags = list(module_source.get("flags") or [])
        if row_flags != module_flags:
            messages.append(
                f"{path}: the module {name} source {file} has the flags "
                f"{module_flags} and the manifest holds {row_flags}"
            )
    return messages


def _row_messages(row: dict, module: dict, path: Path) -> list[str]:
    messages: list[str] = []
    name = module["name"]
    expected = store.module_toolchain(module)
    actual = row.get("toolchain", store.TOOLCHAIN)
    if actual != expected:
        messages.append(
            f"{path}: the module {name} has the toolchain {expected} and "
            f"the manifest holds {actual}"
        )
    messages.extend(_source_messages(row, module, path))
    return messages


def check_matrix(
    modules: list[dict],
    records: list[dict],
    manifest: dict,
    path: Path,
) -> list[str]:
    """Return one message per difference between the manifest and the recipes.

    A row for a module without an identity record is a difference.  A module
    with an identity record and no row is a difference.  An absent source flags
    field equals an empty list.
    """
    messages: list[str] = []
    row_by_name: dict[str, dict] = {}
    for row in manifest.get("modules", []):
        name = row.get("name")
        if name is not None:
            row_by_name[name] = row
    module_by_name = {module["name"]: module for module in modules}
    for name, row in row_by_name.items():
        if name not in module_by_name:
            messages.append(f"{path}: the row {name} has no module")
            continue
        if identity_record(records, name) is None:
            messages.append(f"{path}: the row {name} has no identity record")
            continue
        messages.extend(_row_messages(row, module_by_name[name], path))
    for module in modules:
        name = module["name"]
        if name in row_by_name:
            continue
        if identity_record(records, name) is not None:
            messages.append(
                f"{path}: the module {name} has an identity record and no row"
            )
    return messages


def command_check_matrix(arguments: argparse.Namespace) -> int:
    modules = _load_modules()
    records = _load_records()
    try:
        manifest = _read_document(arguments.manifest)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    messages = check_matrix(modules, records, manifest, arguments.manifest)
    for message in messages:
        print(message)
    if not messages:
        print(
            f"{arguments.manifest}: the toolchain matrix matches the recipes"
        )
    return 1 if messages else 0


def command_migrate(arguments: argparse.Namespace) -> int:
    modules = [
        store.load_module_file(path)
        for path in sorted(store.MODULES_DIR.glob("*.json"))
    ]
    count = migrate_modules(modules, arguments.check)
    print(f"migrated {count} modules")
    return 1 if arguments.check and count else 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Record and apply the compiler toolchain of the recipes."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    migrate_parser = subparsers.add_parser(
        "migrate", help="write the default toolchain into every recipe"
    )
    migrate_parser.add_argument(
        "--check",
        action="store_true",
        help="write nothing and exit 1 when a recipe has no toolchain",
    )

    matrix_parser = subparsers.add_parser(
        "matrix", help="measure the toolchain and flag candidates of a module"
    )
    selection = matrix_parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--module", help="the module record name")
    selection.add_argument(
        "--all",
        action="store_true",
        help="every module",
    )
    matrix_parser.add_argument(
        "--toolchain",
        action="append",
        choices=store.TOOLCHAINS,
        default=None,
        help="restrict the candidates to a toolchain; repeat the flag",
    )
    matrix_parser.add_argument(
        "--manifest",
        type=Path,
        default=None,
        help="the base manifest of a period pass",
    )
    matrix_parser.add_argument(
        "--only-missing",
        action="store_true",
        help="skip a module the resume manifest already measured",
    )
    matrix_parser.add_argument(
        "--resume",
        type=Path,
        default=None,
        help="the manifest of the current pass to resume",
    )
    matrix_parser.add_argument(
        "--original",
        type=Path,
        default=store.DEFAULT_ORIGINAL,
        help="the original LE executable",
    )
    matrix_parser.add_argument(
        "--timeout",
        type=int,
        default=None,
        help="the period build timeout in seconds",
    )
    matrix_parser.add_argument(
        "--dry-run",
        action="store_true",
        help="write the report and no module recipe",
    )
    matrix_parser.add_argument(
        "--out",
        type=Path,
        default=MATRIX_DOCUMENT,
        help="the matrix report path",
    )

    subparsers.add_parser("report", help="print the strict-delta report")

    manifest_parser = subparsers.add_parser(
        "manifest", help="write the committed manifest of the measured modules"
    )
    manifest_parser.add_argument(
        "--report",
        type=Path,
        default=MATRIX_DOCUMENT,
        help="the full matrix report",
    )
    manifest_parser.add_argument(
        "--out",
        type=Path,
        default=MANIFEST_DOCUMENT,
        help="the committed manifest path",
    )

    check_parser = subparsers.add_parser(
        "check-matrix", help="check the committed manifest against the recipes"
    )
    check_parser.add_argument(
        "--manifest",
        type=Path,
        default=MANIFEST_DOCUMENT,
        help="the committed manifest path",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "matrix":
            return command_matrix(arguments)
        if arguments.command == "report":
            return command_report(arguments)
        if arguments.command == "manifest":
            return command_manifest(arguments)
        if arguments.command == "check-matrix":
            return command_check_matrix(arguments)
        return command_migrate(arguments)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
