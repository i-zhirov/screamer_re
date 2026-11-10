/* The test harness for the rebuilt `shade_rect` module.
 *
 * The harness holds the framebuffer pointer `DAT_00121528`, the shade table
 * pointer `DAT_0015d1c`, an 8-row framebuffer, and a 0x100-byte shade table.
 * Every case refills the framebuffer with a fixed pattern, copies the pattern
 * to a reference buffer, runs the rebuilt function on the framebuffer, and
 * applies the observed rule to the reference buffer with a plain nested loop.
 * The case passes when the whole framebuffer, sentinels included, matches the
 * reference.
 *
 * The paths:
 *
 *     0  the rectangle (4, 1, 8, 2): one aligned block per row with a stride
 *     1  the rectangle (3, 2, 15, 1): the x masks to 0 and the width to 12
 *     2  the rectangle (0, 0, 0x280, 3): the full width with a zero stride
 *     3  the rectangle (0x27c, 4, 4, 4): one word per row at the right edge
 *     4  the rectangle (0, 0, 0x280, 6) twice: the table applies two times
 *     5  the identity table: every pixel stays unchanged
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#define STRIDE 0x280
#define ROWS 8
#define BUFFER_SIZE (STRIDE * ROWS + 16)

void shade_rect(int x, int y, int width, int height);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char *DAT_0015d1c;
unsigned char *DAT_00121528;

static unsigned char framebuffer[BUFFER_SIZE];
static unsigned char reference[BUFFER_SIZE];
static unsigned char shade[0x100];
static char text[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void fill_pattern(void)
{
    int index;

    for (index = 0; index < BUFFER_SIZE; index++) {
        framebuffer[index] = (unsigned char)(index * 7 + 3);
    }
}

static void build_shade(int multiplier, int addend)
{
    int index;

    for (index = 0; index < 0x100; index++) {
        shade[index] = (unsigned char)((index * multiplier + addend) & 0xff);
    }
}

static void apply_rule(unsigned char *buffer, int x, int y, int width, int height)
{
    int aligned_x;
    int aligned_width;
    int row;
    int column;

    aligned_x = x & 0xfffc;
    aligned_width = width & 0xfffc;
    for (row = 0; row < height; row++) {
        for (column = 0; column < aligned_width; column++) {
            int offset;

            offset = (y + row) * STRIDE + aligned_x + column;
            buffer[offset] = shade[buffer[offset]];
        }
    }
}

static int buffers_equal(void)
{
    int index;

    for (index = 0; index < BUFFER_SIZE; index++) {
        if (framebuffer[index] != reference[index]) {
            return 0;
        }
    }
    return 1;
}

static int run_case(int x, int y, int width, int height, int twice)
{
    int index;

    fill_pattern();
    for (index = 0; index < BUFFER_SIZE; index++) {
        reference[index] = framebuffer[index];
    }
    shade_rect(x, y, width, height);
    if (twice) {
        shade_rect(x, y, width, height);
    }
    apply_rule(reference, x, y, width, height);
    if (twice) {
        apply_rule(reference, x, y, width, height);
    }
    return buffers_equal();
}

int main(void)
{
    DAT_00121528 = framebuffer;
    DAT_0015d1c = shade;

    build_shade(5, 1);
    put(run_case(4, 1, 8, 2, 0) ? '0' : 'x');
    put(run_case(3, 2, 15, 1, 0) ? '1' : 'x');
    put(run_case(0, 0, STRIDE, 3, 0) ? '2' : 'x');
    put(run_case(0x27c, 4, 4, 4, 0) ? '3' : 'x');
    put(run_case(0, 0, STRIDE, 6, 1) ? '4' : 'x');

    build_shade(1, 0);
    put(run_case(8, 1, 32, 4, 0) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
