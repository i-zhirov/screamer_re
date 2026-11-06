/* The test harness for the rebuilt `main invoker` module.
 *
 * The harness holds the stack words and the four startup stubs.  The main stub
 * records its two arguments, and the exit stub records its call.  The harness
 * checks the argument pair and the exit:
 *
 *     0  the main stub ran with the argument pair
 *     1  the exit stub ran
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void main_invoker(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_00004298;
unsigned int DAT_0000429c;
unsigned int DAT_00027e24;
unsigned int DAT_00027e28;

static char text[8];
static unsigned int got_argc;
static unsigned int got_argv;
static int exit_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void stack_grow_0(void)
{
}

void stack_grow_1(void)
{
}

void game_main(unsigned int argc, unsigned int argv)
{
    got_argc = argc;
    got_argv = argv;
}

void exit_proc(unsigned int code)
{
    exit_calls = exit_calls + 1;
}

int main(void)
{
    DAT_00004298 = 0x1000;
    DAT_0000429c = 0;
    DAT_00027e24 = 1;
    DAT_00027e28 = 0x2000;
    exit_calls = 0;
    main_invoker();
    put(got_argc == 1 && got_argv == 0x2000 ? '0' : 'x');
    put(exit_calls == 1 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
