/* The test harness for the rebuilt `memset_runtime` module.
 *
 * The harness holds the run-time core stub.  The module replicates the value
 * byte over the dword and passes the destination, the value, and the count to
 * the core, so the stub records the three values and fills the buffer.  The
 * program prints the check marker and the exit marker:
 *
 *     V  the core ran with the replicated value and the count
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void *memset_runtime(void *dest, int value, unsigned count);
void *memset_core(void *dest, unsigned value, unsigned count);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char buffer[16];
static unsigned int core_value;
static unsigned int core_count;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void *memset_core(void *dest, unsigned value, unsigned count)
{
    unsigned i;

    core_value = value;
    core_count = count;
    for (i = 0; i < count; i = i + 1) {
        ((unsigned char *)dest)[i] = (unsigned char)(value >> ((i % 4) * 8));
    }
    return dest;
}

int main(void)
{
    unsigned i;
    unsigned ok;

    for (i = 0; i < 16; i = i + 1) {
        buffer[i] = 0;
    }
    memset_runtime(buffer, 0xab, 12);
    ok = core_value == 0xabababab && core_count == 12;
    for (i = 0; i < 12; i = i + 1) {
        if (buffer[i] != 0xab) {
            ok = 0;
        }
    }
    put(ok ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
