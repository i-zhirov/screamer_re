/* The test harness for the rebuilt `race_lap_complete` module.
 *
 * The harness holds the car state, the node count, the global best lap, the
 * race position, the flag byte, the time buffer, the format string, the 100.0
 * scale, and the two call stubs.  The `sprintf` stub formats the
 * `%02d:%02d:%02d` form.  The harness runs nine paths:
 *
 *     0  the new best lap 10.0 updates the car and the global best, formats
 *        `00:00:10`, pushes the effect 7 with 0.2, and pushes the effect 5 with
 *        the subtype 1; the loop clears the flags from index 1 to 4
 *     1  the lap 10.0 stays above the car best 5.0 and the global best 3.0:
 *        only the effect 5 with the subtype 2 draws, and the loop with the
 *        count 3 clears the flags at index 1 and 2 only
 *     2  the lap 50.0 updates the car best from 100.0 but stays above the
 *        global best 20.0: the time does not format, and the effect 5 takes the
 *        subtype 0
 *     3  the lap 3.0 stays above the car best 2.0 but below the global best
 *        10.0: the global best takes 3.0, the time formats the car best as
 *        `00:00:02`, and the next position sets bit 8
 *     4  the lap 61.75 formats as `01:01:75` with the hundredths 75
 *     5  the lap 0.5 formats as `00:00:50` with the hundredths 50, and the
 *        next position sets bit 8
 *     6  the lap 3661.5 formats the minutes above 59 as `61:01:50`
 *     7  the node count 1 leaves the flag loop empty, so the flags stay set
 *     8  the lap equal to the car best and the global best changes nothing and
 *        pushes no effect, and the next position sets bit 8
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_lap_complete(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int race_lap_count;
float race_best_lap;
int race_lap_position;
unsigned char race_lap_flags;
char race_lap_text[12];
char race_time_format[] = "%02d:%02d:%02d";
float race_centi_scale;

static int state[0x200];

static char text[8];
static int effect_calls;
static int effect_type[8];
static int effect_subtype[8];
static float effect_value[8];

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

static int near_value(float left, float right, float tolerance)
{
    float difference;

    difference = left - right;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference < tolerance;
}

static void set_int(int offset, int value)
{
    *(int *)((char *)state + offset) = value;
}

static int get_int(int offset)
{
    return *(int *)((char *)state + offset);
}

static void set_float(int offset, float value)
{
    *(float *)((char *)state + offset) = value;
}

static float get_float(int offset)
{
    return *(float *)((char *)state + offset);
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
        } else {
            *out = '%';
            out = out + 1;
        }
    }
    va_end(arguments);
    *out = 0;
    return out - buffer;
}

void effect_push(int type, int subtype, float value)
{
    if (effect_calls < 8) {
        effect_type[effect_calls] = type;
        effect_subtype[effect_calls] = subtype;
        effect_value[effect_calls] = value;
    }
    effect_calls = effect_calls + 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x200; index++) {
        state[index] = 0;
    }
    race_lap_count = 0;
    race_best_lap = 0.0f;
    race_lap_position = 0;
    race_lap_flags = 0;
    copy(race_lap_text, "00:00:00");
    race_centi_scale = 100.0f;
    effect_calls = 0;
    for (index = 0; index < 8; index++) {
        effect_type[index] = 0;
        effect_subtype[index] = 0;
        effect_value[index] = 0.0f;
    }
}

int main(void)
{
    reset();
    race_lap_count = 5;
    set_int(0x59c, 0);
    set_int(0x5a0, 9);
    set_int(0x5a4, 9);
    set_int(0x5a8, 9);
    set_int(0x5ac, 9);
    set_int(0x5b0, 9);
    set_int(0x684, 2);
    set_float(0x794, 10.0f);
    set_float(0x798, 100.0f);
    race_best_lap = 200.0f;
    race_lap_position = 999;
    race_lap_flags = 0;
    race_lap_complete(state);
    put(get_int(0x59c) == 1
            && get_int(0x5a0) == 9
            && get_int(0x5a4) == 0
            && get_int(0x5a8) == 0
            && get_int(0x5ac) == 0
            && get_int(0x5b0) == 0
            && get_int(0x684) == 3
            && near_value(get_float(0x794), 0.0f, 0.001f)
            && near_value(get_float(0x798), 10.0f, 0.001f)
            && near_value(race_best_lap, 10.0f, 0.001f)
            && same(race_lap_text, "00:00:10")
            && effect_calls == 2
            && effect_type[0] == 7 && effect_subtype[0] == 3
            && near_value(effect_value[0], 0.2f, 0.001f)
            && effect_type[1] == 5 && effect_subtype[1] == 1
            && race_lap_flags == 0
        ? '0' : 'x');

    reset();
    race_lap_count = 3;
    set_int(0x59c, 0);
    set_int(0x5a0, 5);
    set_int(0x5a4, 5);
    set_int(0x5a8, 5);
    set_int(0x5ac, 5);
    set_int(0x684, 7);
    set_float(0x794, 10.0f);
    set_float(0x798, 5.0f);
    race_best_lap = 3.0f;
    race_lap_position = 8;
    race_lap_flags = 0;
    race_lap_complete(state);
    put(get_int(0x59c) == 1
            && get_int(0x5a0) == 5
            && get_int(0x5a4) == 0
            && get_int(0x5a8) == 0
            && get_int(0x5ac) == 5
            && get_int(0x684) == 8
            && near_value(get_float(0x794), 0.0f, 0.001f)
            && near_value(get_float(0x798), 5.0f, 0.001f)
            && near_value(race_best_lap, 3.0f, 0.001f)
            && same(race_lap_text, "00:00:00")
            && effect_calls == 1
            && effect_type[0] == 5 && effect_subtype[0] == 2
            && race_lap_flags == 0
        ? '1' : 'x');

    reset();
    race_lap_count = 4;
    set_int(0x5a0, 1);
    set_int(0x5a4, 1);
    set_int(0x5a8, 1);
    set_int(0x5ac, 1);
    set_int(0x684, 9);
    set_float(0x794, 50.0f);
    set_float(0x798, 100.0f);
    race_best_lap = 20.0f;
    race_lap_position = 1;
    race_lap_flags = 0;
    race_lap_complete(state);
    put(get_int(0x59c) == 1
            && get_int(0x5a0) == 1
            && get_int(0x5a4) == 0
            && get_int(0x5a8) == 0
            && get_int(0x5ac) == 0
            && get_int(0x684) == 10
            && near_value(get_float(0x794), 0.0f, 0.001f)
            && near_value(get_float(0x798), 50.0f, 0.001f)
            && near_value(race_best_lap, 20.0f, 0.001f)
            && same(race_lap_text, "00:00:00")
            && effect_calls == 1
            && effect_type[0] == 5 && effect_subtype[0] == 0
            && race_lap_flags == 0
        ? '2' : 'x');

    reset();
    race_lap_count = 4;
    set_int(0x5a0, 1);
    set_int(0x5a4, 1);
    set_int(0x5a8, 1);
    set_int(0x5ac, 1);
    set_int(0x684, 4);
    set_float(0x794, 3.0f);
    set_float(0x798, 2.0f);
    race_best_lap = 10.0f;
    race_lap_position = 4;
    race_lap_flags = 0;
    race_lap_complete(state);
    put(get_int(0x59c) == 1
            && get_int(0x684) == 5
            && near_value(get_float(0x794), 0.0f, 0.001f)
            && near_value(get_float(0x798), 2.0f, 0.001f)
            && near_value(race_best_lap, 3.0f, 0.001f)
            && same(race_lap_text, "00:00:02")
            && effect_calls == 1
            && effect_type[0] == 7 && effect_subtype[0] == 3
            && near_value(effect_value[0], 0.2f, 0.001f)
            && race_lap_flags == 8
        ? '3' : 'x');

    reset();
    race_lap_count = 2;
    set_int(0x5a0, 7);
    set_int(0x5a4, 7);
    set_int(0x684, 0);
    set_float(0x794, 61.75f);
    set_float(0x798, 100.0f);
    race_best_lap = 200.0f;
    race_lap_position = 999;
    race_lap_flags = 0;
    race_lap_complete(state);
    put(get_int(0x59c) == 1
            && get_int(0x5a0) == 7
            && get_int(0x5a4) == 0
            && get_int(0x684) == 1
            && near_value(get_float(0x794), 0.0f, 0.001f)
            && near_value(get_float(0x798), 61.75f, 0.001f)
            && near_value(race_best_lap, 61.75f, 0.001f)
            && same(race_lap_text, "01:01:75")
            && effect_calls == 2
            && effect_type[0] == 7 && effect_subtype[0] == 3
            && effect_type[1] == 5 && effect_subtype[1] == 1
            && race_lap_flags == 0
        ? '4' : 'x');

    reset();
    race_lap_count = 2;
    set_int(0x5a0, 7);
    set_int(0x5a4, 7);
    set_int(0x684, 2);
    set_float(0x794, 0.5f);
    set_float(0x798, 100.0f);
    race_best_lap = 200.0f;
    race_lap_position = 2;
    race_lap_flags = 0;
    race_lap_complete(state);
    put(get_int(0x59c) == 1
            && get_int(0x684) == 3
            && near_value(get_float(0x794), 0.0f, 0.001f)
            && near_value(get_float(0x798), 0.5f, 0.001f)
            && near_value(race_best_lap, 0.5f, 0.001f)
            && same(race_lap_text, "00:00:50")
            && effect_calls == 1
            && effect_type[0] == 7 && effect_subtype[0] == 3
            && race_lap_flags == 8
        ? '5' : 'x');

    reset();
    race_lap_count = 2;
    set_int(0x5a0, 7);
    set_int(0x5a4, 7);
    set_int(0x684, 10);
    set_float(0x794, 3661.5f);
    set_float(0x798, 4000.0f);
    race_best_lap = 5000.0f;
    race_lap_position = 999;
    race_lap_flags = 0;
    race_lap_complete(state);
    put(get_int(0x59c) == 1
            && get_int(0x684) == 11
            && near_value(get_float(0x794), 0.0f, 0.001f)
            && near_value(get_float(0x798), 3661.5f, 0.001f)
            && near_value(race_best_lap, 3661.5f, 0.001f)
            && same(race_lap_text, "61:01:50")
            && effect_calls == 2
            && effect_type[0] == 7 && effect_subtype[0] == 3
            && effect_type[1] == 5 && effect_subtype[1] == 1
            && race_lap_flags == 0
        ? '6' : 'x');

    reset();
    race_lap_count = 1;
    set_int(0x59c, 0);
    set_int(0x5a0, 3);
    set_int(0x5a4, 3);
    set_int(0x684, 5);
    set_float(0x794, 10.0f);
    set_float(0x798, 5.0f);
    race_best_lap = 3.0f;
    race_lap_position = 0;
    race_lap_flags = 0;
    race_lap_complete(state);
    put(get_int(0x59c) == 1
            && get_int(0x5a0) == 3
            && get_int(0x5a4) == 3
            && get_int(0x684) == 6
            && near_value(get_float(0x794), 0.0f, 0.001f)
            && near_value(get_float(0x798), 5.0f, 0.001f)
            && near_value(race_best_lap, 3.0f, 0.001f)
            && same(race_lap_text, "00:00:00")
            && effect_calls == 1
            && effect_type[0] == 5 && effect_subtype[0] == 0
            && race_lap_flags == 0
        ? '7' : 'x');

    reset();
    race_lap_count = 3;
    set_int(0x5a0, 4);
    set_int(0x5a4, 4);
    set_int(0x5a8, 4);
    set_int(0x684, 4);
    set_float(0x794, 5.0f);
    set_float(0x798, 5.0f);
    race_best_lap = 5.0f;
    race_lap_position = 4;
    race_lap_flags = 0;
    race_lap_complete(state);
    put(get_int(0x59c) == 1
            && get_int(0x5a0) == 4
            && get_int(0x5a4) == 0
            && get_int(0x5a8) == 0
            && get_int(0x684) == 5
            && near_value(get_float(0x794), 0.0f, 0.001f)
            && near_value(get_float(0x798), 5.0f, 0.001f)
            && near_value(race_best_lap, 5.0f, 0.001f)
            && same(race_lap_text, "00:00:00")
            && effect_calls == 0
            && race_lap_flags == 8
        ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
