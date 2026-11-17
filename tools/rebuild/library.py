"""Index and verify the period Watcom libraries with a direct OMF parser.

The tool reads the OMF library pages, walks the records of every module, and
writes one index document.  The index holds the library digest, the page and
dictionary metadata, one entry per public symbol, and the object and prefix
digests of each public extent.  The library bytes stay out of the index.

The matcher tries the record symbol, the record name with a trailing
underscore, the record name, and the entries whose prefix digest matches the
original image, in that order.  The prefix digest zeroes every fixup slot in
the first 16 bytes.  The query also zeroes the LE relocation bytes and the
object fixup positions of each entry, so a link-time address cannot hide a
candidate.  The full
comparison confirms the candidate: the object extent and the original extent
compare through the oracle, with every import and entry slot rewritten to the
neutral import kind.  The byte mask covers the object slot positions on both
sides, because the linked image resolves a library import to an internal target
and often drops the relocation at a call inside the image.  A separate slot
geometry check fails the result when an original slot has no object slot at the
same offset and width, while an object-only slot is allowed.  The library gate
is strict.  Only `exact-bytes` and `exact-relocations` pass.  A `same-shape`,
`same-mnemonics`, or `different` result fails.

The `library` field of a record holds exactly `file`, `sha256`, `module`,
`symbol`, and `object_sha256`.  The `sha256` value is the library digest, and
the `object_sha256` value is the digest of the public extent with every fixup
slot zeroed.

The apply pass writes a strict library result into a record.  It sets the
record name from the library public symbol, the `library` field, the identity
check, the `verified` status, the command evidence, the result, the source
digest, and the input fingerprint.  The record is saved through
`store.save_record`, the only record writer.  A record without a candidate
stays unchanged, and a candidate that does not pass the strict gate writes
nothing.

The manifest pass writes the link manifest: the digest of each index library
and one five-key entry per record that holds a `library` field.  The check
proves that the manifest agrees with the records: every library record has one
entry, every entry has a record, the five keys and the library digests agree,
two entries that name the same object hold the same five keys, and each
library record is `verified`.

Usage:
    python3 tools/rebuild/library.py index [--library-dir DIR] [--out PATH]
    python3 tools/rebuild/library.py verify --address ADDR [--address ADDR ...]
        [--original PATH] [--index PATH] [--library-dir DIR]
    python3 tools/rebuild/library.py apply [--address ADDR ...] [--original PATH]
        [--index PATH] [--library-dir DIR]
    python3 tools/rebuild/library.py manifest [--index PATH] [--out PATH]
    python3 tools/rebuild/library.py check-manifest [--manifest PATH]

Exit code 0 on success, 1 when a target has no strict pass or the manifest has
a message, and 2 on a bad input.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
from dataclasses import dataclass, field
from datetime import date
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import le  # noqa: E402
import oracle  # noqa: E402
import store  # noqa: E402

DEFAULT_LIBRARY_DIR = (
    store.REPOSITORY_ROOT / "work/watcom10/WATCOM/LIB386/DOS"
)
LIBRARIES = ("CLIB3R.LIB", "MATH387R.LIB", "EMU387.LIB")
INDEX_DOCUMENT = store.WORK_DIR / "library-index.json"
LINK_DOCUMENT = store.REBUILD_DIR / "library.json"

OMF_HEADER = 3
LIBRARY_HEADER_RECORD = 0xF0
THEADR = 0x80
MODEND = (0x8A, 0x8B)
LNAMES = (0x96, 0xCA)
SEGDEF = {0x98: 2, 0x99: 4}
PUBDEF = {0x90: 2, 0x91: 4}
LEDATA = {0xA0: 2, 0xA1: 4}
LIDATA = (0xA2, 0xA3)
FIXUPP = (0x9C, 0x9D)
FIXUP_WIDTHS = {
    0: 1,
    1: 2,
    2: 2,
    3: 4,
    4: 1,
    5: 4,
    6: 6,
    9: 4,
    11: 6,
    13: 4,
}
PREFIX_BYTES = 16
STRICT_MATCHES = oracle.STRICT_MATCHES
DICTIONARY_BLOCK_SIZE = 512

LIBRARY_KEYS = ("file", "sha256", "module", "symbol", "object_sha256")

ENTRY_KEYS = (
    "file",
    "sha256",
    "module",
    "module_offset",
    "symbol",
    "segment",
    "offset",
    "size",
    "object_sha256",
    "prefix_sha256",
    "fixups",
)


@dataclass
class OmfRecord:
    command: int
    offset: int
    length: int
    payload: bytes


@dataclass
class OmfPublic:
    name: str
    segment: int
    offset: int
    size: int = 0


@dataclass
class OmfFixup:
    segment: int
    offset: int
    width: int
    location: int
    target: str


@dataclass
class OmfModule:
    name: str
    offset: int
    length: int
    segments: dict[int, dict] = field(default_factory=dict)
    publics: list[OmfPublic] = field(default_factory=list)
    fixups: list[OmfFixup] = field(default_factory=list)


def _sha256(data: bytes) -> str:
    return "sha256:" + hashlib.sha256(data).hexdigest()


def _round_up(value: int, page_length: int) -> int:
    return ((value + page_length - 1) // page_length) * page_length


def _read_index(data: bytes, position: int, path: Path) -> tuple[int, int]:
    if position >= len(data):
        raise ValueError(f"{path}: the record payload is truncated")
    first = data[position]
    if first & 0x80:
        if position + 1 >= len(data):
            raise ValueError(f"{path}: the record payload is truncated")
        return ((first & 0x7F) << 8) | data[position + 1], position + 2
    return first, position + 1


def _read_name(data: bytes, position: int, path: Path) -> tuple[str, int]:
    if position >= len(data):
        raise ValueError(f"{path}: the record payload is truncated")
    count = data[position]
    end = position + 1 + count
    if end > len(data):
        raise ValueError(f"{path}: the record payload is truncated")
    return data[position + 1 : end].decode("ascii", "replace"), end


def _read_record(
    data: bytes, position: int, limit: int, path: Path
) -> tuple[OmfRecord, int]:
    if position + OMF_HEADER > limit:
        raise ValueError(f"{path}: the record at {position:#x} is truncated")
    command = data[position]
    length = int.from_bytes(data[position + 1 : position + 3], "little")
    end = position + OMF_HEADER + length
    if end > limit:
        raise ValueError(f"{path}: the record at {position:#x} is truncated")
    payload = data[position + OMF_HEADER : end]
    if length:
        payload = payload[:-1]
    return OmfRecord(command, position, length, payload), end


def _target_name(method: int) -> str:
    if method in (2, 6):
        return "import"
    if method == 3:
        return "entry"
    return "internal"


def _read_block(
    payload: bytes, position: int, repeat_width: int, path: Path
) -> tuple[bytes, int]:
    end = position + repeat_width + 2
    if end > len(payload):
        raise ValueError(f"{path}: the record payload is truncated")
    repeat = int.from_bytes(payload[position : position + repeat_width], "little")
    position += repeat_width
    block_count = int.from_bytes(payload[position : position + 2], "little")
    position += 2
    if block_count == 0:
        if position >= len(payload):
            raise ValueError(f"{path}: the record payload is truncated")
        count = payload[position]
        position += 1
        data = payload[position : position + count]
        position += count
        if position > len(payload):
            raise ValueError(f"{path}: the record payload is truncated")
    else:
        content = bytearray()
        for _ in range(block_count):
            block, position = _read_block(
                payload, position, repeat_width, path
            )
            content.extend(block)
        data = bytes(content)
    return data * repeat, position


def _parse_segdef(
    payload: bytes, path: Path, preferred: int
) -> tuple[int, int, int, int]:
    candidates = [preferred]
    other = 4 if preferred == 2 else 2
    if other not in candidates:
        candidates.append(other)
    for width in candidates:
        if len(payload) < 1 + width:
            continue
        try:
            position = 1 + width
            name_index, position = _read_index(payload, position, path)
            class_index, position = _read_index(payload, position, path)
            _, position = _read_index(payload, position, path)
        except ValueError:
            continue
        if position == len(payload):
            size = int.from_bytes(payload[1 : 1 + width], "little")
            return width, size, name_index, class_index
    raise ValueError(f"{path}: the segment definition is malformed")


def _parse_pubdef(
    payload: bytes, path: Path, preferred: int
) -> tuple[int, list[tuple[str, int, int]]]:
    candidates = [preferred]
    other = 4 if preferred == 2 else 2
    if other not in candidates:
        candidates.append(other)
    for width in candidates:
        try:
            index = 0
            _, index = _read_index(payload, index, path)
            base_segment, index = _read_index(payload, index, path)
            if base_segment == 0:
                index += 2
            entries = []
            while index < len(payload):
                name, index = _read_name(payload, index, path)
                if index + width > len(payload):
                    raise ValueError(f"{path}: the record payload is truncated")
                offset = int.from_bytes(
                    payload[index : index + width], "little"
                )
                index += width
                _, index = _read_index(payload, index, path)
                entries.append((name, base_segment, offset))
            if index == len(payload):
                return width, entries
        except ValueError:
            continue
    raise ValueError(f"{path}: the public definition is malformed")


def _store_data(
    module: OmfModule, segment_index: int, offset: int, data: bytes
) -> None:
    segment = module.segments.get(segment_index)
    if segment is None:
        segment = {
            "index": segment_index,
            "name": "",
            "class": "",
            "size": offset + len(data),
            "data": bytearray(offset + len(data)),
        }
        module.segments[segment_index] = segment
    needed = offset + len(data)
    if len(segment["data"]) < needed:
        segment["data"].extend(bytes(needed - len(segment["data"])))
    segment["data"][offset:needed] = data
    segment["size"] = max(segment["size"], needed)


def _parse_fixupp(
    command: int,
    payload: bytes,
    module: OmfModule,
    last_data: tuple[int, int] | None,
    pharlap: bool,
    path: Path,
) -> None:
    frame_threads: dict[int, int] = {}
    target_threads: dict[int, int] = {}
    position = 0
    while position < len(payload):
        first = payload[position]
        if first & 0x80:
            if position + 2 > len(payload):
                raise ValueError(f"{path}: the record payload is truncated")
            locat = (payload[position] << 8) | payload[position + 1]
            location = (locat >> 10) & 0x0F
            data_offset = locat & 0x3FF
            position += 2
            if position >= len(payload):
                raise ValueError(f"{path}: the record payload is truncated")
            fixdat = payload[position]
            position += 1
            frame_thread = (fixdat >> 7) & 1
            frame_method = (fixdat >> 4) & 0x07
            target_thread = (fixdat >> 3) & 1
            p_bit = (fixdat >> 2) & 1
            targt = fixdat & 0x03
            if not frame_thread and frame_method in (0, 1, 2):
                _, position = _read_index(payload, position, path)
            if target_thread:
                method = target_threads.get(targt, 0)
            else:
                method = targt
                if method in (0, 1, 2):
                    _, position = _read_index(payload, position, path)
            if not p_bit:
                width = 4 if pharlap or command == 0x9D else 2
                position += width
                if position > len(payload):
                    raise ValueError(f"{path}: the record payload is truncated")
            if location not in FIXUP_WIDTHS:
                slot = data_offset
                if last_data is not None:
                    slot = last_data[1] + data_offset
                raise ValueError(
                    f"{path}: the module {module.name} has an unknown fixup "
                    f"location {location} at slot {slot:#x}"
                )
            if last_data is None:
                raise ValueError(
                    f"{path}: the module {module.name} has a fixup without a "
                    f"data record"
                )
            segment, base = last_data
            module.fixups.append(
                OmfFixup(
                    segment=segment,
                    offset=base + data_offset,
                    width=FIXUP_WIDTHS[location],
                    location=location,
                    target=_target_name(method),
                )
            )
        else:
            position += 1
            frame_thread = (first >> 6) & 1
            method = (first >> 2) & 0x07
            thread = first & 0x03
            if method in (0, 1, 2):
                _, position = _read_index(payload, position, path)
            if frame_thread:
                frame_threads[thread] = method
            else:
                target_threads[thread] = method


def _parse_module(
    data: bytes, start: int, limit: int, path: Path
) -> tuple[OmfModule, int]:
    module = OmfModule(name="", offset=start, length=0)
    names: list[str] = []
    last_data: tuple[int, int] | None = None
    pharlap = False
    position = start
    record_end = start
    while position < limit:
        record, end = _read_record(data, position, limit, path)
        command = record.command
        payload = record.payload
        if command == THEADR:
            module.name, _ = _read_name(payload, 0, path)
        elif command == 0x88:
            if len(payload) >= 2 and payload[1] == 0xAA:
                pharlap = True
        elif command in LNAMES:
            index = 0
            while index < len(payload):
                name, index = _read_name(payload, index, path)
                names.append(name)
        elif command in SEGDEF:
            preferred = 4 if pharlap else SEGDEF[command]
            _, size, name_index, class_index = _parse_segdef(
                payload, path, preferred
            )
            name = names[name_index - 1] if name_index else ""
            class_name = names[class_index - 1] if class_index else ""
            segment_index = len(module.segments) + 1
            module.segments[segment_index] = {
                "index": segment_index,
                "name": name,
                "class": class_name,
                "size": size,
                "data": bytearray(size),
            }
        elif command in PUBDEF:
            preferred = 4 if pharlap else PUBDEF[command]
            _, entries = _parse_pubdef(payload, path, preferred)
            for name, base_segment, offset in entries:
                module.publics.append(
                    OmfPublic(name=name, segment=base_segment, offset=offset)
                )
        elif command in LEDATA:
            width = 4 if pharlap else LEDATA[command]
            index = 0
            segment_index, index = _read_index(payload, index, path)
            offset = int.from_bytes(payload[index : index + width], "little")
            index += width
            chunk = payload[index:]
            _store_data(module, segment_index, offset, chunk)
            last_data = (segment_index, offset)
        elif command in LIDATA:
            width = 4 if pharlap or command == 0xA3 else 2
            index = 0
            segment_index, index = _read_index(payload, index, path)
            offset = int.from_bytes(payload[index : index + width], "little")
            index += width
            repeat_width = 2 if pharlap or command == 0xA2 else 4
            chunk = bytearray()
            while index < len(payload):
                block, index = _read_block(payload, index, repeat_width, path)
                chunk.extend(block)
            _store_data(module, segment_index, offset, bytes(chunk))
            last_data = (segment_index, offset)
        elif command in FIXUPP:
            _parse_fixupp(command, payload, module, last_data, pharlap, path)
        if command in MODEND:
            record_end = end
            break
        position = end
    else:
        raise ValueError(
            f"{path}: the module at {start:#x} has no MODEND record"
        )
    module.length = record_end - start
    _size_publics(module)
    return module, record_end


def _size_publics(module: OmfModule) -> None:
    ordered = sorted(
        module.publics, key=lambda public: (public.segment, public.offset)
    )
    for index, public in enumerate(ordered):
        next_offset = None
        for later in ordered[index + 1 :]:
            if later.segment == public.segment and later.offset > public.offset:
                next_offset = later.offset
                break
        segment = module.segments.get(public.segment)
        segment_size = segment["size"] if segment is not None else public.offset
        if next_offset is not None:
            public.size = max(next_offset - public.offset, 0)
        else:
            public.size = max(segment_size - public.offset, 0)


def _module_document(module: OmfModule) -> dict:
    segments = []
    for index in sorted(module.segments):
        segment = module.segments[index]
        segments.append(
            {
                "index": segment["index"],
                "name": segment["name"],
                "class": segment["class"],
                "size": segment["size"],
                "data": bytes(segment["data"]),
            }
        )
    return {
        "name": module.name,
        "offset": module.offset,
        "length": module.length,
        "segments": segments,
        "publics": [
            {
                "name": public.name,
                "segment": public.segment,
                "offset": public.offset,
                "size": public.size,
            }
            for public in module.publics
        ],
        "fixups": [
            {
                "segment": fixup.segment,
                "offset": fixup.offset,
                "width": fixup.width,
                "location": fixup.location,
                "target": fixup.target,
            }
            for fixup in module.fixups
        ],
    }


def read_library(path: Path) -> dict:
    """Read one library and return its document."""
    data = path.read_bytes()
    if len(data) < 9:
        raise ValueError(f"{path}: the library header is truncated")
    if data[0] != LIBRARY_HEADER_RECORD:
        raise ValueError(
            f"{path}: the first byte is not {LIBRARY_HEADER_RECORD:#x}"
        )
    length = int.from_bytes(data[1:3], "little")
    page_length = length + OMF_HEADER
    dictionary_offset = int.from_bytes(data[3:7], "little")
    dictionary_blocks = int.from_bytes(data[7:9], "little")
    if page_length < 9:
        raise ValueError(f"{path}: the page length is too small")
    limit = dictionary_offset if dictionary_offset else len(data)
    if limit > len(data):
        raise ValueError(f"{path}: the dictionary offset is past the file end")
    modules = []
    position = page_length
    while position < limit:
        if data[position] == THEADR:
            module, end = _parse_module(data, position, limit, path)
            modules.append(_module_document(module))
            next_position = _round_up(end, page_length)
            position = next_position if next_position > position else end
        else:
            position = _round_up(position + 1, page_length)
    if dictionary_offset:
        dictionary_size = min(
            dictionary_blocks * DICTIONARY_BLOCK_SIZE,
            len(data) - dictionary_offset,
        )
        dictionary = data[dictionary_offset : dictionary_offset + dictionary_size]
    else:
        dictionary_size = 0
        dictionary = b""
    return {
        "file": path.name,
        "sha256": _sha256(data),
        "size": len(data),
        "page_length": page_length,
        "dictionary": {
            "offset": dictionary_offset,
            "size": dictionary_size,
            "sha256": _sha256(dictionary),
        },
        "modules": modules,
    }


def _entry_digests(
    segment: dict, public: dict, fixups: list[dict]
) -> tuple[str, str, list[dict]]:
    offset = public["offset"]
    size = public["size"]
    extent = bytes(segment["data"][offset : offset + size])
    relevant = [
        fixup
        for fixup in fixups
        if fixup["segment"] == public["segment"]
        and offset <= fixup["offset"] < offset + size
    ]
    object_bytes = bytearray(extent)
    for fixup in relevant:
        start = fixup["offset"] - offset
        end = min(start + fixup["width"], len(object_bytes))
        object_bytes[start:end] = bytes(max(end - start, 0))
    prefix_size = min(PREFIX_BYTES, size)
    prefix = bytearray(extent[:prefix_size])
    for fixup in relevant:
        start = fixup["offset"] - offset
        if start >= prefix_size:
            continue
        end = min(start + fixup["width"], prefix_size)
        prefix[start:end] = bytes(max(end - start, 0))
    entry_fixups = [
        {
            "offset": fixup["offset"],
            "width": fixup["width"],
            "location": fixup["location"],
            "target": fixup["target"],
        }
        for fixup in relevant
    ]
    return _sha256(bytes(object_bytes)), _sha256(bytes(prefix)), entry_fixups


def build_index(paths: list[Path]) -> dict:
    """Build the index document from the library paths."""
    libraries = []
    entries = []
    for path in sorted(paths, key=lambda item: item.name):
        document = read_library(path)
        symbol_count = 0
        for module in sorted(
            document["modules"], key=lambda item: item["offset"]
        ):
            segments = {
                segment["index"]: segment for segment in module["segments"]
            }
            for public in sorted(
                module["publics"],
                key=lambda item: (item["segment"], item["offset"]),
            ):
                segment = segments.get(public["segment"])
                if segment is None:
                    continue
                object_sha256, prefix_sha256, fixups = _entry_digests(
                    segment, public, module["fixups"]
                )
                entries.append(
                    {
                        "file": document["file"],
                        "sha256": document["sha256"],
                        "module": module["name"],
                        "module_offset": module["offset"],
                        "symbol": public["name"],
                        "segment": public["segment"],
                        "offset": public["offset"],
                        "size": public["size"],
                        "object_sha256": object_sha256,
                        "prefix_sha256": prefix_sha256,
                        "fixups": fixups,
                    }
                )
                symbol_count += 1
        libraries.append(
            {
                "file": document["file"],
                "sha256": document["sha256"],
                "size": document["size"],
                "page_length": document["page_length"],
                "dictionary": document["dictionary"],
                "module_count": len(document["modules"]),
                "symbol_count": symbol_count,
            }
        )
    entries.sort(
        key=lambda entry: (
            entry["file"],
            entry["module_offset"],
            entry["segment"],
            entry["offset"],
        )
    )
    directory = (
        os.path.commonpath([str(path.parent) for path in paths])
        if paths
        else ""
    )
    return {"directory": directory, "libraries": libraries, "entries": entries}


def load_index(path: Path) -> dict:
    """Read and validate an index document."""
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
    for key in ("directory", "libraries", "entries"):
        if key not in document:
            raise ValueError(f"{path}: the document has no {key}")
    if not isinstance(document["libraries"], list):
        raise ValueError(f"{path}: the libraries field is not a list")
    if not isinstance(document["entries"], list):
        raise ValueError(f"{path}: the entries field is not a list")
    for index, entry in enumerate(document["entries"]):
        if not isinstance(entry, dict):
            raise ValueError(f"{path}: the entry {index} is not an object")
        for key in ENTRY_KEYS:
            if key not in entry:
                raise ValueError(
                    f"{path}: the entry {index} has no {key}"
                )
    return document


def format_index(index: dict) -> str:
    """Return one line per library item."""
    lines = []
    for item in index["libraries"]:
        dictionary = item["dictionary"]
        lines.append(
            f"{item['file']} {item['sha256']} size={item['size']}"
            f" page_length={item['page_length']}"
            f" dictionary={dictionary['offset']:#x}/{dictionary['size']}"
            f" modules={item['module_count']}"
            f" symbols={item['symbol_count']}"
        )
    return "\n".join(lines)


def resolve_library(name: str, directory: Path) -> Path:
    """Resolve a library name in the directory, then in its parent."""
    for candidate in (directory, directory.parent):
        try:
            for child in candidate.iterdir():
                if child.is_file() and child.name.upper() == name.upper():
                    return child
        except OSError:
            continue
    raise ValueError(f"{directory / name}: the library is missing")


def entry_for(
    index: dict, file: str, module: str, symbol: str
) -> dict | None:
    """Return the index entry for one file, module, and symbol."""
    for entry in index.get("entries", []):
        if (
            entry["file"] == file
            and entry["module"] == module
            and entry["symbol"] == symbol
        ):
            return entry
    return None


def _prefix_stem(
    original: le.LinearExecutable, record: dict
) -> bytearray | None:
    """The first original bytes with every LE relocation slot zeroed."""
    address = int(record["address"], 16)
    size = record.get("size")
    prefix_size = min(PREFIX_BYTES, size) if size else PREFIX_BYTES
    data = original.read(address, prefix_size)
    if data is None:
        return None
    masked = bytearray(data)
    for slot in original.relocations():
        start = max(slot.offset - address, 0)
        end = min(slot.offset + slot.width - address, len(masked))
        if start < end:
            masked[start:end] = bytes(end - start)
    return masked


def _prefix_digest(masked: bytearray, entry: dict) -> str:
    """Hash the prefix with the object fixup slots zeroed."""
    data = bytearray(masked)
    for fixup in entry.get("fixups", []):
        start = fixup["offset"] - entry["offset"]
        end = min(start + fixup["width"], len(data))
        start = max(start, 0)
        if start < end:
            data[start:end] = bytes(end - start)
    return _sha256(bytes(data))


def _prefix_covered(masked: bytearray, entry: dict) -> bool:
    """Tell whether the object fixups cover every compared prefix byte."""
    covered: set[int] = set()
    for fixup in entry.get("fixups", []):
        start = fixup["offset"] - entry["offset"]
        end = min(start + fixup["width"], len(masked))
        start = max(start, 0)
        if start < end:
            covered.update(range(start, end))
    return bool(masked) and len(covered) == len(masked)


def candidates(
    index: dict, record: dict, original: le.LinearExecutable
) -> list[dict]:
    """Return the matching index entries in the pinned group order.

    The groups are the exact record symbol, the record name with a trailing
    underscore, the record name, and the entries whose prefix digest matches
    the original image.  Each group keeps the index order.  The union is
    de-duplicated by file, module, and symbol.
    """
    entries = index.get("entries", [])
    groups: list[list[dict]] = []
    symbol = record.get("symbol")
    name = record.get("name")
    if symbol:
        groups.append([entry for entry in entries if entry["symbol"] == symbol])
    if name:
        groups.append(
            [entry for entry in entries if entry["symbol"] == name + "_"]
        )
        groups.append([entry for entry in entries if entry["symbol"] == name])
    stem = _prefix_stem(original, record)
    if stem is not None:
        groups.append(
            [
                entry
                for entry in entries
                if not _prefix_covered(stem, entry)
                and entry["prefix_sha256"] == _prefix_digest(stem, entry)
            ]
        )
    seen: set[tuple[str, str, str]] = set()
    result: list[dict] = []
    for group in groups:
        for entry in group:
            key = (entry["file"], entry["module"], entry["symbol"])
            if key in seen:
                continue
            seen.add(key)
            result.append(entry)
    return result


def library_field_from_entry(entry: dict) -> dict:
    """Return the five library keys of an index entry."""
    return {key: entry[key] for key in LIBRARY_KEYS}


def _find_module(library_document: dict, name: str) -> dict:
    for module in library_document.get("modules", []):
        if module["name"] == name:
            return module
    raise ValueError(
        f"{library_document.get('file')}: the module {name} is missing"
    )


def _find_public(module: dict, name: str, library_file: str) -> dict:
    for public in module["publics"]:
        if public["name"] == name:
            return public
    raise ValueError(
        f"{library_file}: the module {module['name']} has no symbol {name}"
    )


def _find_segment(module: dict, index: int) -> dict:
    for segment in module["segments"]:
        if segment["index"] == index:
            return segment
    raise ValueError(
        f"the module {module['name']} has no segment {index}"
    )


def resolve_entry(library_document: dict, library_field: dict) -> dict:
    """Rebuild one entry extent and its digests from the library document."""
    library_file = library_document.get("file", library_field.get("file", ""))
    module = _find_module(library_document, library_field.get("module"))
    public = _find_public(module, library_field.get("symbol"), library_file)
    segment = _find_segment(module, public["segment"])
    object_sha256, prefix_sha256, fixups = _entry_digests(
        segment, public, module["fixups"]
    )
    return {
        "file": library_document["file"],
        "module": module["name"],
        "module_offset": module["offset"],
        "symbol": public["name"],
        "segment": public["segment"],
        "offset": public["offset"],
        "size": public["size"],
        "object_sha256": object_sha256,
        "prefix_sha256": prefix_sha256,
        "fixups": fixups,
    }


def normalize_imports(
    relocations: list[le.Relocation],
) -> list[le.Relocation]:
    """Rewrite every import and entry slot to the neutral import kind.

    The module, ordinal, name offset, and target datum are dropped.  The
    offset and the width stay, so a slot offset and a slot width still compare.
    """
    result = []
    for slot in relocations:
        if slot.kind in ("import", "import-ordinal", "import-name", "entry"):
            result.append(
                le.Relocation(offset=slot.offset, width=slot.width, kind="import")
            )
        else:
            result.append(slot)
    return result


def object_body(
    entry: dict, library_document: dict, base: int, count: int
) -> list[le.Insn]:
    """Decode the object extent linearly at the original address."""
    module = _find_module(library_document, entry["module"])
    segment = _find_segment(module, entry["segment"])
    data = bytes(segment["data"])[entry["offset"] : entry["offset"] + entry["size"]]
    return le.decode_bytes(data, base, count)


def object_fixups(entry: dict, base: int) -> list[le.Relocation]:
    """Build the object slots in the original address space."""
    result = []
    for fixup in entry["fixups"]:
        kind = fixup["target"]
        if kind not in ("internal", "import", "entry"):
            kind = "internal"
        result.append(
            le.Relocation(
                offset=base + fixup["offset"] - entry["offset"],
                width=fixup["width"],
                kind=kind,
            )
        )
    return result


def _instruction_mask(
    insn: le.Insn, relocations: list[le.Relocation]
) -> set[int]:
    """The masked byte positions of one instruction."""
    mask = set(oracle.structural_mask(insn))
    end = insn.address + insn.size
    for slot in relocations:
        if slot.offset + slot.width <= insn.address or slot.offset >= end:
            continue
        relative = slot.offset - insn.address
        for offset in range(slot.width):
            position = relative + offset
            if 0 <= position < insn.size:
                mask.add(position)
    return mask


def masked_bytes(
    instructions: list[le.Insn], relocations: list[le.Relocation]
) -> int:
    """The number of masked bytes of a body."""
    return sum(
        len(_instruction_mask(insn, relocations)) for insn in instructions
    )


def _comparison_slots(
    original_slots: list[le.Relocation],
    object_slots: list[le.Relocation],
) -> list[le.Relocation]:
    """The union of slots that masks both byte streams.

    Every object slot is a link-time target in the relocatable object.  The
    linked image resolves some of them to an internal target and some to a
    nearby address that no longer carries a relocation.  The union therefore
    masks both sides at every object slot position.  The slot geometry is a
    separate check, so the union cannot hide an original-only slot.
    """
    combined = list(object_slots)
    seen = {(slot.offset, slot.width) for slot in combined}
    for slot in original_slots:
        key = (slot.offset, slot.width)
        if key in seen:
            continue
        seen.add(key)
        combined.append(slot)
    return combined


def _geometry_matches(
    original_slots: list[le.Relocation],
    object_slots: list[le.Relocation],
    start: int,
    end: int | None,
) -> bool:
    """Tell whether every original slot has an object slot of the same width."""
    if end is None:
        return True
    object_widths: dict[int, list[int]] = {}
    for slot in object_slots:
        if start <= slot.offset < end:
            object_widths.setdefault(slot.offset, []).append(slot.width)
    for slot in original_slots:
        if not start <= slot.offset < end:
            continue
        widths = object_widths.get(slot.offset)
        if widths is None or slot.width not in widths:
            return False
    return True


def _original_end(record: dict) -> int | None:
    size = record.get("size")
    if not size:
        return None
    return int(record["address"], 16) + size


def _extent_body(
    original: le.LinearExecutable, address: int, size: int
) -> list[le.Insn]:
    """Decode the original extent linearly."""
    data = original.read(address, size)
    if data is None:
        return []
    return le.decode_bytes(
        data,
        address,
        max_instructions=max(size, 1),
        end=address + size,
    )


def _bodies(
    record: dict,
    original: le.LinearExecutable,
    library_document: dict,
    entry: dict,
) -> tuple[
    int,
    int | None,
    list[le.Insn],
    list[le.Insn],
    list[le.Relocation],
    list[le.Relocation],
]:
    """Build the original and object bodies and their normalized slots."""
    address = int(record["address"], 16)
    size = entry.get("size") or 0
    if size:
        original_body = _extent_body(original, address, size)
        end = address + size
    else:
        end = _original_end(record)
        if end is None:
            original_body = le.function_body(original, address)
            if original_body:
                last = original_body[-1]
                end = last.address + last.size
        else:
            original_body = le.function_body(original, address, end=end)
    object_instructions = object_body(
        entry, library_document, address, len(original_body)
    )
    original_slots = normalize_imports(original.relocations())
    object_slots = normalize_imports(object_fixups(entry, address))
    return (
        address,
        end,
        original_body,
        object_instructions,
        original_slots,
        object_slots,
    )


def verify_entry(
    record: dict,
    original: le.LinearExecutable,
    library_document: dict,
    library_field: dict,
) -> dict:
    """Compare the original body with the resolved library object."""
    if library_field.get("sha256") != library_document.get("sha256"):
        raise ValueError(
            f"{library_field.get('file')}: the library digest does not match "
            f"the document"
        )
    entry = resolve_entry(library_document, library_field)
    if library_field.get("object_sha256") != entry["object_sha256"]:
        raise ValueError(
            f"{entry['module']} {entry['symbol']}: the object digest does not "
            f"match the library entry"
        )
    (
        address,
        end,
        original_body,
        object_instructions,
        original_slots,
        object_slots,
    ) = _bodies(record, original, library_document, entry)
    slots = _comparison_slots(original_slots, object_slots)
    result = oracle.compare(
        original_body,
        object_instructions,
        original_relocations=slots,
        rebuilt_relocations=slots,
    )
    geometry = _geometry_matches(original_slots, object_slots, address, end)
    empty = (
        not original_body
        or not object_instructions
        or result.bytes_total == 0
    )
    match = result.match if geometry and not empty else oracle.DIFFERENT
    return {
        "verdict": "pass"
        if geometry and not empty and result.match in STRICT_MATCHES
        else "fail",
        "detail": oracle.VERDICTS[match],
        "match": match,
        "bytes_equal": result.bytes_equal,
        "bytes_total": result.bytes_total,
        "instructions_equal": result.instructions_equal,
        "instructions_total": result.instructions_total,
    }


def _library_document(file: str, library_dir: Path) -> dict:
    return read_library(resolve_library(file, library_dir))


def verify_candidate(
    record: dict,
    original: le.LinearExecutable,
    index: dict,
    library_dir: Path,
) -> tuple[dict, dict] | None:
    """Verify the candidates and return the chosen entry and its check.

    The chosen entry is the first strict pass in candidate order, else the
    first candidate.  A record without a candidate gives None.
    """
    found = candidates(index, record, original)
    if not found:
        return None
    documents: dict[str, dict] = {}
    first: tuple[dict, dict] | None = None
    for entry in found:
        document = documents.get(entry["file"])
        if document is None:
            document = _library_document(entry["file"], library_dir)
            documents[entry["file"]] = document
        field = library_field_from_entry(entry)
        check = verify_entry(record, original, document, field)
        if first is None:
            first = (entry, check)
        if check["match"] in STRICT_MATCHES:
            return entry, check
    return first


def verify(
    record: dict,
    original: le.LinearExecutable,
    index: dict,
    library_dir: Path,
) -> tuple[str, dict] | None:
    """Return the verdict and the check of the chosen candidate."""
    found = verify_candidate(record, original, index, library_dir)
    if found is None:
        return None
    _, check = found
    return check["verdict"], check


def source_digest(entry: dict) -> str:
    """Hash the canonical five-key library object."""
    field = library_field_from_entry(entry)
    canonical = json.dumps(
        field, sort_keys=True, separators=(",", ":")
    ).encode("utf-8")
    return _sha256(canonical)


def format_verify(
    record: dict,
    original: le.LinearExecutable,
    library_document: dict,
    library_field: dict,
) -> str:
    """Return the verify line with the address, verdict, and mask counts."""
    entry = resolve_entry(library_document, library_field)
    (
        address,
        _,
        original_body,
        object_instructions,
        original_slots,
        object_slots,
    ) = _bodies(record, original, library_document, entry)
    original_masked = masked_bytes(original_body, original_slots)
    object_masked = masked_bytes(object_instructions, object_slots)
    check = verify_entry(record, original, library_document, library_field)
    byte_count = sum(insn.size for insn in original_body)
    return (
        f"{address:#x} {entry['file']} {entry['module']} {entry['symbol']}"
        f" {check['verdict']} instructions={len(original_body)}"
        f" bytes={byte_count} original_masked={original_masked}"
        f" object_masked={object_masked}"
    )


def library_mode() -> dict:
    """Return the pinned library mode."""
    return store.run_mode(True, False)


def _symbol_name(symbol: str) -> str:
    """The record name of a library symbol: the symbol without one final `_`."""
    return symbol[:-1] if symbol.endswith("_") else symbol


def _verify_command(record: dict) -> str:
    return (
        "python3 tools/rebuild/library.py verify "
        f"--address {record['address']}"
    )


def _append_evidence(record: dict, text: str) -> None:
    """Add the command evidence item once."""
    items = record.setdefault("evidence", [])
    item = {"kind": "command", "text": text}
    if item not in items:
        items.append(item)


def _without_result_date(record: dict) -> dict:
    """A deep copy of the record without the run date of its result."""
    clone = json.loads(json.dumps(record))
    result = clone.get("result")
    if isinstance(result, dict):
        result.pop("date", None)
    return clone


def _stored_check(
    record: dict,
    original: le.LinearExecutable,
    library_field: dict,
    library_dir: Path,
) -> dict | None:
    """Verify a stored library field against its library document.

    The function returns the identity check, or None when the library file,
    the module, the symbol, or a digest does not resolve.
    """
    try:
        document = _library_document(library_field["file"], library_dir)
        return verify_entry(record, original, document, library_field)
    except (OSError, ValueError):
        return None


def apply_record(
    record: dict,
    original: le.LinearExecutable,
    index: dict,
    library_dir: Path,
) -> str:
    """Write one strict library result into the record.

    The function returns `written` when it saves the record, `skipped` when
    the record has no candidate or it already holds the result, and
    `different` when the chosen candidate does not pass the strict gate.  A
    different candidate writes nothing, so the record keeps its stored result
    and status.  A record without a candidate keeps its stored library field
    only when that field still passes, so a stored result cannot hide a
    changed field.  A second pass on a later date is skipped, because the run
    date is not part of the record identity.
    """
    found = verify_candidate(record, original, index, library_dir)
    if found is None:
        library_field = record.get("library")
        if library_field is None:
            return "skipped"
        check = _stored_check(record, original, library_field, library_dir)
        if check is None or check.get("match") not in STRICT_MATCHES:
            return "different"
        return "skipped"
    entry, check = found
    if check.get("match") not in STRICT_MATCHES:
        return "different"
    before = json.loads(json.dumps(record))
    if not record.get("name"):
        record["name"] = _symbol_name(entry["symbol"])
    record["library"] = library_field_from_entry(entry)
    record["checks"] = ["identity"]
    record["status"] = "verified"
    command = _verify_command(record)
    _append_evidence(record, command)
    fingerprint = store.compute_fingerprint(
        record,
        None,
        library=record["library"],
        original=original.path,
        mode=library_mode(),
    )
    record["result"] = {
        "date": date.today().isoformat(),
        "command": command,
        "tool": "library.py",
        "source_digest": source_digest(record["library"]),
        "fingerprint": fingerprint,
        "mode": library_mode(),
        "checks": {"identity": check},
    }
    if _without_result_date(before) == _without_result_date(record):
        return "skipped"
    store.save_record(record)
    return "written"


def apply(
    records: list[dict],
    original: le.LinearExecutable,
    index: dict,
    library_dir: Path,
    addresses: list[int] | None,
) -> dict:
    """Apply the library result to the records and count the outcomes.

    Without an address list the pass covers every record with the `library`
    provenance.  With an address list it covers the named records.
    """
    if addresses is None:
        selected = [
            record
            for record in records
            if record.get("provenance") == "library"
        ]
    else:
        wanted = set(addresses)
        selected = [
            record
            for record in records
            if "address" in record and int(record["address"], 16) in wanted
        ]
    counts = {"written": 0, "skipped": 0, "different": 0}
    for record in selected:
        outcome = apply_record(record, original, index, library_dir)
        counts[outcome] += 1
    return counts


def _library_field(value: dict) -> dict:
    """Return the five library keys of a record field or a manifest entry."""
    return {key: value.get(key) for key in LIBRARY_KEYS}


def _address_order(value: str) -> int:
    """Sort an address key by its numeric value."""
    try:
        return int(value, 16)
    except (TypeError, ValueError):
        return 0


def manifest_document(records: list[dict], index: dict) -> dict:
    """Return the link manifest of the library records and the libraries.

    The manifest holds the library digests and one entry per record that
    carries a `library` field.  The library bytes stay out of the manifest.
    """
    libraries = {
        item["file"]: item["sha256"]
        for item in sorted(index.get("libraries", []), key=lambda item: item["file"])
    }
    selected = sorted(
        (
            record
            for record in records
            if isinstance(record.get("library"), dict)
        ),
        key=lambda record: _address_order(record["address"]),
    )
    entries = {
        record["address"]: _library_field(record["library"])
        for record in selected
    }
    return {"libraries": libraries, "entries": entries}


def check_manifest(records: list[dict], document: dict, path: Path) -> list[str]:
    """Return one message per violation of the link manifest contract.

    The check covers the records whose identity is the library object.  A
    record without a `library` field does not take part, because its object is
    not the source of its identity.
    """
    entries = document.get("entries", {})
    libraries = document.get("libraries", {})
    messages: list[str] = []
    library_records = {
        record["address"]: record
        for record in records
        if isinstance(record.get("library"), dict)
    }
    for address in sorted(library_records, key=_address_order):
        record = library_records[address]
        field = _library_field(record["library"])
        entry = entries.get(address)
        if entry is None:
            messages.append(
                f"{path}: the record {address} has a library field and no "
                f"manifest entry"
            )
            continue
        entry_field = _library_field(entry)
        entry_differs = entry_field != field
        if entry_differs:
            messages.append(
                f"{path}: the manifest entry {address} differs from the "
                f"record library field"
            )
        if record.get("status") != "verified":
            messages.append(
                f"{path}: the record {address} has the status "
                f"{record.get('status')}, not verified"
            )
        file = field.get("file")
        manifest_sha256 = libraries.get(file)
        record_sha256 = field.get("sha256")
        entry_sha256 = entry_field.get("sha256")
        if manifest_sha256 is None:
            messages.append(
                f"{path}: the record {address} has no library digest for "
                f"{file}"
            )
        elif not entry_differs and manifest_sha256 != record_sha256:
            messages.append(
                f"{path}: the record {address} library digests do not agree: "
                f"record {record_sha256}, entry {entry_sha256}, manifest "
                f"{manifest_sha256}"
            )
    for address in sorted(entries, key=_address_order):
        if address not in library_records:
            messages.append(
                f"{path}: the manifest entry {address} has no record"
            )
    seen: dict[tuple, tuple[str, dict]] = {}
    for address in sorted(entries, key=_address_order):
        entry_field = _library_field(entries[address])
        key = (
            entry_field.get("file"),
            entry_field.get("module"),
            entry_field.get("symbol"),
        )
        if key in seen:
            first_address, first_field = seen[key]
            if first_field != entry_field:
                messages.append(
                    f"{path}: the entries {first_address} and {address} hold "
                    f"the same object {key[0]} {key[1]} {key[2]} with "
                    f"different keys"
                )
        else:
            seen[key] = (address, entry_field)
    return messages


def _load_manifest(path: Path) -> dict:
    """Read and validate a link manifest document."""
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
    for key in ("libraries", "entries"):
        if key not in document:
            raise ValueError(f"{path}: the document has no {key}")
        if not isinstance(document[key], dict):
            raise ValueError(f"{path}: the {key} field is not an object")
    for address, entry in document["entries"].items():
        if not isinstance(entry, dict):
            raise ValueError(f"{path}: the entry {address} is not an object")
    return document


def command_manifest(arguments: argparse.Namespace) -> int:
    """Build the link manifest and write it to the output path."""
    index = load_index(arguments.index)
    records = [
        store.load_record(address) for address in store.candidate_addresses()
    ]
    document = manifest_document(records, index)
    arguments.out.parent.mkdir(parents=True, exist_ok=True)
    arguments.out.write_text(
        json.dumps(document, indent=2, sort_keys=True) + "\n"
    )
    print(
        f"{len(document['entries'])} entries, "
        f"{len(document['libraries'])} libraries"
    )
    return 0


def command_check_manifest(arguments: argparse.Namespace) -> int:
    """Check the link manifest against the records."""
    document = _load_manifest(arguments.manifest)
    records = [
        store.load_record(address) for address in store.candidate_addresses()
    ]
    messages = check_manifest(records, document, arguments.manifest)
    for message in messages:
        print(message)
    if not messages:
        print(f"{arguments.manifest}: the link manifest matches the records")
    return 1 if messages else 0


def command_index(arguments: argparse.Namespace) -> int:
    """Build the index document and print the summary."""
    paths = [
        resolve_library(name, arguments.library_dir) for name in LIBRARIES
    ]
    index = build_index(paths)
    arguments.out.parent.mkdir(parents=True, exist_ok=True)
    arguments.out.write_text(
        json.dumps(index, indent=2, sort_keys=True) + "\n"
    )
    print(format_index(index))
    return 0


def command_verify(arguments: argparse.Namespace) -> int:
    """Verify the selected records against their library objects."""
    original = le.load(arguments.original)
    index = load_index(arguments.index)
    failed = False
    for address in arguments.addresses:
        record = store.load_record(address)
        field = record.get("library")
        if field is None:
            found = verify_candidate(
                record, original, index, arguments.library_dir
            )
            if found is None:
                print(f"{record['address']} no candidate")
                failed = True
                continue
            entry, _ = found
            field = library_field_from_entry(entry)
        document = _library_document(field["file"], arguments.library_dir)
        check = verify_entry(record, original, document, field)
        print(format_verify(record, original, document, field))
        if check["verdict"] != "pass":
            failed = True
    return 1 if failed else 0


def command_apply(arguments: argparse.Namespace) -> int:
    """Write the strict library result into the selected records."""
    if not arguments.original.is_file():
        raise ValueError(f"the original image {arguments.original} is missing")
    original = le.load(arguments.original)
    index = load_index(arguments.index)
    addresses = arguments.addresses or None
    if addresses is None:
        records = [
            store.load_record(address)
            for address in store.candidate_addresses()
        ]
    else:
        records = [store.load_record(address) for address in addresses]
    counts = apply(records, original, index, arguments.library_dir, addresses)
    print(
        f"written {counts['written']}, skipped {counts['skipped']}, "
        f"different {counts['different']}"
    )
    if addresses is None:
        return 0
    failed = counts["different"] > 0 or any(
        record.get("library") is None for record in records
    )
    return 1 if failed else 0


def _address(value: str) -> int:
    try:
        return int(value, 0)
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"not a hexadecimal address: {value}"
        ) from None


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Index and verify the period Watcom OMF libraries."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    index_parser = subparsers.add_parser(
        "index", help="build the library index document"
    )
    index_parser.add_argument(
        "--library-dir",
        type=Path,
        default=DEFAULT_LIBRARY_DIR,
        help="the directory that holds the libraries",
    )
    index_parser.add_argument(
        "--out",
        type=Path,
        default=INDEX_DOCUMENT,
        help="the index document path",
    )
    verify_parser = subparsers.add_parser(
        "verify", help="verify records against their library objects"
    )
    verify_parser.add_argument(
        "--address",
        dest="addresses",
        type=_address,
        action="append",
        required=True,
        help="a record address, in hex; repeat for more records",
    )
    verify_parser.add_argument(
        "--original",
        type=Path,
        default=store.DEFAULT_ORIGINAL,
        help="the original executable",
    )
    verify_parser.add_argument(
        "--index",
        type=Path,
        default=INDEX_DOCUMENT,
        help="the library index document",
    )
    verify_parser.add_argument(
        "--library-dir",
        type=Path,
        default=DEFAULT_LIBRARY_DIR,
        help="the directory that holds the libraries",
    )
    apply_parser = subparsers.add_parser(
        "apply", help="write the strict library result into the records"
    )
    apply_parser.add_argument(
        "--address",
        dest="addresses",
        type=_address,
        action="append",
        help="a record address, in hex; repeat for more records",
    )
    apply_parser.add_argument(
        "--original",
        type=Path,
        default=store.DEFAULT_ORIGINAL,
        help="the original executable",
    )
    apply_parser.add_argument(
        "--index",
        type=Path,
        default=INDEX_DOCUMENT,
        help="the library index document",
    )
    apply_parser.add_argument(
        "--library-dir",
        type=Path,
        default=DEFAULT_LIBRARY_DIR,
        help="the directory that holds the libraries",
    )
    manifest_parser = subparsers.add_parser(
        "manifest", help="build the link manifest of the library records"
    )
    manifest_parser.add_argument(
        "--index",
        type=Path,
        default=INDEX_DOCUMENT,
        help="the library index document",
    )
    manifest_parser.add_argument(
        "--out",
        type=Path,
        default=LINK_DOCUMENT,
        help="the link manifest path",
    )
    check_manifest_parser = subparsers.add_parser(
        "check-manifest", help="check the link manifest against the records"
    )
    check_manifest_parser.add_argument(
        "--manifest",
        type=Path,
        default=LINK_DOCUMENT,
        help="the link manifest path",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "index":
            return command_index(arguments)
        if arguments.command == "verify":
            return command_verify(arguments)
        if arguments.command == "apply":
            return command_apply(arguments)
        if arguments.command == "manifest":
            return command_manifest(arguments)
        if arguments.command == "check-manifest":
            return command_check_manifest(arguments)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
