/* The test harness for the rebuilt `phase_delta` module.
 *
 * The harness calls the module with a saved value and a current value and
 * checks the returned delta.  The six paths cover the forward delta, the
 * complement form with a current value below the saved value, the equal
 * values, the full-range delta, the complement across the sign bit, and the
 * wrap from the top value to zero.
 *
 *    0  saved 10 and current 25 return 15
 *    1  saved 25 and current 10 return 14
 *    2  equal values return 0
 *    3  saved 0 and current 0xffffffff return 0xffffffff
 *    4  saved 0x80000000 and current 0x7fffffff return 0
 *    5  saved 0xffffffff and current 0 return 0xfffffffe
 *    D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

unsigned int phase_delta(unsigned int saved, unsigned int current);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int main(void)
{
    put(phase_delta(10, 25) == 15 ? '0' : 'x');
    put(phase_delta(25, 10) == 14 ? '1' : 'x');
    put(phase_delta(1234, 1234) == 0 ? '2' : 'x');
    put(phase_delta(0, 0xffffffffu) == 0xffffffffu ? '3' : 'x');
    put(phase_delta(0x80000000u, 0x7fffffffu) == 0 ? '4' : 'x');
    put(phase_delta(0xffffffffu, 0) == 0xfffffffeu ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
