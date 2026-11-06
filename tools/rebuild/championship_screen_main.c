/* The test harness for the rebuilt `championship_screen` module.
 *
 * The harness holds the 0x2c-byte championship records, the `champ` text list,
 * the group-start table, the six standings rows, the frame timer, the two float
 * constants, and the footer text.  The stubs record their arguments and their
 * call counts.  The `sprintf` stub reads the format literal, because the module
 * literal is readable in the linked image, and it rebuilds the position, the
 * decimal, the time, and the two summary strings.
 *
 * The harness runs four paths:
 *
 *     0  the type 3 standings table: the two rectangles, the title, the four
 *        headers, six rows with a normal row, an empty row, and the special
 *        row, the point sum, both summary lines, and the footer panel
 *     1  the type 0 centred list with the timer above 10.0: the three list
 *        lines, the `net_race_screen` call, and no footer because the mode is
 *        at least 0x48
 *     2  the type 3 table with the mode at least 0x48: the title only and no
 *        `net_race_screen` call
 *     3  the type 0 list with the timer below 10.0: no `net_race_screen` call
 *        and the footer panel
 *     D  the exit marker
 */
#pragma off (check_stack)

#include <stdarg.h>

void championship_screen(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct champ_record {
    int type;
    int value;
    int title_index;
    int group;
    int field_10;
    int field_14;
    char name[12];
    int field_24;
    int font;
};

unsigned int DAT_000007fd;
char *DAT_00120030[40];
int DAT_001201c0[32];
int DAT_001203dc;
int DAT_001203e4;
float DAT_001203e8[6];
int DAT_00120400[6];
int DAT_00120418[6];
int DAT_00120804;
float DAT_0011d24c;
float DAT_0010124c;
float DAT_00101250;
char *DAT_0011fc60;
struct champ_record DAT_001022bc[12];

static char text[8];
static char champ_text[40][8];
static char footer[8];
static int rect_calls;
static int rect_x;
static int rect_y;
static int rect_width;
static int rect_height;
static int panel_calls;
static char *panel_text[4];
static int panel_y[4];
static int panel_font[4];
static int panel_color[4];
static int text_calls;
static int text_x[40];
static int text_y[40];
static int text_font[40];
static int text_color[40];
static char text_value[40][0x40];
static int measure_calls;
static int measure_font[8];
static char measure_value[8][0x40];
static int sprintf_calls;
static int net_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; ++index) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

static char *copy_end(char *destination, char *source)
{
    while (*source != 0) {
        *destination++ = *source++;
    }
    return destination;
}

static void copy(char *destination, char *source)
{
    while (*source != 0) {
        *destination++ = *source++;
    }
    *destination = 0;
}

static char *put_decimal(char *out, int value)
{
    char digits[12];
    unsigned int number;
    int count;

    count = 0;
    if (value < 0) {
        *out++ = '-';
        number = (unsigned int)(-value);
    } else {
        number = (unsigned int)value;
    }
    do {
        digits[count] = (char)('0' + (number % 10));
        number = number / 10;
        ++count;
    } while (number != 0);
    while (count > 0) {
        --count;
        *out++ = digits[count];
    }
    return out;
}

static char *put_two(char *out, int value)
{
    if (value < 10) {
        *out++ = '0';
    }
    return put_decimal(out, value);
}

static float float_of(unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    return v.f;
}

static void build_champ(int index)
{
    char *end;

    champ_text[index][0] = 'S';
    end = put_decimal(champ_text[index] + 1, index);
    *end = 0;
}

void video_rect_copy(int x, int y, int width, int height)
{
    rect_calls = rect_calls + 1;
    rect_x = x;
    rect_y = y;
    rect_width = width;
    rect_height = height;
}

void panel_draw_centered(char *value, int y, int font, int color)
{
    if (panel_calls < 4) {
        panel_text[panel_calls] = value;
        panel_y[panel_calls] = y;
        panel_font[panel_calls] = font;
        panel_color[panel_calls] = color;
    }
    panel_calls = panel_calls + 1;
}

