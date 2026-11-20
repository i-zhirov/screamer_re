/* The test harness for the rebuilt `angle_filter` module.
 *
 * The harness holds the coefficient table `DAT_0010cf08` and the pi/2 constant
 * `DAT_0010d70`.  The table is a stack-local array, because a global pointer
 * initializer holds a relocated immediate that this minimal build does not
 * patch at run time, so a static table address would read the wrong memory.
 *
 * The harness runs nine paths:
 *
 *     0  a small positive offset: 2.0 moves to 1.42073548 with the sine
 *     1  a small negative offset: 1.0 moves to 1.78963220 with the sine
 *     2  a large positive offset: 3.0 moves to 2.0
 *     3  a large negative offset: 0.0 moves to 3.0
 *     4  an offset near pi/2: 2.5 moves to 1.99749494 with the sine
 *     5  a coefficient above one: 1.5 moves to 1.95885110 with the sine
 *     6  the value equals the target: the value stays 1.0
 *     7  a large offset and a coefficient above one: 5.0 moves to 9.0
 *     8  a negative result: -2.0 moves to -1.0
 *     D  the exit marker
 *
 * The float comparisons use a small tolerance, because the intermediate
 * products need not land on the same bit pattern as the stated decimal value.
 */
#pragma off (check_stack)

void angle_filter(float *value, float target, int step);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
float *DAT_0010cf08;
double DAT_0010d70;

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
    table[4] = 1.0f;
    table[5] = 0.5f;
    table[6] = 0.0f;
    table[7] = 1.0f;
    DAT_0010cf08 = table;
    DAT_0010d70 = 1.5707963705062866;

    value = 2.0f;
    angle_filter(&value, 1.0f, 0);
    ok = almost(value, 1.42073548f, 0.0001f);
    put(ok ? '0' : 'x');

    value = 1.0f;
    angle_filter(&value, 2.0f, 1);
    ok = almost(value, 1.78963220f, 0.0001f);
    put(ok ? '1' : 'x');

    value = 3.0f;
    angle_filter(&value, 1.0f, 0);
    ok = almost(value, 2.0f, 0.0001f);
    put(ok ? '2' : 'x');

    value = 0.0f;
    angle_filter(&value, 4.0f, 1);
    ok = almost(value, 3.0f, 0.0001f);
    put(ok ? '3' : 'x');

    value = 2.5f;
    angle_filter(&value, 1.0f, 4);
    ok = almost(value, 1.99749494f, 0.0001f);
    put(ok ? '4' : 'x');

    value = 1.5f;
    angle_filter(&value, 1.0f, 2);
    ok = almost(value, 1.95885110f, 0.0001f);
    put(ok ? '5' : 'x');

    value = 1.0f;
    angle_filter(&value, 1.0f, 0);
    ok = almost(value, 1.0f, 0.0001f);
    put(ok ? '6' : 'x');

    value = 5.0f;
    angle_filter(&value, 1.0f, 2);
    ok = almost(value, 9.0f, 0.0001f);
    put(ok ? '7' : 'x');

    value = -2.0f;
    angle_filter(&value, 0.0f, 0);
    ok = almost(value, -1.0f, 0.0001f);
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
