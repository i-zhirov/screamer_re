/* The test harness for the rebuilt `object_slide_step` module.
 *
 * The harness holds the object record, the four float constants, the
 * `filter_update` stub, and the `fabs` stub.  The `filter_update` stub records
 * the pointer, the value, and the index of the three calls.  The harness runs
 * four paths:
 *
 *     0  with the guard bit cleared: the slide is zero, and the three calls
 *        hold zero values
 *     1  with a large positive slide: the clamp sets 2.0, and the three calls
 *        hold 1.0, 0.04, and -0.04
 *     2  with a large negative slide: the clamp sets -2.0, and the three calls
 *        hold -1.0, -0.04, and -0.04
 *     3  with a slide inside the range: the three calls hold 0.25, 0.01, and
 *        -0.01
 *     D  the exit marker
 *
 * The constants and the expected values are set from their exact bit patterns.
 * A float literal in this minimal image becomes a read of the constant segment,
 * which this harness does not load, so a literal comparison would compare
 * against zero.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_slide_step(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static float object[0x140];
static int calls;
static float *call_p[3];
static float call_f[3];
static int call_i[3];

float slide_max;
float slide_min;
double slide_half;
double slide_rate;
double slide_epsilon;

static float expected_zero;
static float expected_one;
static float expected_quarter;
static float expected_four;
static float expected_one_hundredth;
static float expected_neg_one;
static float expected_neg_four;
static float expected_neg_one_hundredth;

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

double fabs(double value)
{
    return value < 0.0 ? -value : value;
}

void filter_update(float *p, float f, int i)
{
    if (calls < 3) {
        call_p[calls] = p;
        call_f[calls] = f;
        call_i[calls] = i;
    }
    calls = calls + 1;
}

static void set_object_bits(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *(float *)((char *)object + offset) = v.f;
}

static int close_to(float a, float b)
{
    double d;

    d = (double)a - (double)b;
    return d < slide_epsilon && d > -slide_epsilon;
}

static void reset(void)
{
    unsigned int *bits;
    int index;

    bits = (unsigned int *)object;
    for (index = 0; index < 0x140; index++) {
        bits[index] = 0;
    }
    calls = 0;
}

static void set_constants(void)
{
    set_bits_float(&slide_max, 0x40000000);
    set_bits_float(&slide_min, 0xc0000000);
    set_bits_double(&slide_half, 0x00000000, 0x3fe00000);
    set_bits_double(&slide_rate, 0x47ae147b, 0x3f947ae1);
    set_bits_double(&slide_epsilon, 0xeb1c432d, 0x3f1a36e2);
    set_bits_float(&expected_zero, 0x00000000);
    set_bits_float(&expected_one, 0x3f800000);
    set_bits_float(&expected_quarter, 0x3e800000);
    set_bits_float(&expected_four, 0x3d23d70a);
    set_bits_float(&expected_one_hundredth, 0x3c23d70a);
    set_bits_float(&expected_neg_one, 0xbf800000);
    set_bits_float(&expected_neg_four, 0xbd23d70a);
    set_bits_float(&expected_neg_one_hundredth, 0xbc23d70a);
}

static int calls_ok(void)
{
    return calls == 3
        && call_p[0] == (float *)((char *)object + 0x4c8)
        && call_p[1] == (float *)((char *)object + 0x4b8)
        && call_p[2] == (float *)((char *)object + 0x4b0)
        && call_i[0] == 0x4f && call_i[1] == 0x4c && call_i[2] == 0x4c;
}

int main(void)
{
    set_constants();

    reset();
    *(unsigned char *)((char *)object + 0x25c) = 0;
    object_slide_step(object);
    put(calls_ok()
        && close_to(call_f[0], expected_zero)
        && close_to(call_f[1], expected_zero)
        && close_to(call_f[2], expected_zero) ? '0' : 'x');

    reset();
    *(unsigned char *)((char *)object + 0x25c) = 1;
    set_object_bits(0x418, 0x3f800000);
    set_object_bits(0x2f4, 0x40400000);
    set_object_bits(0x27c, 0x40a00000);
    set_object_bits(0x280, 0x3f800000);
    object_slide_step(object);
    put(calls_ok()
        && close_to(call_f[0], expected_one)
        && close_to(call_f[1], expected_four)
        && close_to(call_f[2], expected_neg_four) ? '1' : 'x');

    reset();
    *(unsigned char *)((char *)object + 0x25c) = 1;
    set_object_bits(0x418, 0x3f800000);
    set_object_bits(0x2f4, 0x40400000);
    set_object_bits(0x27c, 0xbf800000);
    set_object_bits(0x280, 0x00000000);
    object_slide_step(object);
    put(calls_ok()
        && close_to(call_f[0], expected_neg_one)
        && close_to(call_f[1], expected_neg_four)
        && close_to(call_f[2], expected_neg_four) ? '2' : 'x');

    reset();
    *(unsigned char *)((char *)object + 0x25c) = 1;
    set_object_bits(0x418, 0x3f000000);
    set_object_bits(0x2f4, 0x3f800000);
    set_object_bits(0x27c, 0x40000000);
    set_object_bits(0x280, 0x00000000);
    object_slide_step(object);
    put(calls_ok()
        && close_to(call_f[0], expected_quarter)
        && close_to(call_f[1], expected_one_hundredth)
        && close_to(call_f[2], expected_neg_one_hundredth) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
