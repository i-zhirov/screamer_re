/* The test harness for the rebuilt `view_transition_update` module.
 *
 * The harness holds the view block, the frame table, the three constants, and
 * the two callee stubs.  The `object_angle_approach` stub records the target,
 * the value, and the index, but it does not change the target, so the angle
 * cosine tests use the deltas the test sets.  The `screen_mode_set` stub
 * records the width, the height, and the value.
 *
 * The harness runs nine paths:
 *
 *     0  both flags are clear: the function changes nothing
 *     1  the flag +0x54 forms the six deltas and clears the flag
 *     2  the flag +0x58 settles the view: three approach calls, three scaled
 *        deltas, and the screen mode with the horizon at 65 percent
 *     3  the first angle delta stays large: the view does not settle but the
 *        deltas still scale
 *     4  the scaled x delta is exactly 200 against the 200 limit: the strict
 *        compare keeps the view open
 *     5  the scale 0.5 and the deltas -5, 5, and -1 truncate toward zero to
 *        -2, 2, and 0, and the view settles
 *     6  the frame factor 0.5 halves the limit: the delta 100 fails and the
 *        delta 99 settles
 *     7  both flags are set: the function forms the deltas and settles at once
 *     8  the third angle delta stays large: the view does not settle
 *     D  the exit marker
 *
 * The float and double constants arrive from their bit patterns, because a
 * literal in this minimal image reads the constant segment, which this harness
 * does not load.
 */
#pragma off (check_stack)

void view_transition_update(char *view);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float *DAT_0010cf08;
int DAT_001100c0;
float DAT_00118e04;
double view_transition_cos_limit;
float view_transition_distance_limit;
double view_transition_height_scale;

static char text[8];
static char view[0x100];
static float rate_table[0x60];

static int approach_calls;
static float *approach_target[3];
static float approach_value[3];
static int approach_index[3];

static int screen_calls;
static unsigned int screen_width;
static unsigned int screen_height;
static unsigned int screen_value;

void object_angle_approach(float *target, float value, int index)
{
    if (approach_calls < 3) {
        approach_target[approach_calls] = target;
        approach_value[approach_calls] = value;
        approach_index[approach_calls] = index;
    }
    approach_calls = approach_calls + 1;
}

void screen_mode_set(unsigned int width, unsigned int height,
                     unsigned int value)
{
    screen_calls = screen_calls + 1;
    screen_width = width;
    screen_height = height;
    screen_value = value;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_bytes(char *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0;
    }
}

static void set_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void set_double(double *p, unsigned int low, unsigned int high)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = low;
    value.u[1] = high;
    *p = value.d;
}

static void reset(void)
{
    int index;

    clear_bytes(view, 0x100);
    for (index = 0; index < 0x60; index++) {
        rate_table[index] = 0.0f;
    }
    rate_table[0x5a] = 1.0f;
    DAT_0010cf08 = rate_table;
    DAT_001100c0 = 0x12345678;
    DAT_00118e04 = 1.0f;

    set_double(&view_transition_cos_limit, 0x2474538f, 0x3fefff97);
    set_float(&view_transition_distance_limit, 0x43480000);
    set_double(&view_transition_height_scale, 0xcccccccd, 0x3fe4cccc);

    approach_calls = 0;
    approach_target[0] = 0;
    approach_target[1] = 0;
    approach_target[2] = 0;
    approach_value[0] = 0.0f;
    approach_value[1] = 0.0f;
    approach_value[2] = 0.0f;
    approach_index[0] = -1;
    approach_index[1] = -1;
    approach_index[2] = -1;

    screen_calls = 0;
    screen_width = 0;
    screen_height = 0;
    screen_value = 0;
}

static void set_int(int offset, int value)
{
    *(int *)(view + offset) = value;
}

static int int_is(int offset, int value)
{
    return *(int *)(view + offset) == value;
}

static int float_is(int offset, float value)
{
    return *(float *)(view + offset) == value;
}

static int approach_is_ready(void)
{
    return approach_calls == 3
        && approach_target[0] == (float *)(view + 0x3c)
        && approach_target[1] == (float *)(view + 0x40)
        && approach_target[2] == (float *)(view + 0x44)
        && approach_value[0] == 0.0f
        && approach_value[1] == 0.0f
        && approach_value[2] == 0.0f
        && approach_index[0] == 0x19
        && approach_index[1] == 0x19
        && approach_index[2] == 0x19;
}

