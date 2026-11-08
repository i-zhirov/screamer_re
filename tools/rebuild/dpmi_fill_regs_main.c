/* The test harness for the rebuilt `dpmi_fill_regs` module.
 *
 * The harness holds the 12-byte segment set, calls the module, and checks the
 * words.  The start code `_cstart_` in tools/rebuild/chk.asm sets `ES` to `DS`,
 * so the set holds `ES` equal to `DS`, a nonzero code segment, and a nonzero
 * stack segment.  The program prints the check marker and the exit marker:
 *
 *     V  the set holds the expected segment words
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

struct SREGS {
    unsigned short es;
    unsigned short cs;
    unsigned short ss;
    unsigned short ds;
    unsigned short fs;
    unsigned short gs;
};

void dpmi_fill_regs(struct SREGS *sregs);
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

int main(void)
{
    struct SREGS sregs;

    dpmi_fill_regs(&sregs);
    put(sregs.es == sregs.ds && sregs.cs != 0 && sregs.ss != 0 ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
