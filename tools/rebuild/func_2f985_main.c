/* The test harness for the rebuilt `func_2f985` module.
 *
 * The harness holds one pool record, sets the field at `+0x3c`, calls the
 * module, and checks the returned value.  The five paths cover zero, one,
 * 0x1000, 0x12345678, and 0xffffffff.  The other bytes of the record stay
 * clear, so a read of the wrong offset fails the check.
 *
 *    0  the field 0 returns
 *    1  the field 1 returns
 *    2  the field 0x1000 returns
 *    3  the field 0x12345678 returns
 *    4  the field 0xffffffff returns
 *    D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

unsigned int func_2f985(void *pool);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char pool[0x40];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear(void)
{
    int index;

    for (index = 0; index < 0x40; ++index) {
        pool[index] = 0;
    }
}

int main(void)
{
    unsigned int result;

    clear();
    result = func_2f985(pool);
    put(result == 0 ? '0' : 'x');

    clear();
    *(unsigned int *)(pool + 0x3c) = 1;
    result = func_2f985(pool);
    put(result == 1 ? '1' : 'x');

    clear();
    *(unsigned int *)(pool + 0x3c) = 0x1000;
    result = func_2f985(pool);
    put(result == 0x1000 ? '2' : 'x');

    clear();
    *(unsigned int *)(pool + 0x3c) = 0x12345678;
    result = func_2f985(pool);
    put(result == 0x12345678 ? '3' : 'x');

    clear();
    *(unsigned int *)(pool + 0x3c) = 0xffffffff;
    result = func_2f985(pool);
    put(result == 0xffffffff ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
