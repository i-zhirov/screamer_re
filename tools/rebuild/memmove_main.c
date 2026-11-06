/* The test harness for the rebuilt `memmove` module.
 *
 * The harness fills a source buffer with a known pattern, clears the target
 * buffer, calls the module with a byte count that is not a whole dword, and
 * checks the target.  The program prints the check marker and the exit marker:
 *
 *     V  the target holds the source pattern
 *     D  the exit marker
 *
 * The module copies through `repne movsd` and `repne movsb`, which read through
 * DS:ESI and write through ES:EDI.  The module sets `ES` to `DS` itself, so the
 * write reaches the target buffer.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void memmove(void *dest, const void *source, unsigned count);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char source[32];
static unsigned char target[32];

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
    unsigned ok;

    for (i = 0; i < 32; i = i + 1) {
        source[i] = (unsigned char)(i + 1);
        target[i] = 0;
    }
    memmove(target, source, 30);
    ok = 1;
    for (i = 0; i < 30; i = i + 1) {
        if (target[i] != source[i]) {
            ok = 0;
        }
    }
    if (target[30] != 0 || target[31] != 0) {
        ok = 0;
    }
    put(ok ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
