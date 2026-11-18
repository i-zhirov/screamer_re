"""Compare two function bodies with the LE relocations taken into account.

The oracle masks every byte that a fixup slot controls and every byte that
encodes a direct branch target.  It then compares the remaining bytes in
instruction order.  A relocation-only difference passes the strict gate.  A
reordered instruction sequence, an inserted instruction, or a non-relocation
byte difference does not.

Usage:
    from oracle import compare

    result = compare(
        original_instructions,
        rebuilt_instructions,
        original_relocations=original.relocations(),
        rebuilt_relocations=rebuilt.relocations(),
    )

The function does not print.  The caller formats the result.
"""
from __future__ import annotations

import re
import sys
from collections.abc import Callable
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import le  # noqa: E402

EXACT_BYTES = "exact-bytes"
EXACT_RELOCATIONS = "exact-relocations"
SAME_SHAPE = "same-shape"
SAME_MNEMONICS = "same-mnemonics"
DIFFERENT = "different"

STRICT_MATCHES = (EXACT_BYTES, EXACT_RELOCATIONS)
DEFAULT_MATCHES = (EXACT_BYTES, EXACT_RELOCATIONS, SAME_SHAPE, SAME_MNEMONICS)

VERDICTS = {
    EXACT_BYTES: "the modules are byte-identical",
    EXACT_RELOCATIONS: (
        "the instruction sequences match; the differing operands are the "
        "link-time addresses"
    ),
    SAME_SHAPE: (
        "the instruction shapes match; the differing bytes are not relocations"
    ),
    SAME_MNEMONICS: "the modules hold the same instructions in a different order",
    DIFFERENT: "the instruction sequences differ",
}

ADDRESS = re.compile(r"\[(?:0x[0-9a-f]+|\d+)\]")
BRANCH_TARGET = re.compile(r"\b(j[a-z]+|call|loop\w*)\s+0x[0-9a-f]+\b")


@dataclass
class OracleResult:
    """The result of one function comparison."""

    match: str
    bytes_equal: int
    bytes_total: int
    instructions_equal: int
    instructions_total: int
    relocations_equal: int = 0
    relocations_total: int = 0
    differences: list[str] = field(default_factory=list)

    @property
    def verdict(self) -> str:
        return VERDICTS[self.match]

    @property
    def passes_strict(self) -> bool:
        return self.match in STRICT_MATCHES

    @property
    def passes_default(self) -> bool:
        return self.match in DEFAULT_MATCHES


def normalized_operands(insn: le.Insn) -> str:
    """The operand text with every absolute address replaced by a placeholder."""
    text = ADDRESS.sub("[ADDR]", insn.operands)
    return BRANCH_TARGET.sub(r"\1 ADDR", text)


def structural_mask(insn: le.Insn) -> set[int]:
    """The byte offsets of a direct branch target inside the instruction."""
    operand = insn.operands.strip()
    if not operand.startswith("0x"):
        return set()
    mnemonic = insn.mnemonic.lower()
    if not (
        mnemonic.startswith("j")
        or mnemonic == "call"
        or mnemonic.startswith("loop")
    ):
        return set()
    raw = insn.raw
    if not raw:
        return set()
    if 0x70 <= raw[0] <= 0x7F or 0xE0 <= raw[0] <= 0xE3 or raw[0] == 0xEB:
        return {len(raw) - 1}
    if raw[0] in (0xE8, 0xE9):
        return set(range(len(raw) - 4, len(raw)))
    if len(raw) >= 2 and raw[0] == 0x0F and 0x80 <= raw[1] <= 0x8F:
        return set(range(len(raw) - 4, len(raw)))
    return set()


def slot_key(
    slot: le.Relocation,
    symbols: Callable[[le.Relocation], str | None] | None = None,
) -> tuple:
    """A stable identity of a fixup slot.

    An internal target is an object number and an object offset.  When both
    images resolve the slot to a symbol, the key holds the symbol and the
    addend, so a moved target stays equal.  A missing addend counts as zero.
    A target that resolves on only one side is a documented limitation: the
    two images have no common symbol table, so the key falls back to the kind.
    An imported target carries a stable module and ordinal or name offset.
    """
    if slot.kind == "import-ordinal":
        return (slot.kind, slot.module, slot.ordinal)
    if slot.kind == "import-name":
        return (slot.kind, slot.module, slot.name_offset)
    if slot.kind == "entry":
        return (slot.kind, slot.ordinal)
    if symbols is not None:
        symbol = symbols(slot)
        if symbol is not None:
            return (slot.kind, symbol, slot.addend or 0)
    return (slot.kind,)


