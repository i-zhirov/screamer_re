/* The test harness for the rebuilt `race_gauges` module.
 *
 * The harness holds the cockpit resource pointers, the video buffer, the two
 * gauge points, the gauge table, the two angle cells, the mode, the speed, the
 * index, and the phase.  The stubs record the blit source and destination, the
 * filter cell, target, and index, the marker point, radius, bias, and angle,
 * the sprintf buffer, format, and value, and the text draw arguments.
 *
 * The paths:
 *
 *     0  mode 0 sets the clip globals and runs only the common blit
 *     1  mode 1 runs the COCK blit, the two filters, the two markers, the
 *        object table stores, and the object list, with no angle clamp
 *     2  mode 1 clamps the side angle to the side scale
 *     3  mode 2 runs the cdfront blit, the two counter texts, the side filter,
 *        and the local marker, with no angle clamp
 *     4  mode 2 clamps the side angle to the double pi
 *     5  mode 2 truncates the negative counter product toward zero
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_gauges(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct gauge_point {
    int x;
    int y;
};

void *DAT_0018f18;
void *DAT_0018f0c;
void *DAT_0018f10;
int DAT_0018f1c;
int DAT_0018f20;
struct gauge_point DAT_0018f24;
struct gauge_point DAT_0018f30;
int DAT_0018f3c[12];

float DAT_0000fd9c;
float DAT_0000fda0;
float DAT_0000fe28;
float DAT_00100b8;
float DAT_00100bc;
int DAT_00100c0;
float DAT_00100c4;
int DAT_00100c8;

unsigned char DAT_0000055b;
int DAT_0000056c;
int DAT_00000570;
int DAT_00000574;
int DAT_00000578;

int DAT_001cb18;
int DAT_001cb1c;
int DAT_001cb20;
void *DAT_0021528;

char DAT_00000e02[] = "%03d";
char DAT_00000e08[] = "%d";

double gauge_scale_front = 4.084070563316345;
double gauge_scale_side = 3.612831652164459;
double gauge_tick = 0.017;
float gauge_pi = 3.1415927410125732f;
double gauge_pi_max = 3.1415927410125732;

static char text[8];
static int buffer_source;
static int buffer_target;
static void *blit_source[4];
static void *blit_target[4];
static int filter_calls;
static float *filter_cell[4];
static float filter_target[4];
static int filter_index[4];
static int marker_calls;
static struct gauge_point marker_point[4];
static int marker_radius[4];
static int marker_bias[4];
static float marker_angle[4];
static int sprintf_calls;
static char *sprintf_format[4];
static int text_calls;
static int text_x[4];
static int text_y[4];
static char text_copy[4][0x20];
static int text_font[4];
static int text_color[4];
static int object_calls;

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

    for (index = 0; source[index] != 0 && index < 0x1f; index++) {
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

static int close_enough(float left, float right)
{
    return left == right;
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

void data_blit(void *source, void *destination)
{
    if (buffer_source < 4) {
        blit_source[buffer_source] = source;
        blit_target[buffer_source] = destination;
    }
    buffer_source = buffer_source + 1;
}

void filter_update(float *value, float target, int index)
{
    if (filter_calls < 4) {
        filter_cell[filter_calls] = value;
        filter_target[filter_calls] = target;
        filter_index[filter_calls] = index;
    }
    filter_calls = filter_calls + 1;
    *value = target;
}

void gauge_marker_draw(struct gauge_point *point, int radius, int angle_bias,
    float angle)
{
    if (marker_calls < 4) {
        marker_point[marker_calls] = *point;
        marker_radius[marker_calls] = radius;
        marker_bias[marker_calls] = angle_bias;
        marker_angle[marker_calls] = angle;
    }
    marker_calls = marker_calls + 1;
}

void draw_objects(void)
{
    object_calls = object_calls + 1;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *out;
    int index;
    int width;
    int argument;
    char pad;

    out = buffer;
    index = 0;
    argument = 0;
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
            argument = va_arg(arguments, int);
            out = emit_int(out, argument, width, pad);
            index = index + 1;
        } else {
            *out = '%';
            out = out + 1;
        }
    }
    va_end(arguments);
    *out = 0;
    if (sprintf_calls < 4) {
        sprintf_format[sprintf_calls] = (char *)format;
    }
    sprintf_calls = sprintf_calls + 1;
    return out - buffer;
}

void text_draw(int x, int y, char *value, int font, int color)
{
    if (text_calls < 4) {
        text_x[text_calls] = x;
        text_y[text_calls] = y;
        copy(text_copy[text_calls], value);
        text_font[text_calls] = font;
        text_color[text_calls] = color;
    }
    text_calls = text_calls + 1;
}

static void reset(void)
{
    int index;

    DAT_0018f18 = (void *)0x1000;
    DAT_0018f0c = (void *)0x2000;
    DAT_0018f10 = (void *)0x3000;
    DAT_0018f1c = 0;
    DAT_0018f20 = 0;
    DAT_0018f24.x = 0;
    DAT_0018f24.y = 0;
    DAT_0018f30.x = 0;
    DAT_0018f30.y = 0;
    for (index = 0; index < 12; index++) {
        DAT_0018f3c[index] = 0;
    }
    DAT_0000fd9c = 0.0f;
    DAT_0000fda0 = 0.0f;
    DAT_0000fe28 = 0.0f;
    DAT_00100b8 = 0.0f;
    DAT_00100bc = 0.0f;
    DAT_00100c0 = 0;
    DAT_00100c4 = 0.0f;
    DAT_00100c8 = 0;
    DAT_0000055b = 0;
    DAT_0000056c = 0;
    DAT_00000570 = 0;
    DAT_00000574 = 0;
    DAT_00000578 = 0;
    DAT_001cb18 = 0;
    DAT_001cb1c = 0;
    DAT_001cb20 = 0;
    DAT_0021528 = (void *)0x4000;
    buffer_source = 0;
    buffer_target = 0;
    for (index = 0; index < 4; index++) {
        blit_source[index] = 0;
        blit_target[index] = 0;
        filter_cell[index] = 0;
        filter_target[index] = 0.0f;
        filter_index[index] = 0;
        marker_point[index].x = 0;
        marker_point[index].y = 0;
        marker_radius[index] = 0;
        marker_bias[index] = 0;
        marker_angle[index] = 0.0f;
        sprintf_format[index] = 0;
        text_x[index] = 0;
        text_y[index] = 0;
        text_copy[index][0] = 0;
        text_font[index] = 0;
        text_color[index] = 0;
    }
    filter_calls = 0;
    marker_calls = 0;
    sprintf_calls = 0;
    text_calls = 0;
    object_calls = 0;
}

static int blit_is(int index, void *source, void *target)
{
    return index < buffer_source
        && blit_source[index] == source
        && blit_target[index] == target;
}

static int text_is(int index, int x, int y, char *value, int font, int color)
{
    return index < text_calls
        && text_x[index] == x
        && text_y[index] == y
        && same(text_copy[index], value)
        && text_font[index] == font
        && text_color[index] == color;
}

int main(void)
{
    int index;
    float front;
    float side;

    reset();
    DAT_00100c0 = 0;
    DAT_0000055b = 0x11;
    race_gauges();
    put(buffer_source == 1
            && blit_is(0, DAT_0018f18, DAT_0021528)
            && DAT_0000056c == 0 && DAT_00000570 == 0x280
            && DAT_00000574 == 0 && DAT_00000578 == 0x190
            && filter_calls == 0 && marker_calls == 0
            && text_calls == 0 && object_calls == 0
            && DAT_0000055b == 0x11
        ? '0' : 'x');

    reset();
    DAT_00100c0 = 1;
    DAT_0000fe28 = 1.0f;
    DAT_0000fd9c = 2.0f;
    DAT_00100c4 = 1.0f;
    DAT_0000fda0 = 2.0f;
    DAT_0018f1c = 50;
    DAT_0018f20 = 100;
    DAT_00100c8 = 2;
    for (index = 0; index < 12; index++) {
        DAT_0018f3c[index] = (index + 1) * 10;
    }
    front = (float)((double)1.0 / 2.0 * gauge_scale_front);
    side = (float)((double)1.0 / 2.0 * gauge_scale_side);
    race_gauges();
    put(buffer_source == 2
            && blit_is(0, DAT_0018f18, DAT_0021528)
            && blit_is(1, DAT_0018f0c, DAT_0021528)
            && DAT_0000055b == 0xff
            && filter_calls == 2
            && filter_cell[0] == &DAT_00100b8 && close_enough(filter_target[0], front)
            && filter_index[0] == 0x5a
            && filter_cell[1] == &DAT_00100bc && close_enough(filter_target[1], side)
            && filter_index[1] == 0x5a
            && marker_calls == 2
            && marker_point[0].x == 0 && marker_point[0].y == 0
            && marker_radius[0] == 50 && marker_bias[0] == 2
            && close_enough(marker_angle[0], front)
            && marker_point[1].x == 0 && marker_point[1].y == 0
            && marker_radius[1] == 50 && marker_bias[1] == 2
            && close_enough(marker_angle[1], side)
            && DAT_001cb18 == 70 && DAT_001cb1c == 80
            && DAT_001cb20 == 102
            && object_calls == 1
        ? '1' : 'x');

    reset();
    DAT_00100c0 = 1;
    DAT_0000fe28 = 1.0f;
    DAT_0000fd9c = 2.0f;
    DAT_00100c4 = 10.0f;
    DAT_0000fda0 = 1.0f;
    DAT_0018f1c = 50;
    side = (float)gauge_scale_side;
    race_gauges();
    put(filter_calls == 2 && close_enough(filter_target[1], side)
            && close_enough(marker_angle[1], side)
        ? '2' : 'x');

    reset();
    DAT_00100c0 = 2;
    DAT_0000fe28 = 100.0f;
    DAT_00100c4 = 1.0f;
    DAT_0000fda0 = 1.0f;
    DAT_00100c8 = 2;
    side = (float)gauge_pi;
    race_gauges();
    put(buffer_source == 2
            && blit_is(0, DAT_0018f18, DAT_0021528)
            && blit_is(1, DAT_0018f10, DAT_0021528)
            && DAT_0000055b == 0x75
            && sprintf_calls == 2
            && sprintf_format[0] == DAT_00000e02
            && sprintf_format[1] == DAT_00000e08
            && text_is(0, 0x21e, 0x176, "001", 2, 7)
            && text_is(1, 0x260, 0x176, "2", 2, 1)
            && filter_calls == 1
            && filter_cell[0] == &DAT_00100bc && close_enough(filter_target[0], side)
            && filter_index[0] == 0x5a
            && marker_calls == 1
            && marker_point[0].x == 0x10f && marker_point[0].y == 0xa8
            && marker_radius[0] == 0x1e && marker_bias[0] == 2
            && close_enough(marker_angle[0], side)
        ? '3' : 'x');

    reset();
    DAT_00100c0 = 2;
    DAT_0000fe28 = 0.0f;
    DAT_00100c4 = 10.0f;
    DAT_0000fda0 = 1.0f;
    DAT_00100c8 = 2;
    side = (float)gauge_pi_max;
    race_gauges();
    put(filter_calls == 1 && close_enough(filter_target[0], side)
            && sprintf_calls == 2
            && text_is(0, 0x21e, 0x176, "000", 2, 7)
        ? '4' : 'x');

    reset();
    DAT_00100c0 = 2;
    DAT_0000fe28 = 50.0f;
    DAT_00100c4 = 1.0f;
    DAT_0000fda0 = 1.0f;
    DAT_00100c8 = 2;
    race_gauges();
    put(sprintf_calls == 2
            && text_is(0, 0x21e, 0x176, "000", 2, 7)
        ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
