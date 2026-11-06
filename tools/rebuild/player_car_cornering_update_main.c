/* The test harness for the rebuilt `player_car_cornering_update` module.
 *
 * The harness holds the player object, the map position, the track heading,
 * the effect state set, and the effect table.  The `sin`, `atan2`, and `sqrt`
 * stubs live in the local FPU assembly file.  The harness runs eight paths:
 *
 *     0  the inactive flag `+0x40` returns after the nearest node search and
 *        the heading copy
 *     1  a non-zero effect state `+0x148` returns after the heading copy
 *     2  a zero heading error keeps the speed, stores the unit velocity, keeps
 *        the lateral rate at 2000, and acquires the type-4 table entry
 *     3  a speed times error above the gate sets the state 2 and keeps the
 *        table entry
 *     4  the flag `+0x7b0` blocks the state change and acquires the entry
 *     5  a heading error above pi wraps down, the turn and the speed loss use
 *        the sine, and the heading rate uses `+0xc4` minus `+0x6c`
 *     6  a high speed clamps the lateral rate at 5000 and a set `+0x170` keeps
 *        the filtered slip
 *     7  a negative speed clamps the speed at 0
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.  The float comparisons use
 * a small tolerance, because the sine and the arc tangent come from the FPU.
 */
#pragma off (check_stack)

#define OBJECT_SIZE 0x900

void player_car_cornering_update(void *object);
double sin(double value);
double atan2(double y, double x);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int map_position_x;
int map_position_y;
float map_heading;

float corner_pi;
float corner_neg_pi;
float corner_half_pi;
float corner_speed_scale;
double corner_four;
double corner_lateral_bias;
float corner_lateral_limit;
double corner_speed_loss;
float corner_heading_scale;

static double test_ten;
static double test_half;
static float test_track;
static float nearest_heading;
static int nearest_mode;
static int nearest_calls;
static int state_calls;
static int acquire_calls;
static int entry_kind[16];
static unsigned char object[OBJECT_SIZE];
static char text[8];

void track_nearest_node(int mode)
{
    nearest_mode = mode;
    nearest_calls = nearest_calls + 1;
    map_heading = nearest_heading;
}

void effect_state_set(void *value, int state)
{
    char *base = (char *)value;

    state_calls = state_calls + 1;
    if (*(int *)(base + 0x148) == 0) {
        *(int *)(base + 0x148) = state;
    }
}

