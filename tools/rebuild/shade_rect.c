/* The rebuilt `shade_rect` module.
 *
 * The original sits at linear 0x25d28 in STARTH.EXE and holds 41 instructions
 * and 125 bytes up to the `ret` at 0x25da4.  The next record starts at
 * 0x25da8, so the store record keeps the 128-byte extent and sets no separate
 * function size.  The identity check uses 41 instructions.
 *
 * The function takes four arguments in the Watcom register order: `eax` holds
 * the x, `edx` holds the y, `ebx` holds the width, and `ecx` holds the height.
 * The callers set the four arguments before the call; for example
 * `options_screen` passes the full screen 0x280 by 0x190 at 0x2134c, and
 * `race_records_screen` passes the rectangle (0x60, 0x48, 0x1c4, 0x140) at
 * 0x2d4cc.
 *
 * The x and the width are rounded down to a multiple of four.  The function
 * walks the rectangle row by row in the framebuffer at `DAT_00121528`.  Each
 * pixel byte becomes the entry of the 0x100-byte shade table at `DAT_0015d1c`.
 * `palette_shade_build` at 0x258d0 sets `DAT_0015d1c` to the 0x1cc00 table, so
 * the rectangle takes the 0.75 shade map of the palette.
 *
 * The two cells at 0x15d20 and 0x15d24 hold the word count and the row stride.
 * The function reads and writes only those cells, so they are spill slots and
 * not module state; the rebuilt function keeps both values in locals.
 *
 * The original uses `pushal` and `popal` and has no EBP frame.  The function
 * holds no floating-point instruction, calls no function, and needs no FPU
 * stub and no assembly.
 */
extern unsigned char *DAT_0015d1c;
extern unsigned char *DAT_00121528;

void shade_rect(int x, int y, int width, int height)
{
    int words;
    int stride;
    int column;
    int row;
    unsigned char *dest;
    unsigned char *table;

    x &= 0xfffc;
    width &= 0xfffc;
    words = width >> 2;
    stride = 0x280 - width;
    dest = DAT_00121528 + y * 0x280 + x;
    table = DAT_0015d1c;
    row = height;
    do {
        column = words;
        do {
            dest[0] = table[dest[0]];
            dest[1] = table[dest[1]];
            dest[2] = table[dest[2]];
            dest[3] = table[dest[3]];
            dest += 4;
        } while (--column != 0);
        dest += stride;
    } while (--row > 0);
}
