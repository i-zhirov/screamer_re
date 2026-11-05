/* The test harness for the rebuilt `memory_free` module.
 *
 * The harness holds the two DPMI stubs.  The fill stub records its segment
 * pointer.  The call stub records its four arguments and writes a status into
 * the result block.  The harness checks the DPMI request:
 *
 *     0  the call ran with the interrupt 0x31, one block for the input and the
 *        result, the segment set, and the free request and address words
 *     1  the module returned the status at block offset 0x18
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int memory_free(int handle, unsigned int address);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static void *filled_segments;
static int got_interrupt;
static unsigned int *got_regs;
static unsigned int *got_result;
static unsigned int *got_segments;
static int fill_calls;
static int call_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void dpmi_fill_regs(void *segments)
{
    filled_segments = segments;
    fill_calls = fill_calls + 1;
}

int dpmi_call(int vector, void *regs, void *result, void *segments)
{
    got_interrupt = vector;
    got_regs = regs;
    got_result = result;
    got_segments = segments;
    call_calls = call_calls + 1;
    ((unsigned int *)result)[6] = 0x12345678;
    return 0;
}

int main(void)
{
    int status;

    status = memory_free(0, 0x9abcdef0);
    put(fill_calls == 1 && call_calls == 1
        && got_interrupt == 0x31 && got_regs == got_result
        && got_segments == filled_segments
        && got_regs[0] == 0x502 && got_regs[4] == 0x9abc
        && got_regs[5] == 0xdef0 ? '0' : 'x');
    put(status == 0x12345678 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