int main(void)
{
    reset();
    set_int(0x24, 0x33);
    set_int(0x28, 0x44);
    set_int(0x2c, 0x55);
    view_transition_update(view);
    put(int_is(0x24, 0x33) && int_is(0x28, 0x44) && int_is(0x2c, 0x55)
        && int_is(0x54, 0) && int_is(0x58, 0)
        && approach_calls == 0 && screen_calls == 0
        && DAT_001100c0 == 0x12345678 ? '0' : 'x');

    reset();
    set_int(0x00, 10);
    set_int(0x04, 20);
    set_int(0x08, 30);
    set_int(0x18, 40);
    set_int(0x1c, 60);
    set_int(0x20, 90);
    set_float((float *)(view + 0x30), 0x3fc00000);
    set_float((float *)(view + 0x34), 0x40200000);
    set_float((float *)(view + 0x38), 0x40600000);
    set_float((float *)(view + 0x48), 0x3f800000);
    set_float((float *)(view + 0x4c), 0x40000000);
    set_float((float *)(view + 0x50), 0x40800000);
    set_int(0x54, 1);
    view_transition_update(view);
    put(int_is(0x24, 30) && int_is(0x28, 40) && int_is(0x2c, 60)
        && float_is(0x3c, -0.5f) && float_is(0x40, -0.5f)
        && float_is(0x44, 0.5f)
        && int_is(0x54, 0) && int_is(0x58, 0)
        && approach_calls == 0 && screen_calls == 0 ? '1' : 'x');

    reset();
    set_int(0x24, 1);
    set_int(0x28, 2);
    set_int(0x2c, 3);
    set_int(0x58, 1);
    set_int(0x8c, 7);
    set_int(0x90, 400);
    view_transition_update(view);
    put(int_is(0x24, 1) && int_is(0x28, 2) && int_is(0x2c, 3)
        && approach_is_ready()
        && screen_calls == 1 && screen_width == 0x140
        && screen_height == 400 && screen_value == 260
        && DAT_001100c0 == 7 && int_is(0x58, 0) ? '2' : 'x');

    reset();
    set_int(0x24, 4);
    set_int(0x28, 5);
    set_int(0x2c, 6);
    set_float((float *)(view + 0x3c), 0x3f800000);
    set_int(0x58, 1);
    set_int(0x8c, 9);
    set_int(0x90, 100);
    view_transition_update(view);
    put(int_is(0x24, 4) && int_is(0x28, 5) && int_is(0x2c, 6)
        && approach_calls == 3
        && screen_calls == 0 && DAT_001100c0 == 0x12345678
        && int_is(0x58, 1) ? '3' : 'x');

    reset();
    set_int(0x24, -200);
    set_int(0x58, 1);
    set_int(0x90, 100);
    view_transition_update(view);
    put(int_is(0x24, -200) && approach_calls == 3
        && screen_calls == 0 && DAT_001100c0 == 0x12345678
        && int_is(0x58, 1) ? '4' : 'x');

    reset();
    rate_table[0x5a] = 0.5f;
    set_int(0x24, -5);
    set_int(0x28, 5);
    set_int(0x2c, -1);
    set_int(0x58, 1);
    set_int(0x8c, 3);
    set_int(0x90, 100);
    view_transition_update(view);
    put(int_is(0x24, -2) && int_is(0x28, 2) && int_is(0x2c, 0)
        && approach_calls == 3
        && screen_calls == 1 && screen_width == 0x140
        && screen_height == 100 && screen_value == 65
        && DAT_001100c0 == 3 && int_is(0x58, 0) ? '5' : 'x');

    reset();
    set_float(&DAT_00118e04, 0x3f000000);
    set_int(0x24, 100);
    set_int(0x58, 1);
    set_int(0x8c, 5);
    set_int(0x90, 100);
    view_transition_update(view);
    if (!(int_is(0x24, 100) && screen_calls == 0
          && DAT_001100c0 == 0x12345678 && int_is(0x58, 1))) {
        put('x');
    } else {
        set_int(0x24, 99);
        view_transition_update(view);
        put(int_is(0x24, 99) && screen_calls == 1
            && screen_height == 100 && screen_value == 65
            && DAT_001100c0 == 5 && int_is(0x58, 0) ? '6' : 'x');
    }

    reset();
    set_int(0x18, 10);
    set_int(0x1c, 20);
    set_int(0x20, 30);
    set_int(0x54, 1);
    set_int(0x58, 1);
    set_int(0x8c, 1);
    set_int(0x90, 200);
    view_transition_update(view);
    put(int_is(0x24, 10) && int_is(0x28, 20) && int_is(0x2c, 30)
        && int_is(0x54, 0) && int_is(0x58, 0)
        && approach_is_ready()
        && screen_calls == 1 && screen_width == 0x140
        && screen_height == 200 && screen_value == 130
        && DAT_001100c0 == 1 ? '7' : 'x');

    reset();
    set_int(0x24, 1);
    set_int(0x28, 1);
    set_int(0x2c, 1);
    set_float((float *)(view + 0x44), 0x3f800000);
    set_int(0x58, 1);
    set_int(0x90, 100);
    view_transition_update(view);
    put(int_is(0x24, 1) && int_is(0x28, 1) && int_is(0x2c, 1)
        && approach_calls == 3
        && screen_calls == 0 && DAT_001100c0 == 0x12345678
        && int_is(0x58, 1) ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
