/* The test harness for the rebuilt `race_finish_mode_1_update` module.
 *
 * The harness holds the control byte DAT_0018e1d, the finish position
 * DAT_000ffa4, and the `sound_enqueue` stub.  The stub records the effect, the
 * variant mode, and the raw delay bits, so it needs no floating-point compare.
 *
 * The paths:
 *
 *     0  position 1 with a clear control byte: bit 3 is set and the effect is
 *        0xb with the mode 3 and the delay 0
 *     1  position 1 with the control bits 0x11: the result 0x19 keeps the old
 *        bits and the effect is 0xb
 *     2  position 0: the control byte stays clear and the effect is 0xc
 *     3  position 2 with the control bits 0xa5: the byte stays 0xa5 and the
 *        effect is 0xc
 *     4  position -1 with bit 7 set: the byte stays 0x80 and the effect is 0xc
 *     D  the exit marker
 */
#pragma off (check_stack)

void race_finish_mode_1_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MAX_CALLS 4

unsigned char DAT_0018e1d;
int DAT_000ffa4;

static char text[8];
static int call_count;
static int call_effect[MAX_CALLS];
static int call_mode[MAX_CALLS];
static unsigned int call_delay[MAX_CALLS];

void sound_enqueue(int effect, int mode, float delay)
{
    union {
        float f;
        unsigned int u;
    } value;
    int index;

    index = call_count;
    if (index < MAX_CALLS) {
        call_effect[index] = effect;
        call_mode[index] = mode;
        value.f = delay;
        call_delay[index] = value.u;
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

static void check(int number, int ok)
{
    put(ok ? (char)('0' + number) : 'x');
}

static void reset(void)
{
    int index;

    DAT_0018e1d = 0;
    DAT_000ffa4 = 0;
    call_count = 0;
    for (index = 0; index < MAX_CALLS; index++) {
        call_effect[index] = -1;
        call_mode[index] = -1;
        call_delay[index] = 0xffffffff;
    }
}

static int call_ok(int index, int effect)
{
    return index < call_count
        && call_effect[index] == effect
        && call_mode[index] == 3
        && call_delay[index] == 0;
}

int main(void)
{
    reset();
    DAT_000ffa4 = 1;
    race_finish_mode_1_update();
    check(0, DAT_0018e1d == 0x08
        && call_count == 1 && call_ok(0, 0xb));

    reset();
    DAT_000ffa4 = 1;
    DAT_0018e1d = 0x11;
    race_finish_mode_1_update();
    check(1, DAT_0018e1d == 0x19
        && call_count == 1 && call_ok(0, 0xb));

    reset();
    DAT_000ffa4 = 0;
    race_finish_mode_1_update();
    check(2, DAT_0018e1d == 0x00
        && call_count == 1 && call_ok(0, 0xc));

    reset();
    DAT_000ffa4 = 2;
    DAT_0018e1d = 0xa5;
    race_finish_mode_1_update();
    check(3, DAT_0018e1d == 0xa5
        && call_count == 1 && call_ok(0, 0xc));

    reset();
    DAT_000ffa4 = -1;
    DAT_0018e1d = 0x80;
    race_finish_mode_1_update();
    check(4, DAT_0018e1d == 0x80
        && call_count == 1 && call_ok(0, 0xc));

    put('D');
    quit();
    return 0;
}
