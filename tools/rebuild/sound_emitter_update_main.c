/* The test harness for the rebuilt `sound_emitter_update` module.
 *
 * The harness holds the object record, the camera position and angle, the
 * sound table index and owner, the two sound flags, the six constants, and the
 * seven callee stubs.  The stubs record their arguments and return values the
 * harness selects.  The square-root stub returns a selected value for each call
 * and records the argument, so the harness checks the distance arithmetic and
 * the pan arithmetic apart.
 *
 * The harness runs twelve paths:
 *
 *     0  the sound gate is clear: the function returns at once
 *     1  the enable argument is zero: the function returns at once
 *     2  a clear sound below the range: the id comes from the table, the
 *        stored distance is set, the volume copies, the delta rotates, and the
 *        pitch, the volume, and the positive pan are set
 *     3  a set sound above the range: the stop runs, the id clears, and the
 *        function returns before the update
 *     4  a clear sound above the range: no table call and no update
 *     5  a set sound below the range: the id stays, the pitch rises from the
 *        stored distance, and the negative rotated value selects the negative
 *        pan
 *     6  the low bit of `DAT_0011d261` and a set `DAT_0010a8c`: the rotation
 *        and the values run, but the three stores stay off
 *     7  a distance exactly 40000 with a set sound: the sound stays and the
 *        volume is zero
 *     8  a distance exactly 40000 with a clear sound: the sound does not start
 *     9  the low bit of `DAT_0011d261` with a clear `DAT_0010a8c`: the three
 *        stores run
 *     a  a negative pitch term and a zero rotated value: the pitch falls and
 *        the pan stays centered
 *     D  the exit marker
 *
 * The float values and the expected values are set from their exact bit
 * patterns, so the harness does not depend on a float literal or on the
 * constant segment.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void sound_emitter_update(char *object, int enabled);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x60];

int DAT_00008058;
int DAT_0013ccc;
int DAT_0013cd0;
float DAT_0013cd4;
int DAT_0018d88;
unsigned int DAT_0018d94;
unsigned char DAT_0011d261;
int DAT_0010a8c;

double distance_scale;
float sound_range;
float pitch_scale;
float pitch_base;
float volume_scale;
double pan_scale;

static int sqrt_calls;
static double sqrt_argument[4];
static double sqrt_result[4];

static int lookup_calls;
static int lookup_arg;
static int lookup_result;

static int rotate_calls;
static int rotate_in0;
static int rotate_in2;
static float rotate_angle;
static int rotate_value;

static int stop_calls;
static int stop_arg;

static int pitch_calls;
static int pitch_id;
static int pitch_value;

static int volume_calls;
static unsigned int volume_owner;
static int volume_id;
static int volume_value;

static int pan_calls;
static unsigned int pan_owner;
static int pan_id;
static int pan_value;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_float(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *(float *)(object + offset) = v.f;
}

static void set_float_global(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static void set_double_global(double *p, unsigned int lo, unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.u[0] = lo;
    v.u[1] = hi;
    *p = v.d;
}

static void set_sqrt(int index, unsigned int lo, unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.u[0] = lo;
    v.u[1] = hi;
    sqrt_result[index] = v.d;
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int int_is(int offset, int value)
{
    return *(int *)(object + offset) == value;
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

static int float_bits_are(float value, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } x;
    union {
        float f;
        unsigned int u;
    } y;

    x.f = value;
    y.u = bits;
    return x.u == y.u;
}

static int double_bits_are(double value, unsigned int lo, unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } x;

    x.d = value;
    return x.u[0] == lo && x.u[1] == hi;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x60; index++) {
        object[index] = 0;
    }
    DAT_00008058 = 1;
    DAT_0013ccc = 0;
    DAT_0013cd0 = 0;
    set_float_global(&DAT_0013cd4, 0x3f000000);
    DAT_0018d88 = 100;
    DAT_0018d94 = 0x12345678;
    DAT_0011d261 = 0;
    DAT_0010a8c = 0;
    set_double_global(&distance_scale, 0x0, 0x40200000);
    set_float_global(&sound_range, 0x471c4000);
    set_float_global(&pitch_scale, 0x41100000);
    set_float_global(&pitch_base, 0x47800000);
    set_float_global(&volume_scale, 0x46fffe00);
    set_double_global(&pan_scale, 0x0, 0x40dfffc0);
    sqrt_calls = 0;
    for (index = 0; index < 4; index++) {
        sqrt_argument[index] = 0.0;
        sqrt_result[index] = 0.0;
    }
    lookup_calls = 0;
    lookup_arg = 0;
    lookup_result = 0;
    rotate_calls = 0;
    rotate_in0 = 0;
    rotate_in2 = 0;
    rotate_angle = 0.0f;
    rotate_value = 0;
    stop_calls = 0;
    stop_arg = 0;
    pitch_calls = 0;
    pitch_id = 0;
    pitch_value = 0;
    volume_calls = 0;
    volume_owner = 0;
    volume_id = 0;
    volume_value = 0;
    pan_calls = 0;
    pan_owner = 0;
    pan_id = 0;
    pan_value = 0;
}

double sqrt(double value)
{
    int index;

    index = sqrt_calls;
    if (index < 4) {
        sqrt_argument[index] = value;
    }
    sqrt_calls = index + 1;
    if (index < 4) {
        return sqrt_result[index];
    }
    return 0.0;
}

double fabs(double value)
{
    if (value < 0.0) {
        return -value;
    }
    return value;
}

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_arg = index;
    return lookup_result;
}

int sound_stop(int id)
{
    stop_calls = stop_calls + 1;
    stop_arg = id;
    return -1;
}

void vector_rotate(int *out, int *in, float angle)
{
    rotate_calls = rotate_calls + 1;
    rotate_in0 = in[0];
    rotate_in2 = in[2];
    rotate_angle = angle;
    out[0] = rotate_value;
    out[2] = 0;
}

void sound_set_pitch(int id, int value)
{
    pitch_calls = pitch_calls + 1;
    pitch_id = id;
    pitch_value = value;
}

void sound_set_volume(unsigned int owner, int id, int value)
{
    volume_calls = volume_calls + 1;
    volume_owner = owner;
    volume_id = id;
    volume_value = value;
}

void sound_set_pan(unsigned int owner, int id, int value)
{
    pan_calls = pan_calls + 1;
    pan_owner = owner;
    pan_id = id;
    pan_value = value;
}

int main(void)
{
    reset();
    DAT_00008058 = 0;
    set_int(0x38, -1);
    sound_emitter_update((char *)object, 1);
    put(sqrt_calls == 0 && lookup_calls == 0 && rotate_calls == 0
        && pitch_calls == 0 && volume_calls == 0 && pan_calls == 0
        && int_is(0x38, -1) ? '0' : 'x');

    reset();
    set_int(0x38, -1);
    sound_emitter_update((char *)object, 0);
    put(sqrt_calls == 0 && lookup_calls == 0 && rotate_calls == 0
        && pitch_calls == 0 && volume_calls == 0 && pan_calls == 0
        && int_is(0x38, -1) ? '1' : 'x');

    reset();
    set_int(0x14, 7);
    set_int(0x20, 8000);
    set_int(0x28, 0);
    set_int(0x38, -1);
    set_bits_float(0xc, 0x3fc00000);
    set_sqrt(0, 0x0, 0x408f4000);
    set_sqrt(1, 0x667f3bcd, 0x3fe6a09e);
    rotate_value = 4000;
    lookup_result = 107;
    sound_emitter_update((char *)object, 1);
    put(sqrt_calls == 2
        && double_bits_are(sqrt_argument[0], 0x0, 0x412e8480)
        && double_bits_are(sqrt_argument[1], 0x0, 0x3fe00000)
        && lookup_calls == 1 && lookup_arg == 107
        && int_is(0x38, 107) && int_is(0x3c, 8000)
        && float_is(0x40, 0x3fc00000)
        && rotate_calls == 1 && rotate_in0 == -8000 && rotate_in2 == 0
        && float_bits_are(rotate_angle, 0x3f000000)
        && pitch_calls == 1 && pitch_id == 107 && pitch_value == 65536
        && volume_calls == 1 && volume_owner == 0x12345678
        && volume_id == 107 && volume_value == 20970
        && pan_calls == 1 && pan_owner == 0x12345678
        && pan_id == 107 && pan_value == 55937 ? '2' : 'x');

    reset();
    set_int(0x20, 8000);
    set_int(0x28, 0);
    set_int(0x38, 5);
    set_int(0x3c, 1234);
    set_sqrt(0, 0x0, 0x40b77000);
    sound_emitter_update((char *)object, 1);
    put(sqrt_calls == 1 && stop_calls == 1 && stop_arg == 5
        && int_is(0x38, -1) && int_is(0x3c, 1234)
        && lookup_calls == 0 && rotate_calls == 0
        && pitch_calls == 0 && volume_calls == 0 && pan_calls == 0
        ? '3' : 'x');

    reset();
    set_int(0x20, 8000);
    set_int(0x28, 0);
    set_int(0x38, -1);
    set_sqrt(0, 0x0, 0x40b77000);
    sound_emitter_update((char *)object, 1);
    put(sqrt_calls == 1 && lookup_calls == 0 && stop_calls == 0
        && rotate_calls == 0 && pitch_calls == 0 && volume_calls == 0
        && pan_calls == 0 && int_is(0x38, -1) ? '4' : 'x');

    reset();
    set_int(0x20, 8000);
    set_int(0x28, 0);
    set_int(0x38, 9);
    set_int(0x3c, 12000);
    set_bits_float(0xc, 0x3fc00000);
    set_sqrt(0, 0x0, 0x408f4000);
    set_sqrt(1, 0x667f3bcd, 0x3fe6a09e);
    rotate_value = -4000;
    sound_emitter_update((char *)object, 1);
    put(sqrt_calls == 2
        && double_bits_are(sqrt_argument[0], 0x0, 0x412e8480)
        && double_bits_are(sqrt_argument[1], 0x0, 0x3fe00000)
        && lookup_calls == 0 && stop_calls == 0
        && int_is(0x38, 9) && int_is(0x3c, 8000)
        && float_is(0x40, 0x3fc00000)
        && rotate_calls == 1 && rotate_in0 == -8000 && rotate_in2 == 0
        && pitch_calls == 1 && pitch_id == 9 && pitch_value == 101536
        && volume_calls == 1 && volume_value == 20970
        && pan_calls == 1 && pan_value == 9599 ? '5' : 'x');

    reset();
    DAT_0011d261 = 1;
    DAT_0010a8c = 1;
    set_int(0x20, 8000);
    set_int(0x28, 0);
    set_int(0x38, 9);
    set_int(0x3c, 12000);
    set_bits_float(0xc, 0x3fc00000);
    set_sqrt(0, 0x0, 0x408f4000);
    set_sqrt(1, 0x667f3bcd, 0x3fe6a09e);
    rotate_value = -4000;
    sound_emitter_update((char *)object, 1);
    put(sqrt_calls == 2 && rotate_calls == 1
        && int_is(0x38, 9) && int_is(0x3c, 8000)
        && float_is(0x40, 0x3fc00000)
        && pitch_calls == 0 && volume_calls == 0 && pan_calls == 0
        ? '6' : 'x');

    reset();
    set_int(0x20, 8000);
    set_int(0x28, 0);
    set_int(0x38, 9);
    set_int(0x3c, 40000);
    set_bits_float(0xc, 0x3fc00000);
    set_sqrt(0, 0x0, 0x40b38800);
    set_sqrt(1, 0xc5bfedd9, 0x3fdc9f25);
    rotate_value = 8000;
    sound_emitter_update((char *)object, 1);
    put(sqrt_calls == 2
        && double_bits_are(sqrt_argument[0], 0x0, 0x412e8480)
        && double_bits_are(sqrt_argument[1], 0x9999999a, 0x3fc99999)
        && stop_calls == 0 && int_is(0x38, 9) && int_is(0x3c, 40000)
        && pitch_calls == 1 && pitch_value == 65536
        && volume_calls == 1 && volume_value == 0
        && pan_calls == 1 && pan_value == 47421 ? '7' : 'x');

    reset();
    set_int(0x20, 8000);
    set_int(0x28, 0);
    set_int(0x38, -1);
    set_sqrt(0, 0x0, 0x40b38800);
    sound_emitter_update((char *)object, 1);
    put(sqrt_calls == 1 && lookup_calls == 0 && rotate_calls == 0
        && pitch_calls == 0 && volume_calls == 0 && pan_calls == 0
        && int_is(0x38, -1) && int_is(0x3c, 0) ? '8' : 'x');

    reset();
    DAT_0011d261 = 1;
    set_int(0x20, 8000);
    set_int(0x28, 0);
    set_int(0x38, 9);
    set_int(0x3c, 12000);
    set_bits_float(0xc, 0x3fc00000);
    set_sqrt(0, 0x0, 0x408f4000);
    set_sqrt(1, 0x667f3bcd, 0x3fe6a09e);
    rotate_value = -4000;
    sound_emitter_update((char *)object, 1);
    put(sqrt_calls == 2 && rotate_calls == 1
        && int_is(0x38, 9) && int_is(0x3c, 8000)
        && pitch_calls == 1 && pitch_id == 9 && pitch_value == 101536
        && volume_calls == 1 && volume_value == 20970
        && pan_calls == 1 && pan_value == 9599 ? '9' : 'x');

    reset();
    set_int(0x20, 8000);
    set_int(0x28, 0);
    set_int(0x38, 9);
    set_int(0x3c, 1000);
    set_bits_float(0xc, 0x3fc00000);
    set_sqrt(0, 0x0, 0x408f4000);
    set_sqrt(1, 0x0, 0x0);
    rotate_value = 0;
    sound_emitter_update((char *)object, 1);
    put(sqrt_calls == 2
        && double_bits_are(sqrt_argument[1], 0x0, 0x0)
        && int_is(0x3c, 8000)
        && pitch_calls == 1 && pitch_id == 9 && pitch_value == 2536
        && volume_calls == 1 && volume_value == 20970
        && pan_calls == 1 && pan_value == 32768 ? 'a' : 'x');

    put('D');
    quit();
    return 0;
}
