/* The test harness for the rebuilt `vesa_set_mode` module.
 *
 * The harness holds the data area that the module uses and the DOS memory
 * allocator stub.  The stub returns the info block in `edi` and 0 in `eax`, so
 * the module stores the block pointer, builds the real-mode register set, and
 * runs the VBE mode info call and the mode set call through the real-mode
 * interrupt path of DOS/4GW and DOSBox-X.  The program checks that the mode
 * argument reached the data area and prints the markers:
 *
 *     V  the mode argument is stored
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int vesa_set_mode(int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0007c17c;
unsigned int DAT_0007c180;
unsigned int DAT_0007c188;
unsigned int DAT_0007c18c;
unsigned char DAT_0007c190[0x200];
unsigned int DAT_0007c1a8;
unsigned int DAT_0007c1ac;
unsigned short DAT_0007c1b2;
unsigned char info_block[0x200];

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

unsigned dpmi_alloc_dos(unsigned size);

int main(void)
{
    DAT_0007c17c = 0;
    vesa_set_mode(1);
    put(DAT_0007c17c == 1 ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
