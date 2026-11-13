/* The test harness for the rebuilt `func_27f80` module.
 *
 * The harness holds the context pointer DAT_00002dec and the result pointer
 * DAT_00002e88.  The function adds 0x18 to the context pointer and stores the
 * sum in the result pointer.
 *
 * The paths:
 *
 *     0  the first context block: the result is the block plus 0x18
 *     1  the second context block: the result is the block plus 0x18
 *     2  the null context: the result is 0x18
 *     3  the context at the second block plus 0x20: the result adds 0x18
 *     4  two calls in order: the second call sets the final result
 *     D  the exit marker
 *
 * Every path checks the result pointer.
 */
#pragma off (check_stack)

void func_27f80(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_00002dec;
void *DAT_00002e88;

static unsigned char context_a[0x40];
static unsigned char context_b[0x40];
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

static int result_is(void *base)
{
    return DAT_00002e88 == (void *)((char *)base + 0x18);
}

int main(void)
{
    DAT_00002dec = context_a;
    func_27f80();
    put(result_is(context_a) ? '0' : 'x');

    DAT_00002dec = context_b;
    func_27f80();
    put(result_is(context_b) ? '1' : 'x');

    DAT_00002dec = 0;
    func_27f80();
    put(DAT_00002e88 == (void *)0x18 ? '2' : 'x');

    DAT_00002dec = context_b + 0x20;
    func_27f80();
    put(result_is(context_b + 0x20) ? '3' : 'x');

    DAT_00002dec = context_b;
    func_27f80();
    DAT_00002dec = context_a;
    func_27f80();
    put(result_is(context_a) ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
