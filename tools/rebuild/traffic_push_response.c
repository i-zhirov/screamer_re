/* The rebuilt `traffic_push_response` module.
 *
 * The original sits at linear 0x17b54 in STARTH.EXE and holds 70 instructions
 * and 279 bytes up to the `ret` at 0x17c6a.  The next function starts at
 * 0x17c6c, so the store record keeps the 280-byte extent and sets no separate
 * function size.  The identity check uses 70 instructions.
 *
 * The only direct caller is the traffic collision pass at 0x175dc.  It calls
 * this function at 0x1775a with the zero flag on the velocity branch, and at
 * 0x177de with the set flag on the contact branch.  The pass runs from the
 * traffic effect group at 0x15b44 on the player object at object-3 offset
 * 0xf91c, so the record takes the section `The vehicle state chain`.
 *
 * The function always looks up the table entry 1 through `table_lookup` at
 * 0x1f718 and sets the turn rate at `+0x54c` to 5000.0.  With a zero flag it
 * returns at once.
 *
 * With a non-zero flag it keeps the contact distance at `+0x55c` above zero.
 * It forms the two unit terms at `+0x550` and `+0x558` from the deltas at
 * `+0x564` and `+0x56c` over the distance, and it copies both terms to
 * `+0x80c` and `+0x814`.
 *
 * The function then moves the player position at `+0x4b0` and `+0x4b8` by the
 * contact distance at `+0x560` times the unit term, minus the delta.  It
 * truncates the result toward zero through the helper at 0x4d9f4.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * module converts a float to an integer, so the local
 * `traffic_push_response_fpu.asm` makes `__CHP` truncate toward zero.
 */
extern int table_lookup(int index);

void traffic_push_response(char *self, int push)
{
    table_lookup(1);
    *(float *)(self + 0x54c) = 5000.0f;
    if (push == 0) {
        return;
    }
    if (*(int *)(self + 0x55c) == 0) {
        *(int *)(self + 0x55c) = 1;
    }
    *(float *)(self + 0x550) = (float)*(int *)(self + 0x564)
        / (float)*(int *)(self + 0x55c);
    *(float *)(self + 0x80c) = *(float *)(self + 0x550);
    *(float *)(self + 0x558) = (float)*(int *)(self + 0x56c)
        / (float)*(int *)(self + 0x55c);
    *(float *)(self + 0x814) = *(float *)(self + 0x558);
    *(int *)(self + 0x4b0) = (int)(
        ((double)*(int *)(self + 0x560) * (double)*(float *)(self + 0x550)
         - (double)*(int *)(self + 0x564))
        + (double)*(int *)(self + 0x4b0));
    *(int *)(self + 0x4b8) = (int)(
        ((double)*(int *)(self + 0x560) * (double)*(float *)(self + 0x558)
         - (double)*(int *)(self + 0x56c))
        + (double)*(int *)(self + 0x4b8));
}
