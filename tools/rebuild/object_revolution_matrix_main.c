/* The test harness for the rebuilt `object_revolution_matrix` module.
 *
 * The harness holds the object record, the gear table with 36-byte records,
 * the entry table with 56-byte entries, and a sampler stub.  The sampler stub
 * records its three arguments and writes five sentinels to the output, so the
 * harness confirms the call contract.
 *
 * Each entry holds the next index at `+0x10` and three signed integer points
 * at `+0x14` with a three-int stride.  The harness runs six paths:
 *
 *     0  gear flag 0: the control points are the first point of the current
 *        entry, the second point of the current entry, and the first point of
 *        the next entry
 *     1  gear flag 1: the control points are the second point of the current
 *        entry, the first point of the next entry, and the second point of the
 *        next entry
 *     2  a different gear index and entry index: the 36-byte record stride and
 *        the 56-byte entry stride select the expected entries
 *     3  negative integers: the conversion keeps the sign
 *     4  fraction 0.75: the sampler receives the fraction, the output pointer,
 *        and the control-point pointer, and the three z slots stay unchanged
 *     5  gear flag 1 with the end markers: the second point of the next entry
 *        fills the third control point
 *     D  the exit marker
 *
 * The float values and the expected values are set from their exact bit
 * patterns, so the harness does not depend on a float literal or on the
 * constant segment.
 */
#pragma off (check_stack)

void object_revolution_matrix(char *object);
void func_1e7b4(float *output, float *points, float fraction);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];
static unsigned int gear_table[0x100];
static unsigned int entry_table[0x200];

unsigned int DAT_0010f8e4;

static int sampler_calls;
static float *sampler_output;
static float *sampler_points;
static float sampler_fraction;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static int float_bits(float value, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } left;
    union {
        float f;
        unsigned int u;
    } right;

    left.f = value;
    right.u = bits;
    return left.u == right.u;
}

static void set_float(int offset, unsigned int bits)
{
    set_bits_float((float *)((char *)object + offset), bits);
}

static void set_int(int offset, int value)
{
    *(int *)((char *)object + offset) = value;
}

static int int_is(int offset, int value)
{
    return *(int *)((char *)object + offset) == value;
}

static int float_is(int offset, unsigned int bits)
{
    return float_bits(*(float *)((char *)object + offset), bits);
}

static void set_entry(int index, int offset, int value)
{
    *(int *)((char *)entry_table + index * 56 + offset) = value;
}

static void point(int index, int x, int y, int z)
{
    set_entry(index, 0x14, x);
    set_entry(index, 0x18, y);
    set_entry(index, 0x1c, z);
    set_entry(index, 0x20, x + 10);
    set_entry(index, 0x24, y + 10);
    set_entry(index, 0x28, z + 10);
    set_entry(index, 0x2c, x + 20);
    set_entry(index, 0x30, y + 20);
    set_entry(index, 0x34, z + 20);
}

void func_1e7b4(float *output, float *points, float fraction)
{
    sampler_calls = sampler_calls + 1;
    sampler_output = output;
    sampler_points = points;
    sampler_fraction = fraction;
    set_bits_float(output + 0, 0x3fa00000);
    set_bits_float(output + 1, 0x40200000);
    set_bits_float(output + 2, 0x40700000);
    set_bits_float(output + 3, 0x40800000);
    set_bits_float(output + 4, 0x40a00000);
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x500; index++) {
        object[index] = 0;
    }
    for (index = 0; index < 0x100; index++) {
        gear_table[index] = 0;
    }
    for (index = 0; index < 0x200; index++) {
        entry_table[index] = 0;
    }
    DAT_0010f8e4 = (unsigned int)gear_table;
    for (index = 0; index < 8; index++) {
        *(unsigned int *)((char *)gear_table + index * 36) =
            (unsigned int)entry_table;
    }
    sampler_calls = 0;
    sampler_output = 0;
    sampler_points = 0;
    sampler_fraction = 0.0f;
}

static void setup(int gear_index, int entry_index, int flag,
                  unsigned int fraction)
{
    reset();
    set_int(0x248, gear_index);
    set_int(0x24c, entry_index);
    set_int(0x254, flag);
    set_float(0x250, fraction);
}

