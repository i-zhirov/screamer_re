/* The rebuilt `vga_retrace_wait` module.
 *
 * The original sits at linear 0x819f0 in STARTH.EXE and holds 11 instructions
 * and 21 bytes up to the `ret` at 0x81a04.  The next record starts at 0x81a05,
 * so the store record keeps the 21-byte extent and sets no separate function
 * size.  The identity check uses 11 instructions.
 *
 * The body saves all general registers, loads the VGA input status port 0x3da
 * in dx, waits for bit 3 to clear, then waits for bit 3 to set, restores the
 * registers, and returns.  Bit 3 is the vertical retrace flag.
 *
 * The only direct caller is `palette_set` at 0x81a05 with the call site at
 * 0x81a0d.  That module already declares the name `vga_retrace_wait`, so the
 * record keeps the name with the confidence inferred.  The caller sits in the
 * section Program functions, so the record takes the same section.
 *
 * The original has no EBP frame and no floating-point instruction.  C cannot
 * emit the single `in al, dx` read with the port load held outside the loops,
 * so the module calls the `inportb` helper for each read.  The module needs no
 * `-of+`, no assembly source, and no FPU stub.
 */
#include <i86.h>

extern unsigned char inportb(unsigned short port);

void vga_retrace_wait(void)
{
    while ((inportb(0x3da) & 8) != 0) {
    }
    while ((inportb(0x3da) & 8) == 0) {
    }
}
