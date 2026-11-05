/* The rebuilt `panel_draw_centered` module.
 *
 * The original sits at linear 0x2e7cc in STARTH.EXE and holds 34 instructions
 * and 93 bytes: the frame, the four argument slots, the size measure at
 * `0x254a8`, the signed halving, the centering on the 0x140 screen width, the
 * doubled draw call at `0x25020`, and the epilogue.
 *
 * The function measures the text, halves the measured width, centers that width
 * on 0x140, and draws the text with the doubled position.  The measurement
 * takes the two output pointers, the text argument, and the font argument.  The
 * draw takes the position, the text argument, the font argument, and the fifth
 * argument from the stack.
 */
extern void text_measure(int *width, int *height, int text, int font);
extern void text_draw(int x, int y, int text, int font, int color);

void panel_draw_centered(int text, int y, int font, int color)
{
    int width;
    int height;
    int x;

    text_measure(&width, &height, text, font);
    width = width / 2;
    x = (0x140 - width) / 2;
    text_draw(x + x, y + y, text, font, color);
}
