/* The test harness for the rebuilt `memset` module.
 *
 * The harness fills a buffer with a known pattern, calls the module with a byte
 * count of 16 and the fill value 1, and checks the buffer.  The module fills
 * `count >> 2` dwords, so the first 16 bytes become the repeated value and the
 * rest stays the pattern.  The program prints the check marker and the exit
 * marker:
 *
 *     V  the fill reached the first 16 bytes
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void memset(void *dest, unsigned count, unsigned value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char buffer[32];

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
    unsigned i;
    unsigned char dword[4];
    unsigned ok;

    for (i = 0; i < 32; i = i + 1) {
        buffer[i] = 0x55;
    }
    memset(buffer, 16, 1);
    dword[0] = 1;
    dword[1] = 1;
    dword[2] = 1;
    dword[3] = 1;
    ok = 1;
    for (i = 0; i < 16; i = i + 1) {
        if (buffer[i] != dword[i % 4]) {
            ok = 0;
        }
    }
    for (i = 16; i < 32; i = i + 1) {
        if (buffer[i] != 0x55) {
            ok = 0;
        }
    }
    put(ok ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
