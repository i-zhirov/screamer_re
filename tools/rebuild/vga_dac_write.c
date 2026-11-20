/* The rebuilt `vga_dac_write` module.
 *
 * The original sits at linear 0x81a24 in STARTH.EXE and holds 9 instructions
 * and 21 bytes up to the `ret` at 0x81a38.  The next record starts at 0x81a70,
 * so the store record keeps the 76-byte extent and sets no separate function
 * size.  The identity check uses 9 instructions.
 *
 * The palette pointer arrives in ESI.  The function clears the direction flag,
 * writes the DAC write index 0 to port 0x3c8, writes the DAC data port 0x3c9
 * to DX, loads the count 0x300 in ECX, and sends 0x300 bytes from [ESI] to the
 * port with the outsb and loop pair.
 *
 * The only direct caller is `palette_set` at 0x81a05 with the call site at
 * 0x81a15.  It passes the palette pointer in ESI, so the record names the
 * function `vga_dac_write` with the confidence inferred.  The caller sits in
 * the section Program functions, so the record takes the same section.
 *
 * The original has no EBP frame and no floating-point instruction.  C cannot
 * emit the `outsb` instruction, so the module calls the `outportb` helper once
 * per byte.  The module needs no `-of+`, no assembly source, and no FPU stub.
 */
#include <i86.h>

extern void outportb(unsigned short port, unsigned char value);

void vga_dac_write(char *palette)
{
    int index;

    outportb(0x3c8, 0);
    for (index = 0; index < 0x300; ++index) {
        outportb(0x3c9, (unsigned char)palette[index]);
    }
}
