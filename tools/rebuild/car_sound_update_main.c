/* The test harness for the rebuilt `car_sound_update` module.
 *
 * The harness holds the car object, the car records, the mode byte, the two
 * latches, the eight constant globals, and the four callee stubs.  The stubs
 * record the call order and the arguments, and they return the value that the
 * harness selects.  The timer stubs return zero when the enabled flag is zero,
 * because a disabled timer does not fire.
 *
 * The harness runs fourteen paths:
 *
 *     0  the gate is zero: the function returns at once
 *     1  the main path: the angle is outside the tolerance, so the near flag
 *        is zero and the timer 0x14 runs; the speed is below the limit
 *     2  the near flag is one, the mode bit 0x10 is set, and the value at
 *        +0x790 is within the limit: the trigger 0x16 runs
 *     3  the mode bit 0x10 is set and the value at +0x790 is above the limit:
 *        the trigger 0x16 does not run
 *     4  the mode bit 0x10 is set and the engine speed is one: the timer 0x15
 *        fires and sets the latch
 *     5  the latch is set and the engine speed is above one: the timer 0x18
 *        fires and clears the latch
 *     6  the mode bit 0x20 is set, the two counters are equal, and the flag is
 *        set: both triggers 0x1a and 0x13 run
 *     7  the mode bit 0x40 is set and the two records are equal: the trigger
 *        0x1a runs
 *     8  the mode bit 0x40 is set and the records differ: the trigger 0x1a
 *        does not run
 *     9  the record scan finds a record through the float compare
 *     a  the record scan finds a record through the progress difference
 *     b  the record scan finds no record
 *     c  the timer value is above the limit: the timer 0x0f fires
 *     D  the exit marker
 *
 * The constants are set from simple exact values, so the harness does not
 * depend on the constant segment.
 */
#pragma off (check_stack)

void car_sound_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];

int car_audio_gate;
unsigned char car_audio_mode;
int car_audio_latch;
int car_record_count;
unsigned char car_records[0x590 * 2];
int car_compare_a;
int car_compare_b;
int car_compare_c;
int car_flag_a;
float car_angle_bias;
float car_speed_low;
float car_angle_limit;
double car_rate_scale;
double car_limit_scale;
float car_rate_base;
float car_timer_value;

static unsigned char object[0x800];

static int angle_calls;
static float angle_a;
static float angle_b;
static float angle_tol;
static int angle_result;

static int countdown_result;
static int after_result;

#define MAX_CALLS 24
#define KIND_COUNTDOWN 0
#define KIND_AFTER 1
#define KIND_TRIGGER 2

static int call_kind[MAX_CALLS];
static int call_enabled[MAX_CALLS];
static int call_id[MAX_CALLS];
static float call_limit[MAX_CALLS];
static int call_count;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static float float_from_bits(unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    return v.f;
}

static int float_bits_are(float value, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = value;
    return v.u == bits;
}