int main(void)
{
    setup(2, 1, 0, 0x3f800000);
    point(1, 11, 12, 13);
    set_entry(1, 0x10, 4);
    point(4, 41, 42, 43);
    object_revolution_matrix((char *)object);
    put(float_is(0x284, 0x41300000) && float_is(0x288, 0x41400000)
        && float_is(0x290, 0x41a80000) && float_is(0x294, 0x41b00000)
        && float_is(0x29c, 0x42240000) && float_is(0x2a0, 0x42280000)
        && sampler_calls == 1
        && sampler_points == (float *)(object + 0x284)
        && sampler_output == (float *)(object + 0x2a8)
        && float_bits(sampler_fraction, 0x3f800000)
        && float_is(0x2a8, 0x3fa00000)
        ? '0' : 'x');

    setup(2, 1, 1, 0x3f800000);
    point(1, 11, 12, 13);
    set_entry(1, 0x10, 4);
    point(4, 41, 42, 43);
    object_revolution_matrix((char *)object);
    put(float_is(0x284, 0x41a80000) && float_is(0x288, 0x41b00000)
        && float_is(0x290, 0x42240000) && float_is(0x294, 0x42280000)
        && float_is(0x29c, 0x424c0000) && float_is(0x2a0, 0x42500000)
        && sampler_calls == 1
        ? '1' : 'x');

    setup(3, 2, 0, 0x00000000);
    point(2, 7, 8, 9);
    set_entry(2, 0x10, 0);
    point(0, 1, 2, 3);
    object_revolution_matrix((char *)object);
    put(float_is(0x284, 0x40e00000) && float_is(0x288, 0x41000000)
        && float_is(0x290, 0x41880000) && float_is(0x294, 0x41900000)
        && float_is(0x29c, 0x3f800000) && float_is(0x2a0, 0x40000000)
        && sampler_calls == 1
        ? '2' : 'x');

    setup(2, 1, 0, 0x3f800000);
    set_entry(1, 0x14, -11);
    set_entry(1, 0x18, -12);
    set_entry(1, 0x1c, -13);
    set_entry(1, 0x20, -21);
    set_entry(1, 0x24, -22);
    set_entry(1, 0x28, -23);
    set_entry(1, 0x10, 4);
    set_entry(4, 0x14, -41);
    set_entry(4, 0x18, -42);
    set_entry(4, 0x1c, -43);
    set_entry(4, 0x20, -51);
    set_entry(4, 0x24, -52);
    set_entry(4, 0x28, -53);
    object_revolution_matrix((char *)object);
    put(float_is(0x284, 0xc1300000) && float_is(0x288, 0xc1400000)
        && float_is(0x290, 0xc1a80000) && float_is(0x294, 0xc1b00000)
        && float_is(0x29c, 0xc2240000) && float_is(0x2a0, 0xc2280000)
        && sampler_calls == 1
        ? '3' : 'x');

    setup(2, 1, 0, 0x3f400000);
    point(1, 11, 12, 13);
    set_entry(1, 0x10, 4);
    point(4, 41, 42, 43);
    set_int(0x28c, 0x11223344);
    set_int(0x298, 0x22334455);
    set_int(0x2a4, 0x33445566);
    object_revolution_matrix((char *)object);
    put(int_is(0x28c, 0x11223344) && int_is(0x298, 0x22334455)
        && int_is(0x2a4, 0x33445566)
        && sampler_calls == 1
        && float_bits(sampler_fraction, 0x3f400000)
        && sampler_points == (float *)(object + 0x284)
        && sampler_output == (float *)(object + 0x2a8)
        ? '4' : 'x');

    setup(5, 0, 1, 0x3e800000);
    point(0, 11, 12, 13);
    set_entry(0, 0x10, 1);
    point(1, 41, 42, 43);
    object_revolution_matrix((char *)object);
    put(float_is(0x284, 0x41a80000) && float_is(0x288, 0x41b00000)
        && float_is(0x290, 0x42240000) && float_is(0x294, 0x42280000)
        && float_is(0x29c, 0x424c0000) && float_is(0x2a0, 0x42500000)
        && sampler_calls == 1
        && float_bits(sampler_fraction, 0x3e800000)
        ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
