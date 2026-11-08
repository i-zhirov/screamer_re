/* The test harness for the rebuilt `text_draw` module.
 *
 * The harness holds one font with the three glyphs A, B, and C.  Each glyph
 * holds four rows of two-bit packed pixels, so the vertical clip and the
 * destination advance are visible.  The font descriptor holds the height 4,
 * the first character A, and the count 3.
 *
 * The ten paths cover:
 *
 *     0  one A in the default modes, with the row shift and the color base
 *     1  the string AA, which checks the glyph advance of the width plus two
 *     2  the string AB with the left clip past A, so A is skipped whole
 *     3  the string A with the left clip inside A, so the first pixel is cut
 *     4  the font index one past the count, which returns 2
 *     5  the screen y at the bottom edge, which returns 3
 *     6  the screen x at the right edge, which returns 3
 *     7  the control byte 3, which clears the widened mode
 *     8  the top clip inside the glyph, which draws only the lower rows
 *     9  the control byte 1, which sets the uniform cell and the half center
 *     D  the exit marker
 *
 * Each path fills the video buffer with the sentinel 0x5a and checks exact
 * pixels around the glyph, so a wrong stride, a wrong shift, or a wrong
 * advance shows up as a marker x.
 *
 * The strings live in the BSS and are filled at run time.  A pointer to a
 * CONST or DATA cell at a low offset is a null near pointer in the DOS/4G
 * build, so the harness keeps no string literal.
 */
#pragma off (check_stack)

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 210
#define SENTINEL 0x5a

int text_draw(int x, int y, char *text, int font, int color);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char *DAT_00021528;
unsigned char *DAT_00015434;
unsigned char *DAT_00015438;
unsigned char *DAT_0001543c;
int DAT_00015440;
int DAT_00015444;
int DAT_00015448;
int DAT_0001544c;

static char text[8];
static char text_a[2];
static char text_aa[3];
static char text_ab[3];
static char text_control_3[3];
static char text_control_1[3];
static unsigned char video[VIDEO_WIDTH * VIDEO_HEIGHT];
static unsigned char glyph_table[0x500];
static int descriptor[3];
static unsigned char bitmaps[32];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int at(int x, int y)
{
    return video[y * VIDEO_WIDTH + x];
}

static void setup_font(void)
{
    int index;
    int *entry;

    for (index = 0; index < 0x500; index++) {
        glyph_table[index] = 0;
    }
    entry = (int *)glyph_table;
    entry[0] = 3;
    entry[1] = 0;
    entry[2] = 5;
    entry[3] = 8;
    entry[4] = 1;
    entry[5] = 20;

    descriptor[0] = 4;
    descriptor[1] = 'A';
    descriptor[2] = 3;

    for (index = 0; index < 32; index++) {
        bitmaps[index] = 0;
    }
    bitmaps[0] = 0x12;
    bitmaps[1] = 0x30;
    bitmaps[2] = 0x45;
    bitmaps[3] = 0x60;
    bitmaps[4] = 0x78;
    bitmaps[5] = 0x90;
    bitmaps[6] = 0xab;
    bitmaps[7] = 0xc0;
    bitmaps[8] = 0x11;
    bitmaps[9] = 0x22;
    bitmaps[10] = 0x30;
    bitmaps[11] = 0x44;
    bitmaps[12] = 0x55;
    bitmaps[13] = 0x60;
    bitmaps[14] = 0x77;
    bitmaps[15] = 0x88;
    bitmaps[16] = 0x90;
    bitmaps[17] = 0xaa;
    bitmaps[18] = 0xbb;
    bitmaps[19] = 0xc0;
    bitmaps[20] = 0x50;
    bitmaps[21] = 0x60;
    bitmaps[22] = 0x70;
    bitmaps[23] = 0x80;

    text_a[0] = 'A';
    text_a[1] = 0;
    text_aa[0] = 'A';
    text_aa[1] = 'A';
    text_aa[2] = 0;
    text_ab[0] = 'A';
    text_ab[1] = 'B';
    text_ab[2] = 0;
    text_control_3[0] = 3;
    text_control_3[1] = 'A';
    text_control_3[2] = 0;
    text_control_1[0] = 1;
    text_control_1[1] = 'A';
    text_control_1[2] = 0;
}

