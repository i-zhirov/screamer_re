/* The test harness for the rebuilt `overlay_dispatch` module.
 *
 * The harness holds the option flag, the mode, the state, the stack word, the
 * stack table, and the seven step stubs.  The stubs count their calls.  The
 * harness runs the two overlay paths, the mode path, every state case, and the
 * two stack paths:
 *
 *     0  the option flag is clear: the second overlay path runs
 *     1  the mode is 0x20: no case runs
 *     2  the state 9: the shared state entry runs
 *     3  the state 1: the shared state entry runs
 *     4  the state 2: the case at 0x281f8 runs
 *     5  the state 3: the case at 0x282a2 runs
 *     6  the state 4: the case at 0x283da runs
 *     7  the state 5: the case at 0x2837d runs
 *     8  the state 8: the case at 0x28358 runs
 *     9  the state 0 with the empty stack: the two flags set
 *     A  the state 0 with the stack word 3: the table entry moves and the
 *        notify call runs
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void overlay_dispatch(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_000f0002;
unsigned int DAT_0011fb68;
unsigned int DAT_0011d24c;
unsigned int DAT_0011fac4;
unsigned int DAT_0011fb58;
unsigned int DAT_0011fb60;
unsigned int DAT_00120350[8];
unsigned int DAT_00200988;
unsigned int DAT_0011d234;
unsigned int DAT_00007ed8;

static char text[8];
static int calls[7];
static int notify_calls;
static unsigned int notify_value;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void overlay_option_zero(void) { calls[0] = calls[0] + 1; }
void overlay_case_281f8(void) { calls[1] = calls[1] + 1; }
void overlay_case_282a2(void) { calls[2] = calls[2] + 1; }
void overlay_case_282e6(void) { calls[3] = calls[3] + 1; }
void overlay_case_28358(void) { calls[4] = calls[4] + 1; }
void overlay_case_2837d(void) { calls[5] = calls[5] + 1; }
void overlay_case_283da(void) { calls[6] = calls[6] + 1; }

void overlay_notify(unsigned int value)
{
    notify_calls = notify_calls + 1;
    notify_value = value;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 7; ++index) {
        calls[index] = 0;
    }
    for (index = 0; index < 8; ++index) {
        DAT_00120350[index] = 0;
    }
    notify_calls = 0;
    notify_value = 0xffffffff;
    DAT_000f0002 = 1;
    DAT_0011fb68 = 0;
    DAT_0011d24c = 0x11111111;
    DAT_0011fac4 = 0;
    DAT_0011fb58 = 0;
    DAT_0011fb60 = 0x22222222;
    DAT_00200988 = 0x33333333;
    DAT_0011d234 = 0;
    DAT_00007ed8 = 0;
}

static int only_call(int index)
{
    int i;
    int ok;

    ok = calls[index] == 1;
    for (i = 0; i < 7; ++i) {
        if (i != index && calls[i] != 0) {
            ok = 0;
        }
    }
    return ok;
}

static int no_call(void)
{
    int i;

    for (i = 0; i < 7; ++i) {
        if (calls[i] != 0) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    reset();
    DAT_000f0002 = 0;
    DAT_0011fac4 = 2;
    overlay_dispatch();
    put(only_call(0) && DAT_0011fb68 == 0 ? '0' : 'x');

    reset();
    DAT_0011fb68 = 0x20;
    DAT_0011fac4 = 2;
    overlay_dispatch();
    put(no_call() && DAT_0011fb68 == 0x20 && DAT_0011d24c == 0 ? '1' : 'x');

    reset();
    DAT_0011fac4 = 9;
    overlay_dispatch();
    put(only_call(3) && DAT_0011fb68 == 0x20
        && DAT_0011d24c == 0 && DAT_00200988 == 0 ? '2' : 'x');

    reset();
    DAT_0011fac4 = 1;
    overlay_dispatch();
    put(only_call(3) && DAT_0011fb68 == 0x20 ? '3' : 'x');

    reset();
    DAT_0011fac4 = 2;
    overlay_dispatch();
    put(only_call(1) && DAT_0011fb68 == 0x20 ? '4' : 'x');

    reset();
    DAT_0011fac4 = 3;
    overlay_dispatch();
    put(only_call(2) && DAT_0011fb68 == 0x20 ? '5' : 'x');

    reset();
    DAT_0011fac4 = 4;
    overlay_dispatch();
    put(only_call(6) && DAT_0011fb68 == 0x20 ? '6' : 'x');

    reset();
    DAT_0011fac4 = 5;
    overlay_dispatch();
    put(only_call(5) && DAT_0011fb68 == 0x20 ? '7' : 'x');

    reset();
    DAT_0011fac4 = 8;
    overlay_dispatch();
    put(only_call(4) && DAT_0011fb68 == 0x20 ? '8' : 'x');

    reset();
    DAT_0011fac4 = 0;
    DAT_0011fb58 = 0;
    overlay_dispatch();
    put(no_call() && notify_calls == 0
        && DAT_0011d234 == 1 && DAT_00007ed8 == 1
        && DAT_0011fb68 == 0x20 && DAT_0011fb60 == 0x22222222 ? '9' : 'x');

    reset();
    DAT_0011fac4 = 0;
    DAT_0011fb58 = 3;
    DAT_00120350[2] = 0x55667788;
    overlay_dispatch();
    put(no_call() && notify_calls == 1 && notify_value == 0
        && DAT_0011fb60 == 0x55667788 && DAT_0011fb58 == 2
        && DAT_0011fb68 == 0x20 ? 'A' : 'x');

    put('D');
    quit();
    return 0;
}
