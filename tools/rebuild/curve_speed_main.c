/* The test harness for the rebuilt `curve_speed` module.
 *
 * The harness holds the three two-dimensional control points, the float 2.0
 * scale, and the float fraction of each path.  The control points are axis
 * aligned or form a 3-4-5 triangle, so every expected length is an exact small
 * value.
 *
 * The paths:
 *
 *     0  all three points equal at (5, 5) with the fraction 0.5: the zero
 *        guard returns 1.0
 *     1  the fraction 0 with the second point (3, 4): the length is 5
 *     2  the fraction 1 with the third point (3, 4): the length is 5
 *     3  the fraction 0.5 on the x axis with the points 0, (2, 0), and
 *        (4, 0): the length is 2
 *     4  the fraction 0.5 on the y axis with the points 0, (0, 2), and
 *        (0, 4): the length is 2
 *     5  the fraction 0.25 with the second point (1, 0): the length is 0.5,
 *        because the 2.0 scale applies
 *     6  the fraction 0 with the second point (-3, 4): the length is 5
 *     7  the fraction 0.5 with the points 0, (3, 4), and (6, 8): the length
 *        is 5
 *     D  the exit marker
 *
 * The float comparisons use a small bit tolerance, because the square root
 * comes from the FPU.  The float 2.0 arrives from its bit pattern, because the
 * minimal image does not load the constant segment.
 */
#pragma off (check_stack)

float curve_speed(float *curve, float t);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float curve_speed_scale;

static char text[8];
static float curve[9];

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

static void set_point(int index, float x, float y)
{
    curve[index * 3] = x;
    curve[index * 3 + 1] = y;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 9; index++) {
        curve[index] = 0;
    }
}

int main(void)
{
    set_float_bits((char *)&curve_speed_scale, 0, 0x40000000);

    reset();
    set_point(0, 5, 5);
    set_point(1, 5, 5);
    set_point(2, 5, 5);
    put(close_float(curve_speed(curve, 0.5f), 1.0f) ? '0' : 'x');

    reset();
    set_point(1, 3, 4);
    put(close_float(curve_speed(curve, 0.0f), 5.0f) ? '1' : 'x');

    reset();
    set_point(2, 3, 4);
    put(close_float(curve_speed(curve, 1.0f), 5.0f) ? '2' : 'x');

    reset();
    set_point(1, 2, 0);
    set_point(2, 4, 0);
    put(close_float(curve_speed(curve, 0.5f), 2.0f) ? '3' : 'x');

    reset();
    set_point(1, 0, 2);
    set_point(2, 0, 4);
    put(close_float(curve_speed(curve, 0.5f), 2.0f) ? '4' : 'x');

    reset();
    set_point(1, 1, 0);
    put(close_float(curve_speed(curve, 0.25f), 0.5f) ? '5' : 'x');

    reset();
    set_point(1, -3, 4);
    put(close_float(curve_speed(curve, 0.0f), 5.0f) ? '6' : 'x');

    reset();
    set_point(1, 3, 4);
    set_point(2, 6, 8);
    put(close_float(curve_speed(curve, 0.5f), 5.0f) ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
