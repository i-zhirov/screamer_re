"""Diff one rebuilt module against the original executable.

The tool reads the original module and the rebuilt module with tools/le.py,
compares the machine code byte for byte, and compares the instruction
sequences.  The comparison uses the function oracle: it masks every byte that a
fixup slot or a direct branch target controls, then compares the remaining bytes
in instruction order.  A reference to another function or to a data slot
therefore does not fail an exact match, while a reordered sequence does.

`--original-end` bounds the original body at a half-open address, so the body
covers every reachable instruction and does not stop at the first `ret`.  The
oracle compares an intra-function branch target by its instruction position.
It compares an internal fixup target by the symbol identity of the target and
the addend when both images resolve the symbol.

Usage:
    python3 tools/rebuild/diff_module.py \
        --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
        --original-address 0x300ec \
        --original-end 0x300f1 \
        --rebuilt work/rebuild/free.exe \
        --rebuilt-address 0x10000

The rebuilt address comes from `--rebuilt-address` or from the linker map with
`--rebuilt-map` and `--rebuilt-symbol`.  The rebuilt symbol table comes from the
linker map, and the original symbol table comes from the store records.

Exit code 0 when the default gate passes, 1 when the sequences differ.
"""
from __future__ import annotations

import argparse
import re
import sys
from collections.abc import Callable
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import le  # noqa: E402
import oracle  # noqa: E402
import store  # noqa: E402

MAP_SYMBOL = re.compile(
    r"^([0-9a-f]{4}):([0-9a-f]{8})[+*]?\s+(\S+)\s*$", re.IGNORECASE
)
ADDRESS = re.compile(r"\[(?:0x[0-9a-f]+|\d+)\]")
BRANCH_TARGET = re.compile(r"\b(j[a-z]+|call|loop\w*)\s+0x[0-9a-f]+\b")


def instruction_shape(insn: le.Insn) -> str:
    """Name an instruction with every absolute address replaced by a placeholder.

    Two instructions of the same shape differ only in their link-time addresses.  A
    memory operand with a register base keeps its offset, because the stack layout is
    not a link-time address.
    """
    text = ADDRESS.sub("[ADDR]", insn.text)
    return BRANCH_TARGET.sub(r"\1 ADDR", text)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Diff a rebuilt module against the original executable."
    )
    parser.add_argument("--original", required=True, help="the original LE executable")
    parser.add_argument(
        "--original-address", required=True, help="the module linear address, in hex"
    )
    parser.add_argument("--rebuilt", required=True, help="the rebuilt LE executable")
    parser.add_argument(
        "--rebuilt-address",
        help="the module linear address in the rebuilt image, in hex",
    )
    parser.add_argument(
        "--rebuilt-map",
        help="the linker map that holds the module address",
    )
    parser.add_argument(
        "--rebuilt-symbol",
        help="the module symbol in the linker map",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=0,
        help="the instruction count; 0 takes the count from the original",
    )
    parser.add_argument(
        "--original-end",
        type=lambda value: int(value, 0),
        default=None,
        help="the half-open end of the original body, in hex",
    )
    return parser.parse_args()


def map_address(map_path: Path, symbol: str) -> tuple[int, int]:
    """Read a symbol address from a Watcom linker map.

    A map line holds the segment, the offset, and the symbol, for example
    `0001:00000000  free_`.  The segment number names the LE object, and the
    object base turns the object offset into a linear address.
    """
    for line in map_path.read_text(errors="replace").splitlines():
        match = MAP_SYMBOL.match(line.strip())
        if match is None or match.group(3).lower() != symbol.lower():
            continue
        segment = int(match.group(1), 16)
        offset = int(match.group(2), 16)
        return segment, offset
    raise ValueError(f"symbol {symbol} is missing from {map_path}")


def map_symbols(
    map_path: Path, exe: le.LinearExecutable
) -> list[tuple[int, str]]:
    """Read every symbol address from a Watcom linker map.

    A map line holds the segment, the offset, and the symbol, for example
    `0001:00000000  free_`.  The segment number names the LE object, and the
    object base turns the object offset into a linear address.  A line whose
    segment is not an object is skipped.  The pairs are sorted by address, and
    the map order breaks a tie.
    """
    symbols: list[tuple[int, str]] = []
    for line in map_path.read_text(errors="replace").splitlines():
        match = MAP_SYMBOL.match(line.strip())
        if match is None:
            continue
        segment = int(match.group(1), 16)
        offset = int(match.group(2), 16)
        obj = exe.object_by_index(segment)
        if obj is None:
            continue
        symbols.append((obj.reloc_base + offset, match.group(3)))
    return sorted(symbols, key=lambda entry: entry[0])


