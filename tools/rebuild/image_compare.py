"""Compare a rebuilt linear executable with the original image.

The tool reads both images with tools/le.py and compares the whole file, the
header fields, the object table, the page table, the fixup section, and the named
file regions.  The whole-file verdict is exact: the rebuilt image must equal the
original byte for byte.  The region scores show where a difference starts.

The `--coverage` option reports the linked code bytes, the code gap, the data
gap, and the unmodeled ranges.  It reads the planned unmodeled ranges from the
link plan when `--plan` is given.  The coverage row status is `identical`,
`different`, `absent`, or `unmodeled`.

Usage:
    python3 tools/rebuild/image_compare.py \
        --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
        --rebuilt work/rebuild/STARTH.EXE
    python3 tools/rebuild/image_compare.py \
        --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
        --rebuilt work/rebuild/STARTH.EXE --coverage \
        --plan tools/rebuild/link.json

Exit code 0 when the images are byte-identical, 1 when they differ, and 2 on a
bad argument or an unreadable LE image.  With `--coverage`, exit code 0 means
every region is identical, and 1 means a region is different, absent, or
unmodeled.
"""
from __future__ import annotations

import argparse
import json
import sys
from collections.abc import Sequence
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import le  # noqa: E402

HEADER_FIELDS = (
    "cpu",
    "os",
    "module_version",
    "module_flags",
    "page_count",
    "start_object",
    "entry_offset",
    "stack_object",
    "stack_pointer",
    "page_size",
    "last_page_size",
    "fixup_size",
    "loader_section_size",
    "object_table_offset",
    "object_count",
    "object_page_table_offset",
    "fixup_page_table_offset",
    "fixup_record_table_offset",
    "data_pages_offset",
)


@dataclass(frozen=True)
class Region:
    """A named byte range of an image file."""

    name: str
    start: int
    end: int

    @property
    def length(self) -> int:
        return max(0, self.end - self.start)


@dataclass
class Score:
    """The byte score of one comparison."""

    name: str
    equal: int
    total: int

    @property
    def exact(self) -> bool:
        return self.total == 0 or self.equal == self.total

    def line(self) -> str:
        return f"region {self.name}: {self.equal}/{self.total}"


@dataclass
class ImageResult:
    """The result of a whole-image comparison."""

    exact: bool
    equal: int
    total: int
    regions: list[Score] = field(default_factory=list)
    differences: list[str] = field(default_factory=list)

    @property
    def verdict(self) -> str:
        if self.exact:
            return "the images are byte-identical"
        return "the images differ"


def equal_prefix(original: bytes, rebuilt: bytes) -> tuple[int, int]:
    """Return the equal byte count and the larger byte count."""
    equal = sum(1 for left, right in zip(original, rebuilt) if left == right)
    return equal, max(len(original), len(rebuilt))


def _page_offsets(exe: le.LinearExecutable, object_index: int) -> list[int]:
    return [
        page.file_offset
        for page in exe.pages
        if page.object_index == object_index and page.file_offset is not None
    ]


def file_regions(exe: le.LinearExecutable) -> list[Region]:
    """Name the structural regions of an image file.

    The order is the file order: the stub, the LE header, the resident tables,
    the fixup section, one region per object's pages, and the non-resident name
    table.  The table offsets are relative to the LE header.
    """
    header = exe.header
    header_offset = exe.header_offset
    regions = [Region("stub", 0, header_offset)]
    object_table_offset = header.get("object_table_offset", 0)
    if object_table_offset:
        regions.append(
            Region("header", header_offset, header_offset + object_table_offset)
        )
    fixup_page_table_offset = header.get("fixup_page_table_offset", 0)
    if object_table_offset and fixup_page_table_offset:
        regions.append(
            Region(
                "loader",
                header_offset + object_table_offset,
                header_offset + fixup_page_table_offset,
            )
        )
    fixup_size = header.get("fixup_size", 0)
    if fixup_size:
        start = header_offset + fixup_page_table_offset
        regions.append(Region("fixups", start, start + fixup_size))
    for obj in exe.objects:
        offsets = _page_offsets(exe, obj.index)
        if not offsets:
            continue
        end = max(
            page.file_offset + page.data_size
            for page in exe.pages
            if page.object_index == obj.index and page.file_offset is not None
        )
        regions.append(Region(f"obj.{obj.index}", min(offsets), end))
    name_offset = header.get("non_resident_name_table_offset", 0)
    name_size = header.get("non_resident_name_size", 0)
    if name_offset and name_size:
        regions.append(Region("nonresident", name_offset, name_offset + name_size))
    return regions


