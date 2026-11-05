/* The test harness for the rebuilt `object_proximity_check` module.
 *
 * The harness holds the object record, the two reference globals, the three
 * constant globals, and the `sqrt` stub.  The stub records its argument and
 * returns a value the harness selects.  The harness runs six paths:
 *
 *     0  a small position inside the range: the delta 30 and 40 truncates to 3
 *        and 4, the stub sees 25, and the value 50 sets the gate
 *     1  the x delta reaches 35000: the gate clears, the value stays, and the
 *        stub does not run
 *     2  the y delta reaches 35000: the gate clears, the value stays, and the
 *        stub does not run
 *     3  the distance equals 35000: the gate clears and the value is 35000
 *     4  the distance is 40000: the gate clears and the value is 40000
 *     5  the deltas -39 and -49 truncate to -3 and -4, the stub sees 25, and
 *        the value 50 sets the gate
 *     D  the exit marker
 *
 * The constants and the expected values are set from their exact bit patterns,
 * so the harness does not depend on a float literal or on the constant
 * segment.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_proximity_check(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];

int DAT_00113ccc;
int DAT_00113cd0;
double proximity_scale;
double proximity_radius;
float proximity_limit;

static double sqrt_result;
static double sqrt_argument;
static double expected_argument;
static double zero_value;
static int sqrt_calls;

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

static int same_double(double a, double b)
{
    union {
        double d;
        unsigned int u[2];
    } x;
    union {
        double d;
        unsigned int u[2];
    } y;

    x.d = a;
    y.d = b;
    return x.u[0] == y.u[0] && x.u[1] == y.u[1];
}

double sqrt(double value)
{
    sqrt_argument = value;
    sqrt_calls = sqrt_calls + 1;
    return sqrt_result;
}

static void reset(int ref_x, int ref_y, int pos_x, int pos_y)
{
    DAT_00113ccc = ref_x;
    DAT_00113cd0 = ref_y;
    *(int *)(object + 0x498) = pos_x;
    *(int *)(object + 0x4a0) = pos_y;
    *(unsigned int *)(object + 0x414) = 0x12345678;
    *(unsigned int *)(object + 0x4e4) = 0x3f800000;
    sqrt_calls = 0;
    sqrt_argument = zero_value;
    sqrt_result = zero_value;
    expected_argument = zero_value;
}

static void set_constants(void)
{
    set_bits_double(&proximity_scale, 0x9999999a, 0x3fb99999);
    set_bits_double(&proximity_radius, 0x00000000, 0x40240000);
    set_bits_float(&proximity_limit, 0x4708b800);
}

static int value_is(unsigned int bits)
{
    return *(unsigned int *)(object + 0x4e4) == bits;
}

static int gate_is(int gate)
{
    return *(int *)(object + 0x414) == gate;
}

int main(void)
{
    set_constants();
    set_bits_double(&zero_value, 0x00000000, 0x00000000);

    reset(30, 40, 0, 0);
    set_bits_double(&sqrt_result, 0x00000000, 0x40140000);
    set_bits_double(&expected_argument, 0x00000000, 0x40390000);
    object_proximity_check(object);
    put(sqrt_calls == 1 && same_double(sqrt_argument, expected_argument)
        && value_is(0x42480000) && gate_is(1) ? '0' : 'x');

    reset(35000, 0, 0, 0);
    object_proximity_check(object);
    put(sqrt_calls == 0 && value_is(0x3f800000) && gate_is(0) ? '1' : 'x');

    reset(0, 35000, 0, 0);
    object_proximity_check(object);
    put(sqrt_calls == 0 && value_is(0x3f800000) && gate_is(0) ? '2' : 'x');

    reset(21000, 28000, 0, 0);
    set_bits_double(&sqrt_result, 0x00000000, 0x40ab5800);
    set_bits_double(&expected_argument, 0x00000000, 0x41675d72);
    object_proximity_check(object);
    put(sqrt_calls == 1 && same_double(sqrt_argument, expected_argument)
        && value_is(0x4708b800) && gate_is(0) ? '3' : 'x');

    reset(24000, 32000, 0, 0);
    set_bits_double(&sqrt_result, 0x00000000, 0x40af4000);
    set_bits_double(&expected_argument, 0x00000000, 0x416e8480);
    object_proximity_check(object);
    put(sqrt_calls == 1 && same_double(sqrt_argument, expected_argument)
        && value_is(0x471c4000) && gate_is(0) ? '4' : 'x');

    reset(-39, -49, 0, 0);
    set_bits_double(&sqrt_result, 0x00000000, 0x40140000);
    set_bits_double(&expected_argument, 0x00000000, 0x40390000);
    object_proximity_check(object);
    put(sqrt_calls == 1 && same_double(sqrt_argument, expected_argument)
        && value_is(0x42480000) && gate_is(1) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
