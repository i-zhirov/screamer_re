/* The test harness for the rebuilt `direction_angle` module.
 *
 * The harness holds the four angle constants pi / 2, 5 pi / 2, 2 pi, and
 * -2 pi.  The constants are globals, because the harness image does not load
 * the constant segment.
 *
 * The harness supplies the absolute-value function.  The local
 * `direction_angle_fpu.asm` stub supplies the inverse sine from the hardware,
 * because the harness image links no math library and does not load the
 * constant segment.  The absolute-value function sits in this file, because
 * the compiler emits a call for it.
 *
 * The harness runs nine paths:
 *
 *     0  x 0, z 1: 0 plus pi / 2
 *     1  x 0, z -1: 5 pi / 2 minus 0, then minus 2 pi
 *     2  x 0.5, z 1: asin 0.5 plus pi / 2
 *     3  x 0.5, z -1: 5 pi / 2 minus asin 0.5, then minus 2 pi
 *     4  x -0.5, z 1: the absolute value gives the same result as the path 2
 *     5  x -0.5, z -1: the same result as the path 3
 *     6  x 0.25, z 1: asin 0.25 plus pi / 2
 *     7  x 0.25, z -1: 5 pi / 2 minus asin 0.25, then minus 2 pi
 *     8  x 0.125, z -1: the same wrap with a small angle
 *     D  the exit marker
 *
 * The middle argument carries a non-zero value on the path 1, because the
 * function ignores it.  The float compares use a small tolerance, because the
 * intermediate sums need not land on the same bit pattern as the stated
 * decimal value.
 */
#pragma off (check_stack)

float direction_angle(float x, float unused, float z);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
float direction_half_pi;
float direction_five_half_pi;
float direction_turn;
float direction_turn_neg;

double fabs(double value)
{
    return value < 0.0 ? -value : value;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int almost(float value, float expected, float tolerance)
{
    float difference;

    difference = value - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference < tolerance;
}

int main(void)
{
    float value;
    int ok;

    direction_half_pi = 1.57079637f;
    direction_five_half_pi = 7.85398197f;
    direction_turn = 6.28318548f;
    direction_turn_neg = -6.28318548f;

    value = direction_angle(0.0f, 0.0f, 1.0f);
    ok = almost(value, 1.5707963f, 0.001f);
    put(ok ? '0' : 'x');

    value = direction_angle(0.0f, 5.0f, -1.0f);
    ok = almost(value, 1.5707963f, 0.001f);
    put(ok ? '1' : 'x');

    value = direction_angle(0.5f, 0.0f, 1.0f);
    ok = almost(value, 2.0943951f, 0.001f);
    put(ok ? '2' : 'x');

    value = direction_angle(0.5f, 0.0f, -1.0f);
    ok = almost(value, 1.0471976f, 0.001f);
    put(ok ? '3' : 'x');

    value = direction_angle(-0.5f, 0.0f, 1.0f);
    ok = almost(value, 2.0943951f, 0.001f);
    put(ok ? '4' : 'x');

    value = direction_angle(-0.5f, 0.0f, -1.0f);
    ok = almost(value, 1.0471976f, 0.001f);
    put(ok ? '5' : 'x');

    value = direction_angle(0.25f, 0.0f, 1.0f);
    ok = almost(value, 1.8234766f, 0.001f);
    put(ok ? '6' : 'x');

    value = direction_angle(0.25f, 0.0f, -1.0f);
    ok = almost(value, 1.3181161f, 0.001f);
    put(ok ? '7' : 'x');

    value = direction_angle(0.125f, 0.0f, -1.0f);
    ok = almost(value, 1.4454685f, 0.001f);
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