def region_score(
    name: str,
    original: le.LinearExecutable,
    rebuilt: le.LinearExecutable,
    original_region: Region | None,
    rebuilt_region: Region | None,
) -> Score:
    """Score one region across the two images, by region name."""
    original_bytes = b""
    if original_region is not None:
        original_bytes = original.data[original_region.start : original_region.end]
    rebuilt_bytes = b""
    if rebuilt_region is not None:
        rebuilt_bytes = rebuilt.data[rebuilt_region.start : rebuilt_region.end]
    equal, total = equal_prefix(original_bytes, rebuilt_bytes)
    return Score(name, equal, total)


def compare_regions(
    original: le.LinearExecutable, rebuilt: le.LinearExecutable
) -> list[Score]:
    original_regions = {region.name: region for region in file_regions(original)}
    rebuilt_regions = {region.name: region for region in file_regions(rebuilt)}
    names = list(original_regions)
    names.extend(name for name in rebuilt_regions if name not in original_regions)
    return [
        region_score(
            name,
            original,
            rebuilt,
            original_regions.get(name),
            rebuilt_regions.get(name),
        )
        for name in names
    ]


def compare_headers(
    original: le.LinearExecutable, rebuilt: le.LinearExecutable
) -> list[str]:
    differences = []
    for name in HEADER_FIELDS:
        left = original.header.get(name)
        right = rebuilt.header.get(name)
        if left != right:
            differences.append(f"header.{name}: original {left!r}, rebuilt {right!r}")
    return differences


def compare_objects(
    original: le.LinearExecutable, rebuilt: le.LinearExecutable
) -> list[str]:
    differences = []
    if len(original.objects) != len(rebuilt.objects):
        differences.append(
            f"objects: original {len(original.objects)}, "
            f"rebuilt {len(rebuilt.objects)}"
        )
    for left, right in zip(original.objects, rebuilt.objects):
        for name in ("index", "virtual_size", "reloc_base", "flags",
                     "page_table_index", "page_count", "reserved"):
            if getattr(left, name) != getattr(right, name):
                differences.append(
                    f"obj.{left.index}.{name}: original "
                    f"{getattr(left, name)!r}, rebuilt {getattr(right, name)!r}"
                )
    return differences


def compare_pages(
    original: le.LinearExecutable, rebuilt: le.LinearExecutable
) -> list[str]:
    differences = []
    if len(original.pages) != len(rebuilt.pages):
        differences.append(
            f"pages: original {len(original.pages)}, rebuilt {len(rebuilt.pages)}"
        )
    for left, right in zip(original.pages, rebuilt.pages):
        for name in ("object_index", "page_index", "number", "flags",
                     "file_offset", "data_size"):
            if getattr(left, name) != getattr(right, name):
                differences.append(
                    f"page obj.{left.object_index}.{left.page_index}.{name}: "
                    f"original {getattr(left, name)!r}, "
                    f"rebuilt {getattr(right, name)!r}"
                )
    return differences


def compare_fixups(
    original: le.LinearExecutable, rebuilt: le.LinearExecutable
) -> list[str]:
    differences = []
    for name in ("fixup_page_table_offset", "fixup_record_table_offset"):
        if original.header.get(name) != rebuilt.header.get(name):
            differences.append(
                f"header.{name}: original {original.header.get(name)!r}, "
                f"rebuilt {rebuilt.header.get(name)!r}"
            )
    left = original.fixup_section()
    right = rebuilt.fixup_section()
    if left != right:
        equal, total = equal_prefix(left, right)
        differences.append(
            f"fixup section: {equal} of {total} bytes equal"
        )
    return differences


