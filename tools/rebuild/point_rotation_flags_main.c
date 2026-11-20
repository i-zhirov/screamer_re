/* The test harness for the rebuilt `point_rotation_flags` module.
 *
 * The harness holds the two points, the configured rotation result, and the
 * `point_rotate_y` stub.  The stub records the delta x and z values and the
 * angle bits, then writes the configured turned x and z values.  Every path
 * checks the return flags, the call count, the delta values, and the angle bits.
 *
 * The paths:
 *
 *     0  a positive turned x and a positive turned z return 3
 *     1  a positive turned x and a negative turned z return 1
 *     2  a negative turned x and a positive turned z return 2
 *     3  a negative turned x and a negative turned z return 0
 *     4  a zero turned x and a positive turned z return 2
 *     5  a positive turned x and a zero turned z return 1
 *     6  a negative delta with a negative turned x and a positive turned z
 *     7  a large delta with a positive turned x and a negative turned z
 *     D  the exit marker
 *
 * The float angles arrive from their bit patterns, because the minimal image
 * does not load the constant segment.
 */
#pragma off (check_stack)

int point_rotation_flags(int *first, int *second, float angle);
void point_rotate_y(int *out, int *in, float angle);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static int first[3];
static int second[3];
static int rotate_in_x;
static int rotate_in_z;
static unsigned int rotate_angle_bits;
static int rotate_calls;
static int rotate_out_x;
static int rotate_out_z;

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

void point_rotate_y(int *out, int *in, float angle)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = angle;
    rotate_in_x = in[0];
    rotate_in_z = in[2];
    rotate_angle_bits = value.u;
    rotate_calls = rotate_calls + 1;
    out[0] = rotate_out_x;
    out[2] = rotate_out_z;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 3; index++) {
        first[index] = 0;
        second[index] = 0;
    }
    rotate_in_x = 0;
    rotate_in_z = 0;
    rotate_angle_bits = 0;
    rotate_calls = 0;
    rotate_out_x = 0;
    rotate_out_z = 0;
}

static int verify(int first_x, int first_z, int second_x, int second_z,
                  unsigned int angle_bits, int out_x, int out_z,
                  int expected_flags)
{
    int flags;

    reset();
    first[0] = first_x;
    first[2] = first_z;
    second[0] = second_x;
    second[2] = second_z;
    rotate_out_x = out_x;
    rotate_out_z = out_z;
    flags = point_rotation_flags(first, second, float_bits(angle_bits));
    return flags == expected_flags
        && rotate_calls == 1
        && rotate_in_x == second_x - first_x
        && rotate_in_z == second_z - first_z
        && rotate_angle_bits == angle_bits;
}

int main(void)
{
    put(verify(10, 30, 25, 80, 0x3f000000, 1, 1, 3) ? '0' : 'x');
    put(verify(10, 30, 25, 80, 0x3f800000, 1, -1, 1) ? '1' : 'x');
    put(verify(10, 30, 25, 80, 0xbf400000, -1, 1, 2) ? '2' : 'x');
    put(verify(10, 30, 25, 80, 0x3fc00000, -1, -1, 0) ? '3' : 'x');
    put(verify(100, 200, 40, 50, 0x3f000000, 0, 5, 2) ? '4' : 'x');
    put(verify(100, 200, 40, 50, 0x3f000000, 5, 0, 1) ? '5' : 'x');
    put(verify(100, 200, 40, 50, 0xbf000000, -7, 3, 2) ? '6' : 'x');
    put(verify(0x1000, -0x800, -0x40, 0x200, 0x3e800000, 9, -4, 1) ? '7' : 'x');
    put('D');
    quit();
    return 0;
}
