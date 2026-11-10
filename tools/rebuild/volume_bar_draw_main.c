/* The test harness for the rebuilt `volume_bar_draw` module.
 *
 * The harness holds the clip rectangle, the color byte, the color table, the
 * text sink, and the line sink.  The color table is filled at run time, because
 * the minimal image does not load the initialized data segment.  The stubs
 * record their arguments and the current color byte.  The harness runs eleven
 * paths:
 *
 *     0  value 0: the clip rectangle, one label draw, and no tick
 *     1  value 1023: the count truncates to 0, so no tick
 *     2  value 1024: one tick at x 0xaa with the color 0x16
 *     3  value 2047: the count truncates to 1, so one tick
 *     4  value 10240: ten ticks from x 0xaa to x 0xe0, all color 0x16
 *     5  value 11264: eleven ticks, and tick 10 takes the color 0xd6
 *     6  value 20480: twenty ticks, and tick 19 keeps the color 0xd6
 *     7  value 30720: thirty ticks, and tick 29 takes the color 0xd0
 *     8  value 31744: thirty-one ticks, and tick 30 takes the color 0xd0
 *     9  value -1024: a negative count draws no tick and leaves the color
 *     a  value -5000 and the label FX: the negative count and the label pass
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void volume_bar_draw(int value, char *label);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int bar_clip_left;
int bar_clip_right;
int bar_clip_top;
int bar_clip_bottom;
unsigned char bar_color;
unsigned char volume_bar_colors[4];

static char text[8];
static char label_cd[3];
static char label_fx[3];
static char label_mu[3];

static int draw_calls;
static int draw_x;
static int draw_y;
static char *draw_text;
static int draw_font;
static int draw_color;

struct line_record {
    int x1;
    int y1;
    int x2;
    int y2;
    int color;
};

static struct line_record lines[40];
static int line_count;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void text_draw(int x, int y, char *value, int font, int color)
{
    draw_calls = draw_calls + 1;
    draw_x = x;
    draw_y = y;
    draw_text = value;
    draw_font = font;
    draw_color = color;
}

void line_draw(int x1, int y1, int x2, int y2)
{
    if (line_count < 40) {
        lines[line_count].x1 = x1;
        lines[line_count].y1 = y1;
        lines[line_count].x2 = x2;
        lines[line_count].y2 = y2;
        lines[line_count].color = bar_color;
    }
    line_count = line_count + 1;
}

static int line_is(int index, int x1, int y1, int x2, int y2, int color)
{
    return lines[index].x1 == x1 && lines[index].y1 == y1
        && lines[index].x2 == x2 && lines[index].y2 == y2
        && lines[index].color == color;
}

static void reset(void)
{
    int index;

    label_cd[0] = 'C';
    label_cd[1] = 'D';
    label_cd[2] = 0;
    label_fx[0] = 'F';
    label_fx[1] = 'X';
    label_fx[2] = 0;
    label_mu[0] = 'M';
    label_mu[1] = 'U';
    label_mu[2] = 0;

    bar_clip_left = -1;
    bar_clip_right = -1;
    bar_clip_top = -1;
    bar_clip_bottom = -1;
    bar_color = 0xff;
    volume_bar_colors[0] = 0x16;
    volume_bar_colors[1] = 0xd6;
    volume_bar_colors[2] = 0xd0;
    volume_bar_colors[3] = 0xd0;

    draw_calls = 0;
    draw_x = 0;
    draw_y = 0;
    draw_text = 0;
    draw_font = 0;
    draw_color = 0;

    line_count = 0;
    for (index = 0; index < 40; index++) {
        lines[index].x1 = 0;
        lines[index].y1 = 0;
        lines[index].x2 = 0;
        lines[index].y2 = 0;
        lines[index].color = 0;
    }
}

static int clip_is(int left, int right, int top, int bottom)
{
    return bar_clip_left == left && bar_clip_right == right
        && bar_clip_top == top && bar_clip_bottom == bottom;
}

int main(void)
{
    reset();
    volume_bar_draw(0, label_cd);
    put(clip_is(0, 0x280, 0, 0x190) && draw_calls == 1 && draw_x == 0x80
        && draw_y == 0x8a && draw_text == label_cd && draw_font == 5
        && draw_color == 3 && line_count == 0 && bar_color == 0xff
        ? '0' : 'x');

    reset();
    volume_bar_draw(1023, label_cd);
    put(line_count == 0 && clip_is(0, 0x280, 0, 0x190) ? '1' : 'x');

    reset();
    volume_bar_draw(1024, label_cd);
    put(line_count == 1 && line_is(0, 0xaa, 0x8a, 0xaa, 0x94, 0x16)
        ? '2' : 'x');

    reset();
    volume_bar_draw(2047, label_cd);
    put(line_count == 1 && line_is(0, 0xaa, 0x8a, 0xaa, 0x94, 0x16)
        ? '3' : 'x');

    reset();
    volume_bar_draw(10240, label_cd);
    put(line_count == 10 && line_is(0, 0xaa, 0x8a, 0xaa, 0x94, 0x16)
        && line_is(9, 0xe0, 0x8a, 0xe0, 0x94, 0x16) ? '4' : 'x');

    reset();
    volume_bar_draw(11264, label_cd);
    put(line_count == 11 && line_is(9, 0xe0, 0x8a, 0xe0, 0x94, 0x16)
        && line_is(10, 0xe6, 0x8a, 0xe6, 0x94, 0xd6) ? '5' : 'x');

    reset();
    volume_bar_draw(20480, label_cd);
    put(line_count == 20 && line_is(19, 0x11c, 0x8a, 0x11c, 0x94, 0xd6)
        ? '6' : 'x');

    reset();
    volume_bar_draw(30720, label_cd);
    put(line_count == 30 && line_is(29, 0x158, 0x8a, 0x158, 0x94, 0xd0)
        ? '7' : 'x');

    reset();
    volume_bar_draw(31744, label_cd);
    put(line_count == 31 && line_is(30, 0x15e, 0x8a, 0x15e, 0x94, 0xd0)
        ? '8' : 'x');

    reset();
    volume_bar_draw(-1024, label_cd);
    put(line_count == 0 && draw_calls == 1 && bar_color == 0xff
        ? '9' : 'x');

    reset();
    volume_bar_draw(-5000, label_fx);
    put(line_count == 0 && draw_calls == 1 && draw_text == label_fx
        ? 'a' : 'x');

    put('D');
    quit();
    return 0;
}
