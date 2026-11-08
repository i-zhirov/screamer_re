"""Diff one rebuilt module against the original executable.

The tool reads the original module and the rebuilt module with tools/le.py,
compares the machine code byte for byte, and compares the instruction
sequences.  The comparison uses the function oracle: it masks every byte that a
fixup slot or a direct branch target controls, then compares the remaining bytes
in instruction order.  A reference to another function or to a data slot
therefore does not fail an exact match, while a reordered sequence does.

Usage:
    python3 tools/rebuild/diff_module.py \
        --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
        --original-address 0x300ec \
        --rebuilt work/rebuild/free.exe \
        --rebuilt-address 0x10000

The rebuilt address comes from `--rebuilt-address` or from the linker map with
`--rebuilt-map` and `--rebuilt-symbol`.

Exit code 0 when the default gate passes, 1 when the sequences differ.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import le  # noqa: E402
import oracle  # noqa: E402

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
        original_body = le.function_body(original, original_address)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    if not original_body:
        print(f"error: no function body at {original_address:#x}", file=sys.stderr)
        return 1
    if arguments.count:
        count = arguments.count
        original_body = original_body[:count]
        try:
            rebuilt_body = le.decode(rebuilt, rebuilt_address, count)
        except ValueError as error:
            print(f"error: {error}", file=sys.stderr)
            return 1
    else:
        try:
            rebuilt_body = le.function_body(rebuilt, rebuilt_address)
        except ValueError as error:
            print(f"error: {error}", file=sys.stderr)
            return 1

    original_bytes = b"".join(insn.raw for insn in original_body)
    rebuilt_bytes = b"".join(insn.raw for insn in rebuilt_body)

    print(f"original: {arguments.original} at {original_address:#x} "
          f"({len(original_bytes)} bytes, {len(original_body)} instructions)")
    print(f"rebuilt:  {arguments.rebuilt} at {rebuilt_address:#x} "
          f"({len(rebuilt_bytes)} bytes, {len(rebuilt_body)} instructions)")
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

    print_body("original", original_body)
    print()
    print_body("rebuilt", rebuilt_body)
    print()

    original_shapes = [instruction_shape(insn) for insn in original_body]
    rebuilt_shapes = [instruction_shape(insn) for insn in rebuilt_body]
    common = min(len(original_body), len(rebuilt_body))
    opcode_differences = 0
    operand_differences = 0
    for index, (a, b) in enumerate(zip(original_body, rebuilt_body)):
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
    result = oracle.compare(
        original_body,
        rebuilt_body,
        original_relocations=original.relocations(),
        rebuilt_relocations=rebuilt.relocations(),
    )
    if len(original_body) != len(rebuilt_body):
        print(f"instruction count differs: {len(original_body)} and "
              f"{len(rebuilt_body)}")
        for index, (a, b) in enumerate(zip(original_shapes, rebuilt_shapes)):
            if a == b:
                continue
            tail = len(original_body) - index
            if rebuilt_shapes[index + 1 : index + 1 + tail] == original_shapes[index:]:
                extra = rebuilt_body[index]
                print(f"the rebuilt module holds one instruction the "
                      f"original does not: {extra.text}")
            elif original_shapes[index + 1 : index + 1 + tail] == rebuilt_shapes[index:]:
                extra = original_body[index]
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
