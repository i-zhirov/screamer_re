/* The test harness for the rebuilt `sound_render_branch_greater` module.
 *
 * The harness holds the three branch bytes as globals.  The module writes the
 * value 0x7d to every byte.
 *
 * The paths:
 *
 *     0  all three bytes zero: every byte becomes 0x7d
 *     1  all three bytes 0xff: every byte becomes 0x7d
 *     2  all three bytes 0x7c, the sibling value: every byte becomes 0x7d
 *     3  the mixed values 0x7c, 0x00, and 0xff: every byte becomes 0x7d
 *     D  the exit marker
 *
 * Every path checks all three bytes.
 */
#pragma off (check_stack)

void sound_render_branch_greater(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char render_branch_83853;
unsigned char render_branch_83907;
unsigned char render_branch_83bc7;

static char text[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(unsigned char first, unsigned char second, unsigned char third)
{
    render_branch_83853 = first;
    render_branch_83907 = second;
    render_branch_83bc7 = third;
}

static int all_greater(void)
{
    if (render_branch_83853 != 0x7d) {
        return 0;
    }
    if (render_branch_83907 != 0x7d) {
        return 0;
    }
    if (render_branch_83bc7 != 0x7d) {
        return 0;
    }
    return 1;
}

int main(void)
{
    reset(0x00, 0x00, 0x00);
    sound_render_branch_greater();
    put(all_greater() ? '0' : 'x');

    reset(0xff, 0xff, 0xff);
    sound_render_branch_greater();
    put(all_greater() ? '1' : 'x');

    reset(0x7c, 0x7c, 0x7c);
    sound_render_branch_greater();
    put(all_greater() ? '2' : 'x');

    reset(0x7c, 0x00, 0xff);
    sound_render_branch_greater();
    put(all_greater() ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
