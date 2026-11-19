/* The rebuilt `memory_alloc` module at 0x2f172.
 *
 * The original sits at linear 0x2f172 in STARTH.EXE and holds 49 instructions
 * and 147 bytes: the four register saves, the frame, the 0x3c-byte local area,
 * the DPMI function 0x0501 setup, the `dpmi_fill_regs` call at `0x4bf23`, the
 * `dpmi_call` call at `0x4bf4b`, the status test at block offset `0x18`, the
 * address reassembly, the size reassembly, and the epilogue.
 *
 * The function calls the DPMI function 0x0501 (allocate memory block).  The
 * size splits into the high word at block offset 4 and the low word at block
 * offset 8.  The call returns the block address in the words at block offsets
 * `0x10` and `0x14` and the status at block offset `0x18`.  A non-zero status
 * returns 0 and leaves the third argument.  Otherwise the function stores the
 * reassembled address at the third argument and returns the reassembled size
 * words.  The first argument is not used.
 *
 * The module name is `memory_alloc_small`, because the `memory_alloc` recipe
 * belongs to the different function at `0x30050`.
 *
 * The C form with `-od` numbers the locals in the opposite order, so the recipe
 * uses the exact `memory_alloc_small.asm` body.  This C source stays as the
 * readable behavioral reference.
 */
struct dpmi_block {
    unsigned int block[7];
    unsigned int segments[3];
};

extern void dpmi_fill_regs(void *segments);
extern int dpmi_call(int vector, void *regs, void *result, void *segments);

int memory_alloc_small(int handle, int size, unsigned int *address)
{
    struct dpmi_block regs;
    unsigned int value;

    regs.block[0] = 0x501;
    regs.block[1] = size >> 16;
    regs.block[2] = size & 0xffff;
    dpmi_fill_regs(regs.segments);
    dpmi_call(0x31, regs.block, regs.block, regs.segments);
    if (regs.block[6] != 0) {
        return 0;
    }
    *address = (regs.block[4] << 16) | (regs.block[5] & 0xffff);
    value = (regs.block[1] << 16) | (regs.block[2] & 0xffff);
    return value;
}