void text_draw(int x, int y, char *value, int font, int color)
{
    if (text_calls < 40) {
        text_x[text_calls] = x;
        text_y[text_calls] = y;
        text_font[text_calls] = font;
        text_color[text_calls] = color;
        copy(text_value[text_calls], value);
    }
    text_calls = text_calls + 1;
}

void text_measure(int *width, int *height, char *value, int font)
{
    if (measure_calls < 8) {
        measure_font[measure_calls] = font;
        copy(measure_value[measure_calls], value);
    }
    measure_calls = measure_calls + 1;
    *width = 40;
    *height = 8;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *out;
    char *label;

    va_start(arguments, format);
    out = buffer;
    if (format[0] == '-') {
        *out++ = '-';
        out = put_decimal(out, va_arg(arguments, int));
        *out++ = '-';
    } else if (format[0] == 1) {
        *out++ = 1;
        if (format[2] == 'd') {
            out = put_decimal(out, va_arg(arguments, int));
        } else {
            out = put_two(out, va_arg(arguments, int));
            *out++ = '.';
            out = put_two(out, va_arg(arguments, int));
            *out++ = '.';
            out = put_two(out, va_arg(arguments, int));
        }
    } else if (format[0] == '%' && format[1] == 's') {
        label = va_arg(arguments, char *);
        out = copy_end(out, label);
        *out++ = ':';
        if (format[3] == 1) {
            *out++ = 1;
            out = put_two(out, va_arg(arguments, int));
        } else {
            out = put_decimal(out, va_arg(arguments, int));
        }
    }
    *out = 0;
    va_end(arguments);
    sprintf_calls = sprintf_calls + 1;
    return 0;
}

void net_race_screen(void)
{
    net_calls = net_calls + 1;
}

static void reset(void)
{
    int index;

    footer[0] = 'F';
    footer[1] = 'O';
    footer[2] = 'O';
    footer[3] = 'T';
    footer[4] = 0;
    for (index = 0; index < 40; ++index) {
        build_champ(index);
        DAT_00120030[index] = champ_text[index];
    }
    for (index = 0; index < 32; ++index) {
        DAT_001201c0[index] = 0;
    }
    DAT_001201c0[0] = 0;
    DAT_001201c0[1] = 1;
    DAT_001201c0[2] = 2;
    DAT_001201c0[3] = 3;
    DAT_001201c0[4] = 4;
    DAT_001201c0[5] = 5;
    DAT_001201c0[6] = 6;
    DAT_001201c0[17] = 10;
    DAT_001201c0[18] = 14;
    for (index = 0; index < 12; ++index) {
        DAT_001022bc[index].type = 0;
        DAT_001022bc[index].value = 0;
        DAT_001022bc[index].title_index = 0;
        DAT_001022bc[index].group = 0;
        DAT_001022bc[index].field_10 = 0;
        DAT_001022bc[index].field_14 = 0;
        DAT_001022bc[index].name[0] = 0;
        DAT_001022bc[index].field_24 = 0;
        DAT_001022bc[index].font = 0;
    }
    for (index = 0; index < 6; ++index) {
        DAT_001203e8[index] = float_of(0);
        DAT_00120400[index] = -1;
        DAT_00120418[index] = 0;
    }
    DAT_001203dc = 0;
    DAT_001203e4 = 0;
    DAT_00120804 = 0;
    DAT_0011d24c = float_of(0);
    DAT_0010124c = float_of(0x42c80000);
    DAT_00101250 = float_of(0x41200000);
    DAT_0011fc60 = footer;
    DAT_000007fd = 0;
    rect_calls = 0;
    rect_x = 0;
    rect_y = 0;
    rect_width = 0;
    rect_height = 0;
    panel_calls = 0;
    text_calls = 0;
    measure_calls = 0;
    sprintf_calls = 0;
    net_calls = 0;
}

static void set_standings(void)
{
    DAT_001203dc = 1;
    DAT_001022bc[1].type = 3;
    DAT_001022bc[1].value = 0x2a;
    DAT_001022bc[1].group = 0x0a;
    DAT_001022bc[1].title_index = 0;
    DAT_001022bc[1].field_24 = 2;
    DAT_001022bc[1].font = 3;
    DAT_00120400[0] = 5;
    DAT_001203e8[0] = float_of(0x42b50000);
    DAT_00120418[0] = 10;
    DAT_00120400[1] = -1;
    DAT_00120400[2] = 7;
    DAT_001203e8[2] = float_of(0x42740000);
    DAT_00120418[2] = 6;
    DAT_00120400[3] = -1;
    DAT_00120400[4] = -1;
    DAT_00120400[5] = -1;
    DAT_00120804 = 1;
}

