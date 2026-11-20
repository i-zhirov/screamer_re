/* The test harness for the rebuilt `angle_in_arc` module.
 *
 * The harness holds the two float turn constants 2 pi and -2 pi as globals,
 * because the harness image does not load the constant segment.
 *
 * The paths:
 *
 *     0  a positive direction inside the forward arc: 1
 *     1  a positive direction before the arc: 0
 *     2  a positive direction inside the wrapped arc: 1
 *     3  a positive direction in the gap of the wrapped arc: 0
 *     4  a positive direction that needs two downward turns before the test
 *     5  a negative direction outside the forward arc: 1
 *     6  a negative direction inside the forward arc: 0
 *     7  a negative direction inside the wrapped reverse arc: 1
 *     8  a negative direction that needs one upward turn before the test: 1
 *     9  a zero-length arc at the test angle: 1
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

int angle_in_arc(float start, float end, float angle, float direction);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float arc_turn;
float arc_turn_neg;

static char text[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    arc_turn = 6.28318548f;
    arc_turn_neg = -6.28318548f;
}

int main(void)
{
    reset();
    put(angle_in_arc(1.0f, 3.0f, 2.0f, 1.0f) == 1 ? '0' : 'x');

    reset();
    put(angle_in_arc(2.0f, 3.0f, 1.0f, 1.0f) == 0 ? '1' : 'x');

    reset();
    put(angle_in_arc(5.0f, 1.0f, 6.0f, 1.0f) == 1 ? '2' : 'x');

    reset();
    put(angle_in_arc(5.0f, 1.0f, 3.0f, 1.0f) == 0 ? '3' : 'x');

    reset();
    put(angle_in_arc(6.0f, 0.5f, 13.0f, 1.0f) == 1 ? '4' : 'x');

    reset();
    put(angle_in_arc(1.0f, 3.0f, 4.0f, -1.0f) == 1 ? '5' : 'x');

    reset();
    put(angle_in_arc(1.0f, 3.0f, 2.0f, -1.0f) == 0 ? '6' : 'x');

    reset();
    put(angle_in_arc(5.0f, 1.0f, 3.0f, -1.0f) == 1 ? '7' : 'x');

    reset();
    put(angle_in_arc(0.5f, 1.0f, -1.0f, -1.0f) == 1 ? '8' : 'x');

    reset();
    put(angle_in_arc(2.0f, 2.0f, 2.0f, 1.0f) == 1 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
