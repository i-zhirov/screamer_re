/* The test harness for the rebuilt `dpmi_int31_entry` module.
 *
 * The harness holds the interrupt number, the register block, and the segment
 * set, and the dispatcher stub.  The module is the run-time entry that the
 * `dpmi_call` module calls, so the harness calls it through the same register
 * convention.  The dispatcher stub records the interrupt number and the
 * blocks, sets the result registers to known values, and returns, so the
 * module writes the values into the stub result block.
 *
 *     V  the result block holds the stub register values
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void dpmi_int31_entry(int intnum, void *regs, void *result, void *sregs);
#pragma aux dpmi_int31_entry parm [esi] [edi] [edx] [ebx] modify exact [eax ebx ecx edx esi edi];
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

extern unsigned int result_block[8];
unsigned int result_block[8];

static char text[8];
static unsigned int regs[8];
static unsigned int sregs[8];

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
    dpmi_int31_entry(0x31, regs, result_block, sregs);
    put(result_block[0] == 0x11111111 && result_block[1] == 0x22222222
        && result_block[2] == 0x33333333 && result_block[3] == 0x44444444
        && result_block[4] == 0x55555555 ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
