/* The test harness for the rebuilt `player_car_skid_update` module.
 *
 * The harness holds one player object, the frame delta, the input flags, the
 * filter table, the effect handle, and the constants.  The `filter_update`
 * and `filter_smooth` stubs apply the table coefficients, and the table
 * entries are zero except the deceleration scale, so every filter writes its
 * argument exactly.
 *
 * The harness runs nine paths:
 *
 *     0  the deceleration path: the speed scales by 0.5, the shake amplitude
 *        is 28, the heading rises by 5.6, and the effect starts
 *     1  the normal path with a large slip: the amplitude is 0.0135, the rate
 *        is 2000, the sine is 0, the cosine is -1, and the handle releases
 *     2  the normal path with a small slip: the amplitude and the rate clear
 *        and the handle releases
 *     3  the positive amplitude clamp: the amplitude stops at 1.53938
 *     4  the negative amplitude clamp: the amplitude stops at -1.53938
 *     5  the gate above the limit sets +0x170 and keeps the old terms
 *     6  the early flag +0x170 keeps the slip block and the effect quiet
 *     7  the early state +0x148 keeps the handle when the state is 3
 *     8  the input bit 0x40 selects the normal path and keeps +0x110
 *     D  the exit marker
 */
#pragma off (check_stack)

void player_car_skid_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_00118e1d;
float DAT_00118e04;
float *DAT_0010cf08;

float skid_speed_floor;
float skid_rate_scale;
double skid_heading_scale;
double skid_slip_scale;
double skid_gate_bias;
float skid_angle_offset;
double skid_amplitude_scale;
double skid_amplitude_limit;
double skid_root_scale;
float skid_release_floor;
double skid_effect_threshold;

static char object[0x900];
static float table[0x100];
static char text[8];

static int update_calls;
static int update_last_index;
static int smooth_calls;
static int smooth_index;
static int lookup_calls;
static int lookup_arg;
static int release_calls;
static int release_arg;
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

static void reset(void)
{
    int i;

    for (i = 0; i < 0x900; i++) {
        object[i] = 0;
    }
    for (i = 0; i < 0x100; i++) {
        table[i] = 0.0f;
    }
    table[0x62] = 0.5f;
    update_calls = 0;
    update_last_index = 0;
    smooth_calls = 0;
    smooth_index = 0;
    lookup_calls = 0;
    lookup_arg = 0;
    release_calls = 0;
    release_arg = 0;
    effect_calls = 0;
    *(int *)(object + 0x7d0) = -1;
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int int_is(int offset, int value)
{
    return *(int *)(object + offset) == value;
}

static void set_float(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(object + offset) = value.f;
}

static void set_float_value(int offset, float value)
{
    *(float *)(object + offset) = value;
}

static int near_float(int offset, float expected, float tolerance)
{
    float actual;
    float difference;

    actual = *(float *)(object + offset);
    difference = actual - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference <= tolerance;
}

void filter_update(float *p, float f, int i)
{
    update_calls = update_calls + 1;
    update_last_index = i;
    *p = (*p - f) * table[i] + f;
}

void filter_smooth(float *p, float f, int i)
{
    smooth_calls = smooth_calls + 1;
    smooth_index = i;
    *p = f + (*p - f) * table[i];
}

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_arg = index;
    return 100;
}

int object_release(int id)
{
    release_calls = release_calls + 1;
    release_arg = id;
    return -1;
}

void effect_start(void *value)
{
    effect_calls = effect_calls + 1;
}