def _slots_for(
    insn: le.Insn, relocations: list[le.Relocation]
) -> list[tuple[int, le.Relocation]]:
    """The relocation slots that overlap one instruction, by relative offset."""
    found = []
    end = insn.address + insn.size
    for slot in relocations:
        if slot.offset + slot.width <= insn.address or slot.offset >= end:
            continue
        found.append((slot.offset - insn.address, slot))
    found.sort(key=lambda item: item[0])
    return found


def _mask_for(
    instructions: list[le.Insn], relocations: list[le.Relocation]
) -> list[set[int]]:
    masks = []
    for insn in instructions:
        mask = set(structural_mask(insn))
        for relative, slot in _slots_for(insn, relocations):
            for offset in range(slot.width):
                position = relative + offset
                if 0 <= position < insn.size:
                    mask.add(position)
        masks.append(mask)
    return masks


def _bounds(instructions: list[le.Insn]) -> tuple[int, int]:
    """The half-open address range of a function body."""
    if not instructions:
        return (0, 0)
    last = instructions[-1]
    return (instructions[0].address, last.address + last.size)


def _index_by_address(instructions: list[le.Insn]) -> dict[int, int]:
    """Map each instruction address to its position in the body."""
    return {insn.address: index for index, insn in enumerate(instructions)}


def _branch_target(insn: le.Insn) -> int | None:
    """The direct jump or loop target, or None for a call or other instruction."""
    mnemonic = insn.mnemonic.lower()
    if not (mnemonic.startswith("j") or mnemonic.startswith("loop")):
        return None
    operand = insn.operands.strip()
    if not operand.startswith("0x"):
        return None
    return int(operand, 16)


def _branch_difference(
    original: le.Insn,
    rebuilt: le.Insn,
    original_bounds: tuple[int, int],
    rebuilt_bounds: tuple[int, int],
    original_index: dict[int, int],
    rebuilt_index: dict[int, int],
) -> str | None:
    """Compare two branch targets by their instruction position."""
    original_target = _branch_target(original)
    rebuilt_target = _branch_target(rebuilt)
    if original_target is None or rebuilt_target is None:
        return None
    if not original_bounds[0] <= original_target < original_bounds[1]:
        return None
    if not rebuilt_bounds[0] <= rebuilt_target < rebuilt_bounds[1]:
        return None
    left = original_index.get(original_target)
    if left is None:
        return (
            f"branch target: {original_target:#x} is not an instruction address"
        )
    right = rebuilt_index.get(rebuilt_target)
    if right is None:
        return (
            f"branch target: {rebuilt_target:#x} is not an instruction address"
        )
    if left != right:
        return f"branch target: instruction {left} against {right}"
    return None


def _instruction_difference(
    original: le.Insn,
    rebuilt: le.Insn,
    original_mask: set[int],
    rebuilt_mask: set[int],
    original_slots: list[tuple[int, le.Relocation]],
    rebuilt_slots: list[tuple[int, le.Relocation]],
    original_bounds: tuple[int, int],
    rebuilt_bounds: tuple[int, int],
    original_index: dict[int, int],
    rebuilt_index: dict[int, int],
    original_symbols: Callable[[le.Relocation], str | None] | None = None,
    rebuilt_symbols: Callable[[le.Relocation], str | None] | None = None,
) -> str | None:
    """Return the first difference of one instruction pair, or None."""
    if original.mnemonic != rebuilt.mnemonic:
        return f"mnemonic: {original.mnemonic!r} against {rebuilt.mnemonic!r}"
    if original.size != rebuilt.size:
        return f"size: {original.size} against {rebuilt.size}"
    if original_mask != rebuilt_mask:
        return (
            f"mask: {sorted(original_mask)} against {sorted(rebuilt_mask)}"
        )
    for index in range(original.size):
        if index in original_mask:
            continue
        if original.raw[index] != rebuilt.raw[index]:
            return (
                f"byte {index}: {original.raw[index]:#04x} against "
                f"{rebuilt.raw[index]:#04x}"
            )
    branch = _branch_difference(
        original,
        rebuilt,
        original_bounds,
        rebuilt_bounds,
        original_index,
        rebuilt_index,
    )
    if branch is not None:
        return branch
    if len(original_slots) != len(rebuilt_slots):
        return (
            f"relocations: {len(original_slots)} against {len(rebuilt_slots)}"
        )
    for (original_relative, left), (rebuilt_relative, right) in zip(
        original_slots, rebuilt_slots
    ):
        if original_relative != rebuilt_relative:
            return (
                f"relocation offset: {original_relative} against "
                f"{rebuilt_relative}"
            )
        if left.width != right.width:
            return f"relocation width: {left.width} against {right.width}"
        left_symbol = (
            original_symbols(left) if original_symbols is not None else None
        )
        right_symbol = (
            rebuilt_symbols(right) if rebuilt_symbols is not None else None
        )
        if left_symbol is not None and right_symbol is not None:
            left_key = slot_key(left, original_symbols)
            right_key = slot_key(right, rebuilt_symbols)
        else:
            left_key = slot_key(left)
            right_key = slot_key(right)
        if left_key != right_key:
            return f"relocation target: {left_key} against {right_key}"
    return None