int table_entry_acquire(int type, int handle)
{
    int index;

    acquire_calls = acquire_calls + 1;
    if (handle >= 0 && handle < 16 && entry_kind[handle] == type) {
        return handle;
    }
    for (index = 1; index < 16; index++) {
        if (entry_kind[index] == 0) {
            entry_kind[index] = type;
            return index;
        }
    }
    return -1;
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

static void clear_object(void)
{
    int index;

    for (index = 0; index < OBJECT_SIZE; index++) {
        object[index] = 0;
    }
}

static void clear_call_state(void)
{
    int index;

    nearest_mode = -1;
    nearest_calls = 0;
    state_calls = 0;
    acquire_calls = 0;
    for (index = 0; index < 16; index++) {
        entry_kind[index] = 0;
    }
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int get_int(int offset)
{
    return *(int *)(object + offset);
}

static void set_float(int offset, float value)
{
    *(float *)(object + offset) = value;
}

static float get_float(int offset)
{
    return *(float *)(object + offset);
}

static unsigned int float_bits(float value)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = value;
    return v.u;
}

static unsigned int get_bits(int offset)
{
    return float_bits(get_float(offset));
}

static int equal_bits(int offset, unsigned int expected)
{
    return get_bits(offset) == expected;
}

static int close_bits(int offset, float expected)
{
    unsigned int actual = get_bits(offset);
    unsigned int wanted = float_bits(expected);
    unsigned int delta = actual > wanted ? actual - wanted : wanted - actual;

    return delta <= 8;
}

static double absolute_value(double value)
{
    return value < 0.0 ? -value : value;
}

int main(void)
{
    float sine_heading;
    float speed_loss;
    float expected_turn;
    float expected_speed;
    float expected_heading;

    test_ten = 10.0;
    test_half = 0.5;
    test_track = -3.2f;
    corner_pi = 3.1415927410125732f;
    corner_neg_pi = -3.1415927410125732f;
    corner_half_pi = 1.5707963705062866f;
    corner_speed_scale = 2.0f;
    corner_four = 4.0;
    corner_lateral_bias = 2000.0;
    corner_lateral_limit = 5000.0f;
    corner_speed_loss = 0.9;
    corner_heading_scale = 5.0f;

    clear_object();
    clear_call_state();
    nearest_heading = 0.75f;
    set_int(0x40, 0);
    set_int(0x4b0, 11);
    set_int(0x4b8, 22);
    set_float(0x4cc, 5.0f);
    set_float(0x50c, 9.0f);
    player_car_cornering_update(object);
    put(nearest_calls == 1 && nearest_mode == 0
            && map_position_x == 11 && map_position_y == 22
            && equal_bits(0x13c, 0x3f400000u)
            && equal_bits(0x4cc, 0x40a00000u)
            && equal_bits(0x50c, 0x41100000u)
            && get_int(0x148) == 0 && acquire_calls == 0
        ? '0' : 'x');

    clear_object();
    clear_call_state();
    nearest_heading = 1.25f;
    set_int(0x40, 1);
    set_int(0x148, 1);
    set_float(0x4cc, 2.0f);
    set_float(0x50c, 9.0f);
    player_car_cornering_update(object);
    put(nearest_calls == 1 && equal_bits(0x13c, 0x3fa00000u)
            && get_int(0x148) == 1
            && equal_bits(0x50c, 0x41100000u)
            && acquire_calls == 0
        ? '1' : 'x');

    clear_object();
    clear_call_state();
    nearest_heading = 0.0f;
    set_int(0x40, 1);
    set_int(0x44, 3);
    set_int(0x4c, 4);
    set_int(0xc4, 10);
    set_int(0x6c, 2);
    set_int(0x170, 7);
    set_int(0x128, 8);
    set_float(0x4cc, 0.0f);
    set_float(0x50, 0.0f);
    set_float(0x50c, 10.0f);
    set_float(0x180, 1.0f);
    player_car_cornering_update(object);
    put(equal_bits(0x550, 0x3f19999au)
            && equal_bits(0x558, 0x3f4ccccdu)
            && equal_bits(0x54c, 0x44fa0000u)
            && equal_bits(0x50c, 0x41200000u)
            && get_int(0x548) == 0
            && get_int(0x110) == 0
            && get_int(0x4f0) == 0 && get_int(0x4f4) == 0
            && get_int(0x128) == 8
            && get_int(0x148) == 0 && acquire_calls == 1
            && get_int(0x7d4) == 1
        ? '2' : 'x');

    clear_object();
    clear_call_state();
    nearest_heading = 1.5707963705062866f;
    set_int(0x40, 1);
    set_int(0x44, 3);
    set_int(0x4c, 4);
    set_float(0x4cc, 0.0f);
    set_float(0x50, 0.0f);
    set_float(0x50c, 10.0f);
    set_float(0x180, 5.0f);
    set_int(0x7d4, 0x33);
    player_car_cornering_update(object);
    put(get_int(0x148) == 2 && state_calls == 1 && acquire_calls == 0
            && get_int(0x7d4) == 0x33
        ? '3' : 'x');

    clear_object();
    clear_call_state();
    nearest_heading = 1.5707963705062866f;
    set_int(0x40, 1);
    set_int(0x44, 3);
    set_int(0x4c, 4);
    set_int(0x7b0, 1);
    set_float(0x4cc, 0.0f);
    set_float(0x50, 0.0f);
    set_float(0x50c, 10.0f);
    set_float(0x180, 5.0f);
    player_car_cornering_update(object);
    put(get_int(0x148) == 0 && acquire_calls == 1
            && get_int(0x7d4) == 1
        ? '4' : 'x');

    clear_object();
    clear_call_state();
    nearest_heading = test_track;
    set_int(0x40, 1);
    set_int(0x44, 3);
    set_int(0x4c, 4);
    set_int(0xc4, 10);
    set_int(0x6c, 2);
    set_float(0x4cc, 0.0f);
    set_float(0x50, 0.5f);
    set_float(0x50c, 10.0f);
    set_float(0x180, 0.0f);
    sine_heading = (float)absolute_value(sin(test_half));
    speed_loss = (float)(test_ten * corner_speed_loss * (double)sine_heading);
    expected_speed = (float)test_ten - speed_loss;
    expected_turn = (float)(atan2((double)sine_heading, corner_four)
        * (test_ten * (double)corner_speed_scale) + corner_lateral_bias);
    expected_heading = (float)(get_int(0xc4) - get_int(0x6c))
        * sine_heading * corner_heading_scale;
    player_car_cornering_update(object);
    put(equal_bits(0x13c, 0xc04ccccdu)
            && close_bits(0x50c, expected_speed)
            && close_bits(0x548, expected_heading)
            && close_bits(0x54c, expected_turn)
            && equal_bits(0x550, 0x3f19999au)
            && equal_bits(0x558, 0x3f4ccccdu)
            && get_int(0x110) == 0
            && get_int(0x4f0) == 0 && get_int(0x4f4) == 0
            && get_int(0x128) == 0
            && get_int(0x148) == 2 && state_calls == 1
            && acquire_calls == 0
        ? '5' : 'x');

    clear_object();
    clear_call_state();
    nearest_heading = 1.5707963705062866f;
    set_int(0x40, 1);
    set_int(0x44, 3);
    set_int(0x4c, 4);
    set_int(0xc4, 10);
    set_int(0x6c, 2);
    set_int(0x170, 3);
    set_float(0x128, 4.5f);
    set_float(0x4cc, 1.5707963705062866f);
    set_float(0x50, 0.0f);
    set_float(0x50c, 10000.0f);
    set_float(0x180, 1.0f);
    player_car_cornering_update(object);
    put(equal_bits(0x54c, 0x459c4000u)
            && equal_bits(0x50c, 0x447a0000u)
            && equal_bits(0x548, 0x42200000u)
            && equal_bits(0x128, 0x40900000u)
            && get_int(0x110) == 0
            && get_int(0x4f0) == 0 && get_int(0x4f4) == 0
            && acquire_calls == 1
        ? '6' : 'x');

    clear_object();
    clear_call_state();
    nearest_heading = 1.5707963705062866f;
    set_int(0x40, 1);
    set_int(0x44, 3);
    set_int(0x4c, 4);
    set_int(0x170, 1);
    set_float(0x4cc, 1.5707963705062866f);
    set_float(0x50, 0.0f);
    set_float(0x50c, -10.0f);
    set_float(0x180, 1.0f);
    player_car_cornering_update(object);
    put(get_int(0x50c) == 0 && get_bits(0x50c) == 0
            && get_int(0x110) == 0
            && acquire_calls == 1
        ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
