/* The test harness for the rebuilt `object_gear_step` module.
 *
 * The harness holds the object record, the frame delta, the damping table
 * pointer, the damping table, and the four constant globals.  It sets the
 * exact bit patterns before the first call.  The harness runs eight paths:
 *
 *     0  state 1 stays above the target: the timer, the three velocities, and
 *        the three integrated values update, and the counter stays
 *     1  state 2 initializes, integrates above the target, and wraps: the
 *        state falls to 1, the counter sets 1, the timer reflects to -600,
 *        and the odd counter selects -0.6
 *     2  state 1 wraps with the counter at 3: the counter sets 4, the timer
 *        reflects to 600, the even counter selects 0.6, and the state clears
 *     3  state 1 keeps the position at -1: the negative sum truncates toward
 *        zero, and the position stays above the target
 *     4  state 3 initializes and integrates below the wrap threshold: the
 *        state falls to 4 and the value holds 6.0
 *     5  state 4 integrates past the wrap threshold: the value and the state
 *        clear
 *     6  state 2 initializes, integrates below the target, and damps: the
 *        state falls to 1 and the three integrated values update
 *     7  the state 5 selects no case: the record stays as set
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

void object_gear_step(char *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];
static unsigned char table[0x200];

float DAT_00118e04;
unsigned int DAT_0010cf08;
float gear_decrement;
double gear_bounce;
double gear_steer;
float gear_wrap;

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

static void set_bits_global_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static void set_bits_global_double(double *p, unsigned int lo,
                                   unsigned int hi)
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

static void reset(void)
{
    int index;

    for (index = 0; index < 0x500; index++) {
        object[index] = 0;
    }
    for (index = 0; index < 0x200; index++) {
        table[index] = 0;
    }
    DAT_0010cf08 = (unsigned int)table;
    set_bits_global_float(&DAT_00118e04, 0x3f000000);
    set_bits_global_float(&gear_decrement, 0x45bb8000);
    set_bits_global_double(&gear_bounce, 0x9999999a, 0x3fd99999);
    set_bits_global_double(&gear_steer, 0x33333333, 0x3fe33333);
    set_bits_global_float(&gear_wrap, 0x41490fdb);
}

static void setup(int state, int pos, int target, unsigned int timer,
                  unsigned int vel_a, unsigned int vel_b, unsigned int vel_c,
                  unsigned int pos_a, unsigned int pos_b, unsigned int pos_c,
                  int count, unsigned int delta, unsigned int damping)
{
    reset();
    set_int(0x2f8, state);
    set_int(0x49c, pos);
    set_int(0x238, target);
    set_int(0x300, count);
    set_bits_float(0x2fc, timer);
    set_bits_float(0x304, vel_a);
    set_bits_float(0x308, vel_b);
    set_bits_float(0x30c, vel_c);
    set_bits_float(0x314, pos_a);
    set_bits_float(0x318, pos_b);
    set_bits_float(0x31c, pos_c);
    set_bits_global_float(&DAT_00118e04, delta);
    set_bits_table(0x184, damping);
}

int main(void)
{
    setup(1, 10, 0, 0x00000000,
          0x40000000, 0x40800000, 0x40c00000,
          0x42c80000, 0x43480000, 0x43960000,
          0, 0x3f000000, 0x3f800000);
    object_gear_step((char *)object);
    put(int_is(0x2f8, 1) && int_is(0x49c, 10) && int_is(0x300, 0)
        && float_is(0x2fc, 0xc53b8000)
        && float_is(0x304, 0x40000000) && float_is(0x314, 0x42ca0000)
        && float_is(0x308, 0x40800000) && float_is(0x318, 0x434a0000)
        && float_is(0x30c, 0x40c00000) && float_is(0x31c, 0x43978000)
        ? '0' : 'x');

    setup(2, 0, 2000, 0x00000000,
          0x00000000, 0x00000000, 0x00000000,
          0x41200000, 0x41a00000, 0x41f00000,
          0, 0x3e800000, 0x3f800000);
    object_gear_step((char *)object);
    put(int_is(0x2f8, 1) && int_is(0x49c, 2000) && int_is(0x300, 1)
        && float_is(0x2fc, 0xc4160000)
        && float_is(0x304, 0xbf800000) && float_is(0x314, 0x00000000)
        && float_is(0x308, 0x00000000) && float_is(0x318, 0x00000000)
        && float_is(0x30c, 0xbf19999a) && float_is(0x31c, 0x00000000)
        ? '1' : 'x');

    setup(1, 0, 100, 0x00000000,
          0x00000000, 0x00000000, 0x00000000,
          0x00000000, 0x00000000, 0x00000000,
          3, 0x3e800000, 0x3f800000);
    object_gear_step((char *)object);
    put(int_is(0x2f8, 0) && int_is(0x49c, 100) && int_is(0x300, 4)
        && float_is(0x2fc, 0x44160000)
        && float_is(0x30c, 0x3f19999a)
        && float_is(0x314, 0x00000000) && float_is(0x318, 0x00000000)
        && float_is(0x31c, 0x00000000)
        ? '2' : 'x');

    setup(1, 0, -5, 0xc0400000,
          0x00000000, 0x00000000, 0x00000000,
          0x00000000, 0x00000000, 0x00000000,
          0, 0x3f000000, 0x3f800000);
    object_gear_step((char *)object);
    put(int_is(0x2f8, 1) && int_is(0x49c, -1) && int_is(0x300, 0)
        && float_is(0x2fc, 0xc53bb000)
        ? '3' : 'x');

    setup(3, 0, 0, 0x00000000,
          0x00000000, 0x00000000, 0x00000000,
          0x00000000, 0x00000000, 0x00000000,
          0, 0x3e800000, 0x3f800000);
    object_gear_step((char *)object);
    put(int_is(0x2f8, 4)
        && float_is(0x308, 0x41c00000) && float_is(0x318, 0x40c00000)
        ? '4' : 'x');

    setup(4, 0, 0, 0x00000000,
          0x00000000, 0x41c00000, 0x00000000,
          0x00000000, 0x00000000, 0x00000000,
          0, 0x3f19999a, 0x3f800000);
    object_gear_step((char *)object);
    put(int_is(0x2f8, 0) && float_is(0x318, 0x00000000)
        ? '5' : 'x');

    setup(2, 0, -1000, 0x00000000,
          0x00000000, 0x00000000, 0x00000000,
          0x41200000, 0x41a00000, 0x41f00000,
          0, 0x3e800000, 0x3f000000);
    object_gear_step((char *)object);
    put(int_is(0x2f8, 1) && int_is(0x49c, 750) && int_is(0x300, 0)
        && float_is(0x2fc, 0x44bb8000)
        && float_is(0x304, 0x40c00000) && float_is(0x314, 0x41380000)
        && float_is(0x308, 0x40c00000) && float_is(0x318, 0x41ac0000)
        && float_is(0x30c, 0x00000000) && float_is(0x31c, 0x41f00000)
        ? '6' : 'x');

    setup(5, 77, 0, 0x3f800000,
          0x40000000, 0x40400000, 0x40800000,
          0x40a00000, 0x40c00000, 0x40e00000,
          2, 0x3f000000, 0x3f800000);
    object_gear_step((char *)object);
    put(int_is(0x2f8, 5) && int_is(0x49c, 77) && int_is(0x300, 2)
        && float_is(0x2fc, 0x3f800000) && float_is(0x304, 0x40000000)
        && float_is(0x30c, 0x40800000)
        ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