def symbol_at(
    symbols: list[tuple[int, str | None]], address: int
) -> str | None:
    """The symbol at an exact address, or None when no entry equals it.

    Among equal addresses the first map entry wins.
    """
    for entry_address, symbol in symbols:
        if entry_address == address:
            return symbol
    return None


def next_symbol(
    symbols: list[tuple[int, str | None]], address: int
) -> int | None:
    """The smallest symbol address strictly above the address, or None."""
    later = [entry_address for entry_address, _ in symbols if entry_address > address]
    return min(later) if later else None


def _internal_target(
    exe: le.LinearExecutable, slot: le.Relocation
) -> int | None:
    """The linear address of an internal fixup target, or None."""
    if slot.kind != "internal":
        return None
    if slot.object is None or slot.target is None:
        return None
    obj = exe.object_by_index(slot.object)
    if obj is None:
        return None
    return obj.reloc_base + slot.target


def rebuilt_symbol_resolver(
    exe: le.LinearExecutable, symbols: list[tuple[int, str | None]]
) -> Callable[[le.Relocation], str | None]:
    """A resolver that maps an internal slot to the rebuilt map symbol."""

    def resolve(slot: le.Relocation) -> str | None:
        target = _internal_target(exe, slot)
        if target is None:
            return None
        return symbol_at(symbols, target)

    return resolve


def original_symbol_resolver(
    exe: le.LinearExecutable, records: list[dict]
) -> Callable[[le.Relocation], str | None]:
    """A resolver that maps an internal slot to the record symbol.

    The table holds one entry per record.  A record without a symbol gives a
    None entry, so a target on it resolves to None.
    """
    symbols: list[tuple[int, str | None]] = [
        (int(record["address"], 16), record.get("symbol"))
        for record in records
    ]
    symbols.sort(key=lambda entry: entry[0])

    def resolve(slot: le.Relocation) -> str | None:
        target = _internal_target(exe, slot)
        if target is None:
            return None
        return symbol_at(symbols, target)

    return resolve


def load_records() -> list[dict]:
    return [store.load_record(address) for address in store.candidate_addresses()]


def resolve_rebuilt_address(exe: le.LinearExecutable, arguments: argparse.Namespace) -> int:
    if arguments.rebuilt_address:
        return int(arguments.rebuilt_address, 16)
    if not arguments.rebuilt_map or not arguments.rebuilt_symbol:
        raise ValueError("--rebuilt-address or --rebuilt-map with --rebuilt-symbol is needed")
    segment, offset = map_address(Path(arguments.rebuilt_map), arguments.rebuilt_symbol)
    obj = exe.object_by_index(segment)
    if obj is None:
        raise ValueError(f"object {segment} is missing from the rebuilt image")
    return obj.reloc_base + offset


def format_bytes(data: bytes) -> str:
    return " ".join(f"{byte:02x}" for byte in data)


def original_body(
    exe: le.LinearExecutable, address: int, end: int | None, count: int
) -> list[le.Insn]:
    """The original body, bounded by the end over the count."""
    if end is not None:
        return le.function_body(exe, address, end=end)
    if count > 0:
        return le.function_body(exe, address)[:count]
    return le.function_body(exe, address)


def rebuilt_body(
    exe: le.LinearExecutable,
    address: int,
    end: int | None,
    count: int,
    extent: int | None = None,
) -> list[le.Insn]:
    """The rebuilt body, bounded by the rebuilt extent when the map gives it.

    The rebuilt extent is the address of the next symbol in the rebuilt map, so
    a tail jump into the next function stays out of the body.
    """
    if extent is not None:
        return le.function_body(exe, address, end=extent)
    if end is not None:
        return le.function_body(exe, address)
    if count > 0:
        return le.decode(exe, address, count)
    return le.function_body(exe, address)


def print_body(label: str, instructions: list[le.Insn]) -> None:
    print(f"--- {label} ---")
    for insn in instructions:
        print(f"{insn.address:#010x}: {insn.raw.hex():<20} {insn.text}")