def compare_loaded(
    original: le.LinearExecutable, rebuilt: le.LinearExecutable
) -> ImageResult:
    """Compare two parsed images."""
    equal, total = equal_prefix(original.data, rebuilt.data)
    differences: list[str] = []
    differences.extend(compare_headers(original, rebuilt))
    differences.extend(compare_objects(original, rebuilt))
    differences.extend(compare_pages(original, rebuilt))
    differences.extend(compare_fixups(original, rebuilt))
    return ImageResult(
        exact=original.data == rebuilt.data,
        equal=equal,
        total=total,
        regions=compare_regions(original, rebuilt),
        differences=differences,
    )


def compare_images(original: Path, rebuilt: Path) -> ImageResult:
    return compare_loaded(le.load(original), le.load(rebuilt))


@dataclass(frozen=True)
class LinearRange:
    """A planned range of module linear addresses that no module rebuilds."""

    start: int
    end: int
    kind: str = "code"

    @property
    def length(self) -> int:
        return max(0, self.end - self.start)


@dataclass
class CoverageRegion:
    """One row of the whole-image coverage table."""

    name: str
    kind: str
    status: str
    original_bytes: int
    rebuilt_bytes: int
    equal_bytes: int

    def as_dict(self) -> dict:
        return {
            "name": self.name,
            "kind": self.kind,
            "status": self.status,
            "original_bytes": self.original_bytes,
            "rebuilt_bytes": self.rebuilt_bytes,
            "equal_bytes": self.equal_bytes,
        }

    def line(self) -> str:
        gap = max(0, self.original_bytes - self.rebuilt_bytes)
        return (
            f"region {self.name} [{self.kind}]: {self.status}, "
            f"linked {self.rebuilt_bytes} of {self.original_bytes}, gap {gap}"
        )


@dataclass
class CoverageReport:
    """The linked, gap, and unmodeled byte counts of one rebuilt image."""

    regions: list[CoverageRegion] = field(default_factory=list)
    total_bytes: int = 0
    code_linked: int = 0
    code_gap: int = 0
    data_gap: int = 0
    unmodeled_bytes: int = 0
    verdict: str = "identical"

    def as_dict(self) -> dict:
        return {
            "total_bytes": self.total_bytes,
            "code_linked": self.code_linked,
            "code_gap": self.code_gap,
            "data_gap": self.data_gap,
            "unmodeled_bytes": self.unmodeled_bytes,
            "regions": [region.as_dict() for region in self.regions],
            "verdict": self.verdict,
        }


def plan_unmodeled(plan: dict) -> list[LinearRange]:
    """The planned unmodeled ranges of one link plan, as linear ranges."""
    ranges: list[LinearRange] = []
    for entry in plan.get("unmodeled", []):
        start = _plan_integer(entry.get("start"), "start")
        end = _plan_integer(entry.get("end"), "end")
        kind = entry.get("kind", "code")
        if kind not in ("code", "data"):
            raise ValueError(f"the unmodeled kind {kind!r} is not code or data")
        if end <= start:
            raise ValueError(
                f"the unmodeled range {start:#x} to {end:#x} is empty"
            )
        ranges.append(LinearRange(start, end, kind))
    return ranges


def load_unmodeled(path: Path) -> list[LinearRange]:
    """Read the unmodeled ranges from a link plan file."""
    try:
        plan = json.loads(Path(path).read_text())
    except FileNotFoundError:
        raise ValueError(f"{path}: the link plan is missing") from None
    except OSError as error:
        raise ValueError(f"{path}: {error}") from error
    except json.JSONDecodeError as error:
        raise ValueError(f"{path}: broken JSON: {error}") from error
    if not isinstance(plan, dict):
        raise ValueError(f"{path}: the link plan is not a JSON object")
    return plan_unmodeled(plan)


def _plan_integer(value: object, field_name: str) -> int:
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        try:
            return int(value, 0)
        except ValueError:
            pass
    raise ValueError(f"the link plan {field_name} {value!r} is not a number")


def linear_bytes(
    exe: le.LinearExecutable, start: int, length: int
) -> bytes | None:
    """The file bytes of a linear range, or None when part is not mapped."""
    if length <= 0:
        return b""
    end = start + length
    cursor = start
    data = bytearray()
    while cursor < end:
        page = exe.page_at(cursor)
        if page is None or page.file_offset is None:
            return None
        page_end = page.virtual_address + page.virtual_size
        chunk = min(end, page_end) - cursor
        if chunk <= 0:
            return None
        offset = page.file_offset + (cursor - page.virtual_address)
        data.extend(exe.data[offset : offset + chunk])
        cursor += chunk
    return bytes(data)


