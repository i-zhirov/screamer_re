/* The test harness for the rebuilt `spline_interpolate` module.
 *
 * The harness holds the four keyframe points, the pointer frame, the output
 * triple, and the chord scale.  The points are collinear and equally spaced
 * where the expected result is easy to state, and three-dimensional where the
 * three axes must agree.
 *
 * The harness runs nine paths:
 *
 *     0  equal spacing and the fraction 0.5: the truncation of both tangents
 *        gives the position 14 on the x axis
 *     1  the fraction 0 returns the first point
 *     2  the fraction 1 returns the second point
 *     3  the previous point equals the second point: |BD| is zero, the first
 *        tangent stays at the first point, and the result is 13
 *     4  the third point equals the first point: |AC| is zero, the second
 *        tangent stays at the second point, and the result is 16
 *     5  the three-dimensional points and the fraction 0.5 give 7, 9, and 13
 *     6  the three-dimensional fraction 0 returns the first point
 *     7  the three-dimensional fraction 1 returns the second point
 *     8  negative coordinates and the fraction 0.5 give -74: both tangents
 *        truncate toward zero
 *     D  the exit marker
 *
 * The chord scale is the double 3.0, because the minimal image does not load
 * the constant segment.
 */
#pragma off (check_stack)

void spline_interpolate(int **frames, int *out, float t);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static int points[4][3];
static int *frames[4];
static int out[3];

double spline_chord_scale;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_double(double *p, unsigned int low, unsigned int high)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = low;
    value.u[1] = high;
    *p = value.d;
}

static void set_point(int index, int x, int y, int z)
{
    points[index][0] = x;
    points[index][1] = y;
    points[index][2] = z;
}

static void set_case(int dx, int dy, int dz, int ax, int ay, int az,
    int bx, int by, int bz, int cx, int cy, int cz)
{
    set_point(0, dx, dy, dz);
    set_point(1, ax, ay, az);
    set_point(2, bx, by, bz);
    set_point(3, cx, cy, cz);
    frames[0] = points[0];
    frames[1] = points[1];
    frames[2] = points[2];
    frames[3] = points[3];
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
}

static int equal(int x, int y, int z)
{
    return out[0] == x && out[1] == y && out[2] == z;
}

int main(void)
{
    set_bits_double(&spline_chord_scale, 0, 0x40080000);

    set_case(0, 0, 0, 10, 0, 0, 20, 0, 0, 30, 0, 0);
    spline_interpolate(frames, out, 0.5f);
    put(equal(14, 0, 0) ? '0' : 'x');

    set_case(0, 0, 0, 10, 0, 0, 20, 0, 0, 30, 0, 0);
    spline_interpolate(frames, out, 0.0f);
    put(equal(10, 0, 0) ? '1' : 'x');

    set_case(0, 0, 0, 10, 0, 0, 20, 0, 0, 30, 0, 0);
    spline_interpolate(frames, out, 1.0f);
    put(equal(20, 0, 0) ? '2' : 'x');

    set_case(20, 0, 0, 10, 0, 0, 20, 0, 0, 30, 0, 0);
    spline_interpolate(frames, out, 0.5f);
    put(equal(13, 0, 0) ? '3' : 'x');

    set_case(0, 0, 0, 10, 0, 0, 20, 0, 0, 10, 0, 0);
    spline_interpolate(frames, out, 0.5f);
    put(equal(16, 0, 0) ? '4' : 'x');

    set_case(1, 2, 3, 4, 6, 8, 10, 14, 20, 20, 30, 40);
    spline_interpolate(frames, out, 0.5f);
    put(equal(7, 9, 13) ? '5' : 'x');

    set_case(1, 2, 3, 4, 6, 8, 10, 14, 20, 20, 30, 40);
    spline_interpolate(frames, out, 0.0f);
    put(equal(4, 6, 8) ? '6' : 'x');

    set_case(1, 2, 3, 4, 6, 8, 10, 14, 20, 20, 30, 40);
    spline_interpolate(frames, out, 1.0f);
    put(equal(10, 14, 20) ? '7' : 'x');

    set_case(-200, 0, 0, -100, 0, 0, -50, 0, 0, 0, 0, 0);
    spline_interpolate(frames, out, 0.5f);
    put(equal(-74, 0, 0) ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
