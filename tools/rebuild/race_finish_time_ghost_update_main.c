/* The test harness for the rebuilt `race_finish_time_ghost_update` module.
 *
 * The harness holds the finish position DAT_000ffa4.  The module stores the
 * constant 2 in the position.
 *
 * The harness runs these paths:
 *
 *     0  the sentinel fill, then one call: the position holds 2
 *     1  a zero fill and call: the call writes 2
 *     2  two calls in a row: the value stays 2
 *     D  the exit marker
 *
 * Every path checks the position.
 */
#pragma off (check_stack)

void race_finish_time_ghost_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_000ffa4;

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

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void reset(int value)
{
    DAT_000ffa4 = value;
}

static int position_ok(void)
{
    return DAT_000ffa4 == 2;
}

int main(void)
{
    reset(0x1111);
    race_finish_time_ghost_update();
    check('0', position_ok());

    reset(0);
    race_finish_time_ghost_update();
    check('1', position_ok());

    reset(9);
    race_finish_time_ghost_update();
    race_finish_time_ghost_update();
    check('2', position_ok());

    put('D');
    quit();
    return 0;
}
