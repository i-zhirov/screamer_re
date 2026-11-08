/* The test harness for the rebuilt `race_position_update` module.
 *
 * The harness holds the gate byte, the gate value, the mode byte, the old
 * position, the position, the traffic count, the car count, the replay flag,
 * the replay index, the three float thresholds, four traffic records, four car
 * records, and two ghost records.
 *
 * The harness runs eleven paths:
 *
 *     0  the gate bit is clear: the function writes nothing
 *     1  the gate value is zero: the function writes nothing
 *     2  the traffic path counts the active record above the threshold
 *     3  the traffic path with only empty records gives the position 1
 *     4  the car path swaps the place of the first disagreeing record
 *     5  the car path keeps the position when no record disagrees
 *     6  the replay flag is zero: the function writes no position
 *     7  the active ghost with the threshold below the limit gives 2
 *     8  the active ghost with the threshold above the limit gives 1
 *     9  the inactive ghost with the clock above the time gives 2
 *     a  the inactive ghost with the clock below the time gives 1
 *     b  the traffic bit wins over the car bit
 *     c  the mode is zero: the function writes nothing
 *     D  the exit marker
 *
 * The harness sets the float values at run time, because the minimal image
 * does not load the constant segment.
 */
#pragma off (check_stack)

void race_position_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define TRAFFIC_STRIDE 0x2a8
#define CAR_STRIDE 0x590
#define GHOST_STRIDE 0x24
#define MAX_RECORDS 4

unsigned char DAT_0018e1c;
int DAT_000100a0;
int DAT_0000ffa4;
int DAT_0000feb0;
unsigned char DAT_0001d260;
int DAT_0001d254;
float DAT_0000ff9c;
int DAT_0000f8ec;
int DAT_0001d264;
int DAT_00018ed4;
float DAT_000100a8;
float DAT_00018f08;
char DAT_0013cd8[TRAFFIC_STRIDE * MAX_RECORDS];
char DAT_0010ab8[CAR_STRIDE * MAX_RECORDS];
char DAT_0018e98[GHOST_STRIDE * 2];

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

static void clear_bytes(char *base, int size)
{
    int offset;

    for (offset = 0; offset < size; offset++) {
        base[offset] = 0;
    }
}

static void reset(void)
{
    clear_bytes(DAT_0013cd8, TRAFFIC_STRIDE * MAX_RECORDS);
    clear_bytes(DAT_0010ab8, CAR_STRIDE * MAX_RECORDS);
    clear_bytes(DAT_0018e98, GHOST_STRIDE * 2);
    DAT_0018e1c = 1;
    DAT_000100a0 = 1;
    DAT_0001d260 = 0;
    DAT_0000ffa4 = 0;
    DAT_0000feb0 = 0;
    DAT_0000ff9c = 100.0f;
    DAT_0000f8ec = 0;
    DAT_0001d254 = 0;
    DAT_0001d264 = 0;
    DAT_00018ed4 = 0;
    DAT_000100a8 = 0.0f;
    DAT_00018f08 = 0.0f;
}

static void set_traffic(int index, int first, float progress)
{
    char *record = DAT_0013cd8 + index * TRAFFIC_STRIDE;

    *(int *)record = first;
    *(float *)(record + 0x28) = progress;
}

static void set_car(int index, int place, float progress)
{
    char *record = DAT_0010ab8 + index * CAR_STRIDE;

    *(int *)(record + 0x410) = place;
    *(float *)(record + 0x408) = progress;
}

static void set_ghost(int index, int active, float when)
{
    char *record = DAT_0018e98 + index * GHOST_STRIDE;

    *(int *)record = active;
    *(float *)(record + 0x10) = when;
}

static int car_place(int index)
{
    return *(int *)(DAT_0010ab8 + index * CAR_STRIDE + 0x410);
}