def main() -> int:
    arguments = parse_arguments()
    original = le.load(Path(arguments.original))
    rebuilt = le.load(Path(arguments.rebuilt))
    original_address = int(arguments.original_address, 16)
    try:
        rebuilt_address = resolve_rebuilt_address(rebuilt, arguments)
        original_instructions = original_body(
            original, original_address, arguments.original_end, arguments.count
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    if not original_instructions:
        print(f"error: no function body at {original_address:#x}", file=sys.stderr)
        return 1
    rebuilt_symbols = (
        map_symbols(Path(arguments.rebuilt_map), rebuilt)
        if arguments.rebuilt_map
        else None
    )
    rebuilt_extent = (
        next_symbol(rebuilt_symbols, rebuilt_address)
        if rebuilt_symbols is not None
        else None
    )
    try:
        rebuilt_instructions = rebuilt_body(
            rebuilt,
            rebuilt_address,
            arguments.original_end,
            arguments.count,
            extent=rebuilt_extent,
        )
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    original_bytes = b"".join(insn.raw for insn in original_instructions)
    rebuilt_bytes = b"".join(insn.raw for insn in rebuilt_instructions)

    print(f"original: {arguments.original} at {original_address:#x} "
          f"({len(original_bytes)} bytes, {len(original_instructions)} instructions)")
    print(f"rebuilt:  {arguments.rebuilt} at {rebuilt_address:#x} "
          f"({len(rebuilt_bytes)} bytes, {len(rebuilt_instructions)} instructions)")
    print()
    print(f"original bytes: {format_bytes(original_bytes)}")
    print(f"rebuilt bytes:  {format_bytes(rebuilt_bytes)}")
    print()

    equal = sum(1 for a, b in zip(original_bytes, rebuilt_bytes) if a == b)
    total = min(len(original_bytes), len(rebuilt_bytes))
    print(f"byte compare: {equal} of {total} bytes equal, {total - equal} different")
    if len(original_bytes) != len(rebuilt_bytes):
        print(f"byte count differs: {len(original_bytes)} and {len(rebuilt_bytes)}")
    differences = [
        index for index in range(total)
        if original_bytes[index] != rebuilt_bytes[index]
    ]
    if differences:
        first = differences[0]
        last = differences[-1]
        print(f"different bytes: {first}..{last} "
              f"(original {format_bytes(original_bytes[first:last + 1])}, "
              f"rebuilt {format_bytes(rebuilt_bytes[first:last + 1])})")
    print()

    print_body("original", original_instructions)
    print()
    print_body("rebuilt", rebuilt_instructions)
    print()

    original_shapes = [instruction_shape(insn) for insn in original_instructions]
    rebuilt_shapes = [instruction_shape(insn) for insn in rebuilt_instructions]
    common = min(len(original_instructions), len(rebuilt_instructions))
    opcode_differences = 0
    operand_differences = 0
    for index, (a, b) in enumerate(zip(original_instructions, rebuilt_instructions)):
        if a.mnemonic != b.mnemonic:
            opcode_differences += 1
            print(f"opcode difference at {a.address:#x}: {a.text} and {b.text}")
        elif a.operands != b.operands:
            operand_differences += 1
            print(f"operand difference at {a.address:#x}: {a.text} and {b.text}")
    print(f"instruction compare: {common - opcode_differences} of "
          f"{common} instructions equal, "
          f"{opcode_differences} opcodes different, "
          f"{operand_differences} operands different")
    original_symbols = original_symbol_resolver(original, load_records())
    rebuilt_resolver = (
        rebuilt_symbol_resolver(rebuilt, rebuilt_symbols)
        if rebuilt_symbols is not None
        else None
    )
    result = oracle.compare(
        original_instructions,
        rebuilt_instructions,
        original_relocations=original.relocations(),
        rebuilt_relocations=rebuilt.relocations(),
        original_symbols=original_symbols,
        rebuilt_symbols=rebuilt_resolver,
    )
    if len(original_instructions) != len(rebuilt_instructions):
        print(f"instruction count differs: {len(original_instructions)} and "
              f"{len(rebuilt_instructions)}")
        for index, (a, b) in enumerate(zip(original_shapes, rebuilt_shapes)):
            if a == b:
                continue
            tail = len(original_instructions) - index
            if rebuilt_shapes[index + 1 : index + 1 + tail] == original_shapes[index:]:
                extra = rebuilt_instructions[index]
                print(f"the rebuilt module holds one instruction the "
                      f"original does not: {extra.text}")
            elif original_shapes[index + 1 : index + 1 + tail] == rebuilt_shapes[index:]:
                extra = original_instructions[index]
                print(f"the original holds one instruction the rebuilt "
                      f"module does not: {extra.text}")
            break
    print(f"relocation compare: {result.relocations_equal} of "
          f"{result.relocations_total} slots equal")
    for difference in result.differences[:3]:
        print(f"oracle difference: {difference}")
    print(f"verdict: {result.verdict}")
    return 0 if result.passes_default else 1


if __name__ == "__main__":
    raise SystemExit(main())