def mapped_prefix(
    exe: le.LinearExecutable, start: int, length: int
) -> bytes:
    """The file bytes of a linear range, stopping at the first unmapped page."""
    if length <= 0:
        return b""
    end = start + length
    cursor = start
    data = bytearray()
    while cursor < end:
        page = exe.page_at(cursor)
        if page is None or page.file_offset is None:
            break
        page_end = page.virtual_address + page.virtual_size
        chunk = min(end, page_end) - cursor
        if chunk <= 0:
            break
        offset = page.file_offset + (cursor - page.virtual_address)
        data.extend(exe.data[offset : offset + chunk])
        cursor += chunk
    return bytes(data)


def _status(original: bytes, rebuilt: bytes, unmodeled: bool = False) -> str:
    if unmodeled:
        return "unmodeled"
    if len(rebuilt) < len(original):
        return "absent"
    if original == rebuilt:
        return "identical"
    return "different"


def _blocked_segments(
    start: int, end: int, blocked: Sequence[LinearRange]
) -> list[tuple[int, int, str | None]]:
    """Split a span into modeled segments and clipped unmodeled segments."""
    clipped = []
    for entry in blocked:
        left = max(start, entry.start)
        right = min(end, entry.end)
        if left < right:
            clipped.append(LinearRange(left, right, entry.kind))
    clipped.sort(key=lambda entry: (entry.start, entry.end))
    segments: list[tuple[int, int, str | None]] = []
    cursor = start
    for entry in clipped:
        if entry.start > cursor:
            segments.append((cursor, entry.start, None))
        segments.append((entry.start, entry.end, entry.kind))
        cursor = max(cursor, entry.end)
    if cursor < end:
        segments.append((cursor, end, None))
    return segments


def _structural_regions(
    original: le.LinearExecutable, rebuilt: le.LinearExecutable
) -> list[CoverageRegion]:
    rebuilt_by_name = {
        region.name: region
        for region in file_regions(rebuilt)
        if not region.name.startswith("obj.")
    }
    regions = []
    for region in file_regions(original):
        if region.name.startswith("obj."):
            continue
        other = rebuilt_by_name.get(region.name)
        original_bytes = original.data[region.start : region.end]
        rebuilt_bytes = b""
        if other is not None:
            rebuilt_bytes = rebuilt.data[other.start : other.end]
        equal, _ = equal_prefix(original_bytes, rebuilt_bytes)
        regions.append(
            CoverageRegion(
                name=region.name,
                kind="structural",
                status=_status(original_bytes, rebuilt_bytes),
                original_bytes=len(original_bytes),
                rebuilt_bytes=len(rebuilt_bytes),
                equal_bytes=equal,
            )
        )
    return regions


def _object_regions(
    original: le.LinearExecutable,
    rebuilt: le.LinearExecutable,
    unmodeled: Sequence[LinearRange],
) -> list[CoverageRegion]:
    regions: list[CoverageRegion] = []
    for obj in original.objects:
        start = obj.reloc_base
        end = obj.reloc_base + obj.virtual_size
        if end <= start:
            continue
        executable = bool(obj.flags & le.O_EXECUTABLE)
        if not executable:
            original_bytes = mapped_prefix(original, start, end - start)
            rebuilt_bytes = mapped_prefix(rebuilt, start, end - start)
            blocked = _blocked_segments(start, end, unmodeled)
            status = _status(
                original_bytes,
                rebuilt_bytes,
                unmodeled=all(kind is not None for _, _, kind in blocked)
                and bool(blocked),
            )
            equal, _ = equal_prefix(original_bytes, rebuilt_bytes)
            regions.append(
                CoverageRegion(
                    name=f"obj.{obj.index}",
                    kind="data",
                    status=status,
                    original_bytes=len(original_bytes),
                    rebuilt_bytes=len(rebuilt_bytes),
                    equal_bytes=equal,
                )
            )
            continue
        for segment_start, segment_end, blocked_kind in _blocked_segments(
            start, end, unmodeled
        ):
            length = segment_end - segment_start
            original_bytes = linear_bytes(original, segment_start, length) or b""
            rebuilt_bytes = linear_bytes(rebuilt, segment_start, length) or b""
            if blocked_kind is not None:
                original_bytes = bytes(length)
                rebuilt_bytes = b""
            equal, _ = equal_prefix(original_bytes, rebuilt_bytes)
            regions.append(
                CoverageRegion(
                    name=f"obj.{obj.index}.{segment_start:05x}",
                    kind=blocked_kind or "code",
                    status=_status(
                        original_bytes,
                        rebuilt_bytes,
                        unmodeled=blocked_kind is not None,
                    ),
                    original_bytes=length,
                    rebuilt_bytes=len(rebuilt_bytes),
                    equal_bytes=equal,
                )
            )
    return regions