def compare(
    original: list[le.Insn],
    rebuilt: list[le.Insn],
    *,
    original_relocations: list[le.Relocation] | None = None,
    rebuilt_relocations: list[le.Relocation] | None = None,
    original_symbols: Callable[[le.Relocation], str | None] | None = None,
    rebuilt_symbols: Callable[[le.Relocation], str | None] | None = None,
) -> OracleResult:
    """Compare two function bodies and classify the difference.

    A masked byte is a byte that a fixup slot or a direct branch target
    controls.  The comparison requires the same mask on both sides, compares
    every unmasked byte, and compares the relocation kind, width, and stable
    target identity.  An internal target compares by symbol and addend when
    both sides resolve a symbol, and by kind otherwise.  A one-sided symbol is
    a documented limitation: the two images have no common symbol table for it.
    A branch target that stays inside its own function body compares by
    instruction position; a target outside the body stays masked.  Instruction
    order is significant.
    """
    original_slots = original_relocations or []
    rebuilt_slots = rebuilt_relocations or []
    original_masks = _mask_for(original, original_slots)
    rebuilt_masks = _mask_for(rebuilt, rebuilt_slots)
    original_bounds = _bounds(original)
    rebuilt_bounds = _bounds(rebuilt)
    original_index = _index_by_address(original)
    rebuilt_index = _index_by_address(rebuilt)

    original_bytes = b"".join(insn.raw for insn in original)
    rebuilt_bytes = b"".join(insn.raw for insn in rebuilt)
    bytes_equal = sum(
        1 for left, right in zip(original_bytes, rebuilt_bytes) if left == right
    )
    bytes_total = max(len(original_bytes), len(rebuilt_bytes))
    instructions_total = max(len(original), len(rebuilt))

    differences: list[str] = []
    instructions_equal = 0
    relocations_equal = 0
    relocations_total = 0
    if len(original) == len(rebuilt):
        for index, (left, right) in enumerate(zip(original, rebuilt)):
            left_slots = _slots_for(left, original_slots)
            right_slots = _slots_for(right, rebuilt_slots)
            relocations_total += max(len(left_slots), len(right_slots))
            difference = _instruction_difference(
                left,
                right,
                original_masks[index],
                rebuilt_masks[index],
                left_slots,
                right_slots,
                original_bounds,
                rebuilt_bounds,
                original_index,
                rebuilt_index,
                original_symbols,
                rebuilt_symbols,
            )
            if difference is None:
                instructions_equal += 1
                relocations_equal += len(left_slots)
                continue
            differences.append(f"instruction {index}: {difference}")

    if not differences and len(original) == len(rebuilt):
        if original_bytes == rebuilt_bytes:
            match = EXACT_BYTES
        else:
            match = EXACT_RELOCATIONS
    elif len(original) == len(rebuilt) and all(
        left.mnemonic == right.mnemonic for left, right in zip(original, rebuilt)
    ):
        match = SAME_SHAPE
    elif sorted(insn.mnemonic for insn in original) == sorted(
        insn.mnemonic for insn in rebuilt
    ):
        match = SAME_MNEMONICS
    else:
        match = DIFFERENT

    return OracleResult(
        match=match,
        bytes_equal=bytes_equal,
        bytes_total=bytes_total,
        instructions_equal=instructions_equal,
        instructions_total=instructions_total,
        relocations_equal=relocations_equal,
        relocations_total=relocations_total,
        differences=differences,
    )
