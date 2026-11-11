/* The test harness for the rebuilt `race_finish_sla_update` module.
 *
 * The harness holds the control byte DAT_00118e1d, the SLA marker count
 * DAT_00007ed4, the hit count DAT_001100dc, the state cell DAT_001100d4,
 * the finish position DAT_0000ffa4, and the `sound_enqueue` stub.  The stub
 * records the effect, the variant mode, and the raw delay bits, so it needs no
 * floating-point compare.
 *
 * The harness runs seven paths:
 *
 *     0  five markers and three hits: bit 3 is set, bit 7 is clear, the
 *        position is 1, and the effect is 0xb with the mode 3 and the delay 0
 *     1  five markers and five hits with a zero state cell: bits 3 and 7 are set
 *     2  five markers and five hits with a set state cell: bit 7 stays clear
 *     3  the zero count with a zero state cell: the counts match and bit 7 is set
 *     4  the count -1 with the count -1: the counts match and bit 7 is set
 *     5  the control byte 0x11 with two markers and one hit: the result 0x19
 *        keeps the old bits
 *     6  the control byte 0x10 with two markers and two hits: the result 0x98
 *        keeps the old bit
 *     D  the exit marker
 */
#pragma off (check_stack)

void race_finish_sla_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MAX_CALLS 4

unsigned char DAT_00118e1d;
int DAT_00007ed4;
int DAT_001100dc;
int DAT_001100d4;
int DAT_0000ffa4;

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

    DAT_00118e1d = 0;
    DAT_00007ed4 = 0;
    DAT_001100dc = 0;
    DAT_001100d4 = 0;
    DAT_0000ffa4 = 0;
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
    DAT_00007ed4 = 5;
    DAT_001100dc = 3;
    race_finish_sla_update();
    check(0, DAT_00118e1d == 0x08 && DAT_0000ffa4 == 1
        && call_count == 1 && call_ok(0, 0xb));

    reset();
    DAT_00007ed4 = 5;
    DAT_001100dc = 5;
    race_finish_sla_update();
    check(1, DAT_00118e1d == 0x88 && DAT_0000ffa4 == 1
        && call_count == 1 && call_ok(0, 0xb));

    reset();
    DAT_00007ed4 = 5;
    DAT_001100dc = 5;
    DAT_001100d4 = 1;
    race_finish_sla_update();
    check(2, DAT_00118e1d == 0x08
        && call_count == 1 && call_ok(0, 0xb));

    reset();
    race_finish_sla_update();
    check(3, DAT_00118e1d == 0x88
        && call_count == 1 && call_ok(0, 0xb));

    reset();
    DAT_00007ed4 = -1;
    DAT_001100dc = -1;
    race_finish_sla_update();
    check(4, DAT_00118e1d == 0x88
        && call_count == 1 && call_ok(0, 0xb));

    reset();
    DAT_00118e1d = 0x11;
    DAT_00007ed4 = 2;
    DAT_001100dc = 1;
    race_finish_sla_update();
    check(5, DAT_00118e1d == 0x19
        && call_count == 1 && call_ok(0, 0xb));

    reset();
    DAT_00118e1d = 0x10;
    DAT_00007ed4 = 2;
    DAT_001100dc = 2;
    race_finish_sla_update();
    check(6, DAT_00118e1d == 0x98
        && call_count == 1 && call_ok(0, 0xb));

    put('D');
    quit();
    return 0;
}