def coverage_loaded(
    original: le.LinearExecutable,
    rebuilt: le.LinearExecutable,
    unmodeled: Sequence[LinearRange] = (),
) -> CoverageReport:
    """Score the linked code, the code gap, the data gap, and the plan gaps."""
    regions = _structural_regions(original, rebuilt)
    regions.extend(_object_regions(original, rebuilt, unmodeled))
    code = [
        region
        for region in regions
        if region.kind == "code" and region.status != "unmodeled"
    ]
    data = [region for region in regions if region.kind == "data"]
    gaps = [region for region in regions if region.status == "unmodeled"]
    code_linked = sum(region.rebuilt_bytes for region in code)
    code_gap = max(0, sum(region.original_bytes for region in code) - code_linked)
    data_gap = max(
        0,
        sum(region.original_bytes for region in data)
        - sum(region.rebuilt_bytes for region in data),
    )
    if gaps:
        verdict = "unmodeled"
    elif any(region.status == "absent" for region in regions):
        verdict = "absent"
    elif any(region.status == "different" for region in regions):
        verdict = "different"
    else:
        verdict = "identical"
    return CoverageReport(
        regions=regions,
        total_bytes=len(original.data),
        code_linked=code_linked,
        code_gap=code_gap,
        data_gap=data_gap,
        unmodeled_bytes=sum(region.original_bytes for region in gaps),
        verdict=verdict,
    )


def coverage_images(
    original: Path,
    rebuilt: Path,
    unmodeled: Sequence[LinearRange] = (),
) -> CoverageReport:
    return coverage_loaded(
        le.load(original), le.load(rebuilt), unmodeled
    )


def print_coverage(report: CoverageReport) -> None:
    for region in report.regions:
        print(region.line())
    print(
        f"coverage: code linked {report.code_linked}, "
        f"code gap {report.code_gap}, data gap {report.data_gap}, "
        f"unmodeled {report.unmodeled_bytes}"
    )
    print(f"verdict: {report.verdict}")


def print_result(
    original: Path, rebuilt: Path, result: ImageResult
) -> None:
    print(f"original: {original}")
    print(f"rebuilt:  {rebuilt}")
    print()
    print(f"image compare: {result.equal} of {result.total} bytes equal")
    for score in result.regions:
        print(score.line())
    if result.differences:
        print()
        print("differences:")
        for difference in result.differences:
            print(f"  {difference}")
    else:
        print()
        print("differences: none")
    print(f"verdict: {result.verdict}")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compare a rebuilt LE image with the original image."
    )
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--rebuilt", type=Path, required=True)
    parser.add_argument(
        "--coverage",
        action="store_true",
        help="print the linked, gap, and unmodeled byte table",
    )
    parser.add_argument(
        "--plan",
        type=Path,
        help="the link plan that declares the unmodeled ranges",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        original = le.load(arguments.original)
        rebuilt = le.load(arguments.rebuilt)
        if arguments.coverage:
            unmodeled = (
                load_unmodeled(arguments.plan)
                if arguments.plan is not None
                else []
            )
            report = coverage_loaded(original, rebuilt, unmodeled)
            print_coverage(report)
            return 0 if report.verdict == "identical" else 1
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    result = compare_loaded(original, rebuilt)
    print_result(arguments.original, arguments.rebuilt, result)
    return 0 if result.exact else 1


if __name__ == "__main__":
    raise SystemExit(main())
