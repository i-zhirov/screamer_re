/* The rebuilt `player_state_snapshot` module.
 *
 * The original sits at linear 0x1ed04 in STARTH.EXE and holds 115 instructions
 * and 465 bytes up to the `ret` at 0x1eed4.  The next function starts at
 * 0x1eed8, so the store record keeps the 468-byte extent and sets no function
 * size.  The identity check uses 115 instructions.
 *
 * The only direct caller at 0x1ec88 is the mode-10 step body at 0x1ec7c.  That
 * body loads the player object at object-3 offset 0xf91c in `eax` and then
 * calls this function.  The mode-10 step at 0x1eae0 calls the body at the start
 * and again after each car change, and the race session start at 0x12d0c calls
 * the step.
 *
 * The function copies the current player fields into the snapshot block at
 * `+0x7e8` through `+0x838`:
 *
 *     +0x7e8  the global at 0x11521c
 *     +0x7ec  the global at 0x11d250
 *     +0x7f0  the state flag, the mode flag, and the input flag
 *     +0x7f4  the field at +0x7b0
 *     +0x7f8  the global at 0x11d2a0
 *     +0x7fc  the field at +0x684
 *     +0x800  the field at +0x688
 *     +0x804  the field at +0x680
 *     +0x808  the field at +0x78c
 *     +0x818  the view offset at +0x4b0 plus the delta at +0x130
 *     +0x81c  the view offset at +0x4b4 plus the delta at +0x134
 *     +0x820  the view offset at +0x4b8 plus the delta at +0x138
 *     +0x824  the angle at +0x4c8 plus the delta at +0x124
 *     +0x828  the angle at +0x4cc plus the delta at +0x128
 *     +0x82c  the angle at +0x4d0 plus the delta at +0x12c
 *     +0x830  the height at +0x4ec minus 2
 *
 * The state flag at `+0x148` sets bit `0x10000`.  The input byte at 0x118e1c
 * bit 4 sets bit `0x20000` unless the input byte at 0x118e1d bit 4 is also
 * set.  When the state field is not 1, the function also copies `+0x824` and
 * `+0x82c` to `+0x834` and `+0x838`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * local `fpu_stub.asm` supplies the FPU run-time symbols.
 */
extern int DAT_0011521c;
extern int DAT_0011d250;
extern int DAT_0011d2a0;
extern unsigned char DAT_00118e1c;
extern unsigned char DAT_00118e1d;

void player_state_snapshot(char *self)
{
    unsigned int flags;
    unsigned int extra;

    *(int *)(self + 0x7e8) = DAT_0011521c;
    *(int *)(self + 0x7ec) = DAT_0011d250;

    extra = 0;
    if (*(int *)(self + 0x148) != 0) {
        extra = 0x10000;
    }
    flags = *(int *)(self + 0x2fc) | extra;
    flags |= *(int *)(self + 0x7b4);
    extra = 0;
    if ((DAT_00118e1c & 4) != 0 && (DAT_00118e1d & 0x10) == 0) {
        extra = 0x20000;
    }
    *(int *)(self + 0x7f0) = flags | extra;

    *(int *)(self + 0x7f4) = *(int *)(self + 0x7b0);
    *(float *)(self + 0x804) = *(float *)(self + 0x680);
    *(int *)(self + 0x800) = *(int *)(self + 0x688);
    *(int *)(self + 0x7fc) = *(int *)(self + 0x684);
    *(int *)(self + 0x7f8) = DAT_0011d2a0;
    *(float *)(self + 0x808) = *(float *)(self + 0x78c);

    *(int *)(self + 0x818) = *(int *)(self + 0x4b0)
        + *(int *)(self + 0x130);
    *(int *)(self + 0x81c) = *(int *)(self + 0x4b4)
        + *(int *)(self + 0x134);
    *(int *)(self + 0x820) = *(int *)(self + 0x4b8)
        + *(int *)(self + 0x138);

    *(float *)(self + 0x824) = *(float *)(self + 0x4c8)
        + *(float *)(self + 0x124);
    *(float *)(self + 0x828) = *(float *)(self + 0x4cc)
        + *(float *)(self + 0x128);
    *(float *)(self + 0x82c) = *(float *)(self + 0x4d0)
        + *(float *)(self + 0x12c);

    *(int *)(self + 0x830) = *(int *)(self + 0x4ec) - 2;

    if (*(int *)(self + 0x148) != 1) {
        *(float *)(self + 0x834) = *(float *)(self + 0x824);
        *(float *)(self + 0x838) = *(float *)(self + 0x82c);
    }
}
