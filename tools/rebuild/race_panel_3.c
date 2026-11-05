/* The rebuilt `race_panel_3` module.
 *
 * The original sits at linear 0x2e58c in STARTH.EXE and holds 120 instructions
 * and 408 bytes: the six register saves, the frame, the 0x94-byte local block,
 * the two centered title calls at `0x2e7cc`, the text measure at `0x254a8`,
 * the signed halving of the measured width and height, the two centering
 * stores, the text draw at `0x25020`, the title test, the two panel calls at
 * `0x2dfac`, and the epilogue.  The merged store record keeps the 408-byte
 * extent.
 *
 * The function draws the race state 3 title at y 2 with the font 3 and the
 * color 2.  It measures the text at `+0x14` of the 0x34-byte record that the
 * index `DAT_001203c8` selects from the table at `0x1d908`, halves the measured
 * width and height, centers the half width on the 0x140 screen width, and draws
 * the text with the doubled position and the font 2 and the color 2.
 *
 * When `DAT_0012081c` stays below `DAT_00120818` and the low six bits of the
 * frame counter `DAT_000007fd` stay below 0x20, it draws the second title at
 * y 0x20 with the font 1 and the color 2.
 *
 * The player row index gates the two panels.  A non-zero index draws the left
 * panel at (8, 0x64) with the size 2 when `DAT_0012039c` stays below
 * `DAT_001203a8` and `DAT_0012037c` stays above 0.6, and with the size 0
 * otherwise.  The second panel at (0xee, 0x64) is skipped when `DAT_00120818`
 * plus 2 equals the index.  It draws the size 3 when `DAT_0012039c` stays above
 * `DAT_001203a8` and `DAT_0012037c` stays above 0.6, and the size 1 otherwise.
 *
 * The threshold 0.6 is the double constant at object-3 offset 0x12ef.  In the
 * rebuilt image the literal constant lands at DGROUP offset 0, and the loader
 * does not relocate that operand, so the rebuild reads the threshold through the
 * global `race_panel_3_threshold` and the harness sets the exact bit pattern.
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * has no conversion to an integer, so it uses the shared `fpu_stub.asm`.
 */
extern char *DAT_0011fc34;
extern char *DAT_0011fc54;
extern unsigned char DAT_0011d908[];
extern int DAT_001203c8;
extern int DAT_0012039c;
extern int DAT_001203a8;
extern float DAT_0012037c;
extern int DAT_00120818;
extern int DAT_0012081c;
extern unsigned int DAT_000007fd;
extern double race_panel_3_threshold;
extern void panel_draw_centered(char *text, int y, int font, int color);
extern void panel_show(int x, int y, int size);
extern void text_measure(int *width, int *height, char *text, int font);
extern void text_draw(int x, int y, char *text, int font, int color);

void race_panel_3(void)
{
    int width;
    int height;
    int x;
    int y;
    char *text;
    int size;

    panel_draw_centered(DAT_0011fc34, 2, 3, 2);

    text = (char *)DAT_0011d908 + DAT_001203c8 * 0x34 + 0x14;
    text_measure(&width, &height, text, 2);
    width = width / 2;
    height = height / 2;
    x = (0x140 - width) / 2;
    y = 0xc8 - height * 2;
    text_draw(x + x, y + y, text, 2, 2);

    if (DAT_0012081c < DAT_00120818 && (DAT_000007fd & 0x3f) < 0x20) {
        panel_draw_centered(DAT_0011fc54, 0x20, 1, 2);
    }

    if (DAT_001203c8 != 0) {
        if (DAT_0012039c < DAT_001203a8 && DAT_0012037c > race_panel_3_threshold) {
            size = 2;
        } else {
            size = 0;
        }
        panel_show(8, 0x64, size);
    }

    if (DAT_00120818 + 2 != DAT_001203c8) {
        if (DAT_0012039c > DAT_001203a8 && DAT_0012037c > race_panel_3_threshold) {
            size = 3;
        } else {
            size = 1;
        }
        panel_show(0xee, 0x64, size);
    }
}
