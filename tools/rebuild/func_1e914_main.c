/* The test harness for the rebuilt `func_1e914` module.
 *
 * The harness holds the three control points, the six-float output, and the
 * two basis constants.  The points are axis aligned, so the expected position
 * and the expected tangent are exact small values.
 *
 * The harness runs seven paths:
 *
 *     0  t = 0 with the first control point at x: the position is (5, 0, 0)
 *        and the tangent is (1, 0, 0)
 *     1  t = 0 with the first control point at y: the position is (0, 5, 0)
 *        and the tangent is (0, 1, 0)
 *     2  t = 0 with the first control point at z: the position is (0, 0, 5)
 *        and the tangent is (0, 0, 1)
 *     3  t = 0.5 with the third control point at x: the position is
 *        (1.25, 0, 0) and the tangent is (1, 0, 0)
 *     4  t = 1 with the third control point at x: the position is (5, 0, 0)
 *        and the tangent is (1, 0, 0)
 *     5  t = 0 with a negative second control point: the position is
 *        (-5, 0, 0) and the tangent is (1, 0, 0)
 *     6  t = 0 with a positive second control point: the position is
 *        (5, 0, 0) and the tangent is (-1, 0, 0)
 *     D  the exit marker
 *
 * The float comparisons use a small bit tolerance, because the square root
 * comes from the FPU.  The constants arrive from their bit patterns, because
 * the minimal image does not load the constant segment.
 */
#pragma off (check_stack)

void func_1e914(float *out, float *points, float t);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static float points[9];
static float out[6];

double func_1e914_basis_scale;
float func_1e914_sample_step;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_float_bits(char *base, int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(base + offset) = value.f;
}

static void set_double_bits(char *base, unsigned int low, unsigned int high)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = low;
    value.u[1] = high;
    *(double *)base = value.d;
}

static int close_float(float actual, float expected)
{
    union {
        float f;
        unsigned int u;
    } a;
    union {
        float f;
        unsigned int u;
    } b;
    unsigned int delta;

    a.f = actual;
    b.f = expected;
    delta = a.u > b.u ? a.u - b.u : b.u - a.u;
    return delta <= 16;
}

static int same(float x0, float x1, float x2, float x3, float x4, float x5)
{
    return close_float(out[0], x0)
        && close_float(out[1], x1)
        && close_float(out[2], x2)
        && close_float(out[3], x3)
        && close_float(out[4], x4)
        && close_float(out[5], x5);
}

static void set_point(int index, float x, float y, float z)
{
    points[index * 3] = x;
    points[index * 3 + 1] = y;
    points[index * 3 + 2] = z;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 9; index++) {
        points[index] = 0;
    }
    for (index = 0; index < 6; index++) {
        out[index] = 0;
    }
}

int main(void)
{
    set_double_bits((char *)&func_1e914_basis_scale, 0, 0x3fe00000);
    set_float_bits((char *)&func_1e914_sample_step, 0, 0x3b03126f);

    reset();
    set_point(1, 10, 0, 0);
    func_1e914(out, points, 0.0f);
    put(same(5, 0, 0, 1, 0, 0) ? '0' : 'x');

    reset();
    set_point(1, 0, 10, 0);
    func_1e914(out, points, 0.0f);
    put(same(0, 5, 0, 0, 1, 0) ? '1' : 'x');

    reset();
    set_point(1, 0, 0, 10);
    func_1e914(out, points, 0.0f);
    put(same(0, 0, 5, 0, 0, 1) ? '2' : 'x');

    reset();
    set_point(2, 10, 0, 0);
    func_1e914(out, points, 0.5f);
    put(same(1.25f, 0, 0, 1, 0, 0) ? '3' : 'x');

    reset();
    set_point(2, 10, 0, 0);
    func_1e914(out, points, 1.0f);
    put(same(5, 0, 0, 1, 0, 0) ? '4' : 'x');

    reset();
    set_point(0, -10, 0, 0);
    func_1e914(out, points, 0.0f);
    put(same(-5, 0, 0, 1, 0, 0) ? '5' : 'x');

    reset();
    set_point(0, 10, 0, 0);
    func_1e914(out, points, 0.0f);
    put(same(5, 0, 0, -1, 0, 0) ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
