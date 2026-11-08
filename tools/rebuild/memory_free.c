/* The rebuilt `memory_free` module.
 *
 * The original sits at linear 0x2f205 in STARTH.EXE and holds 33 instructions
 * and 90 bytes: the five register saves, the frame, the two argument slots, the
 * local register block and segment set, the `dpmi_fill_regs` call at `0x4bf23`,
 * the `dpmi_call` call at `0x4bf4b`, and the epilogue.
 *
 * The function calls the DPMI function 0x0502 (free memory block).  The local
 * block holds 0x0502 at offset 0, the high address word at offset 0x10, and the
 * low address word at offset 0x14.  The same block receives the result, and the
 * function returns the status dword at offset 0x18.  The first argument is not
 * used.
 */
extern void dpmi_fill_regs(void *segments);
extern int dpmi_call(int vector, void *regs, void *result, void *segments);

int memory_free(int handle, unsigned int address)
{
    unsigned int block[7];
    unsigned int segments[3];

    block[0] = 0x502;
    block[4] = address >> 16;
    block[5] = address & 0xffff;
    dpmi_fill_regs(segments);
    dpmi_call(0x31, block, block, segments);
    return block[6];
}
