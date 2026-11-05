"""Hold the rebuild store: the records, the validation, the seed, and the edits.

The store holds one function record per candidate of STARTH.EXE under
`functions/0x<address>.json` and one module record per build unit under
`modules/<name>.json`.  This tool reads and writes a record, validates every
invariant of the data model, seeds the candidate records from the original image,
and edits one record from the command line.  A mutation that changes the set of
record addresses recomputes every extent before the process ends.

Usage:
    python3 tools/rebuild/store.py seed [--check] [--original PATH]
    python3 tools/rebuild/store.py set --function ADDR [--name NAME] [--role TEXT]
        [--status STATUS] [--module NAME] [--symbol SYM] [--section NAME]
        [--confidence C] [--check build|identity|behavior]
        [--evidence KIND TEXT] [--note TEXT]
    python3 tools/rebuild/store.py note --function ADDR --text TEXT [--date DATE]
    python3 tools/rebuild/store.py list [--status STATUS] [--module NAME] [--json]
    python3 tools/rebuild/store.py check

Exit code 0 on success, 1 on a violation or a seed difference, and 2 on a
missing record, a missing original image, or a bad argument.
"""
from __future__ import annotations

import argparse
import atexit
import hashlib
import json
import re
import subprocess
import struct
import sys
from datetime import date
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import le  # noqa: E402
import schema  # noqa: E402

REBUILD_DIR = Path(__file__).resolve().parent
FUNCTIONS_DIR = REBUILD_DIR / "functions"
MODULES_DIR = REBUILD_DIR / "modules"
CONTRACTS_DIR = REBUILD_DIR / "contracts"
REPOSITORY_ROOT = REBUILD_DIR.parent.parent
WORK_DIR = REPOSITORY_ROOT / "work/rebuild"
TOOLS_DIR = REPOSITORY_ROOT / "tools"
SHARED_SOURCES = ("chk.asm", "stack.asm", "cstart.c")
TOOL_FILES = (
    "rebuild/run.py",
    "rebuild/harness.py",
    "rebuild/diff_module.py",
    "rebuild/oracle.py",
    "rebuild/store.py",
    "le.py",
)
MISSING = b"<missing>"
UNCHANGED_DETAIL = "the inputs are unchanged since the previous run"
STATUS_LADDER = (
    "unknown",
    "not_a_function",
    "named",
    "rebuilt",
    "matching",
    "verified",
)
CHECKS = ("build", "identity", "behavior")
EVIDENCE_KINDS = ("command", "finding", "file", "note")
RUN_STATUSES = ("rebuilt", "matching", "verified")
LOAD_BASE = 0x1C2000
ENTRY_ADDRESS = 0x300F8
EXIT_FLUSH_ADDRESS = 0x300D6
SEED_LIMIT = 10000
DEFAULT_ORIGINAL = (
    Path.home() / "Documents/Screamer.app/Contents/Resources/game/STARTH.EXE"
)
LE_TOOL = Path(__file__).resolve().parent.parent / "le.py"
FINDINGS_DOCUMENT = Path("docs/explanation/findings.md")
ADDRESS_PATTERN = re.compile(r"^0x[0-9a-f]+$")
_extents_dirty = False


def record_path(address: int) -> Path:
    return FUNCTIONS_DIR / f"0x{address:x}.json"


def module_path(name: str) -> Path:
    return MODULES_DIR / f"{name}.json"


def _read_json(path: Path) -> dict:
    try:
        text = path.read_text()
    except FileNotFoundError:
        raise ValueError(f"{path}: the file is missing") from None
    except OSError as error:
        raise ValueError(f"{path}: {error}") from error
    try:
        return json.loads(text)
    except json.JSONDecodeError as error:
        raise ValueError(f"{path}: broken JSON: {error}") from error


