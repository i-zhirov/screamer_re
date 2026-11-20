/* The rebuilt `sound_render_branch_less` module.
 *
 * The original sits at linear 0x8afb2 in STARTH.EXE.  The body holds 5
 * instructions and 18 bytes up to the `ret` at 0x8afc3.  The next recorded
 * function starts at 0x8afc4, so the store record keeps the 18-byte extent and
 * sets no separate function size.  The identity check uses 5 instructions.
 *
 * The body loads 0x7c in AL and stores AL to the bytes at linear 0x83853,
 * 0x83907, and 0x83bc7.  Each byte is the opcode of a conditional jump in a
 * tail block of `sound_event_render` at 0x836ac.  The value 0x7c is the `jl`
 * opcode, and the value 0x7d is the `jge` opcode.  The sibling at 0x8afa0
 * writes 0x7d to the same three bytes.
 *
 * The on-disk operand of each store is the object-relative value 0x73853,
 * 0x73907, and 0x73bc7, which `le.py` prints.  The fixup adds the object 1
 * base 0x10000, so the run-time byte sits at 0x83853, 0x83907, and 0x83bc7.
 * The Ghidra reference list names the three cells `uRam00083853`,
 * `uRam00083907`, and `uRam00083bc7`.
 *
 * The only direct caller is `sound_source_frame_prepare` at 0x8affc with the
 * call site at 0x8b054.  The caller runs this function on every path.  When
 * the sample offset at 0x74528 is not negative, it then calls the sibling at
 * 0x8afa0.  The caller sits in the section `object_update_all callees`, so the
 * record takes the same section.
 *
 * The three code bytes and the sibling value support the name
 * `sound_render_branch_less`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has no EBP frame and holds no floating-point instruction.  The
 * rebuild models the three code bytes as three globals, because the rebuilt
 * `sound_event_render` uses C conditions.  The module uses no assembly source
 * and no FPU stub.
 */
extern unsigned char render_branch_83853;
extern unsigned char render_branch_83907;
extern unsigned char render_branch_83bc7;

void sound_render_branch_less(void)
{
    render_branch_83853 = 0x7c;
    render_branch_83907 = 0x7c;
    render_branch_83bc7 = 0x7c;
}
