/* The test harness for the rebuilt `video_block_swap_a` module.
 *
 * The harness holds the video buffer and the buffer pointer.  The module reads
 * the pointer and byte-swaps and swaps 80 passes of 400 dwords between the
 * front of the buffer and the region `0x9f` dwords ahead.  The harness fills
 * the buffer with the index value, so after the swap the first position holds
 * the byte-swapped far value and the far position holds the byte-swapped first
 * value:
 *
 *     V  the first and the far positions exchanged their byte-swapped values
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void video_block_swap_a(void);
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
    video_block_swap_a();
    ok = buffer[0] == 0x9f000000
        && buffer[0x9f] == 0
        && buffer[1] == 0x9e000000
        && buffer[0x9e] == 0x01000000;
    put(ok ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
