/* The test harness for the rebuilt `memcpy` module.
 *
 * The harness fills a source buffer with the characters `A` to `H`, clears the target
 * buffer, calls the module, and prints the target bytes and the result flag `Y`.
 *
 * The module copies through `rep movsd`, which reads through DS:ESI and writes through
 * ES:EDI.  The harness start code `_cstart_` in tools/rebuild/chk.asm sets ES to DS, so
 * the write reaches the target buffer.  Without that step the copy reads the source
 * correctly and writes to the segment that ES still holds, and the target stays zero.
 *
 * The markers go through the data buffer `text`, because the `print` helper of this harness
 * writes garbage for a string literal (the same fault shows in the output of the driver at
 * work/rebuild/out.txt).
 */
#pragma off (check_stack)

void memcpy(void *dst, const void *src, unsigned count);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char source[8];
static char target[8];
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
    int i;
    int same;

    source[0] = 'A';
    source[1] = 'B';
    source[2] = 'C';
    source[3] = 'D';
    source[4] = 'E';
    source[5] = 'F';
    source[6] = 'G';
    source[7] = 'H';
    for (i = 0; i < 8; i++) {
        target[i] = 0;
    }

    memcpy(target, source, 8);

    same = 1;
    for (i = 0; i < 8; i++) {
        if (target[i] != source[i]) {
            same = 0;
        }
    }
    for (i = 0; i < 8; i++) {
        put(target[i]);
    }
    put((char)(same ? 'Y' : 'N'));
    quit();
    return 0;
}
