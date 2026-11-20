/* The test harness for the rebuilt `angle_within` module.
 *
 * The harness holds the two float turn constants 2 pi and -2 pi as globals,
 * because the harness image does not load the constant segment.
 *
 * The paths:
 *
 *     0  equal angles within a positive tolerance: 1
 *     1  a difference of 0.25 within the tolerance 0.5: 1
 *     2  a difference of 1.0 outside a tolerance 0.5: 0
 *     3  a difference of 5.8 near one full turn: 1
 *     4  a difference of 9.0 that needs one turn reduction: 0
 *     5  a negative angle pair with a difference of 1.5: 0
 *     6  a difference of 19.0 that needs three turn reductions: 1
 *     7  a difference of exactly one full turn: 1
 *     8  a difference of 0.05 with a zero tolerance: 0
 *     9  equal angles with a zero tolerance: 1
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

int angle_within(float a, float b, float tolerance);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];

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

int main(void)
{
    angle_turn = 6.28318548f;
    angle_turn_neg = -6.28318548f;

    put(angle_within(1.0f, 1.0f, 0.5f) == 1 ? '0' : 'x');
    put(angle_within(1.0f, 1.25f, 0.5f) == 1 ? '1' : 'x');
    put(angle_within(1.0f, 2.0f, 0.5f) == 0 ? '2' : 'x');
    put(angle_within(0.0f, 5.8f, 0.5f) == 1 ? '3' : 'x');
    put(angle_within(9.0f, 0.0f, 0.5f) == 0 ? '4' : 'x');
    put(angle_within(0.5f, -1.0f, 0.5f) == 0 ? '5' : 'x');
    put(angle_within(19.0f, 0.0f, 1.0f) == 1 ? '6' : 'x');
    put(angle_within(0.0f, angle_turn, 0.1f) == 1 ? '7' : 'x');
    put(angle_within(1.0f, 1.05f, 0.0f) == 0 ? '8' : 'x');
    put(angle_within(1.0f, 1.0f, 0.0f) == 1 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
