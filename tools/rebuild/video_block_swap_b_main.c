/* The test harness for the rebuilt `video_block_swap_b` module.
 *
 * The harness holds the video buffer and the buffer pointer.  The module reads
 * the pointer and swaps 200 rows of 160 dwords between the front of the buffer
 * and the region `0xf960` dwords ahead.  The harness fills the buffer with the
 * index value, so after the swap the first row holds the values of the far row
 * and the far row holds the values of the first row:
 *
 *     V  the first and the far rows exchanged their values
 *     D  the exit marker
 *
 * The buffer is a static array, and the harness sets the buffer pointer before
 * the call.  The first and the far rows are far enough apart that the swap of
 * the first row touches no other row, so the check stays simple.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void video_block_swap_b(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
void *DAT_00121528;

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
    static unsigned int buffer[0xfa00];
    unsigned int i;
    int ok;

    for (i = 0; i < 0xfa00; i = i + 1) {
        buffer[i] = i;
    }
    DAT_00121528 = buffer;
    video_block_swap_b();
    ok = buffer[0] == 0xf960
        && buffer[159] == 0xf9ff
        && buffer[0xf960] == 0
        && buffer[0xf9ff] == 159;
    put(ok ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
