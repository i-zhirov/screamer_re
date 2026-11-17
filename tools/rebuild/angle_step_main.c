/* The test harness for the rebuilt `angle_step` module.
 *
 * The harness holds the three angle constants pi, 2 pi, and -2 pi.  The
 * constants are globals, because the harness image does not load the constant
 * segment.
 *
 * The harness runs nine paths:
 *
 *     0  the value equals the target: the value stays and the wrap keeps 1.0
 *     1  a small positive difference: 2.0 moves down to 1.75
 *     2  a small negative difference: 1.0 moves up to 1.25
 *     3  a large positive difference: 5.0 takes the short path to 5.25
 *     4  a large negative difference: 1.0 takes the short path to 0.75
 *     5  an overshoot within one step: the value snaps to the target 1.1
 *     6  a negative result: 0.02 wraps through zero to 6.25318527
 *     7  a result of one turn or more: 6.25 wraps down to 0.01681471
 *     8  a result that lands on zero: 0.05 snaps to 0.0
 *     D  the exit marker
 *
 * The float comparisons use a small tolerance, because the intermediate
 * sums need not land on the same bit pattern as the stated decimal value.
 */
#pragma off (check_stack)

void angle_step(float *value, float target, float step);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
float angle_pi;
float angle_turn;
float angle_turn_neg;

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

    angle_pi = 3.14159274f;
    angle_turn = 6.28318548f;
    angle_turn_neg = -6.28318548f;

    value = 1.0f;
    angle_step(&value, 1.0f, 0.5f);
    ok = almost(value, 1.0f, 0.0001f);
    put(ok ? '0' : 'x');

    value = 2.0f;
    angle_step(&value, 1.0f, 0.25f);
    ok = almost(value, 1.75f, 0.0001f);
    put(ok ? '1' : 'x');

    value = 1.0f;
    angle_step(&value, 2.0f, 0.25f);
    ok = almost(value, 1.25f, 0.0001f);
    put(ok ? '2' : 'x');

    value = 5.0f;
    angle_step(&value, 1.0f, 0.25f);
    ok = almost(value, 5.25f, 0.0001f);
    put(ok ? '3' : 'x');

    value = 1.0f;
    angle_step(&value, 5.0f, 0.25f);
    ok = almost(value, 0.75f, 0.0001f);
    put(ok ? '4' : 'x');

    value = 1.0f;
    angle_step(&value, 1.1f, 0.2f);
    ok = almost(value, 1.1f, 0.0001f);
    put(ok ? '5' : 'x');

    value = 0.02f;
    angle_step(&value, -0.2f, 0.05f);
    ok = almost(value, 6.25318527f, 0.0001f);
    put(ok ? '6' : 'x');

    value = 6.25f;
    angle_step(&value, 6.3f, 0.05f);
    ok = almost(value, 0.01681471f, 0.0001f);
    put(ok ? '7' : 'x');

    value = 0.05f;
    angle_step(&value, 0.0f, 0.1f);
    ok = almost(value, 0.0f, 0.0001f);
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
