/* The test harness for the rebuilt `player_car_reset_update` module.
 *
 * The harness holds one player object, the frame delta, the filter table, the
 * slot index, the two flags, the two position globals, the direction result,
 * the slot record array, and the constants.  The `table_lookup`,
 * `sound_enqueue`, `track_direction`, and `effect_start` stubs record their
 * arguments, and every table entry is zero except the damping coefficient, so
 * the velocity filters write their arguments exactly.
 *
 * The harness runs nine paths:
 *
 *     0  state 2 starts the reset: the state falls to 1, the speed scales by
 *        0.6, the heading takes the direction result, the velocity offsets
 *        integrate, and the accumulator stays positive
 *     1  state 1 with a negative accumulator starts the effect: the counter
 *        sets 1, the timer reflects to 1240, the odd counter selects -0.6, and
 *        the velocity sets -1
 *     2  state 1 with the counter at 3 marks the slot record: the counter sets
 *        4, the even counter selects 0.6, the state clears, and the slot
 *        record at index 10 sets
 *     3  state 5 with a gear and the handle -1 allocates the handle, toggles
 *        the low five bits of +0x4bc, advances the heading by 1.2, and starts
 *        the effect
 *     4  state 5 with a zero gear keeps the handle, the bits, and the heading
 *     5  state 6 clears +0x4bc and the state
 *     6  state 3 selects no case and keeps every field
 *     7  state 2 with the replay flag set keeps DAT_00001e18 clear
 *     8  state 1 with the counter at 3 and the flag clear leaves the slot
 *        record clear
 *     D  the exit marker
 *
 * The float values and the expected values come from their bit patterns, so
 * the harness does not depend on a float literal or on the constant segment.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void player_car_reset_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_00118e04;
float *DAT_0010cf08;
unsigned char DAT_00118e1c;
int DAT_00118e6c;
int DAT_00110aa4;
int DAT_00001e18;
int DAT_00108038;
int DAT_00108040;
float DAT_00108048;
int DAT_00101db4[64];

double reset_speed_scale;
float reset_decrement;
double reset_bounce;
double reset_heading_scale;

static char object[0x900];
static float table[0x100];
static char text[8];

static int lookup_calls;
static int lookup_arg;
static int lookup_result;
static int sound_calls;
static int sound_id;
static int sound_value;
static unsigned int sound_extra_bits;
static int track_calls;
static int track_arg;
static float track_result;
static int effect_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int int_is(int offset, int value)
{
    return *(int *)(object + offset) == value;
}

static void set_bits(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(object + offset) = value.f;
}

static int float_is(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } x;
    union {
        float f;
        unsigned int u;
    } y;

    x.f = *(float *)(object + offset);
    y.u = bits;
    return x.u == y.u;
}

static void set_bits_global(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void set_bits_global_double(double *p, unsigned int lo,
                                   unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = lo;
    value.u[1] = hi;
    *p = value.d;
}

static void set_float_table(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)((char *)table + offset) = value.f;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x900; index++) {
        object[index] = 0;
    }
    for (index = 0; index < 0x100; index++) {
        table[index] = 0.0f;
    }
    for (index = 0; index < 64; index++) {
        DAT_00101db4[index] = 0;
    }
    DAT_0010cf08 = table;
    DAT_00118e1c = 0;
    DAT_00118e6c = 0;
    DAT_00110aa4 = 0;
    DAT_00001e18 = 0;
    DAT_00108038 = 0;
    DAT_00108040 = 0;
    DAT_00108048 = 0.0f;
    lookup_calls = 0;
    lookup_arg = 0;
    lookup_result = 100;
    sound_calls = 0;
    sound_id = 0;
    sound_value = 0;
    sound_extra_bits = 0;
    track_calls = 0;
    track_arg = 0;
    track_result = 0.0f;
    effect_calls = 0;
    set_bits_global(&DAT_00118e04, 0x3f000000);
    set_bits_global_double(&reset_speed_scale, 0x33333333, 0x3fe33333);
    set_bits_global(&reset_decrement, 0x45bb8000);
    set_bits_global_double(&reset_bounce, 0x9999999a, 0x3fd99999);
    set_bits_global_double(&reset_heading_scale, 0x33333333, 0x3fd33333);
    set_float_table(0x184, 0x3f800000);
}

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_arg = index;
    return lookup_result;
}

void sound_enqueue(int id, int value, float extra)
{
    union {
        float f;
        unsigned int u;
    } v;

    sound_calls = sound_calls + 1;
    sound_id = id;
    sound_value = value;
    v.f = extra;
    sound_extra_bits = v.u;
}

void track_direction(int value)
{
    track_calls = track_calls + 1;
    track_arg = value;
    DAT_00108048 = track_result;
}

void effect_start(void *value)
{
    effect_calls = effect_calls + 1;
}

int main(void)
{
    reset();
    set_int(0x148, 2);
    set_int(0x4b0, 0x11223344);
    set_int(0x4b8, 0x55667788);
    set_bits(0x50c, 0x42c80000);
    DAT_00110aa4 = 7;
    track_result = 2.0f;
    player_car_reset_update(object);
    put(int_is(0x148, 1) && int_is(0x160, 7) && DAT_00001e18 == 1
        && float_is(0x50c, 0x42700000) && DAT_00108038 == 0x11223344
        && DAT_00108040 == 0x55667788 && float_is(0x4cc, 0x40000000)
        && float_is(0x14c, 0x00000000) && int_is(0x150, 0)
        && float_is(0x154, 0x41400000) && float_is(0x124, 0x40c00000)
        && float_is(0x15c, 0x41400000) && float_is(0x12c, 0x40c00000)
        && float_is(0x128, 0x00000000) && int_is(0x134, 1500)
        && int_is(0x170, 0)
        && lookup_calls == 1 && lookup_arg == 8
        && sound_calls == 1 && sound_id == 9 && sound_value == 3
        && sound_extra_bits == 0x3f000000
        && track_calls == 1 && track_arg == 0 && effect_calls == 0
        ? '0' : 'x');

    reset();
    set_int(0x148, 1);
    set_bits(0x14c, 0xc2c80000);
    set_int(0x134, 0);
    set_bits(0x154, 0x41200000);
    set_bits(0x124, 0x3f800000);
    set_bits(0x15c, 0x41a00000);
    set_bits(0x12c, 0x40000000);
    player_car_reset_update(object);
    put(int_is(0x148, 1) && int_is(0x150, 1) && int_is(0x134, 0)
        && float_is(0x14c, 0x449b0000) && float_is(0x154, 0xbf800000)
        && float_is(0x124, 0x00000000) && float_is(0x12c, 0x00000000)
        && float_is(0x15c, 0xbf19999a)
        && effect_calls == 1 && lookup_calls == 0 && sound_calls == 0
        ? '1' : 'x');

    reset();
    set_int(0x148, 1);
    set_int(0x150, 3);
    set_int(0x160, 2);
    set_bits(0x14c, 0xc2c80000);
    set_int(0x134, 0);
    DAT_00118e1c = 1;
    player_car_reset_update(object);
    put(int_is(0x148, 0) && int_is(0x150, 4) && int_is(0x134, 0)
        && float_is(0x14c, 0x449b0000) && float_is(0x15c, 0x3f19999a)
        && float_is(0x154, 0xbf800000)
        && DAT_00101db4[10] == 1 && DAT_00101db4[0] == 0
        && effect_calls == 1 ? '2' : 'x');

    reset();
    set_int(0x148, 5);
    set_int(0x7ac, 3);
    set_int(0x7d0, -1);
    set_int(0x4bc, 0x12345600);
    set_bits(0x108, 0x40800000);
    set_bits(0x4cc, 0x40000000);
    player_car_reset_update(object);
    put(int_is(0x148, 5) && int_is(0x7ac, 3) && int_is(0x7d0, 100)
        && int_is(0x4bc, 0x1234561f) && float_is(0x4cc, 0x404ccccd)
        && lookup_calls == 1 && lookup_arg == 0 && effect_calls == 1
        ? '3' : 'x');

    reset();
    set_int(0x148, 5);
    set_int(0x7ac, 0);
    set_int(0x7d0, -1);
    set_int(0x4bc, 0x12345678);
    set_bits(0x108, 0x40800000);
    set_bits(0x4cc, 0x40000000);
    player_car_reset_update(object);
    put(int_is(0x148, 5) && int_is(0x7ac, 0) && int_is(0x7d0, -1)
        && int_is(0x4bc, 0x12345678) && float_is(0x4cc, 0x40000000)
        && lookup_calls == 0 && effect_calls == 0 ? '4' : 'x');

    reset();
    set_int(0x148, 6);
    set_int(0x4bc, 0x12345678);
    player_car_reset_update(object);
    put(int_is(0x148, 0) && int_is(0x4bc, 0)
        && lookup_calls == 0 && effect_calls == 0 ? '5' : 'x');

    reset();
    set_int(0x148, 3);
    set_int(0x4bc, 0x12345678);
    set_bits(0x14c, 0xc2c80000);
    set_int(0x134, 11);
    player_car_reset_update(object);
    put(int_is(0x148, 3) && int_is(0x4bc, 0x12345678)
        && float_is(0x14c, 0xc2c80000) && int_is(0x134, 11)
        && lookup_calls == 0 && effect_calls == 0 ? '6' : 'x');

    reset();
    set_int(0x148, 2);
    set_bits(0x50c, 0x42c80000);
    DAT_00118e6c = 5;
    DAT_00001e18 = 0;
    player_car_reset_update(object);
    put(int_is(0x148, 1) && DAT_00001e18 == 0
        && lookup_calls == 1 && sound_calls == 1 ? '7' : 'x');

    reset();
    set_int(0x148, 1);
    set_int(0x150, 3);
    set_int(0x160, 2);
    set_bits(0x14c, 0xc2c80000);
    set_int(0x134, 0);
    DAT_00118e1c = 0;
    player_car_reset_update(object);
    put(int_is(0x148, 0) && int_is(0x150, 4)
        && DAT_00101db4[10] == 0 && effect_calls == 1 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
