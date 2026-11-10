/* The test harness for the rebuilt `angle_approach` module.
 *
 * The harness holds the coefficient table `DAT_0010cf08` and the three angle
 * constants pi, 2 pi, and -2 pi.  The table is a stack-local array, because a
 * global pointer initializer holds a relocated immediate that this minimal
 * build does not patch at run time, so a static table address would read the
 * wrong memory.
 *
 * The harness runs nine paths:
 *
 *     0  the value equals the target: the value stays and the wrap keeps 1.0
 *     1  a small positive difference: 2.0 moves to 1.5 with the coefficient 0.5
 *     2  a small negative difference: 1.0 moves to 1.75 with the coefficient 0.25
 *     3  a large positive difference: 5.0 takes the short path to 6.14159274
 *     4  a large positive difference: 6.0 takes the short path to 0.35840726
 *     5  a negative result: 6.2 wraps through zero to 5.65044355
 *     6  a result of one turn or more: 0.1 wraps down to 0.64955646
 *     7  an equal value of 7.0: the wrap subtracts one turn to 0.71681452
 *     8  a result that lands on zero: 1.0 moves to 0.0 with the coefficient 2.0
 *     D  the exit marker
 *
 * The float comparisons use a small tolerance, because the intermediate
 * products need not land on the same bit pattern as the stated decimal value.
 */
#pragma off (check_stack)

void angle_approach(float *value, float target, int step);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
float *DAT_0010cf08;
float angle_pi;
float angle_turn;
float angle_turn_neg;

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
    float table[8];
    float value;
    int ok;

    table[0] = 0.5f;
    table[1] = 0.25f;
    table[2] = 2.0f;
    table[3] = 0.5f;
    table[4] = 4.0f;
    table[5] = 0.5f;
    table[6] = 0.0f;
    table[7] = 1.0f;
    DAT_0010cf08 = table;
    angle_pi = 3.14159274f;
    angle_turn = 6.28318548f;
    angle_turn_neg = -6.28318548f;

    value = 1.0f;
    angle_approach(&value, 1.0f, 0);
    ok = almost(value, 1.0f, 0.0001f);
    put(ok ? '0' : 'x');

    value = 2.0f;
    angle_approach(&value, 1.0f, 0);
    ok = almost(value, 1.5f, 0.0001f);
    put(ok ? '1' : 'x');

    value = 1.0f;
    angle_approach(&value, 2.0f, 1);
    ok = almost(value, 1.75f, 0.0001f);
    put(ok ? '2' : 'x');

    value = 5.0f;
    angle_approach(&value, 1.0f, 0);
    ok = almost(value, 6.14159274f, 0.0001f);
    put(ok ? '3' : 'x');

    value = 6.0f;
    angle_approach(&value, 1.0f, 0);
    ok = almost(value, 0.35840726f, 0.0001f);
    put(ok ? '4' : 'x');

    value = 6.2f;
    angle_approach(&value, 0.1f, 4);
    ok = almost(value, 5.65044355f, 0.0001f);
    put(ok ? '5' : 'x');

    value = 0.1f;
    angle_approach(&value, 6.2f, 4);
    ok = almost(value, 0.64955646f, 0.0001f);
    put(ok ? '6' : 'x');

    value = 7.0f;
    angle_approach(&value, 7.0f, 6);
    ok = almost(value, 0.71681452f, 0.0001f);
    put(ok ? '7' : 'x');

    value = 1.0f;
    angle_approach(&value, 2.0f, 2);
    ok = almost(value, 0.0f, 0.0001f);
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
