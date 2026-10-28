/* The test harness for the rebuilt `video_fill` module.
 *
 * The harness holds the `video_copy` stub and the `DAT_00121528` pointer.  The
 * stub checks the two size arguments and the buffer pointer, prints its marker
 * when they match, and the program prints the exit marker, so the output shows
 * the chain:
 *
 *     V  `video_copy` ran with 480, 400, and `DAT_00121528`
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void video_fill(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static int marker;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void *DAT_00121528;

void video_copy(unsigned int width, unsigned int height, void *source)
{
    put(width == 480 && height == 400 && source == DAT_00121528 ? 'V' : 'v');
}

int main(void)
{
    DAT_00121528 = &marker;
    video_fill();
    put('D');
    quit();
    return 0;
}
