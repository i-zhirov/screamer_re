/* The rebuilt `pool_init` module.
 *
 * The original sits at linear 0x2f904 in STARTH.EXE and holds 44 instructions
 * and 129 bytes: the six register saves, the frame, the 0x30-byte local area,
 * the DPMI function 0x0500 setup, the `dpmi_fill_regs` call at `0x4bf23`, the
 * `dpmi_call` call at `0x4bf4b`, the status test, the two page conversions, and
 * the epilogue.
 *
 * The function asks DPMI for the free memory information.  The result buffer is
 * at `pool + 0x1c`.  A non-zero status returns 1.  Otherwise the field at
 * `+0x24` is multiplied by 4096, the field at `+0x3c` is multiplied by 4096 when
 * it is positive and set to 0 otherwise, and the function returns 0.
 */
extern void dpmi_fill_regs(void *segments);
extern int dpmi_call(int vector, void *regs, void *result, void *segments);

int pool_init(char *pool)
{
    unsigned int block[7];
    unsigned int segments[3];

    block[0] = 0x500;
    block[5] = (unsigned int)(pool + 0x1c);
    dpmi_fill_regs(segments);
    dpmi_call(0x31, block, block, segments);
    if (block[6] != 0) {
        return 1;
    }
    *(unsigned int *)(pool + 0x24) <<= 12;
    if (*(int *)(pool + 0x3c) > 0) {
        *(unsigned int *)(pool + 0x3c) <<= 12;
    } else {
        *(unsigned int *)(pool + 0x3c) = 0;
    }
    return 0;
}
