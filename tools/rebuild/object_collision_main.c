/* The test harness for the rebuilt `object_collision` module.
 *
 * The harness holds the object record, the 0x590-byte record array, the six
 * constant globals, the two default ids, and the seven call stubs.  The stubs
 * record their arguments and return values the harness selects.  The harness
 * runs ten paths:
 *
 *     0  the gate `DAT_00108058` is clear: the function returns at once
 *     1  the state `+0x414` is clear with a set target: the release runs and
 *        the counter falls by one
 *     2  the counter is 2 and the type is not 0xc: the default id 2 sets, the
 *        deltas rotate, the three values form, and the three stores run
 *     3  the counter is 1 and the type is 0xc: the default id 10 sets and the
 *        angle at `+0x418` copies to `+0x4f4` before the values form
 *     4  the counter is 3 and the scan finds the fourth record: the target 6
 *        sets, the found record clears, the counter returns to 3, and the
 *        negative rotated value selects the negative sign
 *     5  the counter is 3 and the scan finds no record: the target stays -1
 *        and the function returns before the body
 *     6  the flag `+0x578` is set: the distance truncates into `+0x4f0`
 *        before the values form
 *     7  the low bit of `DAT_0011d261` is set and `+0x564` is set: the three
 *        stores stay off while the rotation and the values run
 *     8  the low bit of `DAT_0011d261` is set and `+0x564` is clear: the
 *        three stores run
 *     9  the state `+0x414` is clear with the target -1: the function returns
 *        without the release
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

void object_collision(char *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x600];

int DAT_00108058;
int DAT_00113cc8;
int DAT_0010f8ec;
unsigned char DAT_00110ab8[0x1700];
int DAT_00113ccc;
int DAT_00113cd0;
float DAT_00113cd4;
unsigned char DAT_00118e1c;
unsigned char DAT_0011d261;
unsigned int DAT_00118d94;
float collision_scale;
double collision_bias;
double collision_offset;
float collision_limit;
float collision_gain;
double collision_amplitude;
int collision_ids[2];

static int lookup_calls;
static int lookup_arg;
static int rotate_calls;
static int rotate_in0;
static int rotate_in2;
static float rotate_angle;
static int rotate_value;
static int sqrt_calls;
static double sqrt_argument;
static double sqrt_result;
static int a_calls;
static int a_id;
static int a_value;
static int b_calls;
static unsigned int b_owner;
static int b_id;
static int b_value;
static int c_calls;
static unsigned int c_owner;
static int c_id;
static int c_value;
static int release_calls;
static int release_arg;

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

static void set_global_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static void set_global_double(double *p, unsigned int lo, unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.u[0] = lo;
    v.u[1] = hi;
    *p = v.d;
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

static void set_record(int index, int id, int target, unsigned int distance)
{
    char *record;

    record = (char *)DAT_00110ab8 + index * 0x590;
    *(int *)record = id;
    *(int *)(record + 0x4ec) = target;
    {
        union {
            float f;
            unsigned int u;
        } v;

        v.u = distance;
        *(float *)(record + 0x4e4) = v.f;
    }
}

static int record_target_is(int index, int target)
{
    char *record;

    record = (char *)DAT_00110ab8 + index * 0x590;
    return *(int *)(record + 0x4ec) == target;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x600; index++) {
        object[index] = 0;
    }
    for (index = 0; index < 0x1700; index++) {
        DAT_00110ab8[index] = 0;
    }
    DAT_00108058 = 1;
    DAT_00113cc8 = 0;
    DAT_0010f8ec = 0;
    DAT_00113ccc = 0;
    DAT_00113cd0 = 0;
    set_global_float(&DAT_00113cd4, 0x00000000);
    DAT_00118e1c = 0;
    DAT_0011d261 = 0;
    DAT_00118d94 = 0x12345678;
    set_global_float(&collision_scale, 0x41100000);
    set_global_double(&collision_bias, 0x33333333, 0x40033333);
    set_global_double(&collision_offset, 0x00000000, 0x40ca8000);
    set_global_float(&collision_limit, 0x4708b800);
    set_global_float(&collision_gain, 0x46b80000);
    set_global_double(&collision_amplitude, 0x00000000, 0x40dfffc0);
    collision_ids[0] = 2;
    collision_ids[1] = 10;
    lookup_calls = 0;
    lookup_arg = 0;
    rotate_calls = 0;
    rotate_in0 = 0;
    rotate_in2 = 0;
    rotate_angle = 0.0f;
    rotate_value = 0;
    sqrt_calls = 0;
    sqrt_argument = 0.0;
    sqrt_result = 0.0;
    a_calls = 0;
    a_id = 0;
    a_value = 0;
    b_calls = 0;
    b_owner = 0;
    b_id = 0;
    b_value = 0;
    c_calls = 0;
    c_owner = 0;
    c_id = 0;
    c_value = 0;
    release_calls = 0;
    release_arg = 0;
}

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_arg = index;
    return index;
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

double sqrt(double value)
{
    sqrt_calls = sqrt_calls + 1;
    sqrt_argument = value;
    return sqrt_result;
}

void object_store_a(int id, int value)
{
    a_calls = a_calls + 1;
    a_id = id;
    a_value = value;
}

void object_store_b(unsigned int owner, int id, int value)
{
    b_calls = b_calls + 1;
    b_owner = owner;
    b_id = id;
    b_value = value;
}

void object_store_c(unsigned int owner, int id, int value)
{
    c_calls = c_calls + 1;
    c_owner = owner;
    c_id = id;
    c_value = value;
}

int object_release(int id)
{
    release_calls = release_calls + 1;
    release_arg = id;
    return -1;
}

int main(void)
{
    reset();
    DAT_00108058 = 0;
    set_int(0x4ec, 5);
    set_int(0x414, 1);
    set_int(0x4f0, 100);
    set_bits_float(0x4e4, 0x42a00000);
    object_collision((char *)object);
    put(int_is(0x4ec, 5) && int_is(0x4f0, 100) && int_is(0x414, 1)
        && release_calls == 0 && rotate_calls == 0 && a_calls == 0
        && b_calls == 0 && c_calls == 0 && lookup_calls == 0 ? '0' : 'x');

    reset();
    DAT_00113cc8 = 4;
    set_int(0x414, 0);
    set_int(0x4ec, 5);
    object_collision((char *)object);
    put(release_calls == 1 && release_arg == 5 && int_is(0x4ec, -1)
        && DAT_00113cc8 == 3 && rotate_calls == 0 && a_calls == 0 ? '1' : 'x');

    reset();
    DAT_00113cc8 = 2;
    set_int(0x414, 1);
    set_int(0x4ec, -1);
    set_int(0x2e0, 0x10);
    set_int(0x578, 0);
    set_int(0x4f0, 100);
    set_bits_float(0x4e4, 0x42a00000);
    set_bits_float(0x4f4, 0x41200000);
    set_int(0x498, 1000);
    set_int(0x4a0, 2000);
    DAT_00113ccc = 1030;
    DAT_00113cd0 = 1990;
    set_global_float(&DAT_00113cd4, 0x3f000000);
    rotate_value = 40;
    set_global_double(&sqrt_result, 0x00000000, 0x40000000);
    object_collision((char *)object);
    put(lookup_calls == 1 && lookup_arg == 2 && int_is(0x4ec, 2)
        && DAT_00113cc8 == 3 && rotate_calls == 1 && rotate_in0 == 30
        && rotate_in2 == -10 && float_bits_are(rotate_angle, 0x3f000000)
        && sqrt_calls == 1
        && double_bits_are(sqrt_argument, 0x00000000, 0x3fe00000)
        && a_calls == 1 && a_id == 2 && a_value == 13592
        && b_calls == 1 && b_owner == 0x12345678 && b_id == 2
        && b_value == 23444 && c_calls == 1 && c_owner == 0x12345678
        && c_id == 2 && c_value == 98302 && int_is(0x4f0, 80) ? '2' : 'x');

    reset();
    DAT_00113cc8 = 1;
    DAT_00118e1c = 5;
    set_int(0x414, 1);
    set_int(0x4ec, -1);
    set_int(0x2e0, 0xc);
    set_int(0x578, 0);
    set_int(0x4f0, 100);
    set_bits_float(0x4e4, 0x42a00000);
    set_bits_float(0x4f4, 0x41200000);
    set_bits_float(0x418, 0x40500000);
    set_global_float(&DAT_00113cd4, 0x3f000000);
    rotate_value = 40;
    set_global_double(&sqrt_result, 0x00000000, 0x40000000);
    object_collision((char *)object);
    put(lookup_calls == 1 && lookup_arg == 10 && int_is(0x4ec, 10)
        && DAT_00113cc8 == 2 && float_is(0x4f4, 0x40500000)
        && sqrt_calls == 1 && a_calls == 1 && a_id == 10
        && a_value == 13575 && b_calls == 1 && b_id == 10
        && b_value == 23444 && c_calls == 1 && c_id == 10
        && c_value == 98302 && int_is(0x4f0, 80) ? '3' : 'x');

    reset();
    DAT_00113cc8 = 3;
    DAT_0010f8ec = 4;
    set_record(0, 7, 0, 0x42a00000);
    set_record(1, 8, -1, 0x42b40000);
    set_record(2, 9, 5, 0x428c0000);
    set_record(3, 10, 6, 0x42b40000);
    set_int(0x0, 7);
    set_int(0x414, 1);
    set_int(0x4ec, -1);
    set_int(0x2e0, 0x10);
    set_int(0x578, 0);
    set_int(0x4f0, 100);
    set_bits_float(0x4e4, 0x42a00000);
    set_bits_float(0x4f4, 0x41200000);
    set_int(0x498, 1000);
    set_int(0x4a0, 2000);
    DAT_00113ccc = 1030;
    DAT_00113cd0 = 1990;
    set_global_float(&DAT_00113cd4, 0x3f000000);
    rotate_value = -40;
    set_global_double(&sqrt_result, 0x00000000, 0x40000000);
    object_collision((char *)object);
    put(lookup_calls == 0 && int_is(0x4ec, 6) && DAT_00113cc8 == 3
        && record_target_is(3, -1) && rotate_calls == 1
        && rotate_in0 == 30 && rotate_in2 == -10 && sqrt_calls == 1
        && a_calls == 1 && a_id == 6 && a_value == 13592
        && b_calls == 1 && b_id == 6 && b_value == 23444
        && c_calls == 1 && c_id == 6 && c_value == -32766
        && int_is(0x4f0, 80) ? '4' : 'x');

    reset();
    DAT_00113cc8 = 3;
    DAT_0010f8ec = 4;
    set_record(0, 7, 0, 0x42a00000);
    set_record(1, 8, -1, 0x42b40000);
    set_record(2, 9, 5, 0x428c0000);
    set_record(3, 10, 6, 0x42a00000);
    set_int(0x0, 7);
    set_int(0x414, 1);
    set_int(0x4ec, -1);
    set_int(0x578, 0);
    set_int(0x4f0, 100);
    set_bits_float(0x4e4, 0x42a00000);
    set_bits_float(0x4f4, 0x41200000);
    object_collision((char *)object);
    put(int_is(0x4ec, -1) && DAT_00113cc8 == 3 && int_is(0x4f0, 100)
        && lookup_calls == 0 && rotate_calls == 0 && a_calls == 0
        && b_calls == 0 && c_calls == 0 ? '5' : 'x');

    reset();
    DAT_00113cc8 = 2;
    set_int(0x414, 1);
    set_int(0x4ec, -1);
    set_int(0x2e0, 0x10);
    set_int(0x578, 1);
    set_int(0x4f0, 100);
    set_bits_float(0x4e4, 0x42a00000);
    set_bits_float(0x4f4, 0x41200000);
    set_global_float(&DAT_00113cd4, 0x3f000000);
    rotate_value = 40;
    set_global_double(&sqrt_result, 0x00000000, 0x40000000);
    object_collision((char *)object);
    put(lookup_calls == 1 && lookup_arg == 2 && int_is(0x4ec, 2)
        && DAT_00113cc8 == 3 && a_calls == 1 && a_value == 13592
        && b_value == 23444 && c_value == 98302 && int_is(0x4f0, 80)
        ? '6' : 'x');

    reset();
    DAT_00113cc8 = 2;
    DAT_0011d261 = 1;
    set_int(0x414, 1);
    set_int(0x4ec, -1);
    set_int(0x2e0, 0x10);
    set_int(0x578, 0);
    set_int(0x564, 1);
    set_int(0x4f0, 100);
    set_bits_float(0x4e4, 0x42a00000);
    set_bits_float(0x4f4, 0x41200000);
    set_global_float(&DAT_00113cd4, 0x3f000000);
    rotate_value = 40;
    set_global_double(&sqrt_result, 0x00000000, 0x40000000);
    object_collision((char *)object);
    put(lookup_calls == 1 && int_is(0x4ec, 2) && DAT_00113cc8 == 3
        && rotate_calls == 1 && sqrt_calls == 1 && a_calls == 0
        && b_calls == 0 && c_calls == 0 && int_is(0x4f0, 80) ? '7' : 'x');

    reset();
    DAT_00113cc8 = 2;
    DAT_0011d261 = 1;
    set_int(0x414, 1);
    set_int(0x4ec, -1);
    set_int(0x2e0, 0x10);
    set_int(0x578, 0);
    set_int(0x564, 0);
    set_int(0x4f0, 100);
    set_bits_float(0x4e4, 0x42a00000);
    set_bits_float(0x4f4, 0x41200000);
    set_global_float(&DAT_00113cd4, 0x3f000000);
    rotate_value = 40;
    set_global_double(&sqrt_result, 0x00000000, 0x40000000);
    object_collision((char *)object);
    put(lookup_calls == 1 && int_is(0x4ec, 2) && DAT_00113cc8 == 3
        && rotate_calls == 1 && sqrt_calls == 1 && a_calls == 1
        && a_value == 13592 && b_calls == 1 && b_value == 23444
        && c_calls == 1 && c_value == 98302 && int_is(0x4f0, 80)
        ? '8' : 'x');

    reset();
    DAT_00113cc8 = 2;
    set_int(0x414, 0);
    set_int(0x4ec, -1);
    set_int(0x4f0, 100);
    object_collision((char *)object);
    put(int_is(0x4ec, -1) && DAT_00113cc8 == 2 && int_is(0x4f0, 100)
        && release_calls == 0 && rotate_calls == 0 && a_calls == 0
        ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
