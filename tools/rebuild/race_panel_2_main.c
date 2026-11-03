/* The test harness for the rebuilt `race_panel_2` module.
 *
 * The harness holds the title pointers, the 0x44-byte record table, the
 * indirect text table, the two row cells, the row value, the two counters, the
 * frame count, and the 0.6 threshold.  The `panel_draw_centered` stub records
 * the title call.  The `text_measure` stub returns the width and the height of
 * each call and records the text and the font.  The `text_draw` stub records
 * the doubled position, the text, the font, and the color.  The `sprintf` stub
 * writes the integer and ` KMH` into the buffer.  The `panel_show` stub records
 * the four panel calls.  The harness runs four paths:
 *
 *     0  the index 0 path draws the two table texts, the three title texts, the
 *        formatted value, and the two text panels with the sizes 34 and 49;
 *        the first state panel stays hidden and the second takes the size 3
 *     1  the index 1 path selects the second record and the second indirect
 *        text, the first state panel takes the size 2, and the second state
 *        panel stays hidden
 *     2  the index 2 path holds the count 0, the low row value 0.5 gives the
 *        first state panel the size 0, and the second state panel stays hidden
 *     3  the negative width -3 halves to -1 toward zero, so the first two draws
 *        sit at x 0x140 and the titles at x 0x100
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_panel_2(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char *DAT_0011fc30;
int DAT_001203c4;
unsigned char *DAT_0011d550;
char *DAT_0011fc14[8];
char *DAT_0011fc1c;
char *DAT_0011fc20;
char *DAT_0011fc24;
int DAT_001203b4;
int DAT_0012039c;
int DAT_001203a8;
float DAT_0012037c;
int DAT_001207f0;
double race_panel_2_threshold;

static char text[8];
static char title_0[8];
static char string_a[8];
static char string_b[8];
static char string_c[8];
static char text_a[8];
static char text_b[8];
static char text_c[8];
static char text_d[8];
static char text_e[8];
static char text_f[8];
static unsigned char table[0x44 * 3];
static int title_calls;
static char *title_text;
static int title_y;
static int title_font;
static int title_color;
static int measure_width[4];
static int measure_height[4];
static int measure_calls;
static char *measure_text[4];
static int measure_font[4];
static char measure_copy[4][0x40];
static int draw_calls;
static int draw_x[6];
static int draw_y[6];
static char *draw_text[6];
static int draw_font[6];
static int draw_color[6];
static char draw_copy[6][0x40];
static int show_calls;
static int show_x[7];
static int show_y[7];
static int show_size[7];
static char expected_aa[3];
static char expected_a[2];
static char expected_aaa[4];
static char expected_123[8];
static char expected_neg5[8];
static char expected_7[7];
static char expected_9[7];

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

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0 && index < 0x3f; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
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

static char *record_text(int index)
{
    return (char *)(table + index * 0x44 + 0x14);
}

static void set_record(int index, char *value, int indirect, int number,
                       int first_size, int second_size)
{
    copy(record_text(index), value);
    *(int *)(table + index * 0x44 + 0x30) = indirect;
    *(int *)(table + index * 0x44 + 0x34) = number;
    *(int *)(table + index * 0x44 + 0x38) = first_size;
    *(int *)(table + index * 0x44 + 0x3c) = second_size;
}

void panel_draw_centered(char *value, int y, int font, int color)
{
    title_text = value;
    title_y = y;
    title_font = font;
    title_color = color;
    title_calls = title_calls + 1;
}

void text_measure(int *width, int *height, char *value, int font)
{
    *width = measure_width[measure_calls];
    *height = measure_height[measure_calls];
    measure_text[measure_calls] = value;
    measure_font[measure_calls] = font;
    copy(measure_copy[measure_calls], value);
    measure_calls = measure_calls + 1;
}

void text_draw(int x, int y, char *value, int font, int color)
{
    if (draw_calls < 6) {
        draw_x[draw_calls] = x;
        draw_y[draw_calls] = y;
        draw_text[draw_calls] = value;
        draw_font[draw_calls] = font;
        draw_color[draw_calls] = color;
        copy(draw_copy[draw_calls], value);
    }
    draw_calls = draw_calls + 1;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char digits[12];
    int value;
    int length;
    int index;
    int negative;

    va_start(arguments, format);
    value = va_arg(arguments, int);
    va_end(arguments);
    negative = value < 0;
    length = 0;
    if (value == 0) {
        digits[length] = '0';
        length = length + 1;
    }
    while (value != 0) {
        int digit;

        digit = value % 10;
        if (digit < 0) {
            digit = -digit;
        }
        digits[length] = (char)('0' + digit);
        length = length + 1;
        value = value / 10;
    }
    index = 0;
    if (negative) {
        buffer[index] = '-';
        index = index + 1;
    }
    while (length > 0) {
        length = length - 1;
        buffer[index] = digits[length];
        index = index + 1;
    }
    buffer[index] = ' ';
    buffer[index + 1] = 'K';
    buffer[index + 2] = 'M';
    buffer[index + 3] = 'H';
    buffer[index + 4] = 0;
    buffer[0x7f] = 0;
    return index;
}

void panel_show(int x, int y, int size)
{
    if (show_calls < 7) {
        show_x[show_calls] = x;
        show_y[show_calls] = y;
        show_size[show_calls] = size;
    }
    show_calls = show_calls + 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x44 * 3; index++) {
        table[index] = 0;
    }
    title_0[0] = 'T';
    title_0[1] = '0';
    title_0[2] = 0;
    string_a[0] = 'A';
    string_a[1] = 0;
    string_b[0] = 'B';
    string_b[1] = 0;
    string_c[0] = 'C';
    string_c[1] = 0;
    text_a[0] = 'a';
    text_a[1] = 0;
    text_b[0] = 'b';
    text_b[1] = 0;
    text_c[0] = 'c';
    text_c[1] = 0;
    text_d[0] = 'd';
    text_d[1] = 0;
    text_e[0] = 'e';
    text_e[1] = 0;
    text_f[0] = 'f';
    text_f[1] = 0;
    DAT_0011fc30 = title_0;
    DAT_0011fc1c = string_a;
    DAT_0011fc20 = string_b;
    DAT_0011fc24 = string_c;
    for (index = 0; index < 8; index++) {
        DAT_0011fc14[index] = 0;
    }
    DAT_0011d550 = table;
    DAT_001203c4 = 0;
    DAT_001203b4 = 0;
    DAT_0012039c = 0;
    DAT_001203a8 = 0;
    set_bits_float(&DAT_0012037c, 0x00000000);
    DAT_001207f0 = 0;
    set_bits_double(&race_panel_2_threshold, 0x3fe33333, 0x33333333);
    expected_aa[0] = 'A';
    expected_aa[1] = 'A';
    expected_aa[2] = 0;
    expected_a[0] = 'A';
    expected_a[1] = 0;
    expected_aaa[0] = 'A';
    expected_aaa[1] = 'A';
    expected_aaa[2] = 'A';
    expected_aaa[3] = 0;
    expected_123[0] = '1';
    expected_123[1] = '2';
    expected_123[2] = '3';
    expected_123[3] = ' ';
    expected_123[4] = 'K';
    expected_123[5] = 'M';
    expected_123[6] = 'H';
    expected_123[7] = 0;
    expected_neg5[0] = '-';
    expected_neg5[1] = '5';
    expected_neg5[2] = ' ';
    expected_neg5[3] = 'K';
    expected_neg5[4] = 'M';
    expected_neg5[5] = 'H';
    expected_neg5[6] = 0;
    expected_7[0] = '7';
    expected_7[1] = ' ';
    expected_7[2] = 'K';
    expected_7[3] = 'M';
    expected_7[4] = 'H';
    expected_7[5] = 0;
    expected_9[0] = '9';
    expected_9[1] = ' ';
    expected_9[2] = 'K';
    expected_9[3] = 'M';
    expected_9[4] = 'H';
    expected_9[5] = 0;
    title_calls = 0;
    title_text = 0;
    title_y = 0;
    title_font = 0;
    title_color = 0;
    measure_calls = 0;
    draw_calls = 0;
    show_calls = 0;
}

static void setup_measure(int width_a, int height_a, int width_b, int height_b,
                          int width_c, int height_c)
{
    measure_width[0] = width_a;
    measure_height[0] = height_a;
    measure_width[1] = width_b;
    measure_height[1] = height_b;
    measure_width[2] = width_c;
    measure_height[2] = height_c;
}

static int title_is(char *value, int y, int font, int color)
{
    return title_text == value && title_y == y && title_font == font
        && title_color == color;
}

static int draw_is(int index, int x, int y, char *value, int font, int color)
{
    return draw_x[index] == x && draw_y[index] == y
        && draw_text[index] == value && draw_font[index] == font
        && draw_color[index] == color;
}

static int show_is(int index, int x, int y, int size)
{
    return show_x[index] == x && show_y[index] == y && show_size[index] == size;
}

int main(void)
{
    reset();
    setup_measure(10, 8, 10, 8, 10, 8);
    set_record(0, text_a, 0, 123, 30, 40);
    DAT_0011fc14[0] = text_b;
    DAT_001203c4 = 0;
    DAT_001207f0 = 0;
    DAT_0012039c = 2;
    DAT_001203a8 = 1;
    set_bits_float(&DAT_0012037c, 0x3f333333);
    DAT_001203b4 = 3;
    race_panel_2();
    put(draw_calls == 6 && title_calls == 1 && title_is(title_0, 2, 3, 2)
        && measure_calls == 3 && measure_text[0] == record_text(0)
        && measure_font[0] == 0 && measure_text[1] == text_b
        && measure_font[1] == 0 && measure_font[2] == 1
        && same(measure_copy[2], expected_aaa)
        && draw_is(0, 0x13a, 0x18, record_text(0), 0, 2)
        && draw_is(1, 0x13a, 0x168, text_b, 0, 2)
        && draw_is(2, 0xfa, 0x17c, DAT_0011fc1c, 1, 2)
        && draw_is(3, 0x104, 0x17c, measure_text[2], 1, 2)
        && same(draw_copy[3], expected_123)
        && draw_is(4, 0xfa, 0x184, DAT_0011fc20, 1, 2)
        && draw_is(5, 0xfa, 0x18c, DAT_0011fc24, 1, 2)
        && show_calls == 3
        && show_is(0, 0x82, 0xc3, 34)
        && show_is(1, 0x82, 0xc7, 49)
        && show_is(2, 0xee, 0x64, 3) ? '0' : 'x');

    reset();
    setup_measure(10, 8, 10, 8, 10, 8);
    set_record(1, text_c, 1, -5, 0, 1);
    DAT_0011fc14[1] = text_d;
    DAT_001203c4 = 1;
    DAT_001207f0 = -10;
    DAT_0012039c = 0;
    DAT_001203a8 = 1;
    set_bits_float(&DAT_0012037c, 0x3f333333);
    DAT_001203b4 = 2;
    race_panel_2();
    put(draw_calls == 6 && title_calls == 1 && title_is(title_0, 2, 3, 2)
        && measure_calls == 3 && measure_text[0] == record_text(1)
        && measure_text[1] == text_d && same(measure_copy[2], expected_aa)
        && draw_is(0, 0x13a, 0x18, record_text(1), 0, 2)
        && draw_is(1, 0x13a, 0x168, text_d, 0, 2)
        && draw_is(2, 0xfa, 0x17c, DAT_0011fc1c, 1, 2)
        && draw_is(3, 0x104, 0x17c, measure_text[2], 1, 2)
        && same(draw_copy[3], expected_neg5)
        && draw_is(4, 0xfa, 0x184, DAT_0011fc20, 1, 2)
        && draw_is(5, 0xfa, 0x18c, DAT_0011fc24, 1, 2)
        && show_calls == 3 && show_is(0, 0x82, 0xc3, 4)
        && show_is(1, 0x82, 0xc7, 10) && show_is(2, 8, 0x64, 2) ? '1' : 'x');

    reset();
    setup_measure(10, 8, 10, 8, 10, 8);
    set_record(2, text_e, 0, 7, 5, 2);
    DAT_0011fc14[0] = text_f;
    DAT_001203c4 = 2;
    DAT_001207f0 = -9;
    DAT_0012039c = 1;
    DAT_001203a8 = 2;
    set_bits_float(&DAT_0012037c, 0x3f000000);
    DAT_001203b4 = 0;
    race_panel_2();
    put(draw_calls == 6 && title_calls == 1 && title_is(title_0, 2, 3, 2)
        && measure_calls == 3 && measure_text[0] == record_text(2)
        && measure_text[1] == text_f && measure_copy[2][0] == 0
        && draw_is(0, 0x13a, 0x18, record_text(2), 0, 2)
        && draw_is(1, 0x13a, 0x168, text_f, 0, 2)
        && draw_is(3, 0x104, 0x17c, measure_text[2], 1, 2)
        && same(draw_copy[3], expected_7)
        && show_calls == 3 && show_is(0, 0x82, 0xc3, 9)
        && show_is(1, 0x82, 0xc7, 11) && show_is(2, 8, 0x64, 0) ? '2' : 'x');

    reset();
    setup_measure(-3, 8, -3, 8, -3, 8);
    set_record(0, text_a, 0, 9, 30, 40);
    DAT_0011fc14[0] = text_b;
    DAT_001203c4 = 0;
    DAT_001207f0 = 0;
    DAT_0012039c = 2;
    DAT_001203a8 = 1;
    set_bits_float(&DAT_0012037c, 0x3f333333);
    DAT_001203b4 = 1;
    race_panel_2();
    put(draw_calls == 6 && title_calls == 1 && title_is(title_0, 2, 3, 2)
        && measure_calls == 3 && same(measure_copy[2], expected_a)
        && draw_is(0, 0x140, 0x18, record_text(0), 0, 2)
        && draw_is(1, 0x140, 0x168, text_b, 0, 2)
        && draw_is(2, 0x100, 0x17c, DAT_0011fc1c, 1, 2)
        && draw_is(3, 0xfe, 0x17c, measure_text[2], 1, 2)
        && same(draw_copy[3], expected_9)
        && draw_is(4, 0x100, 0x184, DAT_0011fc20, 1, 2)
        && draw_is(5, 0x100, 0x18c, DAT_0011fc24, 1, 2)
        && show_calls == 3
        && show_is(0, 0x7f, 0xc3, 34)
        && show_is(1, 0x7f, 0xc7, 49)
        && show_is(2, 0xee, 0x64, 3) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
