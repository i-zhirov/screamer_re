/* The test harness for the rebuilt `exit` module.
 *
 * `main` sets the three run-time hook slots and calls the module with the code 0.
 * Each hook prints a marker, so the output shows the chain of the module: the first
 * hook, the second hook, the second hook again from the helper, the third hook,
 * and the terminator.
 *
 * The markers go through the data buffer `text`, because the `print` helper of this
 * harness writes garbage for a string literal (the same fault shows in the output of
 * the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void exit(int status);
__declspec(__aborts) void exit_terminate(int status);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void (*exit_hook_1)(void);
void (*exit_hook_2)(void);
void (*exit_hook_3)(void);

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

static void hook_1(void)
{
    put('A');
}

static void hook_2(void)
{
    put('B');
}

static void hook_3(void)
{
    put('C');
}

void exit_terminate(int status)
{
    put('T');
    quit();
}

int main(void)
{
    exit_hook_1 = hook_1;
    exit_hook_2 = hook_2;
    exit_hook_3 = hook_3;
    exit(0);
    quit();
    return 0;
}
