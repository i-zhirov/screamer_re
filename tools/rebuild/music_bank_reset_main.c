/* The test harness for the rebuilt `music_bank_reset` module.
 *
 * The harness holds a music bank record with known field values, calls the
 * module, and checks the three fields.
 *
 *     V  the three fields hold the reset values
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void music_bank_reset(void *bank);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char bank[0x20];

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
    unsigned int ok;

    *(unsigned int *)(bank + 4) = 0x11111111;
    *(unsigned int *)(bank + 0x10) = 0x22222222;
    *(unsigned int *)(bank + 0x14) = 0x33333333;
    music_bank_reset(bank);
    ok = *(unsigned int *)(bank + 4) == 0xffffffff
        && *(unsigned int *)(bank + 0x10) == 0
        && *(unsigned int *)(bank + 0x14) == 0;
    put(ok ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