int main(void)
{
    reset();
    DAT_0018e1c = 0;
    DAT_0001d260 = 0x10;
    DAT_0000ffa4 = 7;
    race_position_update();
    put(DAT_0000ffa4 == 7 && DAT_0000feb0 == 0 ? '0' : 'x');

    reset();
    DAT_000100a0 = 0;
    DAT_0001d260 = 0x10;
    DAT_0000ffa4 = 7;
    race_position_update();
    put(DAT_0000ffa4 == 7 && DAT_0000feb0 == 0 ? '1' : 'x');

    reset();
    DAT_0001d260 = 0x10;
    DAT_0001d254 = 4;
    DAT_0000ff9c = 100.0f;
    DAT_0000ffa4 = 99;
    set_traffic(0, 5, 200.0f);
    set_traffic(1, -1, 200.0f);
    set_traffic(2, 7, 100.0f);
    set_traffic(3, 9, 50.0f);
    race_position_update();
    put(DAT_0000ffa4 == 2 && DAT_0000feb0 == 99 ? '2' : 'x');

    reset();
    DAT_0001d260 = 0x10;
    DAT_0001d254 = 2;
    DAT_0000ff9c = 100.0f;
    DAT_0000ffa4 = 5;
    set_traffic(0, -1, 200.0f);
    set_traffic(1, -1, 200.0f);
    race_position_update();
    put(DAT_0000ffa4 == 1 && DAT_0000feb0 == 5 ? '3' : 'x');

    reset();
    DAT_0001d260 = 1;
    DAT_0000f8ec = 2;
    DAT_0000ff9c = 100.0f;
    DAT_0000ffa4 = 2;
    set_car(0, 1, 50.0f);
    set_car(1, 4, 200.0f);
    race_position_update();
    put(DAT_0000ffa4 == 1 && car_place(0) == 2 && car_place(1) == 4
        && DAT_0000feb0 == 2 ? '4' : 'x');

    reset();
    DAT_0001d260 = 1;
    DAT_0000f8ec = 2;
    DAT_0000ff9c = 100.0f;
    DAT_0000ffa4 = 2;
    set_car(0, 2, 200.0f);
    set_car(1, 1, 200.0f);
    race_position_update();
    put(DAT_0000ffa4 == 2 && car_place(0) == 2 && car_place(1) == 1
        && DAT_0000feb0 == 2 ? '5' : 'x');

    reset();
    DAT_0001d260 = 4;
    DAT_0001d264 = 0;
    DAT_0000ffa4 = 9;
    race_position_update();
    put(DAT_0000ffa4 == 9 && DAT_0000feb0 == 9 ? '6' : 'x');

    reset();
    DAT_0001d260 = 4;
    DAT_0001d264 = 1;
    DAT_00018ed4 = 0;
    DAT_0000ff9c = 10.0f;
    DAT_00018f08 = 20.0f;
    DAT_0000ffa4 = 9;
    set_ghost(1, 1, 0.0f);
    race_position_update();
    put(DAT_0000ffa4 == 2 && DAT_0000feb0 == 9 ? '7' : 'x');

    reset();
    DAT_0001d260 = 4;
    DAT_0001d264 = 1;
    DAT_00018ed4 = 0;
    DAT_0000ff9c = 30.0f;
    DAT_00018f08 = 20.0f;
    DAT_0000ffa4 = 9;
    set_ghost(1, 1, 0.0f);
    race_position_update();
    put(DAT_0000ffa4 == 1 && DAT_0000feb0 == 9 ? '8' : 'x');

    reset();
    DAT_0001d260 = 4;
    DAT_0001d264 = 1;
    DAT_00018ed4 = 1;
    DAT_000100a8 = 60.0f;
    DAT_0000ffa4 = 9;
    set_ghost(0, 0, 50.0f);
    race_position_update();
    put(DAT_0000ffa4 == 2 && DAT_0000feb0 == 9 ? '9' : 'x');

    reset();
    DAT_0001d260 = 4;
    DAT_0001d264 = 1;
    DAT_00018ed4 = 1;
    DAT_000100a8 = 40.0f;
    DAT_0000ffa4 = 9;
    set_ghost(0, 0, 50.0f);
    race_position_update();
    put(DAT_0000ffa4 == 1 && DAT_0000feb0 == 9 ? 'a' : 'x');

    reset();
    DAT_0001d260 = 0x11;
    DAT_0000f8ec = 1;
    DAT_0001d254 = 1;
    DAT_0000ff9c = 100.0f;
    DAT_0000ffa4 = 2;
    set_traffic(0, 5, 200.0f);
    set_car(0, 1, 50.0f);
    race_position_update();
    put(DAT_0000ffa4 == 2 && car_place(0) == 1 && DAT_0000feb0 == 2
        ? 'b' : 'x');

    reset();
    DAT_0001d260 = 0;
    DAT_0000ffa4 = 6;
    race_position_update();
    put(DAT_0000ffa4 == 6 && DAT_0000feb0 == 6 ? 'c' : 'x');

    put('D');
    quit();
    return 0;
}
