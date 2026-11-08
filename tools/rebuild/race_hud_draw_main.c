/* The test harness for the rebuilt `race_hud_draw` module.
 *
 * The harness holds the race clock, the lap clock, the lap value, the two
 * counters, the replay flag, the frame counter, the five format strings, the
 * two clock buffers, and the call stubs.  The `sprintf` stub formats the
 * `%d`, the `%d%c`, and the `%02d:%02d:%02d` forms.  The harness runs five
 * paths:
 *
 *     0  the normal path formats the clock 125.75 as `02:05:75`, draws the
 *        centered lap value 3, the counters, the two clocks, the static label,
 *        the gauge panel, and the radar
 *     1  the replay flag is set and the frame counter bit 0x80 is set: only the
 *        replay label `R` draws after the clock format
 *     2  the replay flag is set and the frame counter bit is clear: nothing
 *        draws, and the clock 3661.5 formats as `61:01:50`
 *     3  the display value 20 stays above the position 10, the lap value -2.7
 *        truncates to -2, the clock 1.99 formats as `00:01:99`, and the offset
 *        -5 draws as `-5`
 *     4  the display value equals the position 10 and rises to 11, the clock
 *        59.99 formats as `00:59:99`, and the lap clock 2.5 formats as
 *        `00:02:50`
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_hud_draw(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float race_clock;
float race_lap_number;
float race_lap_clock;
int race_lap_display;
int race_lap_offset;
int race_position;
int race_replay;
unsigned char race_frame_counter;

char race_time_format[] = "%02d:%02d:%02d";
char race_count_format[] = "%d";
char race_value_format[] = "%d%c";
char race_lap_format[] = "%d";
char race_offset_format[] = "%d";
char race_best_format[] = "%02d:%02d:%02d";
char race_replay_label[] = "R";
char race_clock_text[12];
char race_clock_label[12];

float race_centi_scale;

static char text[8];
static char centered_copy[0x40];
static int centered_calls;
static int centered_y;
static int centered_font;
static int centered_color;
static int draw_calls;
static int draw_x[8];
static int draw_y[8];
static char draw_copy[8][0x40];
static int draw_font[8];
static int draw_color[8];
static int measure_calls;
static int measure_width;
static int measure_height;
static char measure_copy[0x20];
static int measure_font;
static int gauge_calls;
static int radar_calls;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0 && index < 0x3f; index++) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; index++) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

static char *emit_int(char *out, int value, int width, char pad)
{
    char digits[12];
    unsigned int magnitude;
    int negative;
    int length;
    int total;

    negative = value < 0;
    magnitude = negative ? (unsigned int)(-value) : (unsigned int)value;
    length = 0;
    if (magnitude == 0) {
        digits[length] = '0';
        length = length + 1;
    }
    while (magnitude != 0) {
        digits[length] = (char)('0' + magnitude % 10);
        length = length + 1;
        magnitude = magnitude / 10;
    }
    total = length + (negative ? 1 : 0);
    if (negative) {
        *out = '-';
        out = out + 1;
    }
    while (total < width) {
        *out = pad;
        out = out + 1;
        total = total + 1;
    }
    while (length > 0) {
        length = length - 1;
        *out = digits[length];
        out = out + 1;
    }
    return out;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *out;
    int index;
    int width;
    char pad;

    out = buffer;
    index = 0;
    va_start(arguments, format);
    while (format[index] != 0) {
        if (format[index] != '%') {
            *out = format[index];
            out = out + 1;
            index = index + 1;
            continue;
        }
        index = index + 1;
        pad = ' ';
        width = 0;
        if (format[index] == '0') {
            pad = '0';
            index = index + 1;
        }
        while (format[index] >= '0' && format[index] <= '9') {
            width = width * 10 + (format[index] - '0');
            index = index + 1;
        }
        if (format[index] == 'd') {
            out = emit_int(out, va_arg(arguments, int), width, pad);
            index = index + 1;
        } else if (format[index] == 'c') {
            *out = (char)va_arg(arguments, int);
            out = out + 1;
            index = index + 1;
        } else {
            *out = '%';
            out = out + 1;
        }
    }
    va_end(arguments);
    *out = 0;
    return out - buffer;
}

void panel_draw_centered(char *value, int y, int font, int color)
{
    centered_calls = centered_calls + 1;
    copy(centered_copy, value);
    centered_y = y;
    centered_font = font;
    centered_color = color;
}

void text_draw(int x, int y, char *value, int font, int color)
{
    if (draw_calls < 8) {
        draw_x[draw_calls] = x;
        draw_y[draw_calls] = y;
        copy(draw_copy[draw_calls], value);
        draw_font[draw_calls] = font;
        draw_color[draw_calls] = color;
    }
    draw_calls = draw_calls + 1;
}

void text_measure(int *width, int *height, char *value, int font)
{
    *width = measure_width;
    *height = measure_height;
    copy(measure_copy, value);
    measure_font = font;
    measure_calls = measure_calls + 1;
}

void race_gauges(void)
{
    gauge_calls = gauge_calls + 1;
}

void race_radar(void)
{
    radar_calls = radar_calls + 1;
}

static void reset(void)
{
    int index;

    race_clock = 0.0f;
    race_lap_number = 0.0f;
    race_lap_clock = 0.0f;
    race_lap_display = 0;
    race_lap_offset = 0;
    race_position = 0;
    race_replay = 0;
    race_frame_counter = 0;
    copy(race_clock_text, "00:00:00");
    copy(race_clock_label, "00:00:00");
    race_centi_scale = 100.0f;
    centered_calls = 0;
    centered_y = 0;
    centered_font = 0;
    centered_color = 0;
    draw_calls = 0;
    for (index = 0; index < 8; index++) {
        draw_x[index] = 0;
        draw_y[index] = 0;
        draw_copy[index][0] = 0;
        draw_font[index] = 0;
        draw_color[index] = 0;
    }
    measure_calls = 0;
    measure_width = 0;
    measure_height = 0;
    measure_copy[0] = 0;
    measure_font = 0;
    gauge_calls = 0;
    radar_calls = 0;
}

static int draw_is(int index, int x, int y, char *value, int font, int color)
{
    return index < draw_calls
        && draw_x[index] == x
        && draw_y[index] == y
        && same(draw_copy[index], value)
        && draw_font[index] == font
        && draw_color[index] == color;
}

int main(void)
{
    reset();
    race_clock = 125.75f;
    race_lap_number = 3.0f;
    race_lap_clock = 61.25f;
    race_lap_display = 4;
    race_lap_offset = 7;
    race_position = 10;
    race_replay = 0;
    race_frame_counter = 0;
    measure_width = 9;
    measure_height = 4;
    race_hud_draw();
    put(centered_calls == 1 && same(centered_copy, "3")
            && centered_y == 0x10 && centered_font == 6
            && centered_color == 2
            && draw_calls == 6
            && draw_is(0, 0x10, 0x16, "5:", 1, 5)
            && draw_is(1, 19, 0x28, "11", 4, 5)
            && draw_is(2, 0x228, 0x16, "7", 1, 5)
            && draw_is(3, 0x10, 0x70, "02:05:75", 3, 2)
            && draw_is(4, 0x10, 0x90, "01:01:25", 3, 2)
            && draw_is(5, 0x10, 0xb0, "00:00:00", 3, 2)
            && measure_calls == 1 && same(measure_copy, "5:")
            && measure_font == 1
            && gauge_calls == 1 && radar_calls == 1
            && same(race_clock_text, "02:05:75")
        ? '0' : 'x');

    reset();
    race_clock = 1.0f;
    race_replay = 5;
    race_frame_counter = 0x80;
    race_hud_draw();
    put(centered_calls == 0 && draw_calls == 1
            && draw_is(0, 0x10, 0x154, "R", 0, 6)
            && measure_calls == 0 && gauge_calls == 0 && radar_calls == 0
            && same(race_clock_text, "00:00:01")
        ? '1' : 'x');

    reset();
    race_clock = 3661.5f;
    race_replay = 5;
    race_frame_counter = 0;
    race_hud_draw();
    put(centered_calls == 0 && draw_calls == 0
            && measure_calls == 0 && gauge_calls == 0 && radar_calls == 0
            && same(race_clock_text, "61:01:50")
        ? '2' : 'x');

    reset();
    race_clock = 0.0f;
    race_lap_number = -2.7f;
    race_lap_clock = 1.99f;
    race_lap_display = 20;
    race_lap_offset = -5;
    race_position = 10;
    measure_width = 3;
    measure_height = 2;
    race_hud_draw();
    put(centered_calls == 1 && same(centered_copy, "-2")
            && draw_calls == 6
            && draw_is(0, 0x10, 0x16, "20:", 1, 5)
            && draw_is(1, 13, 0x28, "11", 4, 5)
            && draw_is(2, 0x228, 0x16, "-5", 1, 5)
            && draw_is(3, 0x10, 0x70, "00:00:00", 3, 2)
            && draw_is(4, 0x10, 0x90, "00:01:99", 3, 2)
            && draw_is(5, 0x10, 0xb0, "00:00:00", 3, 2)
            && measure_calls == 1
            && gauge_calls == 1 && radar_calls == 1
        ? '3' : 'x');

    reset();
    race_clock = 59.99f;
    race_lap_number = 1.0f;
    race_lap_clock = 2.5f;
    race_lap_display = 10;
    race_lap_offset = 0;
    race_position = 10;
    measure_width = 0;
    measure_height = 0;
    race_hud_draw();
    put(centered_calls == 1 && same(centered_copy, "1")
            && draw_calls == 6
            && draw_is(0, 0x10, 0x16, "11:", 1, 5)
            && draw_is(1, 10, 0x28, "11", 4, 5)
            && draw_is(2, 0x228, 0x16, "0", 1, 5)
            && draw_is(3, 0x10, 0x70, "00:59:99", 3, 2)
            && draw_is(4, 0x10, 0x90, "00:02:50", 3, 2)
            && draw_is(5, 0x10, 0xb0, "00:00:00", 3, 2)
            && measure_calls == 1
            && gauge_calls == 1 && radar_calls == 1
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