static void reset(void)
{
    int index;

    for (index = 0; index < VIDEO_WIDTH * VIDEO_HEIGHT; index++) {
        video[index] = SENTINEL;
    }
    DAT_00021528 = video;
    DAT_00015434 = glyph_table;
    DAT_00015438 = (unsigned char *)descriptor;
    DAT_0001543c = bitmaps;
    DAT_00015440 = 1;
    DAT_00015444 = 0;
    DAT_00015448 = 200;
    DAT_0001544c = 0;
}

static int row_is(int y, int first, int second, int third)
{
    return at(11, y) == first
        && at(12, y) == second
        && at(13, y) == third;
}

int main(void)
{
    setup_font();

    reset();
    put(text_draw(10, 20, text_a, 0, 2) == 0
        && at(9, 20) == SENTINEL
        && at(10, 20) == SENTINEL
        && row_is(20, 0x21, 0x22, 0x23)
        && at(14, 20) == SENTINEL
        && row_is(21, 0x24, 0x25, 0x26)
        && row_is(22, 0x27, 0x28, 0x29)
        && row_is(23, 0x2a, 0x2b, 0x2c)
        && at(9, 21) == SENTINEL ? '0' : 'x');

    reset();
    put(text_draw(10, 20, text_aa, 0, 2) == 0
        && row_is(20, 0x21, 0x22, 0x23)
        && at(16, 20) == 0x21
        && at(17, 20) == 0x22
        && at(18, 20) == 0x23
        && at(19, 20) == SENTINEL
        && at(16, 21) == 0x24
        && at(17, 21) == 0x25
        && at(18, 21) == 0x26 ? '1' : 'x');

    reset();
    DAT_0001544c = 15;
    put(text_draw(10, 20, text_ab, 0, 2) == 0
        && at(11, 20) == SENTINEL
        && at(12, 20) == SENTINEL
        && at(13, 20) == SENTINEL
        && at(16, 20) == 0x21
        && at(17, 20) == 0x21
        && at(18, 20) == 0x22
        && at(19, 20) == 0x22
        && at(20, 20) == 0x23
        && at(21, 20) == SENTINEL
        && at(16, 21) == 0x24
        && at(17, 21) == 0x24
        && at(18, 21) == 0x25
        && at(19, 21) == 0x25
        && at(20, 21) == 0x26 ? '2' : 'x');

    reset();
    DAT_0001544c = 12;
    put(text_draw(10, 20, text_a, 0, 2) == 0
        && at(11, 20) == SENTINEL
        && at(12, 20) == 0x22
        && at(13, 20) == 0x23
        && at(14, 20) == SENTINEL
        && at(11, 21) == SENTINEL
        && at(12, 21) == 0x25
        && at(13, 21) == 0x26 ? '3' : 'x');

    reset();
    put(text_draw(10, 20, text_a, 1, 2) == 2
        && at(11, 20) == SENTINEL ? '4' : 'x');

    reset();
    DAT_00015448 = 20;
    put(text_draw(10, 20, text_a, 0, 2) == 3
        && at(11, 20) == SENTINEL ? '5' : 'x');

    reset();
    put(text_draw(640, 0, text_a, 0, 2) == 3
        && at(0, 0) == SENTINEL ? '6' : 'x');

    reset();
    put(text_draw(10, 20, text_control_3, 0, 2) == 0
        && at(9, 20) == SENTINEL
        && at(10, 20) == 0x21
        && at(11, 20) == 0x22
        && at(12, 20) == 0x23
        && at(13, 20) == SENTINEL
        && at(10, 21) == 0x24
        && at(11, 21) == 0x25
        && at(12, 21) == 0x26 ? '7' : 'x');

    reset();
    DAT_00015444 = 22;
    put(text_draw(10, 20, text_a, 0, 2) == 0
        && at(11, 20) == SENTINEL
        && at(11, 21) == SENTINEL
        && row_is(22, 0x27, 0x28, 0x29)
        && row_is(23, 0x2a, 0x2b, 0x2c) ? '8' : 'x');

    reset();
    put(text_draw(10, 20, text_control_1, 0, 2) == 0
        && at(11, 20) == SENTINEL
        && at(12, 20) == 0x21
        && at(13, 20) == 0x22
        && at(14, 20) == 0x23
        && at(15, 20) == SENTINEL
        && at(12, 21) == 0x24
        && at(13, 21) == 0x25
        && at(14, 21) == 0x26 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