static void set_list(void)
{
    DAT_001203dc = 0;
    DAT_001022bc[0].type = 0;
    DAT_001022bc[0].group = 0x11;
    DAT_001022bc[0].font = 3;
}

int main(void)
{
    reset();
    set_standings();
    DAT_000007fd = 0x40;
    championship_screen();
    put(rect_calls == 2 && rect_x == 0x20 && rect_y == 0x48
        && rect_width == 0x240 && rect_height == 0x110
        && panel_calls == 2
        && panel_text[0] == champ_text[6] && panel_y[0] == 2
        && panel_font[0] == 2 && panel_color[0] == 2
        && panel_text[1] == DAT_0011fc60 && panel_y[1] == 0xbe
        && panel_font[1] == 0 && panel_color[1] == 2
        && text_calls == 30 && net_calls == 0
        && same(text_value[0], champ_text[1]) && text_x[0] == 0x38
        && same(text_value[1], champ_text[0]) && text_x[1] == 0xb0
        && same(text_value[2], champ_text[2]) && text_x[2] == 0x154
        && same(text_value[3], champ_text[3]) && text_x[3] == 0x1ec
        && text_y[0] == 0x60 && text_color[0] == 0xf && text_font[0] == 0
        && same(text_value[4], "-1-") && text_x[4] == 0x4c
        && text_y[4] == 0x7c
        && same(text_value[5], "\x015") && text_x[5] == 0xb0
        && same(text_value[6], "\x0101.30.50") && text_x[6] == 0x118
        && same(text_value[7], "\x0110") && text_x[7] == 0x214
        && same(text_value[8], "-2-") && text_y[8] == 0x98
        && same(text_value[9], "\x01-") && same(text_value[10], "\x01--.--.--")
        && same(text_value[11], "\x01-")
        && same(text_value[12], "-3-") && text_y[12] == 0xb4
        && same(text_value[13], "\x0110")
        && same(text_value[14], "\x0100.00.00")
        && same(text_value[15], "\x010")
        && same(text_value[28], "S5:\x0110") && text_x[28] == 256
        && text_y[28] == 0x12c
        && same(text_value[29], "S4:42") && text_x[29] == 300
        && text_y[29] == 0x15c
        && DAT_001203e4 == 10 && measure_calls == 2
        && sprintf_calls == 11 ? '0' : 'x');

    reset();
    set_list();
    DAT_000007fd = 0x50;
    DAT_0011d24c = float_of(0x41400000);
    championship_screen();
    put(rect_calls == 0 && panel_calls == 0 && net_calls == 1
        && text_calls == 3 && measure_calls == 3
        && same(text_value[0], champ_text[10]) && text_x[0] == 300
        && text_y[0] == 0x40 && text_font[0] == 3 && text_color[0] == 2
        && same(text_value[1], champ_text[11]) && text_y[1] == 0x48
        && same(text_value[2], champ_text[12]) && text_y[2] == 0x50
        && measure_font[0] == 3 && DAT_001203e4 == 0 ? '1' : 'x');

    reset();
    set_standings();
    DAT_000007fd = 0x50;
    DAT_0011d24c = float_of(0x41a00000);
    championship_screen();
    put(rect_calls == 2 && panel_calls == 1
        && panel_text[0] == champ_text[6] && net_calls == 0
        && text_calls == 30 && DAT_001203e4 == 10 ? '2' : 'x');

    reset();
    set_list();
    DAT_000007fd = 0x40;
    DAT_0011d24c = float_of(0x40a00000);
    championship_screen();
    put(rect_calls == 0 && panel_calls == 1
        && panel_text[0] == DAT_0011fc60 && panel_y[0] == 0xbe
        && panel_font[0] == 0 && panel_color[0] == 2
        && net_calls == 0 && text_calls == 3 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
