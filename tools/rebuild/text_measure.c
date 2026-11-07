/* The rebuilt `text_measure` module.
 *
 * The original sits at linear 0x254a8 in STARTH.EXE and holds 104 instructions
 * and 360 bytes up to the `ret` at 0x2560f.  The next function starts at
 * 0x25610, so the store record keeps the 376-byte extent and sets no separate
 * function size.  The identity check uses 104 instructions.
 *
 * The four arguments arrive in the Watcom register order: the width pointer in
 * eax, the height pointer in edx, the text pointer in ebx, and the font index
 * in ecx.  The function saves the four registers and restores them at the end,
 * so it writes its results only through the two output pointers.
 *
 * The font index selects a 12-byte record from the table at DAT_00015438.  The
 * record holds the height at +0x00, the first character at +0x04, and the
 * character count at +0x08.  The same index selects a 0x500-byte glyph row from
 * the table at DAT_00015434.  Each glyph holds its advance width in the first
 * 32-bit slot of 8 bytes.
 *
 * The function scans the glyph row for the widest advance and adds 2.  It then
 * walks the text byte by byte.  A visible character adds its own advance, or
 * the widest advance plus 2 when the monospace mode is on.  The control bytes 1
 * and 2 clear and set the monospace mode, and the control bytes 3 and 4 clear
 * and set the widened mode that adds one pixel to each glyph advance.
 */
extern char *DAT_00015434;
extern char *DAT_00015438;
extern int DAT_00015440;

void text_measure(int *width, int *height, char *text, int font)
{
    char *glyphs;
    char *record;
    int first;
    int count;
    int widest;
    int monospace;
    int widened;
    int index;
    int character;

    if ((unsigned int)font >= (unsigned int)DAT_00015440) {
        return;
    }

    record = DAT_00015438 + font * 0xc;
    *height = *(int *)record;
    first = *(int *)(record + 4);
    count = *(int *)(record + 8);

    glyphs = DAT_00015434 + font * 0x500;
    widest = 0;
    for (index = 0; index < count; index++) {
        int candidate;

        candidate = *(int *)(glyphs + index * 8);
        if (widest <= candidate) {
            widest = candidate;
        }
    }
    widest = widest + 2;

    monospace = 1;
    widened = 1;
    *width = 0;
    for (;;) {
        character = (unsigned char)*text;
        if (character == 0) {
            break;
        }
        if (character >= first && character < first + count) {
            char *glyph;
            int advance;

            glyph = glyphs + (character - first) * 8;
            if ((monospace & 0xf) != 0) {
                advance = *(int *)glyph;
                if ((widened & 0xf) != 0) {
                    advance = advance + 1;
                }
                advance = advance + 1;
            } else {
                advance = widest;
            }
            *width = *width + advance;
        }
        if (character == 1) {
            monospace = 0;
        } else if (character == 2) {
            monospace = character;
        } else if (character == 3) {
            widened = 0;
        } else if (character == 4) {
            widened = character;
        }
        text = text + 1;
    }
}
