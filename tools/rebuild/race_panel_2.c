/* The rebuilt `race_panel_2` module.
 *
 * The original sits at linear 0x2e21c in STARTH.EXE and holds 265 instructions
 * and 879 bytes: the five register saves, the frame, the 0x94-byte local
 * block, the centered title call at `0x2e7cc`, the two measured table texts,
 * the three `text_measure` calls at `0x254a8`, the six `text_draw` calls at
 * `0x25020`, the `sprintf` call at `0x4d32c` with the format `%d KMH`, the four
 * `panel_show` calls at `0x2dfac`, the two floating-point threshold compares,
 * and the epilogue.  The merged store record keeps the 880-byte extent.
 *
 * The index `DAT_001203c4` selects a 0x44-byte record from the table at object-3
 * offset `0x1d550`.  The text pointer at `+0x14` is measured with the font 0,
 * halved with a signed division, and drawn at the doubled centered position.
 * The field at `+0x30` selects a text pointer from the table at `0x1fc14`, and
 * that text is measured and drawn the same way with a y of `0xc8 - 5 * height`.
 *
 * The function then builds a string of `DAT_001203b4` letters `A`, measures it
 * with the font 1, centers the half width plus 0x40, and draws the title at
 * `DAT_0011fc1c`.  The field at `+0x34` goes through `sprintf` with `%d KMH` and
 * is drawn at `(x + saved) * 2`.  The titles at `DAT_0011fc20` and
 * `DAT_0011fc24` follow one half height apart, and each one has a panel at
 * `(x + saved, y + 1)` with the size `field + 4` and `field + 9`.
 *
 * The two state panels match `race_panel_3`: the first panel at (8, 0x64)
 * appears when the index is not zero, and the second panel at (0xee, 0x64)
 * appears when `DAT_001207f0` plus 11 differs from the index.  The sizes 2 and
 * 3 need `DAT_0012037c` above the 0.6 threshold.  The threshold is the double
 * at object-3 offset 0x12e7, linear 0x1012e7.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  It has no
 * conversion to an integer, so it uses the shared `fpu_stub.asm`.
 */
extern char *DAT_0011fc30;
extern int DAT_001203c4;
extern unsigned char *DAT_0011d550;
extern char *DAT_0011fc14[];
extern char *DAT_0011fc1c;
extern char *DAT_0011fc20;
extern char *DAT_0011fc24;
extern int DAT_001203b4;
extern int DAT_0012039c;
extern int DAT_001203a8;
extern float DAT_0012037c;
extern int DAT_001207f0;
extern double race_panel_2_threshold;
extern void panel_draw_centered(char *text, int y, int font, int color);
extern void panel_show(int x, int y, int size);
extern void text_measure(int *width, int *height, char *text, int font);
extern void text_draw(int x, int y, char *text, int font, int color);
extern int sprintf(char *buffer, const char *format, ...);

void race_panel_2(void)
{
    char buffer[0x80];
    int width;
    int height;
    int x;
    int y;
    int saved;
    char *text;
    int index;
    int count;
    int size;

    panel_draw_centered(DAT_0011fc30, 2, 3, 2);

    index = DAT_001203c4;
    text = (char *)(DAT_0011d550 + index * 0x44) + 0x14;
    text_measure(&width, &height, text, 0);
    width = width / 2;
    height = height / 2;
    x = (0x140 - width) / 2;
    y = height * 3;
    text_draw(x + x, y + y, text, 0, 2);

    text = DAT_0011fc14[*(int *)(DAT_0011d550 + index * 0x44 + 0x30)];
    text_measure(&width, &height, text, 0);
    width = width / 2;
    height = height / 2;
    x = (0x140 - width) / 2;
    y = 0xc8 - height * 5;
    text_draw(x + x, y + y, text, 0, 2);

    for (count = 0; count < DAT_001203b4; count++) {
        buffer[count] = 'A';
    }
    buffer[count] = 0;
    text_measure(&width, &height, buffer, 1);
    width = width / 2;
    height = height / 2;
    saved = width;
    width = width + 0x40;
    x = (0x140 - width) / 2;
    y = 0xd2 - height * 5;
    text_draw(x + x, y + y, DAT_0011fc1c, 1, 2);
    sprintf(buffer, "%d KMH", *(int *)(DAT_0011d550 + index * 0x44 + 0x34));
    text_draw((x + saved) * 2, y + y, buffer, 1, 2);
    y = y + height;
    text_draw(x + x, y + y, DAT_0011fc20, 1, 2);
    panel_show(x + saved, y + 1,
        *(int *)(DAT_0011d550 + index * 0x44 + 0x38) + 4);
    y = y + height;
    text_draw(x + x, y + y, DAT_0011fc24, 1, 2);
    panel_show(x + saved, y + 1,
        *(int *)(DAT_0011d550 + index * 0x44 + 0x3c) + 9);

    if (index != 0) {
        if (DAT_0012039c < DAT_001203a8
            && DAT_0012037c > race_panel_2_threshold) {
            size = 2;
        } else {
            size = 0;
        }
        panel_show(8, 0x64, size);
    }

    if (DAT_001207f0 + 11 != index) {
        if (DAT_0012039c > DAT_001203a8
            && DAT_0012037c > race_panel_2_threshold) {
            size = 3;
        } else {
            size = 1;
        }
        panel_show(0xee, 0x64, size);
    }
}
