"""Compare a rebuilt linear executable with the original image.

The tool reads both images with tools/le.py and compares the whole file, the
header fields, the object table, the page table, the fixup section, and the named
file regions.  The whole-file verdict is exact: the rebuilt image must equal the
original byte for byte.  The region scores show where a difference starts.

Usage:
    python3 tools/rebuild/image_compare.py \
        --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
        --rebuilt work/rebuild/STARTH.EXE

Exit code 0 when the images are byte-identical, 1 when they differ, and 2 on a
bad argument or an unreadable LE image.
"""
from __future__ import annotations

import argparse
import sys
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
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        original = le.load(arguments.original)
        rebuilt = le.load(arguments.rebuilt)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    result = compare_loaded(original, rebuilt)
    print_result(arguments.original, arguments.rebuilt, result)
    return 0 if result.exact else 1


if __name__ == "__main__":
    raise SystemExit(main())
