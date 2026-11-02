/* The test harness for the rebuilt `object_integrate` module.
 *
 * The harness holds the object record, the frame delta, the damping table
 * pointer, the damping table, and the two constant globals.  It sets the exact
 * bit patterns before the first call.  The harness runs eight paths:
 *
 *     0  the positions differ from the target: the velocity scales by the
 *        factor and the frame delta, each sum truncates, the factor stays
 *        above the threshold, and the timer clears
 *     1  the factor stays below the threshold: the factor clears
 *     2  the positions equal the target and the timer stays below the dwell
 *        threshold: the timer accumulates and the positions stay
 *     3  the dwell timer reaches the threshold: both positions snap by the
 *        scale and truncate
 *     4  a negative velocity produces a negative fraction: each sum truncates
 *        toward zero, so the result is -3 and 3
 *     5  a negative position snaps from -7: the snap truncates toward zero,
 *        so the result is -3 and 3
 *     6  the factor equals the threshold: the factor stays
 *     7  the damping reduces the factor from 12 to 6: the factor clears
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

void object_integrate(char *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];
static unsigned char table[0x200];

float DAT_00118e04;
unsigned int DAT_0010cf08;
float integrate_factor_threshold;
float integrate_dwell_threshold;

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

static void set_bits_table(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *(float *)(table + offset) = v.f;
}

static void set_bits_global(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
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

static void reset(void)
{
    int index;

    for (index = 0; index < 0x500; index++) {
        object[index] = 0;
    }
    DAT_0010cf08 = (unsigned int)table;
    set_bits_global(&integrate_factor_threshold, 0x41200000);
    set_bits_global(&integrate_dwell_threshold, 0x40400000);
}

static void setup(int pos_x, int pos_y, unsigned int vel_x,
                  unsigned int vel_y, unsigned int factor,
                  unsigned int delta, unsigned int damping,
                  unsigned int snap, int target_x, int target_y,
                  unsigned int timer)
{
    reset();
    set_int(0x454, pos_x);
    set_int(0x45c, pos_y);
    set_bits_float(0x460, vel_x);
    set_bits_float(0x468, vel_y);
    set_bits_float(0x450, factor);
    set_bits_float(0x46c, timer);
    set_int(0x160, target_x);
    set_int(0x168, target_y);
    set_bits_global(&DAT_00118e04, delta);
    set_bits_table(0x13c, damping);
    set_bits_table(0x180, snap);
}

int main(void)
{
    setup(100, 200, 0x40000000, 0xc0800000, 0x41a00000,
          0x3f800000, 0x3f800000, 0x3f000000, 0, 0, 0x40a00000);
    object_integrate((char *)object);
    put(int_is(0x454, 140) && int_is(0x45c, 120)
        && float_is(0x450, 0x41a00000) && float_is(0x46c, 0x00000000)
        ? '0' : 'x');

    setup(0, 0, 0x00000000, 0x00000000, 0x40a00000,
          0x3f800000, 0x3f800000, 0x3f000000, 1, 1, 0x00000000);
    object_integrate((char *)object);
    put(int_is(0x454, 0) && int_is(0x45c, 0)
        && float_is(0x450, 0x00000000) && float_is(0x46c, 0x00000000)
        ? '1' : 'x');

    setup(7, 8, 0x00000000, 0x00000000, 0x41a00000,
          0x3f800000, 0x3f800000, 0x3f000000, 7, 8, 0x3fc00000);
    object_integrate((char *)object);
    put(int_is(0x454, 7) && int_is(0x45c, 8)
        && float_is(0x46c, 0x40200000) ? '2' : 'x');

    setup(7, 8, 0x00000000, 0x00000000, 0x41a00000,
          0x3f800000, 0x3f800000, 0x3f000000, 7, 8, 0x40000000);
    object_integrate((char *)object);
    put(int_is(0x454, 3) && int_is(0x45c, 4)
        && float_is(0x46c, 0x40400000) ? '3' : 'x');

    setup(0, 0, 0xc0e00000, 0x40e00000, 0x41200000,
          0x3d4ccccd, 0x3f800000, 0x3f000000, 0, 0, 0x00000000);
    object_integrate((char *)object);
    put(int_is(0x454, -3) && int_is(0x45c, 3)
        && float_is(0x450, 0x41200000) && float_is(0x46c, 0x00000000)
        ? '4' : 'x');

    setup(-7, 7, 0x00000000, 0x00000000, 0x41a00000,
          0x00000000, 0x3f800000, 0x3f000000, -7, 7, 0x40400000);
    object_integrate((char *)object);
    put(int_is(0x454, -3) && int_is(0x45c, 3)
        && float_is(0x46c, 0x40400000) ? '5' : 'x');

    setup(0, 0, 0x00000000, 0x00000000, 0x41200000,
          0x3f800000, 0x3f800000, 0x3f000000, 1, 1, 0x00000000);
    object_integrate((char *)object);
    put(float_is(0x450, 0x41200000) ? '6' : 'x');

    setup(0, 0, 0x00000000, 0x00000000, 0x41400000,
          0x3f800000, 0x3f000000, 0x3f000000, 1, 1, 0x00000000);
    object_integrate((char *)object);
    put(float_is(0x450, 0x00000000) ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
