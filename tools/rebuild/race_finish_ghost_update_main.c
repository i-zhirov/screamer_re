/* The test harness for the rebuilt `race_finish_ghost_update` module.
 *
 * The harness holds the replay flag DAT_001d264, the replay index DAT_0018ed4,
 * the clock DAT_00100a8, the position DAT_000ffa4, the control byte
 * DAT_0018e1d, two ghost records at DAT_0018e8c, and the `sound_enqueue` stub.
 *
 * The harness runs seven paths:
 *
 *     0  the replay flag is zero and the position is 1: the function keeps the
 *        position, sets bit 3 of the control byte, and enqueues the effect 0xb
 *     1  the replay flag is zero and the position is 2: the function keeps the
 *        position, keeps the control byte, and enqueues the effect 0xc
 *     2  the flag is set and the clock is above the other ghost time: the
 *        position becomes 2 and the effect is 0xc
 *     3  the flag is set and the clock is below the other ghost time: the
 *        position becomes 1, bit 3 is set, and the effect is 0xb
 *     4  the clock equals the other ghost time: the compare gives the position
 *        2
 *     5  the index selects the other record: only the other ghost time counts
 *     6  the index 1 selects the ghost record 0, and the clock is above it: the
 *        position becomes 2
 *     D  the exit marker
 *
 * Every path checks the call count, the effect, the variant mode 3, and the
 * delay 0.  The harness sets the ghost times at run time, because the minimal
 * image does not load the constant segment.
 */
#pragma off (check_stack)

void race_finish_ghost_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define GHOST_STRIDE 0x24
#define GHOST_COUNT 2
#define MAX_CALLS 8

int DAT_001d264;
int DAT_0018ed4;
float DAT_00100a8;
int DAT_000ffa4;
unsigned char DAT_0018e1d;
char DAT_0018e8c[GHOST_STRIDE * GHOST_COUNT];

static char text[8];

static int call_count;
static int call_effect[MAX_CALLS];
static int call_mode[MAX_CALLS];
static float call_delay[MAX_CALLS];

void sound_enqueue(int effect, int mode, float delay)
{
    int index;

    index = call_count;
    if (index < MAX_CALLS) {
        call_effect[index] = effect;
        call_mode[index] = mode;
        call_delay[index] = delay;
    }
    call_count = index + 1;
}

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

static void set_ghost(int index, float when)
{
    char *record = DAT_0018e8c + index * GHOST_STRIDE;

    *(float *)(record + 0x1c) = when;
}

static void reset(void)
{
    int index;

    clear_bytes(DAT_0018e8c, GHOST_STRIDE * GHOST_COUNT);
    DAT_001d264 = 0;
    DAT_0018ed4 = 0;
    DAT_00100a8 = 0.0f;
    DAT_000ffa4 = 0;
    DAT_0018e1d = 0;
    call_count = 0;
    for (index = 0; index < MAX_CALLS; index++) {
        call_effect[index] = -1;
        call_mode[index] = -1;
        call_delay[index] = -1.0f;
    }
}

static int call_is(int index, int effect)
{
    return index < call_count
        && call_effect[index] == effect
        && call_mode[index] == 3
        && call_delay[index] == 0.0f;
}

int main(void)
{
    reset();
    DAT_000ffa4 = 1;
    race_finish_ghost_update();
    put(call_count == 1 && call_is(0, 0xb) && DAT_000ffa4 == 1
        && (DAT_0018e1d & 8) != 0 ? '0' : 'x');

    reset();
    DAT_000ffa4 = 2;
    race_finish_ghost_update();
    put(call_count == 1 && call_is(0, 0xc) && DAT_000ffa4 == 2
        && DAT_0018e1d == 0 ? '1' : 'x');

    reset();
    DAT_001d264 = 1;
    DAT_0018ed4 = 0;
    DAT_00100a8 = 60.0f;
    DAT_000ffa4 = 9;
    set_ghost(0, 10.0f);
    set_ghost(1, 50.0f);
    race_finish_ghost_update();
    put(call_count == 1 && call_is(0, 0xc) && DAT_000ffa4 == 2
        && DAT_0018e1d == 0 ? '2' : 'x');

    reset();
    DAT_001d264 = 1;
    DAT_0018ed4 = 1;
    DAT_00100a8 = 40.0f;
    DAT_000ffa4 = 9;
    set_ghost(0, 50.0f);
    set_ghost(1, 10.0f);
    race_finish_ghost_update();
    put(call_count == 1 && call_is(0, 0xb) && DAT_000ffa4 == 1
        && (DAT_0018e1d & 8) != 0 ? '3' : 'x');

    reset();
    DAT_001d264 = 1;
    DAT_0018ed4 = 0;
    DAT_00100a8 = 50.0f;
    DAT_000ffa4 = 9;
    set_ghost(1, 50.0f);
    race_finish_ghost_update();
    put(call_count == 1 && call_is(0, 0xc) && DAT_000ffa4 == 2
        ? '4' : 'x');

    reset();
    DAT_001d264 = 1;
    DAT_0018ed4 = 0;
    DAT_00100a8 = 49.5f;
    DAT_000ffa4 = 9;
    set_ghost(0, 1.0f);
    set_ghost(1, 50.0f);
    race_finish_ghost_update();
    put(call_count == 1 && call_is(0, 0xb) && DAT_000ffa4 == 1
        && (DAT_0018e1d & 8) != 0 ? '5' : 'x');

    reset();
    DAT_001d264 = 1;
    DAT_0018ed4 = 1;
    DAT_00100a8 = 60.0f;
    DAT_000ffa4 = 9;
    set_ghost(0, 50.0f);
    set_ghost(1, 1.0f);
    race_finish_ghost_update();
    put(call_count == 1 && call_is(0, 0xc) && DAT_000ffa4 == 2
        ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
