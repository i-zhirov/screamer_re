/* The test harness for the rebuilt `sound_queue_update` module.
 *
 * The harness holds the sound flag, the frame delta, the timer, the status,
 * the table id, the head delay, and the queue head.  The four callees are
 * stubs that count the calls and record the arguments.  The float values
 * arrive from their bit patterns, because a float literal in this minimal
 * image reads the constant segment, which this harness does not load.
 *
 * The paths:
 *
 *     0  the clear flag returns and touches nothing
 *     1  the status -1 adds the delta to the timer, and the empty queue
 *        returns
 *     2  the timer stays below the head delay: the promote runs and the start
 *        does not
 *     3  the timer passes the delay and the start returns a sound: the status
 *        takes the sound and the queue stays
 *     4  the start returns -1: the pop runs and the timer clears
 *     5  a playing sound with a zero check result: the queue stays
 *     6  a playing sound with a non-zero check result: the pop runs and the
 *        timer clears
 *     7  the timer equals the delay: the start runs
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void sound_queue_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_001d2c8;
float DAT_00118e08;
float DAT_0011907c;
int DAT_00119070;
int DAT_00119074;
float DAT_00119088;
int DAT_00119080;

static char text[8];
static int promote_calls;
static int start_calls;
static int pop_calls;
static int check_calls;
static int start_result;
static int check_result;
static int check_arg_a;
static int check_arg_b;
static int *start_arg;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_f(float *dest, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *dest = value.f;
}

static unsigned int f_bits(float source)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = source;
    return value.u;
}

static void reset(void)
{
    DAT_001d2c8 = 1;
    set_f(&DAT_00118e08, 0x3f800000);
    set_f(&DAT_0011907c, 0x00000000);
    DAT_00119070 = -1;
    DAT_00119074 = 0;
    set_f(&DAT_00119088, 0x00000000);
    DAT_00119080 = -1;
    promote_calls = 0;
    start_calls = 0;
    pop_calls = 0;
    check_calls = 0;
    start_result = 0;
    check_result = 0;
    check_arg_a = 0;
    check_arg_b = 0;
    start_arg = 0;
}

void sound_queue_promote(void)
{
    promote_calls = promote_calls + 1;
}

int func_24a28(int *entry)
{
    start_arg = entry;
    start_calls = start_calls + 1;
    return start_result;
}

void func_24918(void)
{
    pop_calls = pop_calls + 1;
}

int func_1f834(int id, int status)
{
    check_arg_a = id;
    check_arg_b = status;
    check_calls = check_calls + 1;
    return check_result;
}

int main(void)
{
    reset();
    DAT_001d2c8 = 0;
    DAT_00119080 = 10;
    set_f(&DAT_0011907c, 0x40a00000);
    sound_queue_update();
    put(DAT_00119070 == -1 && f_bits(DAT_0011907c) == 0x40a00000
        && promote_calls == 0 && start_calls == 0 && pop_calls == 0
        && check_calls == 0 ? '0' : 'x');

    reset();
    DAT_00119080 = -1;
    set_f(&DAT_0011907c, 0x40a00000);
    set_f(&DAT_00118e08, 0x40000000);
    sound_queue_update();
    put(DAT_00119070 == -1 && f_bits(DAT_0011907c) == 0x40e00000
        && promote_calls == 0 && start_calls == 0 && pop_calls == 0
        ? '1' : 'x');

    reset();
    DAT_00119080 = 10;
    set_f(&DAT_0011907c, 0x00000000);
    set_f(&DAT_00118e08, 0x3f800000);
    set_f(&DAT_00119088, 0x41200000);
    sound_queue_update();
    put(DAT_00119070 == -1 && f_bits(DAT_0011907c) == 0x3f800000
        && promote_calls == 1 && start_calls == 0 && pop_calls == 0
        ? '2' : 'x');

    reset();
    DAT_00119080 = 10;
    set_f(&DAT_0011907c, 0x41180000);
    set_f(&DAT_00118e08, 0x3f800000);
    set_f(&DAT_00119088, 0x41200000);
    start_result = 7;
    sound_queue_update();
    put(DAT_00119070 == 7 && f_bits(DAT_0011907c) == 0x41280000
        && promote_calls == 1 && start_calls == 1 && pop_calls == 0
        && start_arg == &DAT_00119080 ? '3' : 'x');

    reset();
    DAT_00119080 = 10;
    set_f(&DAT_0011907c, 0x41180000);
    set_f(&DAT_00118e08, 0x3f800000);
    set_f(&DAT_00119088, 0x41200000);
    start_result = -1;
    sound_queue_update();
    put(DAT_00119070 == -1 && f_bits(DAT_0011907c) == 0x00000000
        && promote_calls == 1 && start_calls == 1 && pop_calls == 1
        ? '4' : 'x');

    reset();
    DAT_00119070 = 3;
    DAT_00119074 = 42;
    set_f(&DAT_0011907c, 0x40800000);
    check_result = 0;
    sound_queue_update();
    put(DAT_00119070 == 3 && f_bits(DAT_0011907c) == 0x40800000
        && check_calls == 1 && check_arg_a == 42 && check_arg_b == 3
        && pop_calls == 0 ? '5' : 'x');

    reset();
    DAT_00119070 = 3;
    DAT_00119074 = 42;
    set_f(&DAT_0011907c, 0x40800000);
    check_result = 1;
    sound_queue_update();
    put(DAT_00119070 == 3 && f_bits(DAT_0011907c) == 0x00000000
        && check_calls == 1 && pop_calls == 1 ? '6' : 'x');

    reset();
    DAT_00119080 = 10;
    set_f(&DAT_0011907c, 0x00000000);
    set_f(&DAT_00118e08, 0x3f800000);
    set_f(&DAT_00119088, 0x3f800000);
    start_result = 5;
    sound_queue_update();
    put(DAT_00119070 == 5 && f_bits(DAT_0011907c) == 0x3f800000
        && promote_calls == 1 && start_calls == 1 && pop_calls == 0
        ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
