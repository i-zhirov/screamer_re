/* The rebuilt `race_panel_4` module.
 *
 * The original sits at linear 0x2e724 in STARTH.EXE and holds 50 instructions
 * and 167 bytes: the four register saves, the frame, the 0x94-byte local
 * block, the centered title call at `0x2e7cc`, the `sprintf` call at `0x4d32c`
 * with the format `\001%s`, the first draw call at `0x25020`, the x shift from
 * the count at `DAT_0011d244`, the second draw call at `0x25020`, and the
 * epilogue.
 *
 * The function draws the title that the pointer at `DAT_0011fc38` selects.  It
 * formats the player name at `DAT_001203cc` into the local buffer and draws the
 * text at (0x106, 0x98).  Then it shifts the x position by 21 pixels for each
 * count unit and draws the separator `\001-` one row lower.
 */
extern char *DAT_0011fc38;
extern unsigned int DAT_0011d244;
extern char DAT_001203cc[];
extern void panel_draw_centered(char *text, int y, int font, int color);
extern int sprintf(char *buffer, const char *format, ...);
extern void draw(int x, int y, char *text, int font, int color);

void race_panel_4(void)
{
    char buffer[0x80];
    int x;
    int y;

    panel_draw_centered(DAT_0011fc38, 2, 3, 2);
    sprintf(buffer, "\001%s", DAT_001203cc);
    x = 0x83;
    y = 0x4c;
    draw(x + x, y + y, buffer, 2, 2);
    y = y + 0xc;
    x = x + DAT_0011d244 * 21;
    draw(x + x, y + y, "\001-", 2, 2);
}
