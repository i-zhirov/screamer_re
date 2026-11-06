/* The test harness for the rebuilt `sound_driver_set` module.
 *
 * The harness holds the state words, the HMI request block, and the driver
 * call stub.  The module builds the request block from the argument and the
 * state words and calls the driver through the stub.  The harness checks the
 * block fields and the call:
 *
 *     V  the request block holds the argument and the state words
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void sound_driver_set(int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_00116684;
unsigned short DAT_0011668c;
unsigned int DAT_0011669a;
unsigned int DAT_001166a4;
unsigned char DAT_00116680;

static char text[8];
static unsigned char hmi_block[0x80];
static int setup_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void sd_setup(void)
{
    setup_calls = setup_calls + 1;
}

int main(void)
{
    int ok;

    DAT_00116684 = 1;
    DAT_0011668c = 1;
    DAT_00116680 = 0x55;
    DAT_001166a4 = (unsigned int)hmi_block;
    setup_calls = 0;
    sound_driver_set(0x11);
    ok = setup_calls == 1
        && DAT_0011669a == 0x11
        && hmi_block[0] == 0x1a
        && hmi_block[1] == 0x55
        && hmi_block[2] == 0xc
        && hmi_block[0x12] == 2
        && hmi_block[0x40] == 1
        && hmi_block[0x41] == 0x11
        && *(unsigned int *)(hmi_block + 0x42) == 0;
    put(ok ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
