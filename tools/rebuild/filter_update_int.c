/* The rebuilt `filter_update_int` module.
 *
 * The original sits at linear 0x22bb4 in STARTH.EXE and holds 21 instructions
 * and 54 bytes up to the `ret 8` at 0x22be7.  The two padding bytes at
 * 0x22bea and 0x22beb precede the next function at 0x22bec, so the store
 * record keeps the 56-byte extent and sets no separate function size.  The
 * identity check uses 21 instructions.
 *
 * The function is the integer form of the one-pole filter at 0x22b80.  It
 * reads the integer cell, moves it toward the target with the coefficient
 * `DAT_0010cf08[index]`, truncates the result, and writes the integer back.
 * The first argument arrives in `eax`; the float argument makes the Watcom
 * compiler pass the remaining arguments on the stack, so the callee cleans
 * them with `ret 8`.
 *
 * Two rebuilt callers pass the target and the index: `player_view_orbit` at
 * 0x1a3b8 calls it at 0x1a548 for the view height, and `credits_scroll` at
 * 0x27c34 calls it at 0x27c95 for the line position.  The function sits next
 * to the sibling `filter_update`, so the module takes the section The timing
 * and filter layer.
 *
 * The original calls the truncate helper at 0x4d9f4 before the `fistp`.  The
 * shared `fpu_stub.asm` leaves the round-to-nearest mode, so the local
 * `filter_update_int_fpu.asm` makes `__CHP` truncate the value on the FPU
 * stack.  The original has the EBP frame, so the module builds with `-of+`.
 */
extern float *DAT_0010cf08;

void filter_update_int(int *value, float target, int index)
{
    *value = (int)(((float)*value - target) * DAT_0010cf08[index] + target);
}
