/* The test harness for the rebuilt `sound_effect_trigger` module.
 *
 * The harness holds the 27-entry timer array and the `sound_enqueue` stub.  The
 * stub records the effect, the variant mode, and the raw delay bits.  The reset
 * function clears every timer and the call log.
 *
 * The harness runs seven paths:
 *
 *     0  the armed cell 0: the call returns 1, the effect 3 with the mode 3 and
 *        the delay bits 0x3f333333 is enqueued, and the cell is latched
 *     1  the latched cell -1.0: the call returns 0, enqueues nothing, and
 *        leaves the cell
 *     2  the positive cell 100 with the mode -2: the call returns 1, enqueues
 *        the effect 5 with the mode -2, and latches the cell
 *     3  the latched effect 0: the call returns 0 and enqueues nothing
 *     4  the armed last cell 26: the call returns 1 and latches the cell
 *     5  the cell 0xbf800001 next to the latch: the call returns 1, so only the
 *        exact bits 0xbf800000 pass the latch test
 *     6  the cell 0xbf7fffff below the latch: the call returns 1
 *     D  the exit marker
 */
#pragma off (check_stack)

#define EFFECT_COUNT 27
#define MAX_CALLS 4
#define LATCHED 0xbf800000u
#define DELAY_BITS 0x3f333333u

int sound_effect_trigger(int effect, int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_001922c[EFFECT_COUNT];

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

    for (index = 0; index < EFFECT_COUNT; index++) {
        DAT_001922c[index] = 0;
    }
    call_count = 0;
    for (index = 0; index < MAX_CALLS; index++) {
        call_effect[index] = -1;
        call_mode[index] = -1;
        call_delay[index] = 0xffffffffu;
    }
}

static int enqueued(int effect, int mode)
{
    return call_count == 1
        && call_effect[0] == effect
        && call_mode[0] == mode
        && call_delay[0] == DELAY_BITS;
}

int main(void)
{
    int result;

    reset();
    result = sound_effect_trigger(3, 3);
    check(0, result == 1 && DAT_001922c[3] == LATCHED && enqueued(3, 3));

    reset();
    DAT_001922c[3] = LATCHED;
    result = sound_effect_trigger(3, 3);
    check(1, result == 0 && DAT_001922c[3] == LATCHED && call_count == 0);

    reset();
    DAT_001922c[5] = 100;
    result = sound_effect_trigger(5, -2);
    check(2, result == 1 && DAT_001922c[5] == LATCHED && enqueued(5, -2));

    reset();
    DAT_001922c[0] = LATCHED;
    result = sound_effect_trigger(0, 7);
    check(3, result == 0 && call_count == 0);

    reset();
    result = sound_effect_trigger(26, 3);
    check(4, result == 1 && DAT_001922c[26] == LATCHED && enqueued(26, 3));

    reset();
    DAT_001922c[3] = 0xbf800001u;
    result = sound_effect_trigger(3, 3);
    check(5, result == 1 && DAT_001922c[3] == LATCHED && enqueued(3, 3));

    reset();
    DAT_001922c[3] = 0xbf7fffffu;
    result = sound_effect_trigger(3, 3);
    check(6, result == 1 && DAT_001922c[3] == LATCHED && enqueued(3, 3));

    put('D');
    quit();
    return 0;
}
