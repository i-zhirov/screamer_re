/* The test harness for the rebuilt `text_measure` module.
 *
 * The harness holds the two font tables, the font count, and the measured text.
 * The font 0 record holds the height 12, the first character 0x20, and the
 * count 8.  The glyph row holds the advances 5, 7, 3, 2, 6, 4, 1, and 8, so the
 * widest advance plus 2 is 10.
 *
 * The harness runs eight paths:
 *
 *     0  the font 1 is out of range, so the outputs keep their sentinels
 *     1  the text holds two visible characters and the width is 16
 *     2  the control byte 1 selects the monospace advance 10 twice
 *     3  the control byte 2 restores the glyph advance with the widen bit
 *     4  the control byte 3 clears the widen bit and the width is 6
 *     5  the control byte 4 sets the widen bit and the width is 7
 *     6  a character below the first and a character past the range add zero
 *     7  two higher glyphs add the advances 3 and 2 with the widen bit for
 *        the width 9
 *     D  the exit marker
 */
#pragma off (check_stack)

void text_measure(int *width, int *height, char *text, int font);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char glyph_data[2 * 0x500];
char font_data[2 * 0xc];
char *DAT_00015434;
char *DAT_00015438;
int DAT_00015440;

static char text[8];
static char string[16];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void set_glyph(int index, int advance)
{
    *(int *)(glyph_data + index * 8) = advance;
}

static int measure(char *value, int font, int *height)
{
    int width;

    width = -1;
    *height = -1;
    text_measure(&width, height, value, font);
    return width;
}

int main(void)
{
    int height;
    int width;

    DAT_00015434 = glyph_data;
    DAT_00015438 = font_data;
    DAT_00015440 = 1;

    *(int *)(font_data + 0) = 12;
    *(int *)(font_data + 4) = 0x20;
    *(int *)(font_data + 8) = 8;
    set_glyph(0, 5);
    set_glyph(1, 7);
    set_glyph(2, 3);
    set_glyph(3, 2);
    set_glyph(4, 6);
    set_glyph(5, 4);
    set_glyph(6, 1);
    set_glyph(7, 8);

    string[0] = 0x20;
    string[1] = 0x21;
    string[2] = 0;
    width = measure(string, 1, &height);
    check('0', width == -1 && height == -1);

    string[0] = 0x20;
    string[1] = 0x21;
    string[2] = 0;
    width = measure(string, 0, &height);
    check('1', width == 16 && height == 12);

    string[0] = 0x01;
    string[1] = 0x20;
    string[2] = 0x20;
    string[3] = 0;
    width = measure(string, 0, &height);
    check('2', width == 20 && height == 12);

    string[0] = 0x01;
    string[1] = 0x20;
    string[2] = 0x02;
    string[3] = 0x20;
    string[4] = 0;
    width = measure(string, 0, &height);
    check('3', width == 17 && height == 12);

    string[0] = 0x03;
    string[1] = 0x20;
    string[2] = 0;
    width = measure(string, 0, &height);
    check('4', width == 6 && height == 12);

    string[0] = 0x04;
    string[1] = 0x20;
    string[2] = 0;
    width = measure(string, 0, &height);
    check('5', width == 7 && height == 12);

    string[0] = 0x10;
    string[1] = 0x30;
    string[2] = 0;
    width = measure(string, 0, &height);
    check('6', width == 0 && height == 12);

    string[0] = 0x22;
    string[1] = 0x23;
    string[2] = 0;
    width = measure(string, 0, &height);
    check('7', width == 9 && height == 12);

    put('D');
    quit();
    return 0;
}
