/* The test harness for the rebuilt `background_scale` module.
 *
 * The harness holds the decoded background buffer, the video buffer, the
 * camera record, the four input globals, and the two output globals.  It runs
 * four paths:
 *
 *     0  negative offset: the offset is -469, the source row is 234, the
 *        column is 0, the function writes 20 row pairs, and the fill starts
 *        at row 40
 *     1  positive offset with wrap: the offset is 27, the source row is 13,
 *        the wrapped span is 73, the function writes 35 row pairs, and the
 *        fill starts at row 70
 *     2  column offset: the offset is -469 and the column is 511, so the
 *        source window crosses the row boundary
 *     3  negative offset with wrap: the offset is -100, the source row is 50,
 *        the wrapped span is 100, the function writes 49 row pairs, and the
 *        fill starts at row 98
 *     D  the exit marker
 *
 * The harness fills the source with a byte pattern and the destination with
 * the sentinel 0xaaaaaaaa.  It checks the rendered rows byte for byte and the
 * filled rows dword for dword.  The next row after the fill must keep the
 * sentinel, so the fill bounds are exact.
 */
#pragma off (check_stack)

#define SOURCE_STRIDE 512
#define SOURCE_ROWS 256
#define DEST_STRIDE 640
#define DEST_ROWS 104
#define SENTINEL 0xaaaaaaaa

void background_scale(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_000724cc;
int DAT_0007a7dc;
int *DAT_0007a7ac;
int DAT_0007a7c4;
int DAT_0007a7bc;
unsigned char *DAT_000214e8;
unsigned char *DAT_00121528;
int DAT_00073f10;
int DAT_00073dec;

static char text[8];
static int camera[8];
static unsigned char source_buffer[SOURCE_ROWS * SOURCE_STRIDE];
static unsigned int dest_buffer[DEST_ROWS * DEST_STRIDE / 4];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void prepare(void)
{
    int index;

    for (index = 0; index < SOURCE_ROWS * SOURCE_STRIDE; index++) {
        source_buffer[index] = (unsigned char)(index * 13 + 7);
    }
    for (index = 0; index < DEST_ROWS * DEST_STRIDE / 4; index++) {
        dest_buffer[index] = SENTINEL;
    }
    for (index = 0; index < 8; index++) {
        camera[index] = 0;
    }
    DAT_0007a7ac = camera;
    DAT_000214e8 = source_buffer;
    DAT_00121528 = (unsigned char *)dest_buffer;
    DAT_000724cc = 0;
    DAT_0007a7dc = 0;
    DAT_0007a7c4 = 100;
    DAT_0007a7bc = 0x11223344;
    DAT_00073f10 = 0;
    DAT_00073dec = 0;
}

static int row_matches(int dest_row, int source_row, int column)
{
    unsigned char *target;
    unsigned char *source;
    int index;

    target = (unsigned char *)dest_buffer + dest_row * DEST_STRIDE;
    source = source_buffer + source_row * SOURCE_STRIDE + column;
    for (index = 0; index < 0x140; index++) {
        if (target[index * 2] != source[index]) {
            return 0;
        }
        if (target[index * 2 + 1] != source[index]) {
            return 0;
        }
    }
    return 1;
}

static int row_pair_matches(int dest_row, int source_row, int column)
{
    return row_matches(dest_row, source_row, column)
        && row_matches(dest_row + 1, source_row, column);
}

static int fill_matches(int first_row, int last_row, unsigned int color)
{
    int row;

    for (row = first_row; row <= last_row; row++) {
        if (dest_buffer[row * (DEST_STRIDE / 4)] != color) {
            return 0;
        }
        if (dest_buffer[row * (DEST_STRIDE / 4) + DEST_STRIDE / 4 - 1] != color) {
            return 0;
        }
    }
    return 1;
}

static int sentinel_at(int row)
{
    return dest_buffer[row * (DEST_STRIDE / 4)] == SENTINEL;
}

int main(void)
{
    prepare();
    DAT_000724cc = 128;
    DAT_0007a7dc = 0;
    camera[1] = 0x7ff;
    DAT_0007a7c4 = 100;
    DAT_0007a7bc = 0x11223344;
    background_scale();
    put(DAT_00073f10 == -469
        && DAT_00073dec == 0
        && row_pair_matches(0, 234, 0)
        && row_pair_matches(2, 235, 0)
        && fill_matches(40, 101, 0x11223344)
        && sentinel_at(102) ? '0' : 'x');

    prepare();
    DAT_000724cc = 128;
    DAT_0007a7dc = 0x1f0;
    camera[1] = 0x7ff;
    DAT_0007a7c4 = 100;
    DAT_0007a7bc = 0x11223344;
    background_scale();
    put(DAT_00073f10 == 27
        && DAT_00073dec == 0
        && row_pair_matches(0, 13, 0)
        && row_pair_matches(2, 14, 0)
        && fill_matches(70, 101, 0x11223344)
        && sentinel_at(102) ? '1' : 'x');

    prepare();
    DAT_000724cc = 128;
    DAT_0007a7dc = 0;
    camera[1] = 0x400;
    DAT_0007a7c4 = 100;
    DAT_0007a7bc = 0x11223344;
    background_scale();
    put(DAT_00073f10 == -469
        && DAT_00073dec == 511
        && row_pair_matches(0, 234, 511)
        && row_pair_matches(2, 235, 511)
        && fill_matches(40, 101, 0x11223344)
        && sentinel_at(102) ? '2' : 'x');

    prepare();
    DAT_000724cc = 0;
    DAT_0007a7dc = 0x172;
    camera[1] = 0x7ff;
    DAT_0007a7c4 = 100;
    DAT_0007a7bc = 0x55667788;
    background_scale();
    put(DAT_00073f10 == -100
        && DAT_00073dec == 0
        && row_pair_matches(0, 50, 0)
        && row_pair_matches(2, 51, 0)
        && fill_matches(98, 101, 0x55667788)
        && sentinel_at(102) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
