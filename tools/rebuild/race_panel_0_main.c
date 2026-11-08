/* The test harness for the rebuilt `race_panel_0` module.
 *
 * The harness holds the race object with the width at +0x134 and the height at
 * +0x120, the two smoothed axis positions, the frame delta, the three
 * constants, the race state, the frame counter, and the two title pointers.
 * The `panel_show` stub records the two calls.  The `panel_draw_centered` stub
 * records the title call.  The harness runs five paths:
 *
 *     0  the x position moves up by the scaled delta and truncates, the y
 *        position snaps down to the target, and the state 0 draws no title
 *     1  the x position snaps down, the y position moves up, and the state 1
 *        draws the first title
 *     2  the state 2 draws the second title while the blink phase stays low
 *     3  the blink phase is 0x50, so the state 1 draws no title
 *     4  a negative x sum -31.5 truncates toward zero to -31, so the
 *        truncating FPU stub is exercised
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void race_panel_0(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char *DAT_0015ccc;
float DAT_0018e08;
float DAT_00020824;
float DAT_00020828;
float race_panel_0_x_step;
float race_panel_0_y_step;
float race_panel_0_x_offset;
unsigned int DAT_00020988;
unsigned int DAT_000007fd;
char *DAT_0011fc58;
char *DAT_0011fc5c;

static char text[8];
static unsigned char object[0x200];
static char title_a[8];
static char title_b[8];
static int panel_calls;
static int panel_x[2];
static int panel_y[2];
static int panel_size[2];
static int draw_calls;
static char *draw_text;
static int draw_y;
static int draw_font;
static int draw_color;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static void set_field(int offset, int value)
{
    *(int *)(object + offset) = value;
}

void panel_show(int x, int y, int size)
{
    if (panel_calls < 2) {
        panel_x[panel_calls] = x;
        panel_y[panel_calls] = y;
        panel_size[panel_calls] = size;
    }
    panel_calls = panel_calls + 1;
}

void panel_draw_centered(char *value, int y, int font, int color)
{
    draw_text = value;
    draw_y = y;
    draw_font = font;
    draw_color = color;
    draw_calls = draw_calls + 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x200; index++) {
        object[index] = 0;
    }
    DAT_0015ccc = object;
    title_a[0] = 'A';
    title_a[1] = 0;
    title_b[0] = 'B';
    title_b[1] = 0;
    DAT_0011fc58 = title_a;
    DAT_0011fc5c = title_b;
    DAT_00020988 = 0;
    DAT_000007fd = 0;
    set_bits_float(&DAT_0018e08, 0x3f800000);
    set_bits_float(&DAT_00020824, 0x00000000);
    set_bits_float(&DAT_00020828, 0x00000000);
    set_bits_float(&race_panel_0_x_step, 0x3f800000);
    set_bits_float(&race_panel_0_y_step, 0x3f800000);
    set_bits_float(&race_panel_0_x_offset, 0x00000000);
    panel_calls = 0;
    draw_calls = 0;
    draw_text = 0;
    draw_y = 0;
    draw_font = 0;
    draw_color = 0;
}

static void setup(int width, int height, unsigned int pos_x,
                  unsigned int pos_y, unsigned int delta,
                  unsigned int x_step, unsigned int y_step,
                  unsigned int offset)
{
    reset();
    set_field(0x134, width);
    set_field(0x120, height);
    set_bits_float(&DAT_00020824, pos_x);
    set_bits_float(&DAT_00020828, pos_y);
    set_bits_float(&DAT_0018e08, delta);
    set_bits_float(&race_panel_0_x_step, x_step);
    set_bits_float(&race_panel_0_y_step, y_step);
    set_bits_float(&race_panel_0_x_offset, offset);
}

static int axis_is(int index, int x, int y, int size)
{
    return panel_x[index] == x && panel_y[index] == y
        && panel_size[index] == size;
}

int main(void)
{
    setup(0x100, 0x80, 0x42480000, 0x43480000, 0x40000000,
          0x3e800000, 0x3f800000, 0x00000000);
    race_panel_0();
    put(panel_calls == 2 && axis_is(0, 50, 0, 0xf)
        && axis_is(1, 128, 0x10, 0xe) && draw_calls == 0 ? '0' : 'x');

    setup(0x100, 0x80, 0x43160000, 0x42c80000, 0x3f800000,
          0x3f800000, 0x41200000, 0x00000000);
    DAT_00020988 = 1;
    race_panel_0();
    put(panel_calls == 2 && axis_is(0, 96, 0, 0xf)
        && axis_is(1, 110, 0x10, 0xe)
        && draw_calls == 1 && draw_text == title_a
        && draw_y == 0xbe && draw_font == 0 && draw_color == 2 ? '1' : 'x');

    setup(0x100, 0x80, 0x42c00000, 0x43000000, 0x3f800000,
          0x3f800000, 0x3f800000, 0x00000000);
    DAT_00020988 = 2;
    race_panel_0();
    put(panel_calls == 2 && axis_is(0, 96, 0, 0xf)
        && axis_is(1, 128, 0x10, 0xe)
        && draw_calls == 1 && draw_text == title_b
        && draw_y == 0xbe && draw_font == 0 && draw_color == 2 ? '2' : 'x');

    setup(0x100, 0x80, 0x42c00000, 0x43000000, 0x3f800000,
          0x3f800000, 0x3f800000, 0x00000000);
    DAT_00020988 = 1;
    DAT_000007fd = 0x50;
    race_panel_0();
    put(panel_calls == 2 && draw_calls == 0 ? '3' : 'x');

    setup(0x2fc, 0x80, 0xc2000000, 0x43000000, 0x3f800000,
          0x00000000, 0x3f800000, 0x3f000000);
    race_panel_0();
    put(panel_calls == 2 && axis_is(0, -31, 0, 0xf)
        && axis_is(1, 128, 0x10, 0xe) && draw_calls == 0 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
