/* The test harness for the rebuilt `filter_update_int` module.
 *
 * The harness holds the coefficient table `DAT_0010cf08` and one integer cell.
 * The module reads the table entry, moves the cell toward the target, truncates
 * the result, and writes the cell back.
 *
 * The harness runs these paths:
 *
 *     0  an equal value: 100 stays 100 with the coefficient 0.5
 *     1  a higher target: 100 moves to 150 with the coefficient 0.5
 *     2  a lower target: 100 moves to 25 with the coefficient 0.25
 *     3  the full step: 100 moves to 0 with the coefficient 0.0
 *     4  a positive half: 11 truncates to 5 with the coefficient 0.5
 *     5  a negative half: -11 truncates to -5 with the coefficient 0.5
 *     6  the index selects the coefficient: 100 truncates to 12 with 0.125
 *     7  the target term: 0 moves to 60 toward 80 with the coefficient 0.25
 *     D  the exit marker
 *
 * The table is a stack-local array, because a global pointer initializer holds
 * a relocated immediate that this minimal build does not patch at run time, so
 * a static table address would read the wrong memory.
 */
#pragma off (check_stack)

void filter_update_int(int *value, float target, int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
float *DAT_0010cf08;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(int *value, float target, int index, int expected, char marker)
{
    filter_update_int(value, target, index);
    put(*value == expected ? marker : 'x');
}

int main(void)
{
    float table[6];
    int value;

    table[0] = 0.5f;
    table[1] = 0.25f;
    table[2] = 0.125f;
    table[3] = 1.0f;
    table[4] = 0.75f;
    table[5] = 0.0f;
    DAT_0010cf08 = table;

    value = 100;
    check(&value, 100.0f, 0, 100, '0');

    value = 100;
    check(&value, 200.0f, 0, 150, '1');

    value = 100;
    check(&value, 0.0f, 1, 25, '2');

    value = 100;
    check(&value, 0.0f, 5, 0, '3');

    value = 11;
    check(&value, 0.0f, 0, 5, '4');

    value = -11;
    check(&value, 0.0f, 0, -5, '5');

    value = 100;
    check(&value, 0.0f, 2, 12, '6');

    value = 0;
    check(&value, 80.0f, 1, 60, '7');

    put('D');
    quit();
    return 0;
}
