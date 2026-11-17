/* The test harness for the rebuilt `player_screen_offset` module.
 *
 * The harness holds the world reference `DAT_00110a20` and `DAT_00110a28`, the
 * screen limit, and one player record.  The module writes the screen offsets at
 * `+0x118`, `+0x11c`, and `+0x120`.
 *
 * The harness runs these paths:
 *
 *     0  the world reference on the x axis: the offset is on the negative x
 *     1  the world reference on the second horizontal axis: the offset is on
 *        the negative second axis
 *     2  the fractional ratios at 7 and 24 over the length 25: the negative
 *        offsets truncate toward zero
 *     3  a non-zero record position with the same ratios: the positive offsets
 *        truncate toward zero
 *     4  a negative x difference: the offset turns positive
 *     5  the limit 10.0: the two offsets follow the smaller scale
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because the minimal image
 * does not load the constant segment.
 */
#pragma off (check_stack)

void player_screen_offset(char *record);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define RECORD_SIZE 0x600

static char text[8];
static char record[RECORD_SIZE];

int DAT_00110a20;
int DAT_00110a28;
float player_screen_offset_limit;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_bytes(char *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0;
    }
}

static void set_int(int offset, int value)
{
    *(int *)(record + offset) = value;
}

static int int_is(int offset, int value)
{
    return *(int *)(record + offset) == value;
}

static void set_limit(unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    player_screen_offset_limit = value.f;
}

static void reset(void)
{
    clear_bytes(record, RECORD_SIZE);
    DAT_00110a20 = 0;
    DAT_00110a28 = 0;
    set_limit(0x41a00000);
}

int main(void)
{
    reset();
    set_int(0x4b4, 100);
    DAT_00110a20 = 30;
    player_screen_offset(record);
    put(int_is(0x118, -20)
        && int_is(0x11c, 80)
        && int_is(0x120, 0) ? '0' : 'x');

    reset();
    set_int(0x4b4, 50);
    DAT_00110a28 = 40;
    player_screen_offset(record);
    put(int_is(0x118, 0)
        && int_is(0x11c, 30)
        && int_is(0x120, -20) ? '1' : 'x');

    reset();
    set_int(0x4b4, 10);
    DAT_00110a20 = 7;
    DAT_00110a28 = 24;
    player_screen_offset(record);
    put(int_is(0x118, -5)
        && int_is(0x11c, -10)
        && int_is(0x120, -19) ? '2' : 'x');

    reset();
    set_int(0x4b0, 100);
    set_int(0x4b4, 200);
    set_int(0x4b8, 100);
    DAT_00110a20 = 107;
    DAT_00110a28 = 124;
    player_screen_offset(record);
    put(int_is(0x118, 94)
        && int_is(0x11c, 180)
        && int_is(0x120, 80) ? '3' : 'x');

    reset();
    set_int(0x4b4, -50);
    DAT_00110a20 = -30;
    player_screen_offset(record);
    put(int_is(0x118, 20)
        && int_is(0x11c, -70)
        && int_is(0x120, 0) ? '4' : 'x');

    reset();
    set_limit(0x41200000);
    DAT_00110a20 = 7;
    DAT_00110a28 = 24;
    player_screen_offset(record);
    put(int_is(0x118, -2)
        && int_is(0x11c, -20)
        && int_is(0x120, -9) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
