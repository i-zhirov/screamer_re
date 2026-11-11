/* The test harness for the rebuilt `cd_audio_tick` module.
 *
 * The harness holds the tick counter, the state flag, the driver gate, and the
 * four callee stubs.  The disc info and Q-channel stubs return the configured
 * status, the table build stub counts its calls, and the sound driver stub
 * records its argument.  Each path checks the counter, the state flag, the
 * gate, and the call counts.
 *
 * The paths:
 *
 *     0  the first tick with the Q status 0: no state change
 *     1  the second tick with the Q status 2: the state flag is set
 *     2  the third tick with the Q status 0x0f: the state flag is set
 *     3  the fourth tick with the missing disc and a clear gate: the flag is
 *        set, the counter is clear, and the driver is not called
 *     4  the fourth tick with the missing disc and a set gate: the driver is
 *        called with 0
 *     5  the fourth tick with a disc and a clear state flag: the Q-channel
 *        read does not run
 *     6  the fourth tick with a disc, a set flag, and a non-zero Q status: the
 *        state flag stays set and the table is not built
 *     7  the fourth tick with a disc, a set flag, a zero Q status, and a clear
 *        gate: the flag is clear, the table is built, and the driver is not
 *        called
 *     8  the fourth tick with a disc, a set flag, a zero Q status, and a set
 *        gate: the driver is called with 0
 *     D  the exit marker
 */
#pragma off (check_stack)

void cd_audio_tick(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0016000d;
int DAT_00166688;
int DAT_0016669a;

static char text[8];
static int disc_value;
static int q_value;
static int disc_calls;
static int q_calls;
static int table_calls;
static int sound_calls;
static int sound_argument;
static int sound_ok;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

int cd_audio_disc_info_request(void)
{
    disc_calls = disc_calls + 1;
    return disc_value;
}

int cd_audio_q_channel_read(void)
{
    q_calls = q_calls + 1;
    return q_value;
}

void cd_audio_table_build(void)
{
    table_calls = table_calls + 1;
}

void sound_driver_set(int value)
{
    sound_calls = sound_calls + 1;
    sound_argument = value;
    if (value != 0) {
        sound_ok = 0;
    }
}

static void reset(int counter, int flag, int gate, int disc, int q)
{
    DAT_0016000d = counter;
    DAT_00166688 = flag;
    DAT_0016669a = gate;
    disc_value = disc;
    q_value = q;
    disc_calls = 0;
    q_calls = 0;
    table_calls = 0;
    sound_calls = 0;
    sound_argument = -1;
    sound_ok = 1;
}

int main(void)
{
    reset(0, 0, 0, 0x00, 0x00);
    cd_audio_tick();
    check('0', DAT_0016000d == 1 && DAT_00166688 == 0
        && disc_calls == 0 && q_calls == 1
        && table_calls == 0 && sound_calls == 0);

    reset(1, 0, 0, 0x00, 0x02);
    cd_audio_tick();
    check('1', DAT_0016000d == 2 && DAT_00166688 == 1
        && disc_calls == 0 && q_calls == 1
        && table_calls == 0 && sound_calls == 0);

    reset(2, 0, 0, 0x00, 0x0f);
    cd_audio_tick();
    check('2', DAT_0016000d == 3 && DAT_00166688 == 1
        && disc_calls == 0 && q_calls == 1
        && table_calls == 0 && sound_calls == 0);

    reset(3, 0, 0, 0xff, 0x00);
    cd_audio_tick();
    check('3', DAT_0016000d == 0 && DAT_00166688 == 1
        && disc_calls == 1 && q_calls == 0
        && table_calls == 0 && sound_calls == 0);

    reset(3, 0, 1, 0xff, 0x00);
    cd_audio_tick();
    check('4', DAT_0016000d == 0 && DAT_00166688 == 1
        && disc_calls == 1 && q_calls == 0
        && table_calls == 0 && sound_calls == 1
        && sound_argument == 0 && sound_ok);

    reset(3, 0, 0, 0x00, 0x00);
    cd_audio_tick();
    check('5', DAT_0016000d == 0 && DAT_00166688 == 0
        && disc_calls == 1 && q_calls == 0
        && table_calls == 0 && sound_calls == 0);

    reset(3, 1, 0, 0x00, 0x01);
    cd_audio_tick();
    check('6', DAT_0016000d == 0 && DAT_00166688 == 1
        && disc_calls == 1 && q_calls == 1
        && table_calls == 0 && sound_calls == 0);

    reset(3, 1, 0, 0x00, 0x00);
    cd_audio_tick();
    check('7', DAT_0016000d == 0 && DAT_00166688 == 0
        && disc_calls == 1 && q_calls == 1
        && table_calls == 1 && sound_calls == 0);

    reset(3, 1, 1, 0x00, 0x00);
    cd_audio_tick();
    check('8', DAT_0016000d == 0 && DAT_00166688 == 0
        && disc_calls == 1 && q_calls == 1
        && table_calls == 1 && sound_calls == 1
        && sound_argument == 0 && sound_ok);

    put('D');
    quit();
    return 0;
}
