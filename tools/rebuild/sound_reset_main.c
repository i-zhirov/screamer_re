/* The test harness for the rebuilt `sound_reset` module.
 *
 * The harness holds the sound status, the timer, the queue head, the two
 * per-effect arrays with one extra sentinel entry each, and the frame counter.
 * The reset case fills every field and every array entry with a sentinel, so a
 * missed write shows.
 *
 * The paths:
 *
 *     0  the clear bit 1 of the counter: the status, the timer, and the queue
 *        head reset, the array clears, and the counters all take 2
 *     1  the counter 2: bit 1 set and bit 0 clear gives the counters 0
 *     2  the counter 3: both bits set gives the counters 1
 *     3  the counter 1: bit 1 clear keeps the counters at 2
 *     4  a second call with another counter overwrites the whole state
 *     D  the exit marker
 *
 * The timer holds its bit pattern as an integer, like the original, so the
 * harness needs no float symbol and no constant segment.
 */
#pragma off (check_stack)

#define EFFECT_COUNT 27
#define STORE_COUNT 28

void sound_reset(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00119070;
int DAT_0011907c;
int DAT_00119080;
int DAT_001922c[STORE_COUNT];
int DAT_00191c0[STORE_COUNT];
int DAT_000007fd;

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

static void reset_case(void)
{
    int index;

    DAT_00119070 = 0x1234;
    DAT_0011907c = 0x11111111;
    DAT_00119080 = 0x5678;
    for (index = 0; index < STORE_COUNT; index++) {
        DAT_001922c[index] = 0x7e7e7e7e;
        DAT_00191c0[index] = 0x7e7e7e7e;
    }
}

static int clears_zero(void)
{
    int index;

    for (index = 0; index < EFFECT_COUNT; index++) {
        if (DAT_001922c[index] != 0) {
            return 0;
        }
    }
    return DAT_001922c[EFFECT_COUNT] == 0x7e7e7e7e;
}

static int counters_are(int value)
{
    int index;

    for (index = 0; index < EFFECT_COUNT; index++) {
        if (DAT_00191c0[index] != value) {
            return 0;
        }
    }
    return DAT_00191c0[EFFECT_COUNT] == 0x7e7e7e7e;
}

static int head_state_ok(void)
{
    return DAT_00119070 == -1
        && DAT_0011907c == 0x3f333333
        && DAT_00119080 == -1;
}

int main(void)
{
    reset_case();
    DAT_000007fd = 0;
    sound_reset();
    put(head_state_ok() && clears_zero() && counters_are(2) ? '0' : 'x');

    reset_case();
    DAT_000007fd = 2;
    sound_reset();
    put(head_state_ok() && clears_zero() && counters_are(0) ? '1' : 'x');

    reset_case();
    DAT_000007fd = 3;
    sound_reset();
    put(head_state_ok() && clears_zero() && counters_are(1) ? '2' : 'x');

    reset_case();
    DAT_000007fd = 1;
    sound_reset();
    put(head_state_ok() && clears_zero() && counters_are(2) ? '3' : 'x');

    reset_case();
    DAT_000007fd = 0;
    sound_reset();
    DAT_000007fd = 3;
    sound_reset();
    put(head_state_ok() && clears_zero() && counters_are(1) ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