static void set_float(int offset, float value)
{
    *(float *)(object + offset) = value;
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static void record_call(int kind, int enabled, int id, float limit)
{
    if (call_count < MAX_CALLS) {
        call_kind[call_count] = kind;
        call_enabled[call_count] = enabled;
        call_id[call_count] = id;
        call_limit[call_count] = limit;
    }
    call_count = call_count + 1;
}

static int call_is(int index, int kind, int enabled, int id, unsigned int bits)
{
    return call_count > index
        && call_kind[index] == kind
        && call_enabled[index] == enabled
        && call_id[index] == id
        && float_bits_are(call_limit[index], bits);
}

static int call_is_trigger(int index, int id, int value)
{
    return call_count > index
        && call_kind[index] == KIND_TRIGGER
        && call_id[index] == id
        && call_enabled[index] == value;
}

static int has_id(int id)
{
    int i;

    for (i = 0; i < call_count && i < MAX_CALLS; i++) {
        if (call_id[i] == id && call_kind[i] != KIND_TRIGGER)
            return 1;
    }
    return 0;
}

int angle_within(float a, float b, float tolerance)
{
    angle_calls = angle_calls + 1;
    angle_a = a;
    angle_b = b;
    angle_tol = tolerance;
    return angle_result;
}

int effect_countdown(int enabled, int id, float limit)
{
    record_call(KIND_COUNTDOWN, enabled, id, limit);
    return enabled != 0 ? countdown_result : 0;
}

int effect_after(int enabled, int id, float limit)
{
    record_call(KIND_AFTER, enabled, id, limit);
    return enabled != 0 ? after_result : 0;
}

int effect_trigger(int id, int value)
{
    record_call(KIND_TRIGGER, value, id, 0.0f);
    return 0;
}

static void reset(void)
{
    int i;

    for (i = 0; i < 0x800; i++)
        object[i] = 0;
    for (i = 0; i < 0x590 * 2; i++)
        car_records[i] = 0;

    car_audio_gate = 0;
    car_audio_mode = 0;
    car_audio_latch = 0;
    car_record_count = 0;
    car_compare_a = 0;
    car_compare_b = 0;
    car_compare_c = 0;
    car_flag_a = 0;
    car_angle_bias = 0.5f;
    car_speed_low = 100.0f;
    car_angle_limit = 10.0f;
    car_rate_scale = 3.0;
    car_limit_scale = 50.0;
    car_rate_base = 2.0f;
    car_timer_value = 0.0f;

    angle_calls = 0;
    angle_a = 0.0f;
    angle_b = 0.0f;
    angle_tol = 0.0f;
    angle_result = 0;
    countdown_result = 0;
    after_result = 0;
    call_count = 0;
}

static void set_record_float(int index, int offset, float value)
{
    *(float *)(car_records + index * 0x590 + offset) = value;
}

static void set_record_int(int index, int offset, int value)
{
    *(int *)(car_records + index * 0x590 + offset) = value;
}

int main(void)
{
    reset();
    car_audio_gate = 0;
    car_sound_update(object);
    put(call_count == 0 && angle_calls == 0 ? '0' : 'x');

    reset();
    car_audio_gate = 1;
    set_float(0x13c, 1.0f);
    set_float(0x4cc, 0.25f);
    set_float(0x50c, 50.0f);
    set_float(0x4f4, 3.0f);
    set_int(0x678, 4);
    set_int(0x688, 1);
    set_int(0x788, 0);
    car_audio_mode = 0;
    car_sound_update(object);
    put(angle_calls == 1
        && float_bits_are(angle_a, 0x3fc00000)
        && float_bits_are(angle_b, 0x3e800000)
        && float_bits_are(angle_tol, 0x3f490fdb)
        && call_count == 7
        && call_is(0, KIND_AFTER, 0, 0x10, 0x40a00000)
        && call_is(1, KIND_AFTER, 1, 0x11, 0x41000000)
        && call_is(2, KIND_COUNTDOWN, 0, 0x18, 0x40a00000)
        && call_is(3, KIND_COUNTDOWN, 4, 0x14, 0x00000000)
        && call_is(4, KIND_COUNTDOWN, 3, 0x0a, 0x40c00000)
        && call_is(5, KIND_COUNTDOWN, 0, 0x0d, 0x41000000)
        && call_is(6, KIND_AFTER, 0, 0x0f, 0x00000000) ? '1' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    car_audio_mode = 0x17;
    set_float(0x790, 10.0f);
    set_float(0x50c, 200.0f);
    car_sound_update(object);
    put(call_count == 9
        && call_is_trigger(2, 0x16, 3)
        && !has_id(0x14) ? '2' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    car_audio_mode = 0x17;
    set_float(0x790, 11.0f);
    set_float(0x50c, 200.0f);
    car_sound_update(object);
    put(call_count == 8 && !has_id(0x16) && !has_id(0x14) ? '3' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    countdown_result = 1;
    car_audio_mode = 0x10;
    set_float(0x790, 11.0f);
    set_float(0x50c, 200.0f);
    set_int(0x688, 1);
    car_sound_update(object);
    put(car_audio_latch == 1
        && call_is(2, KIND_COUNTDOWN, 1, 0x15, 0x40a00000)
        && call_is(3, KIND_COUNTDOWN, 0, 0x18, 0x40a00000) ? '4' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    countdown_result = 1;
    car_audio_latch = 1;
    car_audio_mode = 0;
    set_float(0x50c, 200.0f);
    set_int(0x688, 2);
    set_int(0x788, 2);
    car_sound_update(object);
    put(car_audio_latch == 0
        && call_is(2, KIND_COUNTDOWN, 1, 0x18, 0x40a00000) ? '5' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    car_audio_mode = 0x20;
    car_compare_a = 7;
    car_compare_b = 7;
    car_flag_a = 1;
    set_float(0x50c, 200.0f);
    car_sound_update(object);
    put(call_count == 8
        && call_is_trigger(3, 0x1a, 3)
        && call_is_trigger(7, 0x13, 3) ? '6' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    car_audio_mode = 0x40;
    car_compare_c = 9;
    car_compare_b = 0;
    set_int(0x7c0, 9);
    set_float(0x50c, 200.0f);
    car_sound_update(object);
    put(call_count == 7 && call_is_trigger(3, 0x1a, 3) ? '7' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    car_audio_mode = 0x40;
    car_compare_c = 9;
    car_compare_b = 0;
    set_int(0x7c0, 8);
    set_float(0x50c, 200.0f);
    car_sound_update(object);
    put(call_count == 6 && !has_id(0x1a) ? '8' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    car_audio_mode = 3;
    car_record_count = 1;
    set_float(0x790, 11.0f);
    set_int(0x684, 2);
    set_float(0x680, 0.0f);
    set_float(0x50c, 200.0f);
    set_record_int(0, 0x40c, 1);
    set_record_float(0, 0x408, -1000.0f);
    car_sound_update(object);
    put(call_count == 8
        && call_is(6, KIND_COUNTDOWN, 1, 0x17, 0x40000000) ? '9' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    car_audio_mode = 3;
    car_record_count = 1;
    set_float(0x790, 11.0f);
    set_int(0x684, 5);
    set_float(0x680, 0.0f);
    set_float(0x50c, 200.0f);
    set_record_int(0, 0x40c, 1);
    set_record_float(0, 0x408, 0.0f);
    car_sound_update(object);
    put(call_count == 8
        && call_is(6, KIND_COUNTDOWN, 1, 0x17, 0x40000000) ? 'a' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    car_audio_mode = 3;
    car_record_count = 1;
    set_float(0x790, 11.0f);
    set_int(0x684, 1);
    set_float(0x680, 0.0f);
    set_float(0x50c, 200.0f);
    set_record_int(0, 0x40c, 1);
    set_record_float(0, 0x408, 0.0f);
    car_sound_update(object);
    put(call_count == 8
        && call_is(6, KIND_COUNTDOWN, 0, 0x17, 0x40000000) ? 'b' : 'x');

    reset();
    car_audio_gate = 1;
    angle_result = 1;
    car_timer_value = 100.0f;
    set_float(0x50c, 200.0f);
    car_sound_update(object);
    put(call_count == 6
        && call_is(5, KIND_AFTER, 1, 0x0f, 0x00000000) ? 'c' : 'x');

    put('D');
    quit();
    return 0;
}
