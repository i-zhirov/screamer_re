/* The test harness for the rebuilt `race_records_screen` module.
 *
 * The harness holds the six-entry title pointer table, the two header pointers,
 * the selected track and the selected entry, the float 100.0, the six lap
 * records, and the six race records.  The `video_rect_copy` and
 * `panel_draw_centered` stubs record their arguments and copy the text of the
 * row, because the module reuses one buffer.  The `sprintf` stub reads the
 * format literal and rebuilds the position, the name, and the time.
 *
 * The harness runs four paths:
 *
 *     0  track 1 and entry 0: the first race row has a replay flag and the
 *        suffix ` R`, the other rows have the two trailing spaces, and the
 *        first race row uses the color 1
 *     1  track 3 and entry 2: the table offset follows the track, the third
 *        race row uses the color 1, and the other rows use the color 0
 *     2  track 5 and no selected entry: no row uses the color 1, the last race
 *        row has the replay suffix, and a lap time above 59 minutes keeps the
 *        minute 60
 *     D  the exit marker
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_records_screen(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define RECORD_SIZE 0x50
#define TRACK_COUNT 6
#define PANEL_LIMIT 16

int DAT_0012098c;
int DAT_00120990;
char *DAT_0011fbfc[TRACK_COUNT];
char *DAT_0011fc28;
char *DAT_0011fc2c;
float DAT_001012a2;
unsigned char DAT_00120430[TRACK_COUNT * RECORD_SIZE];
unsigned char DAT_00120610[TRACK_COUNT * RECORD_SIZE];

static char text[8];
static char title[TRACK_COUNT][4];
static char header_one[8];
static char header_two[8];
static int rect_calls;
static int rect_x;
static int rect_y;
static int rect_width;
static int rect_height;
static int panel_calls;
static char panel_text[PANEL_LIMIT][0x40];
static int panel_y[PANEL_LIMIT];
static int panel_font[PANEL_LIMIT];
static int panel_color[PANEL_LIMIT];

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

static int string_length(char *value)
{
    int length;

    length = 0;
    while (value[length] != 0) {
        length = length + 1;
    }
    return length;
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
    if (panel_calls < PANEL_LIMIT) {
        copy(panel_text[panel_calls], value);
        panel_y[panel_calls] = y;
        panel_font[panel_calls] = font;
        panel_color[panel_calls] = color;
    }
    panel_calls = panel_calls + 1;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *out;
    const char *p;
    char *label;
    int width;
    int length;
    int value;

    va_start(arguments, format);
    out = buffer;
    p = format;
    while (*p != 0) {
        if (*p != '%') {
            *out++ = *p++;
            continue;
        }
        p = p + 1;
        width = 0;
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p = p + 1;
        }
        if (*p == 'd') {
            value = va_arg(arguments, int);
            if (width == 2) {
                out = put_two(out, value);
            } else {
                out = put_decimal(out, value);
            }
            p = p + 1;
        } else if (*p == 's') {
            label = va_arg(arguments, char *);
            length = string_length(label);
            while (width > length) {
                *out++ = ' ';
                width = width - 1;
            }
            while (*label != 0) {
                *out++ = *label++;
            }
            p = p + 1;
        } else {
            *out++ = '%';
        }
    }
    *out = 0;
    va_end(arguments);
    return 0;
}

static void set_float(unsigned char *table, int track, int slot,
                      unsigned int bits)
{
    *(float *)(table + track * RECORD_SIZE + slot * 4) = float_of(bits);
}

static void set_name(unsigned char *table, int track, int slot,
                     char first, char second, char third)
{
    char *name;

    name = (char *)(table + track * RECORD_SIZE + 0x14 + slot * 4);
    name[0] = first;
    name[1] = second;
    name[2] = third;
    name[3] = 0;
}

static void set_flag(unsigned char *table, int track, int slot, int value)
{
    *(int *)(table + track * RECORD_SIZE + 0x28 + slot * 4) = value;
}

static void reset(void)
{
    int index;

    for (index = 0; index < TRACK_COUNT; ++index) {
        title[index][0] = 'T';
        title[index][1] = (char)('0' + index);
        title[index][2] = 0;
        title[index][3] = 0;
        DAT_0011fbfc[index] = title[index];
    }
    header_one[0] = 'H';
    header_one[1] = 'D';
    header_one[2] = 'R';
    header_one[3] = '1';
    header_one[4] = 0;
    header_two[0] = 'H';
    header_two[1] = 'D';
    header_two[2] = 'R';
    header_two[3] = '2';
    header_two[4] = 0;
    DAT_0011fc28 = header_one;
    DAT_0011fc2c = header_two;
    DAT_0012098c = 0;
    DAT_00120990 = -1;
    DAT_001012a2 = float_of(0x42c80000);
    for (index = 0; index < TRACK_COUNT * RECORD_SIZE; ++index) {
        DAT_00120430[index] = 0;
        DAT_00120610[index] = 0;
    }
    rect_calls = 0;
    rect_x = 0;
    rect_y = 0;
    rect_width = 0;
    rect_height = 0;
    panel_calls = 0;
}

static void set_track_one(void)
{
    DAT_0012098c = 1;
    DAT_00120990 = 0;
    set_float(DAT_00120610, 1, 0, 0x42828000);
    set_float(DAT_00120610, 1, 1, 0x428d0000);
    set_float(DAT_00120610, 1, 2, 0x42a18000);
    set_float(DAT_00120610, 1, 3, 0x42b40000);
    set_float(DAT_00120610, 1, 4, 0x42c80000);
    set_name(DAT_00120610, 1, 0, 'A', 'B', 'C');
    set_name(DAT_00120610, 1, 1, 'D', 'E', 'F');
    set_name(DAT_00120610, 1, 2, 'G', 'H', 'I');
    set_name(DAT_00120610, 1, 3, 'J', 'K', 'L');
    set_name(DAT_00120610, 1, 4, 'M', 'N', 'O');
    set_flag(DAT_00120610, 1, 0, 1);
    set_float(DAT_00120430, 1, 0, 0x42710000);
    set_float(DAT_00120430, 1, 1, 0x42760000);
    set_float(DAT_00120430, 1, 2, 0x427b0000);
    set_float(DAT_00120430, 1, 3, 0x427c0000);
    set_float(DAT_00120430, 1, 4, 0x42800000);
    set_name(DAT_00120430, 1, 0, 'P', 'Q', 'R');
    set_name(DAT_00120430, 1, 1, 'S', 'T', 'U');
    set_name(DAT_00120430, 1, 2, 'V', 'W', 'X');
    set_name(DAT_00120430, 1, 3, 'Y', 'Z', 'A');
    set_name(DAT_00120430, 1, 4, 'B', 'C', 'D');
}

static void set_track_three(void)
{
    DAT_0012098c = 3;
    DAT_00120990 = 2;
    set_float(DAT_00120610, 3, 0, 0x42fb0000);
    set_float(DAT_00120610, 3, 1, 0x43020000);
    set_float(DAT_00120610, 3, 2, 0x43074000);
    set_float(DAT_00120610, 3, 3, 0x430c0000);
    set_float(DAT_00120610, 3, 4, 0x43118000);
    set_name(DAT_00120610, 3, 0, 'R', 'A', 'C');
    set_name(DAT_00120610, 3, 1, 'R', 'A', 'C');
    set_name(DAT_00120610, 3, 2, 'S', 'E', 'L');
    set_name(DAT_00120610, 3, 3, 'R', 'A', 'C');
    set_name(DAT_00120610, 3, 4, 'R', 'A', 'C');
    set_float(DAT_00120430, 3, 0, 0x42760000);
    set_float(DAT_00120430, 3, 1, 0x427a0000);
    set_float(DAT_00120430, 3, 2, 0x427e0000);
    set_float(DAT_00120430, 3, 3, 0x42810000);
    set_float(DAT_00120430, 3, 4, 0x42830000);
    set_name(DAT_00120430, 3, 0, 'L', 'A', 'P');
    set_name(DAT_00120430, 3, 1, 'L', 'A', 'P');
    set_name(DAT_00120430, 3, 2, 'S', 'E', 'T');
    set_name(DAT_00120430, 3, 3, 'L', 'A', 'P');
    set_name(DAT_00120430, 3, 4, 'L', 'A', 'P');
}

static void set_track_five(void)
{
    int slot;

    DAT_0012098c = 5;
    DAT_00120990 = -1;
    for (slot = 0; slot < 5; ++slot) {
        set_float(DAT_00120610, 5, slot, 0x42c80000 + slot * 0x8000);
        set_float(DAT_00120430, 5, slot, 0x42800000 + slot * 0x8000);
        set_name(DAT_00120610, 5, slot, 'F', 'I', 'V');
        set_name(DAT_00120430, 5, slot, 'F', 'L', 'P');
    }
    set_float(DAT_00120430, 5, 0, 0x45610400);
    set_name(DAT_00120430, 5, 0, 'L', 'O', 'G');
    set_flag(DAT_00120610, 5, 4, 2);
}

static int colors_are_zero(void)
{
    int index;

    for (index = 3; index < 13; ++index) {
        if (panel_color[index] != 0) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    reset();
    set_track_one();
    race_records_screen();
    put(rect_calls == 2
        && rect_x == 0x60
        && rect_y == 0x48
        && rect_width == 0x1c4
        && rect_height == 0x140
        && panel_calls == 13
        && panel_y[0] == 2
        && panel_font[0] == 3
        && panel_color[0] == 0
        && same(panel_text[0], title[1])
        && panel_y[1] == 0x28
        && panel_font[1] == 0
        && panel_color[1] == 0
        && same(panel_text[1], header_one)
        && panel_y[2] == 0x78
        && panel_font[2] == 0
        && panel_color[2] == 0
        && same(panel_text[2], header_two)
        && panel_y[3] == 0x3c
        && panel_font[3] == 1
        && panel_color[3] == 1
        && same(panel_text[3], "\x011. ABC 01.05.25 R")
        && panel_y[4] == 0x46
        && panel_font[4] == 1
        && panel_color[4] == 0
        && same(panel_text[4], "\x012. DEF 01.10.50  ")
        && same(panel_text[7], "\x015. MNO 01.40.00  ")
        && panel_y[8] == 0x8c
        && panel_font[8] == 1
        && panel_color[8] == 0
        && same(panel_text[8], "\x011. PQR 01.00.25  ")
        && same(panel_text[12], "\x015. BCD 01.04.00  ")
        && panel_y[12] == 0xb4
        ? '0' : 'x');

    reset();
    set_track_three();
    race_records_screen();
    put(panel_calls == 13
        && same(panel_text[0], title[3])
        && panel_color[3] == 0
        && panel_color[5] == 1
        && panel_color[6] == 0
        && same(panel_text[5], "\x013. SEL 02.15.25  ")
        && panel_color[8] == 0
        && same(panel_text[8], "\x011. LAP 01.01.50  ")
        ? '1' : 'x');

    reset();
    set_track_five();
    race_records_screen();
    put(panel_calls == 13
        && colors_are_zero()
        && same(panel_text[0], title[5])
        && same(panel_text[7], "\x015. FIV 01.41.00 R")
        && same(panel_text[8], "\x011. LOG 60.00.25  ")
        && same(panel_text[12], "\x015. FLP 01.05.00  ")
        ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
