/* The rebuilt `dpmi_free_dos` module.
 *
 * The original sits at linear 0x2666c in STARTH.EXE.  The body holds 6
 * instructions and 20 bytes up to the `ret` at 0x2667f.  The next recorded
 * function starts at 0x27700, so the store record keeps the 4244-byte extent
 * and sets no separate function size.  The identity check uses 6 instructions.
 *
 * The only direct caller is the wrapper at 0x25f14.  It saves the registers,
 * calls `sound_driver_set(0)` at 0x262a8, calls this function at 0x25f1c,
 * restores the registers, and returns.  The caller is not a record yet.
 *
 * The body loads the DPMI function 0x0101 in EAX and the word at
 * `DAT_001666a2` in DX.  The word holds the real-mode segment of the DOS
 * memory block that the helper at 0x26640 allocates through the DPMI function
 * 0x0100.  When the segment is zero the body returns at once.  Otherwise it
 * runs INT 31h to free the block and returns.
 *
 * The paired allocator, the segment cell, and the DPMI function support the
 * name `dpmi_free_dos`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.  `engine_shutdown` at 0x128ac calls the
 * wrapper at 0x128d9, so the data flow gives the section Subsystem shutdown.
 *
 * The original has no EBP frame and holds no floating-point instruction.  C
 * cannot issue the INT 31h instruction, so the module calls the
 * `dpmi_free_dos_block` helper.  The module uses no assembly source and no FPU
 * stub.
 */
extern unsigned short DAT_001666a2;
extern void dpmi_free_dos_block(unsigned short segment);

void dpmi_free_dos(void)
{
    unsigned short segment;

    segment = DAT_001666a2;
    if (segment != 0) {
        dpmi_free_dos_block(segment);
    }
}
