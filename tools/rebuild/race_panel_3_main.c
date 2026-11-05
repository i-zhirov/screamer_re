/* The test harness for the rebuilt `race_panel_3` module.
 *
 * The harness holds the two title pointers, the 0x34-byte record table, the
 * player row index, the two row cells, the row value, the two counters, the
 * 0.6 threshold, the frame counter, and the four stubs.  The `panel_draw_centered` stub records
 * the title call.  The `text_measure` stub returns the width and the height and
 * records the text pointer and the font.  The `text_draw` stub records the
 * doubled position, the text, the font, and the color.  The `panel_show` stub
 * records the two panel calls.  The harness runs six paths:
 *
 *     0  the index 0 path draws no left panel, the second title stays hidden,
 *        and the right panel takes the size 1
 *     1  the index 1 path draws the second title, the left panel takes the size
 *        2, and the right panel takes the size 1
 *     2  the row cell 2 above the cell 1 removes the left panel, and the right
 *        panel takes the size 3
 *     3  the row value 0.5 sits below the 0.6 threshold and the right panel
 *        takes the size 1
 *     4  the index 2 path equals the counter plus 2 and skips the right panel
 *     5  the negative width -3 truncates to -1 toward zero and centers at
 *        0xa0, so the doubled position is 0x140
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void race_panel_3(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char *DAT_0011fc34;
char *DAT_0011fc54;
unsigned char DAT_0011d908[0x34 * 4];
int DAT_001203c8;
int DAT_0012039c;
int DAT_001203a8;
float DAT_0012037c;
int DAT_00120818;
int DAT_0012081c;
unsigned int DAT_000007fd;
double race_panel_3_threshold;

static char text[8];
static char title_a[8];
static char title_b[8];
static int panel_calls;
static char *panel_text[2];
static int panel_y[2];
static int panel_font[2];
static int panel_color[2];
static int measure_calls;
static int measure_width;
static int measure_height;
static char *measure_text;
static int measure_font;
static int draw_calls;
static int draw_x;
static int draw_y;
static char *draw_text;
static int draw_font;
static int draw_color;
static int show_calls;
static int show_x[2];
static int show_y[2];
static int show_size[2];

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

static void set_bits_double(double *p, unsigned int high, unsigned int low)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.u[0] = low;
    v.u[1] = high;
    *p = v.d;
}

static char *entry_text(int index)
{
    return (char *)DAT_0011d908 + index * 0x34 + 0x14;
}

void panel_draw_centered(char *value, int y, int font, int color)
{
    if (panel_calls < 2) {
        panel_text[panel_calls] = value;
        panel_y[panel_calls] = y;
        panel_font[panel_calls] = font;
        panel_color[panel_calls] = color;
    }
    panel_calls = panel_calls + 1;
}

void panel_show(int x, int y, int size)
{
    if (show_calls < 2) {
        show_x[show_calls] = x;
        show_y[show_calls] = y;
        show_size[show_calls] = size;
    }
    show_calls = show_calls + 1;
}

void text_measure(int *width, int *height, char *value, int font)
{
    *width = measure_width;
    *height = measure_height;
    measure_text = value;
    measure_font = font;
    measure_calls = measure_calls + 1;
}

void text_draw(int x, int y, char *value, int font, int color)
{
    draw_x = x;
    draw_y = y;
    draw_text = value;
    draw_font = font;
    draw_color = color;
    draw_calls = draw_calls + 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x34 * 4; index++) {
        DAT_0011d908[index] = 0;
    }
    title_a[0] = 'A';
    title_a[1] = 0;
    title_b[0] = 'B';
    title_b[1] = 0;
    DAT_0011fc34 = title_a;
    DAT_0011fc54 = title_b;
    DAT_001203c8 = 0;
    DAT_0012039c = 0;
    DAT_001203a8 = 0;
    set_bits_float(&DAT_0012037c, 0x00000000);
    DAT_00120818 = 0;
    DAT_0012081c = 0;
    DAT_000007fd = 0;
    panel_calls = 0;
    measure_calls = 0;
    measure_width = 0;
    measure_height = 0;
    measure_text = 0;
    measure_font = 0;
    draw_calls = 0;
    draw_x = 0;
    draw_y = 0;
    draw_text = 0;
    draw_font = 0;
    draw_color = 0;
    show_calls = 0;
}

static void setup(int index, int width, int height, int cell_a, int cell_b,
                  unsigned int row_value, int counter_a, int counter_b,
                  unsigned int frame)
{
    reset();
    set_bits_double(&race_panel_3_threshold, 0x3fe33333, 0x33333333);
    DAT_001203c8 = index;
    measure_width = width;
    measure_height = height;
    DAT_0012039c = cell_a;
    DAT_001203a8 = cell_b;
    set_bits_float(&DAT_0012037c, row_value);
    DAT_00120818 = counter_a;
    DAT_0012081c = counter_b;
    DAT_000007fd = frame;
}

static int title_is(int index, char *value, int y, int font, int color)
{
    return panel_text[index] == value && panel_y[index] == y
        && panel_font[index] == font && panel_color[index] == color;
}

static int show_is(int index, int x, int y, int size)
{
    return show_x[index] == x && show_y[index] == y
        && show_size[index] == size;
}

int main(void)
{
    setup(0, 0x40, 0x10, 0, 0, 0x00000000, 1, 5, 0);
    race_panel_3();
    put(panel_calls == 1 && title_is(0, title_a, 2, 3, 2)
        && measure_calls == 1 && measure_text == entry_text(0)
        && measure_font == 2
        && draw_calls == 1 && draw_x == 0x120 && draw_y == 0x170
        && draw_text == entry_text(0) && draw_font == 2 && draw_color == 2
        && show_calls == 1 && show_is(0, 0xee, 0x64, 1) ? '0' : 'x');

    setup(1, 0x40, 0x10, 0, 1, 0x3f800000, 1, 0, 0);
    race_panel_3();
    put(panel_calls == 2 && title_is(0, title_a, 2, 3, 2)
        && title_is(1, title_b, 0x20, 1, 2)
        && measure_calls == 1 && measure_text == entry_text(1)
        && draw_calls == 1 && draw_x == 0x120 && draw_y == 0x170
        && draw_text == entry_text(1)
        && show_calls == 2 && show_is(0, 8, 0x64, 2)
        && show_is(1, 0xee, 0x64, 1) ? '1' : 'x');

    setup(1, 0x42, 0x11, 2, 1, 0x3f800000, 1, 5, 0x20);
    race_panel_3();
    put(panel_calls == 1 && title_is(0, title_a, 2, 3, 2)
        && measure_calls == 1 && measure_text == entry_text(1)
        && draw_calls == 1 && draw_x == 0x11e && draw_y == 0x170
        && show_calls == 2 && show_is(0, 8, 0x64, 0)
        && show_is(1, 0xee, 0x64, 3) ? '2' : 'x');

    setup(1, 0x40, 0x10, 0, 1, 0x3f000000, 1, 5, 0);
    race_panel_3();
    put(panel_calls == 1 && measure_calls == 1
        && draw_calls == 1 && draw_x == 0x120 && draw_y == 0x170
        && show_calls == 2 && show_is(0, 8, 0x64, 0)
        && show_is(1, 0xee, 0x64, 1) ? '3' : 'x');

    setup(2, 0x40, 0x10, 0, 1, 0x3f800000, 0, 5, 0);
    race_panel_3();
    put(panel_calls == 1 && measure_calls == 1
        && measure_text == entry_text(2)
        && draw_calls == 1 && draw_text == entry_text(2)
        && show_calls == 1 && show_is(0, 8, 0x64, 2) ? '4' : 'x');

    setup(1, -3, 0x10, 0, 1, 0x3f800000, 1, 5, 0);
    race_panel_3();
    put(panel_calls == 1 && measure_calls == 1
        && draw_calls == 1 && draw_x == 0x140 && draw_y == 0x170
        && show_calls == 2 && show_is(0, 8, 0x64, 2)
        && show_is(1, 0xee, 0x64, 1) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