def _write_json(path: Path, record: dict) -> None:
    """Write a record through a temporary file and a replace.

    A failed write leaves the previous file in place.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n")
    temporary.replace(path)


def load_record(address: int) -> dict:
    return _read_json(record_path(address))


def save_record(record: dict) -> None:
    global _extents_dirty
    address = int(record["address"], 16)
    path = record_path(address)
    new_address = not path.exists()
    _write_json(path, record)
    if new_address:
        _extents_dirty = True


def remove_record(address: int) -> None:
    global _extents_dirty
    record_path(address).unlink(missing_ok=True)
    _extents_dirty = True


def sync_extents() -> int:
    """Recompute the extents when a mutation changed the record addresses.

    `save_record` marks a new address and `remove_record` marks a deletion.  The
    seed, the command dispatch, and the exit hook call this function, so a later
    migration cannot leave a stale reach behind.
    """
    global _extents_dirty
    if not _extents_dirty:
        return 0
    _extents_dirty = False
    return recompute_sizes()


def save_result(address: int, result: dict, status: str | None) -> None:
    record = load_record(address)
    record["result"] = result
    if status is not None:
        record["status"] = status
    save_record(record)


def load_module(name: str) -> dict:
    return _read_json(module_path(name))


def save_module_result(name: str, result: dict, status: str | None) -> None:
    record = load_module(name)
    record["result"] = result
    if status is not None:
        record["status"] = status
    _write_json(module_path(name), record)


def candidate_addresses() -> list[int]:
    if not FUNCTIONS_DIR.is_dir():
        return []
    addresses = []
    for path in FUNCTIONS_DIR.glob("0x*.json"):
        if not ADDRESS_PATTERN.match(path.stem):
            continue
        addresses.append(int(path.stem, 16))
    return sorted(addresses)


PADDING_BYTES = (0x00, 0x90, 0xCC)


def _jump_span(image: le.LinearExecutable, address: int, data: bytes) -> bool:
    """Tell whether `address` sits inside a jump or a call instruction.

    A data table can hold a jump opcode just before a real function entry, so a
    relative jump counts only when its target is mapped.  The indirect form
    `jmp/call dword ptr [abs]` of a thunk counts without a target.
    """
    base = address - 5
    for step in range(1, 6):
        position = 5 - step
        if position < 0:
            break
        opcode = data[position]
        if opcode in (0xE9, 0xE8):
            length = 5
        elif opcode == 0xEB:
            length = 2
        elif opcode == 0xFF and data[position + 1] in (0x15, 0x25):
            length = 6
        else:
            continue
        if step >= length:
            continue
        if opcode == 0xFF:
            return True
        if opcode == 0xEB:
            relative = struct.unpack_from("<b", data, position + 1)[0]
            target = base + position + 2 + relative
        else:
            relative = struct.unpack_from("<i", data, position + 1)[0]
            target = base + position + length + relative
        if image.object_at(target) is not None:
            return True
    return False


def non_entry_addresses(image: le.LinearExecutable) -> list[tuple[int, str]]:
    """The record addresses that are not a function entry, with the reason.

    Three forms are caught: a stale candidate at the `push ebp` of a prologue
    that saves registers first, an address inside a run of alignment padding, and
    an address inside a jump or call instruction such as a thunk.
    """
    offenders: list[tuple[int, str]] = []
    for address in candidate_addresses():
        data = image.read(address - 5, 12)
        if data is None or len(data) != 12:
            continue
        if data[5:8] == b"\x55\x89\xe5" and le.prologue_start(data[:8], 5) != 5:
            offenders.append((address, "sits inside a prologue"))
            continue
        if data[5] in PADDING_BYTES and (
            data[4] in PADDING_BYTES or data[6] in PADDING_BYTES
        ):
            offenders.append((address, "sits in alignment padding"))
            continue
        if _jump_span(image, address, data):
            offenders.append((address, "sits inside a jump"))
    return offenders


def advance_status(checks: list[str], verdicts: dict[str, str]) -> str | None:
    """Return the status the declared checks support, or None for no write.

    The rules are exclusive in this order.
    """
    declared = list(checks)
    if not declared:
        return None
    if any(verdicts.get(name) == "skip" for name in declared):
        return None
    if "build" in declared:
        if verdicts.get("build") != "pass":
            return None
        if "identity" not in declared:
            return "rebuilt"
    if "identity" in declared:
        if verdicts.get("identity") != "pass":
            return "rebuilt"
        if "behavior" in declared and verdicts.get("behavior") != "pass":
            return "matching"
        return "verified"
    if verdicts.get("behavior") == "pass":
        return "verified"
    return None


def supported_status(record: dict) -> str | None:
    result = record.get("result")
    if not result:
        return None
    verdicts = {
        name: check.get("verdict")
        for name, check in result.get("checks", {}).items()
    }
    return advance_status(record.get("checks", []), verdicts)


def is_stale(record: dict, fingerprint: str) -> bool:
    """Tell whether the recorded result no longer matches the current inputs.

    The fingerprint covers the full input set of the declared checks: the module
    sources, the shared harness sources, the recipe fields, the record fields,
    the participating tools, the original image, and the built artifacts.
    """
    result = record.get("result")
    if not result:
        return False
    return result.get("fingerprint") != fingerprint


def source_is_stale(record: dict, digest: str) -> bool:
    """Tell whether the recorded result no longer matches the module sources.

    The status page uses the module-source digest, because it needs no game
    image and no built artifact.  The runner uses the full fingerprint instead.
    """
    result = record.get("result")
    if not result:
        return False
    return result.get("source_digest") != digest


def source_digest(module: dict, root: Path | None = None) -> str:
    base = Path(root) if root is not None else REBUILD_DIR
    digest = hashlib.sha256()
    for source in sorted(module.get("sources", []), key=lambda entry: entry["file"]):
        name = source["file"]
        digest.update(name.encode("utf-8"))
        digest.update(b"\x00")
        digest.update((base / name).read_bytes())
    return f"sha256:{digest.hexdigest()}"


def _file_bytes(path: Path) -> bytes:
    """The file content, or a fixed marker for a missing file.

    A missing original image or a missing built artifact is a changed input, so
    the fingerprint of a record that once hashed the file differs and the record
    is stale.  A missing tool or source file makes the marker part of the
    fingerprint too, so the record is stale rather than an error.
    """
    try:
        return path.read_bytes()
    except OSError:
        return MISSING


def _record_fields(record: dict) -> bytes:
    """The canonical bytes of the record fields that steer the declared checks."""
    fields = {
        key: record[key]
        for key in ("address", "symbol", "count", "checks", "module")
        if key in record
    }
    return json.dumps(fields, sort_keys=True, separators=(",", ":")).encode("utf-8")


def _recipe_fields(module: dict) -> bytes:
    """The canonical bytes of the module recipe, without the run outcome."""
    recipe = {
        key: value
        for key, value in module.items()
        if key not in ("status", "result")
    }
    return json.dumps(recipe, sort_keys=True, separators=(",", ":")).encode("utf-8")


def compute_fingerprint(
    record: dict,
    module: dict,
    *,
    rebuild_dir: Path | None = None,
    work_dir: Path | None = None,
    tools_dir: Path | None = None,
    original: Path | None = None,
) -> str:
    """The SHA-256 over every input of the declared checks of one record.

    The inputs are, in order: the module recipe fields, the module sources by
    file name, the shared harness sources, the record fields that steer the
    checks, the participating tool sources, the original image, and the built
    artifacts.  Every file is hashed by content, and a missing file contributes
    a fixed marker, so a deleted artifact or a moved game makes the record stale
    instead of raising.
    """
    base = Path(rebuild_dir) if rebuild_dir is not None else REBUILD_DIR
    work = Path(work_dir) if work_dir is not None else WORK_DIR
    tools = Path(tools_dir) if tools_dir is not None else TOOLS_DIR
    original_path = Path(original) if original is not None else DEFAULT_ORIGINAL
    digest = hashlib.sha256()

    def feed(label: str, data: bytes) -> None:
        digest.update(label.encode("utf-8"))
        digest.update(b"\x00")
        digest.update(data)

    feed("recipe", _recipe_fields(module))
    for source in sorted(module.get("sources", []), key=lambda entry: entry["file"]):
        name = source["file"]
        feed(f"source:{name}", _file_bytes(base / name))
    for name in SHARED_SOURCES:
        feed(f"shared:{name}", _file_bytes(base / name))
    feed("record", _record_fields(record))
    for name in TOOL_FILES:
        feed(f"tool:{name}", _file_bytes(tools / name))
    feed("original", _file_bytes(original_path))
    feed(f"exe:{module['exe']}", _file_bytes(work / module["exe"]))
    if "map" in module:
        feed(f"map:{module['map']}", _file_bytes(work / module["map"]))
    return f"sha256:{digest.hexdigest()}"


def _finding_headings(root: Path) -> set[str]:
    document = root / FINDINGS_DOCUMENT
    if not document.is_file():
        return set()
    headings = set()
    for line in document.read_text(errors="replace").splitlines():
        if not line.startswith("#"):
            continue
        headings.add(line.strip())
        headings.add(line.lstrip("#").strip())
    return headings


def _evidence_violations(path: Path, record: dict, root: Path) -> list[str]:
    violations = []
    for entry in record.get("evidence", []):
        kind = entry.get("kind")
        text = entry.get("text", "")
        if kind == "finding" and text not in _finding_headings(root):
            violations.append(
                f"{path}: the finding {text!r} is not a heading of findings.md"
            )
        if kind == "file" and not (root / text).exists():
            violations.append(
                f"{path}: the file evidence {text!r} does not exist"
            )
    return violations


def _status_violations(
    path: Path, record: dict, status: object, result: dict | None
) -> list[str]:
    """The rules that compare the stored status with the last result."""
    violations = []
    if status not in STATUS_LADDER:
        violations.append(
            f"{path}: the status {status!r} is not a status ladder value"
        )
        return violations
    checks = record.get("checks", [])
    if status in ("named", "matching", "verified") and not record.get("name"):
        violations.append(f"{path}: the status {status} requires a name")
    if status in RUN_STATUSES and result is None:
        violations.append(f"{path}: the status {status} requires a result")
    if result is None:
        return violations
    check_results = result.get("checks", {})
    for check in checks:
        if check not in check_results:
            violations.append(f"{path}: the check {check} has no check result")
    if status == "verified" and "identity" in checks:
        identity = check_results.get("identity")
        if identity is None or identity.get("verdict") != "pass":
            violations.append(
                f"{path}: the status verified requires a passing identity check"
            )
    supported = supported_status(record)
    if (
        status in RUN_STATUSES
        and supported is not None
        and STATUS_LADDER.index(status) > STATUS_LADDER.index(supported)
    ):
        violations.append(
            f"{path}: the status {status} is stronger than the result "
            f"supports ({supported})"
        )
    return violations


def _schema_violations(path: Path, record: dict, name: str) -> list[str]:
    """Return the JSON Schema violations of one record, or no messages.

    The contracts stay under `contracts/`.  A test store without the contracts
    has no schema to validate, so the semantic checks alone apply.
    """
    schema_path = REBUILD_DIR / "contracts" / name
    if not schema_path.is_file():
        return []
    document = json.loads(schema_path.read_text())
    return [
        f"{path}: schema: {message}"
        for message in schema.validate(
            record, document, base_dir=schema_path.parent
        )
    ]


def validate_record(
    path: Path, record: dict | None = None, root: Path | None = None
) -> list[str]:
    """Return one message per broken invariant of a function record."""
    base = Path(root) if root is not None else REPOSITORY_ROOT
    if record is None:
        try:
            record = json.loads(path.read_text())
        except FileNotFoundError:
            return [f"{path}: the record file is missing"]
        except OSError as error:
            return [f"{path}: {error}"]
        except json.JSONDecodeError as error:
            return [f"{path}: broken JSON: {error}"]
    if not isinstance(record, dict):
        return [f"{path}: the record is not a JSON object"]
    violations: list[str] = []
    if path.stem != record.get("address"):
        violations.append(
            f"{path}: the file name does not equal the address "
            f"{record.get('address')!r}"
        )
    status = record.get("status")
    result = record.get("result")
    checks = record.get("checks", [])
    violations.extend(_status_violations(path, record, status, result))
    for check in checks:
        if check not in CHECKS:
            violations.append(f"{path}: the check {check!r} is not a check name")
    module = record.get("module")
    if module is not None:
        if not module_path(module).is_file():
            violations.append(
                f"{path}: the module {module!r} has no module record"
            )
        if not record.get("symbol"):
            violations.append(f"{path}: the module {module!r} requires a symbol")
    if "identity" in checks:
        if module is None:
            violations.append(f"{path}: the check identity requires a module")
        elif module_path(module).is_file():
            try:
                module_record = load_module(module)
            except ValueError:
                module_record = None
            if module_record is not None and "map" not in module_record:
                violations.append(
                    f"{path}: the module {module!r} has no map for the "
                    f"identity check"
                )
    violations.extend(_evidence_violations(path, record, base))
    return violations


def validate_module(
    path: Path, record: dict | None = None, root: Path | None = None
) -> list[str]:
    """Return one message per broken invariant of a module record."""
    if record is None:
        try:
            record = json.loads(path.read_text())
        except FileNotFoundError:
            return [f"{path}: the module file is missing"]
        except OSError as error:
            return [f"{path}: {error}"]
        except json.JSONDecodeError as error:
            return [f"{path}: broken JSON: {error}"]
    if not isinstance(record, dict):
        return [f"{path}: the record is not a JSON object"]
    violations: list[str] = []
    if path.stem != record.get("name"):
        violations.append(
            f"{path}: the file name does not equal the name "
            f"{record.get('name')!r}"
        )
    status = record.get("status")
    if status is not None:
        violations.extend(
            _status_violations(path, record, status, record.get("result"))
        )
    for check in record.get("checks", []):
        if check not in CHECKS:
            violations.append(f"{path}: the check {check!r} is not a check name")
    return violations


def scan_candidates(original: Path) -> list[int]:
    """Run the prologue scan of le.py and add the run-time starts.

    The prologue scan misses a run-time function without an EBP frame: the entry
    holds a jump, and the exit helper starts with a register save.
    """
    completed = subprocess.run(
        [
            sys.executable,
            str(LE_TOOL),
            "funcs",
            str(original),
            "--limit",
            str(SEED_LIMIT),
        ],
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise ValueError(
            f"the scan of {original} failed: {completed.stderr.strip()}"
        )
    candidates = set()
    for line in completed.stdout.splitlines():
        line = line.strip()
        if line.startswith("0x"):
            candidates.add(int(line, 16))
    candidates.add(ENTRY_ADDRESS)
    candidates.add(EXIT_FLUSH_ADDRESS)
    return sorted(candidates)


def seed_diff(
    candidates: list[int], records: dict[int, dict]
) -> tuple[list[int], list[int]]:
    """Return the added addresses and the lost addresses.

    A lost address is an `unknown` record outside the candidates.
    """
    known = set(candidates)
    added = sorted(address for address in known if address not in records)
    lost = sorted(
        address
        for address, record in records.items()
        if record.get("status") == "unknown" and address not in known
    )
    return added, lost


def seed(check: bool, original: Path) -> int:
    if not original.is_file():
        print(f"error: the original image {original} is missing", file=sys.stderr)
        return 2
    candidates = scan_candidates(original)
    addresses = candidate_addresses()
    records = {address: load_record(address) for address in addresses}
    added, lost = seed_diff(candidates, records)
    overlaps = overlap_addresses(list(records.values()))
    print(f"added: {len(added)}")
    print(f"lost: {len(lost)}")
    print(f"overlaps: {len(overlaps)}")
    if check:
        for address, extra in overlaps:
            print(
                f"{record_path(address)}: the size reaches {extra} bytes "
                "past the next entry"
            )
        if added or lost or overlaps:
            return 1
        print("no difference")
        return 0
    merged = sorted(set(candidates) | set(addresses))
    following = dict(zip(merged, merged[1:]))
    image = le.load(original)
    for address in added:
        record: dict = {
            "address": f"0x{address:x}",
            "status": "unknown",
            "checks": [],
            "evidence": [],
            "notes": [],
        }
        obj = image.object_at(address)
        if obj is not None:
            record["object_offset"] = f"0x{address - obj.reloc_base:x}"
        if address in following:
            record["size"] = following[address] - address
        save_record(record)
    for address in lost:
        remove_record(address)
    sync_extents()
    return 0


def command_set(arguments: argparse.Namespace) -> int:
    record = load_record(arguments.function)
    if arguments.clear_evidence:
        record["evidence"] = []
    if arguments.name is not None:
        record["name"] = arguments.name
    if arguments.role is not None:
        record["role"] = arguments.role
    if arguments.status is not None:
        record["status"] = arguments.status
    if arguments.module is not None:
        record["module"] = arguments.module
    if arguments.symbol is not None:
        record["symbol"] = arguments.symbol
    if arguments.section is not None:
        record["section"] = arguments.section
    if arguments.confidence is not None:
        record["confidence"] = arguments.confidence
    if arguments.implementation is not None:
        record["implementation"] = arguments.implementation
    for check in arguments.check:
        checks = record.setdefault("checks", [])
        if check not in checks:
            checks.append(check)
    for kind, text in arguments.evidence:
        if kind not in EVIDENCE_KINDS:
            raise ValueError(
                f"the evidence kind {kind!r} is not one of "
                f"{', '.join(EVIDENCE_KINDS)}"
            )
        record.setdefault("evidence", []).append({"kind": kind, "text": text})
    for text in arguments.note:
        record.setdefault("notes", []).append(
            {"date": date.today().isoformat(), "text": text}
        )
    save_record(record)
    print(f"updated {record_path(arguments.function)}")
    return 0


def command_note(arguments: argparse.Namespace) -> int:
    record = load_record(arguments.function)
    record.setdefault("notes", []).append(
        {"date": arguments.date, "text": arguments.text}
    )
    save_record(record)
    print(f"updated {record_path(arguments.function)}")
    return 0


def command_list(arguments: argparse.Namespace) -> int:
    records = []
    for address in candidate_addresses():
        record = load_record(address)
        if arguments.status is not None and record.get("status") != arguments.status:
            continue
        if arguments.module is not None and record.get("module") != arguments.module:
            continue
        records.append(record)
    if MODULES_DIR.is_dir():
        for path in sorted(MODULES_DIR.glob("*.json")):
            record = _read_json(path)
            if arguments.status is not None and record.get("status") != arguments.status:
                continue
            if arguments.module is not None and record.get("name") != arguments.module:
                continue
            records.append(record)
    if arguments.json:
        print(json.dumps(records, indent=2, sort_keys=True))
        return 0
    for record in records:
        if "address" in record:
            name = record.get("name", "(unnamed)")
            print(f"{record['address']} {name} {record.get('status')}")
        else:
            print(f"{record.get('name')} (module) {record.get('status')}")
    return 0


def overlap_addresses(records: list[dict]) -> list[tuple[int, int]]:
    """The records whose `size` reaches past the next entry.

    Returns the address and the number of bytes past the next entry.  The `size`
    is the distance to the next entry of the moment the record was seeded, so a
    moved entry leaves a stale reach.
    """
    ordered = sorted(records, key=lambda record: int(record["address"], 16))
    offenders: list[tuple[int, int]] = []
    for record, following in zip(ordered, ordered[1:]):
        address = int(record["address"], 16)
        size = record.get("size")
        if size is None:
            continue
        next_address = int(following["address"], 16)
        if address + size > next_address:
            offenders.append((address, address + size - next_address))
    return offenders


def recompute_sizes() -> int:
    """Rewrite `size` as the distance to the next entry and return the count.

    The seed writes the field only for a new record, so an entry that moves
    leaves the survivors with a stale reach.  The last record holds no next
    entry, so its field is removed.
    """
    addresses = candidate_addresses()
    updated = 0
    for index, address in enumerate(addresses):
        record = load_record(address)
        if index + 1 < len(addresses):
            size = addresses[index + 1] - address
            if record.get("size") == size:
                continue
            record["size"] = size
        elif "size" in record:
            del record["size"]
        else:
            continue
        save_record(record)
        updated += 1
    return updated


def command_check(original: Path | None) -> int:
    violations: list[str] = []
    count = 0
    records: list[dict] = []
    for address in candidate_addresses():
        count += 1
        path = record_path(address)
        violations.extend(validate_record(path))
        record = load_record(address)
        records.append(record)
        violations.extend(
            _schema_violations(path, record, "record.schema.json")
        )
    for address, extra in overlap_addresses(records):
        violations.append(
            f"{record_path(address)}: the size reaches {extra} bytes past the next entry"
        )
    if MODULES_DIR.is_dir():
        for path in sorted(MODULES_DIR.glob("*.json")):
            count += 1
            violations.extend(validate_module(path))
            try:
                module_record = load_module(path.stem)
            except ValueError:
                continue
            violations.extend(
                _schema_violations(path, module_record, "module.schema.json")
            )
    if original is not None:
        if not original.is_file():
            print(f"error: the original image {original} is missing", file=sys.stderr)
            return 2
        for address, reason in non_entry_addresses(le.load(original)):
            violations.append(f"{record_path(address)}: the address {reason}")
    for violation in violations:
        print(violation)
    print(f"{count} records, {len(violations)} violations")
    return 1 if violations else 0


def _address(value: str) -> int:
    try:
        return int(value, 0)
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"not a hexadecimal address: {value}"
        ) from None


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Read, write, and validate the rebuild store records."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    seed_parser = subparsers.add_parser(
        "seed", help="write a record for each new candidate"
    )
    seed_parser.add_argument(
        "--check",
        action="store_true",
        help="write nothing and exit 1 on a difference",
    )
    seed_parser.add_argument(
        "--original",
        type=Path,
        default=DEFAULT_ORIGINAL,
        help="the original LE executable",
    )

    set_parser = subparsers.add_parser("set", help="edit one record")
    set_parser.add_argument(
        "--function",
        type=_address,
        required=True,
        help="the module linear address, in hex",
    )
    set_parser.add_argument("--name", help="the recovered name")
    set_parser.add_argument("--role", help="one sentence")
    set_parser.add_argument(
        "--status", choices=STATUS_LADDER, help="the status ladder value"
    )
    set_parser.add_argument("--module", help="the module record name")
    set_parser.add_argument("--symbol", help="the linker map symbol")
    set_parser.add_argument("--section", help="a section name of program-map.md")
    set_parser.add_argument(
        "--confidence",
        choices=("observed", "derived", "inferred"),
        help="the confidence label",
    )
    set_parser.add_argument(
        "--implementation",
        choices=("c", "assembly"),
        help="the source language of the rebuilt function",
    )
    set_parser.add_argument(
        "--check",
        action="append",
        default=[],
        choices=CHECKS,
        help="declare a check; repeatable",
    )
    set_parser.add_argument(
        "--clear-evidence",
        action="store_true",
        help="drop the current evidence entries before the additions",
    )
    set_parser.add_argument(
        "--evidence",
        nargs=2,
        action="append",
        default=[],
        metavar=("KIND", "TEXT"),
        help="add an evidence entry; repeatable",
    )
    set_parser.add_argument(
        "--note", action="append", default=[], help="append a note; repeatable"
    )

    note_parser = subparsers.add_parser("note", help="append a note to one record")
    note_parser.add_argument(
        "--function",
        type=_address,
        required=True,
        help="the module linear address, in hex",
    )
    note_parser.add_argument("--text", required=True, help="the note text")
    note_parser.add_argument(
        "--date", default=date.today().isoformat(), help="the note date"
    )

    list_parser = subparsers.add_parser("list", help="list the store records")
    list_parser.add_argument(
        "--status", choices=STATUS_LADDER, help="keep one status"
    )
    list_parser.add_argument("--module", help="keep one module")
    list_parser.add_argument("--json", action="store_true", help="print JSON")

    subparsers.add_parser(
        "size", help="rewrite the size of every record as the distance to the next entry"
    )
    check_parser = subparsers.add_parser(
        "check", help="prove every invariant of the model"
    )
    check_parser.add_argument(
        "--original",
        type=Path,
        help="the original LE executable; adds the prologue-entry check",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "seed":
            return seed(arguments.check, arguments.original)
        if arguments.command == "set":
            return command_set(arguments)
        if arguments.command == "note":
            return command_note(arguments)
        if arguments.command == "list":
            return command_list(arguments)
        if arguments.command == "size":
            print(f"updated {recompute_sizes()} records")
            return 0
        return command_check(arguments.original)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    finally:
        sync_extents()


atexit.register(sync_extents)


if __name__ == "__main__":
    raise SystemExit(main())
