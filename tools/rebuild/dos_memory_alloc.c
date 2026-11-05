/* The rebuilt `dos_memory_alloc` module.
 *
 * The original sits at linear 0x2f0fc in STARTH.EXE and holds 41 instructions
 * and 118 bytes: the four register saves, the frame, the 0x3c-byte local area,
 * the DPMI function 0x0100 setup, the `dpmi_fill_regs` call at `0x4bf23`, the
 * `dpmi_call` call at `0x4bf4b`, the status test, the segment shift, the
 * selector store, and the epilogue.
 *
 * The function calls the DPMI function 0x0100 (allocate DOS memory block).  The
 * paragraph count is `(size >> 4) + 1`.  The call returns the real-mode segment
 * in the result word 0 and the selector in the result word 3.  A non-zero
 * status returns 0.  Otherwise the function returns the segment times 16 and
 * stores the selector at the third argument.  The first argument is not used.
 */
extern void dpmi_fill_regs(void *segments);
extern int dpmi_call(int vector, void *regs, void *result, void *segments);

int dos_memory_alloc(int handle, unsigned int size, unsigned int *selector)
{
    unsigned int block[7];
    unsigned int segments[3];
    unsigned int linear;

    block[0] = 0x100;
    block[1] = (size >> 4) + 1;
    dpmi_fill_regs(segments);
    dpmi_call(0x31, block, block, segments);
    if (block[6] != 0) {
        return 0;
    }
    linear = block[0] << 4;
    *selector = block[3] & 0xffff;
    return linear;
}
