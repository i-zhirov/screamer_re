/* The rebuilt `palette_set` module.
 *
 * The original sits at linear 0x81a05 in STARTH.EXE.  The body holds 17
 * instructions and 30 bytes up to the `ret` at 0x81a22.  The next function
 * starts at 0x81a24, so the store record keeps the 31-byte extent and sets no
 * separate function size.  The identity check uses 17 instructions.
 *
 * The function saves all general registers and the segment registers, waits
 * for the VGA retrace through the helper at 0x819f0, and writes the 0x300-byte
 * palette through the helper at 0x81a24.  The palette pointer arrives in EAX.
 *
 * The helper at 0x819f0 polls bit 3 of the input status port 0x3da.  The
 * helper at 0x81a24 writes the DAC write index 0 to port 0x3c8 and sends the
 * 0x300 bytes to the DAC data port 0x3c9.
 *
 * The original has no EBP frame, so the module builds with the default frame
 * option.  The function holds no floating-point instruction, so the module
 * needs no FPU stub.  The module uses no assembly implementation.
 */
extern void vga_retrace_wait(void);
extern void vga_dac_write(char *palette);

void palette_set(char *palette)
{
    vga_retrace_wait();
    vga_dac_write(palette);
}
