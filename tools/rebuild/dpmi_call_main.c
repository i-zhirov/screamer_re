/* The test harness for the rebuilt `dpmi_call` module.
 *
 * The harness holds the DPMI entry stub.  The module passes the interrupt
 * number, the general block, the result block, and the segment set to the
 * stub, and the stub writes a magic value into the result block.  The module
 * reads the value back, so the program checks the returned value and prints
 * the markers:
 *
 *     V  the stub ran and the returned value is the magic value
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

unsigned int dpmi_call(int intnum, void *regs, void *result, void *sregs);
void dpmi_int31_entry(int intnum, void *regs, void *result, void *sregs);
#pragma aux dpmi_int31_entry parm [esi] [edi] [edx] [ebx] modify exact [eax];
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned int regs[8];
static unsigned int sregs[8];
static unsigned int magic = 0x12345678;
static int got_intnum;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void dpmi_int31_entry(int intnum, void *regs_ptr, void *result_ptr, void *sregs_ptr)
{
    got_intnum = intnum;
    *(unsigned int *)result_ptr = magic;
}

int main(void)
{
    unsigned int result;

    result = dpmi_call(0x10, regs, regs, sregs);
    put(got_intnum == 0x10 && result == magic ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
