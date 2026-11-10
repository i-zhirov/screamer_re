/* The test harness for the rebuilt `func_1e670` module.
 *
 * The harness holds the three control points at the three-float stride, the
 * two-float output, and the basis scale.  The points are axis aligned, so the
 * expected position is an exact small value.
 *
 * The harness runs seven paths:
 *
 *     0  t = 0 with the second control point at x: the position is (5, 0)
 *     1  t = 0 with the second control point at y: the position is (0, 5)
 *     2  t = 0.5 with the third control point at x: the position is (1.25, 0)
 *     3  t = 1 with the third control point at x: the position is (5, 0)
 *     4  t = 0 with a negative first control point: the position is (-5, 0)
 *     5  t = 0.5 with all three control points: the position is (1, 8)
 *     6  t = 0 with the second control point at positive x: the position is
 *        (5, 0)
 *     D  the exit marker
 *
 * The float comparisons use a small bit tolerance.  The constant arrives from
 * its bit pattern, because the minimal image does not load the constant
 * segment.
 */
#pragma off (check_stack)

void func_1e670(float *out, float *points, float t);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static float points[9];
static float out[2];

double func_1e670_basis_scale;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
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

static int same(float x0, float x1)
{
    return close_float(out[0], x0) && close_float(out[1], x1);
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
    for (index = 0; index < 2; index++) {
        out[index] = 0;
    }
}

int main(void)
{
    set_double_bits((char *)&func_1e670_basis_scale, 0, 0x3fe00000);

    reset();
    set_point(1, 10, 0, 0);
    func_1e670(out, points, 0.0f);
    put(same(5, 0) ? '0' : 'x');

    reset();
    set_point(1, 0, 10, 0);
    func_1e670(out, points, 0.0f);
    put(same(0, 5) ? '1' : 'x');

    reset();
    set_point(2, 10, 0, 0);
    func_1e670(out, points, 0.5f);
    put(same(1.25f, 0) ? '2' : 'x');

    reset();
    set_point(2, 10, 0, 0);
    func_1e670(out, points, 1.0f);
    put(same(5, 0) ? '3' : 'x');

    reset();
    set_point(0, -10, 0, 0);
    func_1e670(out, points, 0.0f);
    put(same(-5, 0) ? '4' : 'x');

    reset();
    set_point(0, 4, 0, 0);
    set_point(1, 0, 8, 0);
    set_point(2, 4, 16, 0);
    func_1e670(out, points, 0.5f);
    put(same(1, 8) ? '5' : 'x');

    reset();
    set_point(1, 10, 0, 0);
    func_1e670(out, points, 0.0f);
    put(same(5, 0) ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
