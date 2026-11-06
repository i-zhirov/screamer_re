/* The test harness for the rebuilt `engine_shutdown` module.
 *
 * Each of the eight callees prints a marker, so the output shows the call order of the
 * module.
 *
 * The markers go through the data buffer `text`, because the `print` helper of this harness
 * writes garbage for a string literal (the same fault shows in the output of the driver at
 * work/rebuild/out.txt).
 */
#pragma off (check_stack)

void engine_shutdown(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_000062b0[16];
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

void f1(void)
{
    put('1');
}

void f2(void)
{
    put('2');
}

void f3(int value)
{
    put('3');
}

void f4(int value)
{
    put('4');
}

void f5(void)
{
    put('5');
}

void f6(void)
{
    put('6');
}

void f7(void)
{
    put('7');
}

void f8(void)
{
    put('8');
}

int main(void)
{
    engine_shutdown();
    quit();
    return 0;
}
