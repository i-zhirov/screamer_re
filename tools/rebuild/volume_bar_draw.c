/* The rebuilt `volume_bar_draw` module.
 *
 * The original sits at linear 0x23dfc in STARTH.EXE and holds 63 instructions
 * and 199 bytes up to the `ret` at 0x23ec2.  The next function starts at
 * 0x23ed0, so the store record keeps the 212-byte extent and sets no separate
 * function size.  The identity check uses 63 instructions.
 *
 * The only direct caller is `options_screen` at 0x2195b.  It calls the function
 * for the selected audio row and passes the volume value in eax and the label
 * pointer in edx.  The label table at object-3 offset 0x1f74 holds `CD`, `FX`,
 * and `MU`, so the caller names the function `volume_bar_draw` with the
 * confidence inferred and the section `race_loop callees`.
 *
 * The function divides the value by 1024 to get the tick count.  It sets the
 * clip rectangle to the full screen, then draws the label at (0x80, 0x8a) with
 * the font 5 and the color 3.  For each tick it takes the color from the table
 * at object-3 offset 0x1fa4 in groups of ten and writes the color to the byte
 * `bar_color`.  The tick is a vertical line at x = 0xaa + index * 6 from y
 * 0x8a to y 0x94.
 *
 * The color table arrives through a global, because the minimal image does not
 * load the initialized data segment.  The original has the EBP frame, so the
 * module builds with `-of+`.  The original uses no floating-point instruction,
 * so the module needs no FPU stub.
 */
extern void text_draw(int x, int y, char *text, int font, int color);
extern void line_draw(int x1, int y1, int x2, int y2);

extern int bar_clip_left;
extern int bar_clip_right;
extern int bar_clip_top;
extern int bar_clip_bottom;
extern unsigned char bar_color;
extern unsigned char volume_bar_colors[4];

void volume_bar_draw(int value, char *label)
{
    int count;
    int index;
    int x;

    count = value / 1024;
    bar_clip_left = 0;
    bar_clip_right = 0x280;
    bar_clip_top = 0;
    bar_clip_bottom = 0x190;
    text_draw(0x80, 0x8a, label, 5, 3);
    for (index = 0; index < count; index++) {
        bar_color = volume_bar_colors[index / 10];
        x = index * 6 + 0xaa;
        line_draw(x, 0x8a, x, 0x94);
    }
}
