/* The test harness for the rebuilt `bios_video` module.
 *
 * The harness holds the two DPMI helpers.  The fill helper writes the live segment
 * registers into the set, and the call helper reports the interrupt number, so the output shows
 * the chain: the module reaches the fill helper, reaches the call helper, and returns.
 *
 * The markers go through the data buffer `text`, because the `print` helper of this harness
 * writes garbage for a string literal (the same fault shows in the output of the driver at
 * work/rebuild/out.txt).
 */
#pragma off (check_stack)

void bios_video(int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

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

void dpmi_fill_regs(void *sregs, void *regs, void *regs2, void *sregs2)
{
    put('F');
}

void dpmi_call(int intnum)
{
    put('C');
}

int main(void)
{
    bios_video(0x13);
    put('D');
    quit();
    return 0;
}
