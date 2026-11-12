/* The test harness for the rebuilt `replay_write_begin` module.
 *
 * The harness holds the frame buffer gate, the control byte, the replay write
 * mode, and the replay index.  Every cell starts at a distinct sentinel value,
 * so a missing store or an extra store fails the path.
 *
 * The paths:
 *
 *     0  gate 0 with the flags 0xa5: every cell keeps its sentinel
 *     1  gate 1 with the flags 0x00: the write mode and the index become 0,
 *        and the control byte takes 0x40
 *     2  gate 1 with the flags 0xa5: the control byte keeps the low bits and
 *        takes the value 0xe5
 *     3  gate 1 with the flags 0x7f: the set playback bit stays and the value
 *        stays 0x7f
 *     4  gate 1 with the flags 0xff: the value stays 0xff
 *     D  the exit marker
 */
#pragma off (check_stack)

void replay_write_begin(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_00018e60;
unsigned char DAT_00018e1c;
unsigned int DAT_00018e64;
unsigned int DAT_00018e6c;

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

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void reset(unsigned int gate, unsigned char flags)
{
    DAT_00018e60 = gate;
    DAT_00018e1c = flags;
    DAT_00018e64 = 0x22222222;
    DAT_00018e6c = 0x11111111;
}

static int untouched(unsigned char flags)
{
    return DAT_00018e1c == flags
        && DAT_00018e64 == 0x22222222
        && DAT_00018e6c == 0x11111111;
}

static int started(unsigned char flags)
{
    return DAT_00018e1c == flags
        && DAT_00018e64 == 0
        && DAT_00018e6c == 0;
}

int main(void)
{
    reset(0, 0xa5);
    replay_write_begin();
    check('0', untouched(0xa5));

    reset(1, 0x00);
    replay_write_begin();
    check('1', started(0x40));

    reset(1, 0xa5);
    replay_write_begin();
    check('2', started(0xe5));

    reset(1, 0x7f);
    replay_write_begin();
    check('3', started(0x7f));

    reset(1, 0xff);
    replay_write_begin();
    check('4', started(0xff));

    put('D');
    quit();
    return 0;
}
