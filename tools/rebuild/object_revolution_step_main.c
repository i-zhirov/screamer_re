/* The test harness for the rebuilt `object_revolution_step` module.
 *
 * The harness holds the object record, the two globals, the gear table, the
 * entry table, and the four callee stubs.  The stubs record the call order and
 * the arguments, and they return values the harness selects.  The harness runs
 * six paths:
 *
 *     0  the fraction stays below 1.0: the axis divides the step, the matrix
 *        builds once, the two positions truncate toward zero, the output takes
 *        the entry at +0xc, and the five state copies and the angle run
 *     1  the fraction crosses 1.0 with gear 0: the gear sets 1, the index stays,
 *        the matrix builds twice, and the distance runs once with 0
 *     2  the fraction crosses 1.0 with gear 1: the gear clears, the index takes
 *        the entry at +0x10, and the output takes the new entry
 *     3  the fraction stays below 1.0 with negative positions: the truncation
 *        toward zero gives -2 and 3
 *     4  the fraction equals 1.0: the equality wraps, so the gear sets 1
 *     5  the matrix differs from the stored position by 3 and 3: the distance
 *        argument is 18
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

void object_revolution_step(char *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];
static unsigned int gear_table[0x100];
static unsigned int entry_table[0x200];

float DAT_00118e04;
unsigned int DAT_0010f8e4;

static float axis_result;
static float matrix_x;
static float matrix_y;
static double length_result;
static float angle_result;

static int axis_calls;
static int matrix_calls;
static int length_calls;
static int angle_calls;
static float axis_fraction;
static double length_argument;
static float angle_x;
static float angle_y;
static float angle_z;
static int matrix_gears[4];
static float matrix_fractions[4];

static void put(char a)
{
    text[0] = a;
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
    } v;

    v.u = bits;
    *p = v.f;
}

static void set_bits_double(double *p, unsigned int lo, unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.u[0] = lo;
    v.u[1] = hi;
    *p = v.d;
}

static int float_bits(float value, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = value;
    return v.u == bits;
}

static int double_bits(double value, unsigned int lo, unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } v;
    union {
        double d;
        unsigned int u[2];
    } w;

    v.d = value;
    w.u[0] = lo;
    w.u[1] = hi;
    return v.u[0] == w.u[0] && v.u[1] == w.u[1];
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

static void set_entry(int index, int offset, unsigned int value)
{
    *(unsigned int *)((char *)entry_table + index * 56 + offset) = value;
}

void object_revolution_matrix(char *object)
{
    int index;

    index = matrix_calls;
    if (index < 4) {
        matrix_gears[index] = *(int *)(object + 0x254);
        matrix_fractions[index] = *(float *)(object + 0x250);
    }
    matrix_calls = matrix_calls + 1;
    *(float *)(object + 0x2a8) = matrix_x;
    *(float *)(object + 0x2ac) = matrix_y;
}

float object_revolution_axis(float *matrix, float fraction)
{
    (void)matrix;
    axis_fraction = fraction;
    axis_calls = axis_calls + 1;
    return axis_result;
}

double object_revolution_length(double value)
{
    length_argument = value;
    length_calls = length_calls + 1;
    return length_result;
}

float object_revolution_angle(float x, float y, float z)
{
    angle_x = x;
    angle_y = y;
    angle_z = z;
    angle_calls = angle_calls + 1;
    return angle_result;
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
        gear_table[(index * 36) / 4] = (unsigned int)entry_table;
    }
    axis_calls = 0;
    matrix_calls = 0;
    length_calls = 0;
    angle_calls = 0;
    axis_fraction = 0.0f;
    length_argument = 0.0;
    angle_x = 0.0f;
    angle_y = 0.0f;
    angle_z = 0.0f;
    set_bits_float(&axis_result, 0x3f800000);
    set_bits_double(&length_result, 0x00000000, 0x00000000);
    set_bits_float(&angle_result, 0x3fa00000);
    set_bits_float(&matrix_x, 0x00000000);
    set_bits_float(&matrix_y, 0x00000000);
}

static void setup(unsigned int speed, unsigned int delta, unsigned int fraction,
                  int gear, int gear_index, int sub_index,
                  unsigned int mx, unsigned int my)
{
    reset();
    set_float(0x418, speed);
    set_bits_float(&DAT_00118e04, delta);
    set_float(0x250, fraction);
    set_int(0x254, gear);
    set_int(0x248, gear_index);
    set_int(0x24c, sub_index);
    set_bits_float(&matrix_x, mx);
    set_bits_float(&matrix_y, my);
}

int main(void)
{
    setup(0x3f800000, 0x3f000000, 0x00000000, 0, 2, 3,
          0x40fccccd, 0xc04ccccd);
    set_entry(3, 0xc, 0x12345678);
    set_float(0x264, 0x41300000);
    set_float(0x268, 0x41400000);
    set_float(0x26c, 0x41500000);
    set_float(0x2b4, 0x41a80000);
    set_float(0x2b8, 0x41b80000);
    set_float(0x27c, 0x41600000);
    object_revolution_step((char *)object);
    put(float_is(0x250, 0x3f000000) && int_is(0x254, 0)
        && int_is(0x24c, 3) && int_is(0x258, 7) && int_is(0x260, -3)
        && int_is(0x25c, 0x12345678)
        && float_is(0x270, 0x41300000) && float_is(0x278, 0x41500000)
        && float_is(0x264, 0x41a80000) && float_is(0x26c, 0x41b80000)
        && float_is(0x280, 0x41600000) && float_is(0x27c, 0x3fa00000)
        && axis_calls == 1 && matrix_calls == 1 && length_calls == 0
        && angle_calls == 1
        && float_bits(angle_x, 0x41a80000)
        && float_bits(angle_y, 0x41400000)
        && float_bits(angle_z, 0x41b80000)
        ? '0' : 'x');

    setup(0x3f800000, 0x3f000000, 0x3f4ccccd, 0, 2, 3,
          0x40000000, 0xbf800000);
    set_int(0x258, 2);
    set_int(0x260, -1);
    set_entry(3, 0xc, 0x12345678);
    object_revolution_step((char *)object);
    put(float_is(0x250, 0x3f000000) && int_is(0x254, 1)
        && int_is(0x24c, 3) && int_is(0x258, 2) && int_is(0x260, -1)
        && int_is(0x25c, 0x12345678)
        && axis_calls == 2 && matrix_calls == 2 && length_calls == 1
        && angle_calls == 1
        && double_bits(length_argument, 0x00000000, 0x00000000)
        && matrix_gears[0] == 1 && float_bits(matrix_fractions[0], 0x00000000)
        && matrix_gears[1] == 1 && float_bits(matrix_fractions[1], 0x3f000000)
        ? '1' : 'x');

    setup(0x3f800000, 0x3f000000, 0x3f4ccccd, 1, 2, 3,
          0x40400000, 0xc0000000);
    set_int(0x258, 3);
    set_int(0x260, -2);
    set_entry(3, 0x10, 5);
    set_entry(5, 0xc, 0x11223344);
    object_revolution_step((char *)object);
    put(float_is(0x250, 0x3f000000) && int_is(0x254, 0)
        && int_is(0x24c, 5) && int_is(0x258, 3) && int_is(0x260, -2)
        && int_is(0x25c, 0x11223344)
        && axis_calls == 2 && matrix_calls == 2 && length_calls == 1
        && angle_calls == 1
        && double_bits(length_argument, 0x00000000, 0x00000000)
        ? '2' : 'x');

    setup(0x00000000, 0x3f000000, 0x3f000000, 0, 2, 3,
          0xc02ccccd, 0x4079999a);
    set_entry(3, 0xc, 0x12345678);
    object_revolution_step((char *)object);
    put(float_is(0x250, 0x3f000000) && int_is(0x254, 0)
        && int_is(0x258, -2) && int_is(0x260, 3)
        && int_is(0x25c, 0x12345678)
        && axis_calls == 1 && matrix_calls == 1 && length_calls == 0
        && angle_calls == 1
        ? '3' : 'x');

    setup(0x3f800000, 0x3f000000, 0x3f000000, 0, 2, 3,
          0x3f800000, 0x3f800000);
    set_int(0x258, 1);
    set_int(0x260, 1);
    set_entry(3, 0xc, 0x12345678);
    object_revolution_step((char *)object);
    put(float_is(0x250, 0x3f000000) && int_is(0x254, 1)
        && int_is(0x24c, 3) && int_is(0x258, 1) && int_is(0x260, 1)
        && int_is(0x25c, 0x12345678)
        && axis_calls == 2 && matrix_calls == 2 && length_calls == 1
        && angle_calls == 1
        && double_bits(length_argument, 0x00000000, 0x00000000)
        ? '4' : 'x');

    setup(0x3f800000, 0x3f000000, 0x3f4ccccd, 0, 2, 3,
          0x40a00000, 0x41400000);
    set_int(0x258, 2);
    set_int(0x260, 9);
    set_entry(3, 0xc, 0x12345678);
    set_bits_double(&length_result, 0x00000000, 0x40140000);
    object_revolution_step((char *)object);
    put(float_is(0x250, 0x3f000000) && int_is(0x254, 1)
        && int_is(0x24c, 3) && int_is(0x258, 5) && int_is(0x260, 12)
        && int_is(0x25c, 0x12345678)
        && axis_calls == 2 && matrix_calls == 2 && length_calls == 1
        && angle_calls == 1
        && double_bits(length_argument, 0x00000000, 0x40320000)
        ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
