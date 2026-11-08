/* The test harness for the rebuilt `race_view_reset` module.
 *
 * The harness holds four 0x590-byte car records at DAT_00010ab8, the car count,
 * the shared loop counter, the five shared view cells, the seven 0x14-byte view
 * slots, and the byte buffer for the markers.
 *
 * The harness runs four paths:
 *
 *     0  three cars: the four view flags clear, the view mode takes 8, the
 *        fourth record stays untouched, the five cells and the counter take
 *        zero, and the chosen flags clear
 *     1  zero cars: no record changes, but the cells and the counter still
 *        take zero and the chosen flags clear
 *     2  one car: the first record changes and the second record stays
 *        untouched
 *     3  two cars: the first two records change and the third record stays
 *        untouched
 *     D  the exit marker
 *
 * The whole record area starts as 0xa5, so an untouched field reads the
 * pattern 0xa5a5a5a5.  The view slot fields other than the chosen flag keep
 * their pattern, so the check shows that the function touches only the chosen
 * flag.
 */
#pragma off (check_stack)

struct view_slot {
    int kind;
    int mode;
    int distance;
    int video;
    int chosen;
};

void race_view_reset(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0000f8ec;
int DAT_0000634c;
unsigned char DAT_00010ab8[0x590 * 4];
int DAT_0010a74;
int DAT_0010a78;
int DAT_0010a94;
int DAT_0010a9c;
int DAT_0010aa8;
struct view_slot DAT_00101da4[7];

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

static int field(int index, int offset)
{
    return *(int *)(DAT_00010ab8 + index * 0x590 + offset);
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x590 * 4; index++) {
        DAT_00010ab8[index] = 0xa5;
    }
    DAT_0000f8ec = 0;
    DAT_0000634c = 0x22222222;
    DAT_0010a74 = 0x22222222;
    DAT_0010a78 = 0x22222222;
    DAT_0010a94 = 0x22222222;
    DAT_0010a9c = 0x22222222;
    DAT_0010aa8 = 0x22222222;
    for (index = 0; index < 7; index++) {
        DAT_00101da4[index].kind = 0x100 + index;
        DAT_00101da4[index].mode = 0x200 + index;
        DAT_00101da4[index].distance = 0x300 + index;
        DAT_00101da4[index].video = 0x400 + index;
        DAT_00101da4[index].chosen = 0x22222222;
    }
}

static int cleared(int index)
{
    return field(index, 0x54c) == 0
        && field(index, 0x550) == 0
        && field(index, 0x56c) == 0
        && field(index, 0x574) == 0
        && field(index, 0x580) == 8;
}

static int untouched(int index)
{
    return field(index, 0x000) == 0xa5a5a5a5
        && field(index, 0x4f8) == 0xa5a5a5a5
        && field(index, 0x54c) == 0xa5a5a5a5
        && field(index, 0x580) == 0xa5a5a5a5;
}

static int cells_cleared(void)
{
    return DAT_0010a74 == 0
        && DAT_0010a78 == 0
        && DAT_0010a94 == 0
        && DAT_0010a9c == 0
        && DAT_0010aa8 == 0;
}

static int slots_cleared(void)
{
    int index;

    for (index = 0; index < 7; index++) {
        if (DAT_00101da4[index].chosen != 0) {
            return 0;
        }
    }
    return 1;
}

static int slots_preserved(void)
{
    int index;

    for (index = 0; index < 7; index++) {
        if (DAT_00101da4[index].kind != 0x100 + index
            || DAT_00101da4[index].mode != 0x200 + index
            || DAT_00101da4[index].distance != 0x300 + index
            || DAT_00101da4[index].video != 0x400 + index) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int ok;

    reset();
    DAT_0000f8ec = 3;
    race_view_reset();
    ok = cleared(0) && cleared(1) && cleared(2)
        && untouched(3)
        && DAT_0000634c == 3
        && cells_cleared()
        && slots_cleared()
        && slots_preserved();
    put(ok ? '0' : 'x');

    reset();
    DAT_0000f8ec = 0;
    race_view_reset();
    ok = untouched(0) && untouched(3)
        && DAT_0000634c == 0
        && cells_cleared()
        && slots_cleared()
        && slots_preserved();
    put(ok ? '1' : 'x');

    reset();
    DAT_0000f8ec = 1;
    race_view_reset();
    ok = cleared(0) && untouched(1) && untouched(3)
        && DAT_0000634c == 1
        && cells_cleared()
        && slots_cleared()
        && slots_preserved();
    put(ok ? '2' : 'x');

    reset();
    DAT_0000f8ec = 2;
    race_view_reset();
    ok = cleared(0) && cleared(1) && untouched(2) && untouched(3)
        && DAT_0000634c == 2
        && cells_cleared()
        && slots_cleared()
        && slots_preserved();
    put(ok ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
