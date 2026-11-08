/* The test harness for the rebuilt `player_state_snapshot` module.
 *
 * The harness holds the player object, the five globals, and the input bytes.
 * It sets distinct values in the source fields and checks the snapshot block:
 *
 *     0  the four simple copies and the baseline state flag
 *     1  the reordered field copies and the two float copies
 *     2  the three view-offset sums
 *     3  the three angle sums and the height minus 2
 *     4  the two conditional copies when the state is not 1
 *     5  the state flag 0x10000 and the input flag 0x20000
 *     6  the state 1 keeps 0x10000, sets 0x20000, and skips the two copies
 *     7  the control bit 0x10 clears the input flag
 *     8  a clear input bit 4 clears the input flag
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void player_state_snapshot(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011521c;
int DAT_0011d250;
int DAT_0011d2a0;
unsigned char DAT_00118e1c;
unsigned char DAT_00118e1d;

static char text[8];
static char object[0x900];

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

static void set_int(char *base, int offset, int value)
{
    *(int *)(base + offset) = value;
}

static unsigned int get_int(char *base, int offset)
{
    return *(unsigned int *)(base + offset);
}

static void prepare(void)
{
    clear_bytes(object, 0x900);
    set_int(object, 0x148, 0);
    set_int(object, 0x2fc, 0x00000101);
    set_int(object, 0x7b4, 0x00000010);
    set_int(object, 0x7b0, 0x11111111);
    set_int(object, 0x684, 0x22222222);
    set_int(object, 0x688, 0x33333333);
    set_int(object, 0x680, 0x3f800000);
    set_int(object, 0x78c, 0x40200000);
    set_int(object, 0x4b0, 10);
    set_int(object, 0x4b4, 20);
    set_int(object, 0x4b8, 30);
    set_int(object, 0x130, 1);
    set_int(object, 0x134, 2);
    set_int(object, 0x138, 3);
    set_int(object, 0x4c8, 0x3f800000);
    set_int(object, 0x4cc, 0x40000000);
    set_int(object, 0x4d0, 0x40800000);
    set_int(object, 0x124, 0x3f000000);
    set_int(object, 0x128, 0x3e800000);
    set_int(object, 0x12c, 0x3e000000);
    set_int(object, 0x4ec, 100);
    set_int(object, 0x834, 0xdeadbeef);
    set_int(object, 0x838, 0xfeedface);
    DAT_0011521c = 0x10203040;
    DAT_0011d250 = 0x50607080;
    DAT_0011d2a0 = 0x90a0b0c0;
    DAT_00118e1c = 0;
    DAT_00118e1d = 0;
}

int main(void)
{
    prepare();
    player_state_snapshot(object);
    put(get_int(object, 0x7e8) == 0x10203040
        && get_int(object, 0x7ec) == 0x50607080
        && get_int(object, 0x7f4) == 0x11111111
        && get_int(object, 0x7f8) == 0x90a0b0c0
        && get_int(object, 0x7f0) == 0x00000111 ? '0' : 'x');
    put(get_int(object, 0x7fc) == 0x22222222
        && get_int(object, 0x800) == 0x33333333
        && get_int(object, 0x804) == 0x3f800000
        && get_int(object, 0x808) == 0x40200000 ? '1' : 'x');
    put(get_int(object, 0x818) == 11
        && get_int(object, 0x81c) == 22
        && get_int(object, 0x820) == 33 ? '2' : 'x');
    put(get_int(object, 0x824) == 0x3fc00000
        && get_int(object, 0x828) == 0x40100000
        && get_int(object, 0x82c) == 0x40840000
        && get_int(object, 0x830) == 98 ? '3' : 'x');
    put(get_int(object, 0x834) == 0x3fc00000
        && get_int(object, 0x838) == 0x40840000 ? '4' : 'x');

    prepare();
    set_int(object, 0x148, 2);
    set_int(object, 0x2fc, 0x00000202);
    set_int(object, 0x7b4, 0x00000020);
    DAT_00118e1c = 4;
    DAT_00118e1d = 0;
    player_state_snapshot(object);
    put(get_int(object, 0x7f0) == 0x00030222
        && get_int(object, 0x834) == 0x3fc00000
        && get_int(object, 0x838) == 0x40840000 ? '5' : 'x');

    prepare();
    set_int(object, 0x148, 1);
    set_int(object, 0x2fc, 0x40);
    set_int(object, 0x7b4, 0x80);
    DAT_00118e1c = 4;
    DAT_00118e1d = 0;
    player_state_snapshot(object);
    put(get_int(object, 0x7f0) == 0x000300c0
        && get_int(object, 0x834) == 0xdeadbeef
        && get_int(object, 0x838) == 0xfeedface ? '6' : 'x');

    prepare();
    set_int(object, 0x148, 2);
    DAT_00118e1c = 4;
    DAT_00118e1d = 0x10;
    player_state_snapshot(object);
    put(get_int(object, 0x7f0) == 0x00010111 ? '7' : 'x');

    prepare();
    set_int(object, 0x148, 2);
    DAT_00118e1c = 0;
    DAT_00118e1d = 0;
    player_state_snapshot(object);
    put(get_int(object, 0x7f0) == 0x00010111 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