int main(void)
{
    skid_speed_floor = 2000.0f;
    skid_rate_scale = 7.0f;
    skid_heading_scale = 0.2;
    skid_slip_scale = 0.0001;
    skid_gate_bias = 0.5;
    skid_angle_offset = -1.5707964f;
    skid_amplitude_scale = 0.00015;
    skid_amplitude_limit = 1.539380443096161;
    skid_root_scale = 2.0;
    skid_release_floor = 10.0f;
    skid_effect_threshold = 0.23;

    DAT_0010cf08 = table;

    reset();
    DAT_00118e04 = 1.0f;
    DAT_00118e1d = 0;
    set_float(0x110, 0x42200000);
    set_float(0x50c, 0x43480000);
    set_float(0x480, 0x447a0000);
    set_float(0x2f4, 0x3f800000);
    set_float(0x184, 0x41200000);
    set_float(0x188, 0x447a0000);
    set_float(0x4cc, 0x41200000);
    player_car_skid_update(object);
    put(near_float(0x4f0, 28.0f, 0.001f)
        && near_float(0x4cc, (float)(10.0 + (double)28.0f * 0.2), 0.001f)
        && near_float(0x110, 20.0f, 0.001f)
        && near_float(0x128, 39.5979805f, 0.001f)
        && near_float(0x4f4, 0.0f, 0.001f)
        && near_float(0x4fc, 0.0f, 0.001f)
        && near_float(0x500, 0.0f, 0.001f)
        && near_float(0x508, 0.0f, 0.001f)
        && update_calls == 2 && update_last_index == 0x50
        && smooth_calls == 1 && smooth_index == 0x5a
        && lookup_calls == 1 && lookup_arg == 0
        && effect_calls == 1 && release_calls == 0
        && int_is(0x7d0, 100) ? '0' : 'x');

    reset();
    DAT_00118e04 = 1.0f;
    DAT_00118e1d = 0;
    set_int(0x7d0, 50);
    set_float(0x50c, 0x42c80000);
    set_float(0x2f4, 0x3f800000);
    set_float(0x670, 0x42c80000);
    set_float(0x2f8, 0x42c80000);
    set_float(0x184, 0x41200000);
    set_float(0x188, 0x447a0000);
    set_float(0x4cc, 0x3fc90fdb);
    set_float(0x500, 0x40e00000);
    set_float(0x508, 0xc0e00000);
    player_car_skid_update(object);
    put(near_float(0x4f0, (float)(90.0 * 0.00015), 0.001f)
        && near_float(0x4f4, 2000.0f, 0.001f)
        && near_float(0x500, 0.0f, 0.001f)
        && near_float(0x508, -1.0f, 0.001f)
        && near_float(0x4fc, 2000.0f, 0.001f)
        && near_float(0x128, 0.0190919f, 0.001f)
        && int_is(0x7d0, -1) && int_is(0x170, 0)
        && release_calls == 1 && release_arg == 50
        && lookup_calls == 0 && effect_calls == 0
        && update_calls == 1 && update_last_index == 0x50
        && smooth_calls == 1 ? '1' : 'x');

    reset();
    DAT_00118e04 = 1.0f;
    DAT_00118e1d = 0;
    set_int(0x7d0, 50);
    set_float(0x50c, 0x3f800000);
    set_float(0x2f4, 0x3f800000);
    set_float(0x670, 0x42c80000);
    set_float(0x2f8, 0x42c80000);
    set_float(0x184, 0x41200000);
    set_float(0x188, 0x447a0000);
    set_float(0x4cc, 0x3fc90fdb);
    set_float(0x500, 0x40e00000);
    set_float(0x508, 0xc0e00000);
    player_car_skid_update(object);
    put(near_float(0x4f0, 0.0f, 0.001f)
        && near_float(0x4f4, 0.0f, 0.001f)
        && near_float(0x4fc, 0.0f, 0.001f)
        && near_float(0x128, 0.0f, 0.001f)
        && near_float(0x500, 7.0f, 0.001f)
        && near_float(0x508, -7.0f, 0.001f)
        && int_is(0x7d0, -1)
        && release_calls == 1 && release_arg == 50
        && lookup_calls == 0 && effect_calls == 0
        && update_calls == 1 && update_last_index == 0x50
        && smooth_calls == 1 ? '2' : 'x');

    reset();
    DAT_00118e04 = 1.0f;
    DAT_00118e1d = 0;
    set_float(0x50c, 0x43480000);
    set_float(0x2f4, 0x3f800000);
    set_float(0x670, 0x461c4000);
    set_float(0x2f8, 0x42c80000);
    set_float(0x184, 0x41200000);
    set_float(0x188, 0x47c35000);
    set_float(0x4cc, 0x3fc90fdb);
    player_car_skid_update(object);
    put(near_float(0x4f0, (float)skid_amplitude_limit, 0.001f)
        && near_float(0x4f4, 800000.0f, 0.5f)
        && near_float(0x500, 0.0f, 0.001f)
        && near_float(0x508, -1.0f, 0.001f)
        && near_float(0x128, 2.1770127f, 0.001f)
        && int_is(0x7d0, 100)
        && lookup_calls == 1 && lookup_arg == 0
        && effect_calls == 1 && release_calls == 0 ? '3' : 'x');

    reset();
    DAT_00118e04 = 1.0f;
    DAT_00118e1d = 0;
    set_float(0x50c, 0x43480000);
    set_float(0x2f4, 0xbf800000);
    set_float(0x670, 0x461c4000);
    set_float(0x2f8, 0x42c80000);
    set_float(0x184, 0x41200000);
    set_float(0x188, 0x47c35000);
    set_float(0x4cc, 0x3fc90fdb);
    player_car_skid_update(object);
    put(near_float(0x4f0, (float)(-skid_amplitude_limit), 0.001f)
        && near_float(0x4f4, 800000.0f, 0.5f)
        && near_float(0x500, 0.0f, 0.001f)
        && near_float(0x508, -1.0f, 0.001f)
        && int_is(0x7d0, 100)
        && lookup_calls == 1 && effect_calls == 1
        && release_calls == 0 ? '4' : 'x');

    reset();
    DAT_00118e04 = 1.0f;
    DAT_00118e1d = 0;
    set_float(0x50c, 0x42c80000);
    set_float(0x2f4, 0x3f800000);
    set_float(0x670, 0x42c80000);
    set_float(0x184, 0x41200000);
    set_float(0x114, 0x41200000);
    set_float(0x188, 0x447a0000);
    set_float(0x4cc, 0x3fc90fdb);
    set_float_value(0x4f0, 3.0f);
    set_float_value(0x4f4, 4.0f);
    player_car_skid_update(object);
    put(int_is(0x170, 1)
        && near_float(0x4f0, 3.0f, 0.001f)
        && near_float(0x4f4, 4.0f, 0.001f)
        && near_float(0x4fc, 0.0f, 0.001f)
        && near_float(0x128, 4.2426405f, 0.001f)
        && int_is(0x7d0, 100)
        && lookup_calls == 1 && effect_calls == 1
        && release_calls == 0 ? '5' : 'x');

    reset();
    DAT_00118e04 = 1.0f;
    DAT_00118e1d = 0;
    set_int(0x170, 1);
    set_float(0x110, 0x42200000);
    set_float(0x4cc, 0x40a00000);
    player_car_skid_update(object);
    put(near_float(0x4cc, 5.0f, 0.001f)
        && near_float(0x110, 40.0f, 0.001f)
        && near_float(0x4f0, 0.0f, 0.001f)
        && near_float(0x4f4, 0.0f, 0.001f)
        && near_float(0x4fc, 0.0f, 0.001f)
        && near_float(0x128, 0.0f, 0.001f)
        && int_is(0x7d0, -1)
        && lookup_calls == 0 && effect_calls == 0 && release_calls == 0
        && update_calls == 1 && smooth_calls == 1 ? '6' : 'x');

    reset();
    DAT_00118e04 = 1.0f;
    DAT_00118e1d = 0;
    set_int(0x148, 3);
    set_int(0x7d0, 50);
    set_float(0x4cc, 0x40a00000);
    player_car_skid_update(object);
    put(near_float(0x4cc, 5.0f, 0.001f)
        && int_is(0x7d0, 50)
        && int_is(0x148, 3)
        && release_calls == 0 && lookup_calls == 0 && effect_calls == 0
        && update_calls == 1 && smooth_calls == 1 ? '7' : 'x');

    reset();
    DAT_00118e04 = 1.0f;
    DAT_00118e1d = 0x40;
    set_float(0x110, 0x42200000);
    set_float(0x50c, 0x42c80000);
    set_float(0x2f4, 0x3f800000);
    set_float(0x670, 0x42c80000);
    set_float(0x2f8, 0x42c80000);
    set_float(0x184, 0x41200000);
    set_float(0x188, 0x447a0000);
    set_float(0x4cc, 0x3fc90fdb);
    player_car_skid_update(object);
    put(near_float(0x110, 40.0f, 0.001f)
        && near_float(0x4f0, (float)(90.0 * 0.00015), 0.001f)
        && near_float(0x4f4, 2000.0f, 0.001f)
        && int_is(0x7d0, -1)
        && lookup_calls == 0 && effect_calls == 0 && release_calls == 0
        && update_calls == 1 && smooth_calls == 1 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
