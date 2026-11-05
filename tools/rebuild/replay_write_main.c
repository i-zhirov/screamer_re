/* The test harness for the rebuilt `replay_write` module.
 *
 * The harness holds the replay pointer, the enable flag, the index, the two
 * end markers, and the replay globals.  It runs five paths:
 *
 *     0  the clear enable flag writes no record, keeps the index, and keeps
 *        the second end marker
 *     1  an ordinary index writes the two float fields and the six 16-bit
 *        fields, advances the index, and sets the second marker to the index
 *     2  an index one below the end leaves the index at the last slot and sets
 *        the second marker to the end minus two
 *     3  an index whose advance reaches the end stops at the last slot and sets
 *        the second marker to the end minus two
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void replay_write(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char *DAT_00118e5c;
unsigned int DAT_00118e60;
unsigned int DAT_00118e64;
unsigned int DAT_00118e68;
unsigned int DAT_00118e70;
float DAT_00118e04;
float DAT_00118e08;
short DAT_00119880;
short DAT_00119884;
short DAT_00119888;
short DAT_0011988c;
short DAT_00119890;
short DAT_00119894;

static char text[8];
static unsigned char records[5 * 0x14];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(unsigned int enable, unsigned int index, unsigned int end)
{
    int offset;

    for (offset = 0; offset < 5 * 0x14; ++offset) {
        records[offset] = 0;
    }
    DAT_00118e5c = records;
    DAT_00118e60 = enable;
    DAT_00118e64 = index;
    DAT_00118e68 = end;
    DAT_00118e70 = 0xaaaa;
    *(unsigned int *)&DAT_00118e04 = 0x3fc00000;
    *(unsigned int *)&DAT_00118e08 = 0xc0200000;
    DAT_00119880 = -1;
    DAT_00119884 = -2;
    DAT_00119888 = 0x7fff;
    DAT_0011988c = (short)0x8000;
    DAT_00119890 = 0x1234;
    DAT_00119894 = -0x1234;
}

static int blank(void)
{
    int offset;

    for (offset = 0; offset < 5 * 0x14; ++offset) {
        if (records[offset] != 0) {
            return 0;
        }
    }
    return 1;
}

static int written(unsigned int index)
{
    unsigned char *record;

    record = records + index * 0x14;
    return *(unsigned int *)(record + 0) == 0x3fc00000
        && *(unsigned int *)(record + 4) == 0xc0200000
        && *(short *)(record + 8) == -1
        && *(short *)(record + 0xa) == -2
        && *(short *)(record + 0xc) == 0x7fff
        && *(short *)(record + 0xe) == (short)0x8000
        && *(short *)(record + 0x10) == 0x1234
        && *(short *)(record + 0x12) == -0x1234;
}

int main(void)
{
    reset(0, 1, 4);
    replay_write();
    put(DAT_00118e64 == 1 && DAT_00118e70 == 0xaaaa && blank() ? '0' : 'x');

    reset(1, 0, 4);
    replay_write();
    put(DAT_00118e64 == 1 && DAT_00118e70 == 1 && written(0) ? '1' : 'x');

    reset(1, 2, 4);
    replay_write();
    put(DAT_00118e64 == 3 && DAT_00118e70 == 2 && written(2) ? '2' : 'x');

    reset(1, 3, 4);
    replay_write();
    put(DAT_00118e64 == 3 && DAT_00118e70 == 2 && written(3) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
