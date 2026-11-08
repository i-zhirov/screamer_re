/* The test harness for the rebuilt `replay_read` module.
 *
 * The harness holds the replay pointer, the index, the two end markers, and
 * three 20-byte records.  The harness runs four paths:
 *
 *     0  the first record at index 0 copies the two dword fields and the six
 *        signed 16-bit fields, advances the index to 1, and returns 0
 *     1  the record at index 2 ends at the first marker and returns 1
 *     2  the record at index 1 ends at the second marker and returns 1
 *     3  a record before the markers returns 0
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int replay_read(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char *DAT_00118e5c;
unsigned int DAT_00118e64;
unsigned int DAT_00118e68;
unsigned int DAT_00118e70;
unsigned int DAT_00118e04;
unsigned int DAT_00118e08;
int DAT_00119880;
int DAT_00119884;
int DAT_00119888;
int DAT_0011988c;
int DAT_00119890;
int DAT_00119894;

static char text[8];
static unsigned char records[3 * 0x14];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(unsigned int index, unsigned int first, unsigned int second)
{
    int offset;

    for (offset = 0; offset < 3 * 0x14; ++offset) {
        records[offset] = 0;
    }
    DAT_00118e5c = records;
    DAT_00118e64 = index;
    DAT_00118e68 = first;
    DAT_00118e70 = second;
    DAT_00118e04 = 0;
    DAT_00118e08 = 0;
    DAT_00119880 = 0;
    DAT_00119884 = 0;
    DAT_00119888 = 0;
    DAT_0011988c = 0;
    DAT_00119890 = 0;
    DAT_00119894 = 0;
}

static void fill(unsigned int index, unsigned int low, unsigned int high)
{
    unsigned char *record;

    record = records + index * 0x14;
    *(unsigned int *)(record + 0) = low;
    *(unsigned int *)(record + 4) = high;
    *(short *)(record + 8) = -1;
    *(short *)(record + 0xa) = -2;
    *(short *)(record + 0xc) = 0x7fff;
    *(short *)(record + 0xe) = (short)0x8000;
    *(short *)(record + 0x10) = 0x1234;
    *(short *)(record + 0x12) = -0x1234;
}

int main(void)
{
    int result;

    reset(0, 3, 4);
    fill(0, 0x11223344, 0x55667788);
    result = replay_read();
    put(result == 0 && DAT_00118e64 == 1
        && DAT_00118e04 == 0x11223344 && DAT_00118e08 == 0x55667788
        && DAT_00119880 == -1 && DAT_00119884 == -2
        && DAT_00119888 == 0x7fff && DAT_0011988c == -0x8000
        && DAT_00119890 == 0x1234 && DAT_00119894 == -0x1234 ? '0' : 'x');

    reset(2, 3, 4);
    fill(2, 0x01020304, 0x05060708);
    result = replay_read();
    put(result == 1 && DAT_00118e64 == 3
        && DAT_00118e04 == 0x01020304 && DAT_00118e08 == 0x05060708
        && DAT_00119888 == 0x7fff && DAT_00119894 == -0x1234 ? '1' : 'x');

    reset(1, 3, 2);
    fill(1, 0x0a0b0c0d, 0x0e0f1011);
    result = replay_read();
    put(result == 1 && DAT_00118e64 == 2
        && DAT_00118e04 == 0x0a0b0c0d && DAT_00118e08 == 0x0e0f1011 ? '2' : 'x');

    reset(1, 4, 5);
    fill(1, 0x11121314, 0x15161718);
    result = replay_read();
    put(result == 0 && DAT_00118e64 == 2
        && DAT_00118e04 == 0x11121314 && DAT_00118e08 == 0x15161718 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
