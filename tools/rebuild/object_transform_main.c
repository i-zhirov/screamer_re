/* The test harness for the rebuilt `object_transform` module.
 *
 * The harness holds the object record, the `object_integrate` stub, and the
 * `object_state_save` stub.  The integrate stub records its argument and adds
 * 5 to `+0x454` and 7 to `+0x45c`, so the later sums see the integrated
 * offsets.  The save stub records its argument and the call order.  The harness
 * runs three paths:
 *
 *     0  flag `+0x2f8` zero: the module writes both position sums, copies the
 *        height, writes the three float sums, the height minus two, and the
 *        vertical sum
 *     1  flag `+0x2f8` nonzero: the height copy at `+0x49c` stays, and the
 *        other stores run
 *     2  negative values: the integer sums, the float sums, and the vertical
 *        sum keep the sign
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

void object_transform(char *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];

static int integrate_calls;
static int save_calls;
static int save_after_integrate;
static char *integrate_arg;
static char *save_arg;

void object_integrate(char *object)
{
    integrate_calls = integrate_calls + 1;
    integrate_arg = object;
    *(int *)(object + 0x454) = *(int *)(object + 0x454) + 5;
    *(int *)(object + 0x45c) = *(int *)(object + 0x45c) + 7;
}

void object_state_save(char *object)
{
    save_calls = save_calls + 1;
    save_arg = object;
    if (integrate_calls == 1) {
        save_after_integrate = 1;
    }
}

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

static int same_float(float a, float b)
{
    union {
        float f;
        unsigned int u;
    } x;
    union {
        float f;
        unsigned int u;
    } y;

    x.f = a;
    y.f = b;
    return x.u == y.u;
}

static int calls_ok(void)
{
    return integrate_calls == 1
        && save_calls == 1
        && save_after_integrate
        && integrate_arg == (char *)object
        && save_arg == (char *)object;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x500; index++) {
        object[index] = 0;
    }
    integrate_calls = 0;
    save_calls = 0;
    save_after_integrate = 0;
    integrate_arg = 0;
    save_arg = 0;
}

static void set_floats(unsigned int a, unsigned int vx, unsigned int b,
                       unsigned int vy, unsigned int c, unsigned int vz,
                       unsigned int vz2)
{
    set_bits_float((float *)(object + 0x23c), a);
    set_bits_float((float *)(object + 0x314), vx);
    set_bits_float((float *)(object + 0x27c), b);
    set_bits_float((float *)(object + 0x318), vy);
    set_bits_float((float *)(object + 0x244), c);
    set_bits_float((float *)(object + 0x31c), vz);
    set_bits_float((float *)(object + 0x4c8), vz2);
}

static int float_is(int offset, unsigned int bits)
{
    float expected;

    set_bits_float(&expected, bits);
    return same_float(*(float *)(object + offset), expected);
}

int main(void)
{
    reset();
    *(unsigned int *)(object + 0x258) = 100;
    *(unsigned int *)(object + 0x454) = 10;
    *(unsigned int *)(object + 0x260) = 200;
    *(unsigned int *)(object + 0x45c) = 20;
    *(unsigned int *)(object + 0x2f8) = 0;
    *(unsigned int *)(object + 0x238) = 42;
    set_floats(0x3fc00000, 0x40100000, 0xbf800000,
               0x3f000000, 0x40000000, 0x40400000, 0x3e800000);
    object_transform((char *)object);
    put(calls_ok()
        && *(unsigned int *)(object + 0x498) == 115
        && *(unsigned int *)(object + 0x4a0) == 227
        && *(unsigned int *)(object + 0x49c) == 42
        && *(unsigned int *)(object + 0x2c4) == 40
        && float_is(0x4a4, 0x40700000)
        && float_is(0x4a8, 0xbf000000)
        && float_is(0x4ac, 0x40a00000)
        && float_is(0x4bc, 0xbe800000) ? '0' : 'x');

    reset();
    *(unsigned int *)(object + 0x258) = 100;
    *(unsigned int *)(object + 0x454) = 10;
    *(unsigned int *)(object + 0x260) = 200;
    *(unsigned int *)(object + 0x45c) = 20;
    *(unsigned int *)(object + 0x2f8) = 1;
    *(unsigned int *)(object + 0x238) = 42;
    *(unsigned int *)(object + 0x49c) = 0x12345678;
    set_floats(0x3fc00000, 0x40100000, 0xbf800000,
               0x3f000000, 0x40000000, 0x40400000, 0x3e800000);
    object_transform((char *)object);
    put(calls_ok()
        && *(unsigned int *)(object + 0x498) == 115
        && *(unsigned int *)(object + 0x4a0) == 227
        && *(unsigned int *)(object + 0x49c) == 0x12345678
        && *(unsigned int *)(object + 0x2c4) == 40
        && float_is(0x4a4, 0x40700000)
        && float_is(0x4bc, 0xbe800000) ? '1' : 'x');

    reset();
    *(unsigned int *)(object + 0x258) = 0xffffff9c;
    *(unsigned int *)(object + 0x454) = 0xfffffff6;
    *(unsigned int *)(object + 0x260) = 50;
    *(unsigned int *)(object + 0x45c) = 0xffffffc4;
    *(unsigned int *)(object + 0x2f8) = 0;
    *(unsigned int *)(object + 0x238) = 0xfffffff9;
    set_floats(0x3f800000, 0xbf800000, 0xc0000000,
               0xbf000000, 0xc0400000, 0xc0800000, 0xbf800000);
    object_transform((char *)object);
    put(calls_ok()
        && *(unsigned int *)(object + 0x498) == 0xffffff97
        && *(unsigned int *)(object + 0x4a0) == 0xfffffffd
        && *(unsigned int *)(object + 0x49c) == 0xfffffff9
        && *(unsigned int *)(object + 0x2c4) == 0xfffffff7
        && float_is(0x4a4, 0x00000000)
        && float_is(0x4a8, 0xc0200000)
        && float_is(0x4ac, 0xc0e00000)
        && float_is(0x4bc, 0xc0600000) ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
