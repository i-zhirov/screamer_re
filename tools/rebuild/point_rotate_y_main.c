/* The test harness for the rebuilt `point_rotate_y` module.
 *
 * The harness holds one input point and one output point, both with three
 * integers.  The function turns the (x, z) pair by the angle and leaves the y
 * value.  The harness computes the expected x and z with the same hardware
 * `cos` and `sin` stubs and the same truncation helper, so the comparison
 * follows the original arithmetic.  It also checks that the function leaves the
 * input point and the middle output value unchanged.
 *
 * The paths:
 *
 *     0  the angle 0 keeps all three values and does not change the input
 *     1  the angle 0.5 turns the point (100, 200, 300)
 *     2  the angle 1.0 turns the point (-100, -200, 300)
 *     3  the angle -0.75 turns the point (10, -20, 30)
 *     4  the angle 1.5 turns the point (0x1000, -0x800, 0x400)
 *     5  the angle 0.5 keeps the distinct y value -999 and turns (40, 50)
 *     D  the exit marker
 *
 * The float angles arrive from their bit patterns, because the minimal image
 * does not load the constant segment.
 */
#pragma off (check_stack)

void point_rotate_y(int *out, int *in, float angle);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

double cos(double value);
double sin(double value);

static char text[8];
static int in_point[3];
static int out_point[3];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static float float_bits(unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    return value.f;
}

static int verify(int x, int y, int z, float angle)
{
    float cosine;
    float sine;
    int expected_x;
    int expected_z;
    int ok;

    in_point[0] = x;
    in_point[1] = y;
    in_point[2] = z;
    out_point[0] = 0x55555555;
    out_point[1] = 0x44444444;
    out_point[2] = 0x55555555;

    point_rotate_y(out_point, in_point, angle);

    cosine = (float)cos((double)angle);
    sine = (float)sin((double)angle);
    expected_x = (int)(x * cosine + z * sine);
    expected_z = (int)(z * cosine - x * sine);

    ok = out_point[0] == expected_x
        && out_point[1] == 0x44444444
        && out_point[2] == expected_z;
    ok = ok
        && in_point[0] == x
        && in_point[1] == y
        && in_point[2] == z;
    return ok;
}

int main(void)
{
    put(verify(7, 11, -13, 0.0f) ? '0' : 'x');
    put(verify(100, 200, 300, float_bits(0x3f000000)) ? '1' : 'x');
    put(verify(-100, -200, 300, float_bits(0x3f800000)) ? '2' : 'x');
    put(verify(10, -20, 30, float_bits(0xbf400000)) ? '3' : 'x');
    put(verify(0x1000, -0x800, 0x400, float_bits(0x3fc00000)) ? '4' : 'x');
    put(verify(40, -999, 50, float_bits(0x3f000000)) ? '5' : 'x');
    put('D');
    quit();
    return 0;
}
