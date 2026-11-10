/* The test harness for the rebuilt `race_finish_laps_update` module.
 *
 * The harness holds the control word, the finish timer, the two source floats,
 * the sound handle, the player slot, the mode, the result cells, and the state
 * flag.  The stubs record the sound release and the selected finish handler.
 *
 * The harness runs eight paths:
 *
 *     0  the gate bit is clear: the function returns and changes nothing
 *     1  mode 1: the release runs, the result cells copy, and the mode 1 handler runs
 *     2  mode 2: the mode 2 handler runs
 *     3  mode 4: the mode 4 handler runs
 *     4  mode 0x10: the mode 0x10 handler runs
 *     5  mode 0x20: the mode 0x20 handler runs
 *     6  mode 0x40: the mode 0x40 handler runs
 *     7  mode 0: the default path runs the release and the control update only
 *     D  the exit marker
 *
 * The two source floats hold distinct values, so the check confirms the copy
 * order: DAT_00100a8 goes to the race time and DAT_00100b4 to the lap time.
 */
#pragma off (check_stack)

void race_finish_laps_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];

float DAT_00100a8;
float DAT_00100b4;
int DAT_00100e8;
int DAT_000ffa4;
int DAT_0011d260;
float DAT_0011d268;
float DAT_0011d26c;
int DAT_0011d270;
int DAT_0011df0;
unsigned short DAT_00118e1c;
int DAT_00118e28;

static int release_calls;
static int release_arg;
static int release_order;
static int handler_calls;
static int handler_id;
static int handler_order;
static int call_order;

void sound_handle_release(int handle)
{
    release_calls++;
    release_arg = handle;
    release_order = ++call_order;
}

void finish_mode_1(void)
{
    handler_calls++;
    handler_id = 1;
    handler_order = ++call_order;
}

void finish_mode_2(void)
{
    handler_calls++;
    handler_id = 2;
    handler_order = ++call_order;
}

void finish_mode_4(void)
{
    handler_calls++;
    handler_id = 4;
    handler_order = ++call_order;
}

void finish_mode_16(void)
{
    handler_calls++;
    handler_id = 16;
    handler_order = ++call_order;
}

void finish_mode_32(void)
{
    handler_calls++;
    handler_id = 32;
    handler_order = ++call_order;
}

void finish_mode_64(void)
{
    handler_calls++;
    handler_id = 64;
    handler_order = ++call_order;
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

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

static void reset(void)
{
    DAT_00118e1c = 8;
    DAT_00118e28 = 0x12345678;
    DAT_00100a8 = 12.5f;
    DAT_00100b4 = 34.25f;
    DAT_00100e8 = 0x1234;
    DAT_000ffa4 = 3;
    DAT_0011d260 = 1;
    DAT_0011d268 = -1.0f;
    DAT_0011d26c = -1.0f;
    DAT_0011d270 = -1;
    DAT_0011df0 = 0;
    release_calls = 0;
    release_arg = 0;
    release_order = 0;
    handler_calls = 0;
    handler_id = 0;
    handler_order = 0;
    call_order = 0;
}

static int result_ok(void)
{
    return release_calls == 1
        && release_arg == 0x1234
        && release_order == 1
        && DAT_0011d268 == 12.5f
        && DAT_0011d26c == 34.25f
        && DAT_0011d270 == 3
        && DAT_0011df0 == 1
        && DAT_00118e28 == 0
        && DAT_00118e1c == 0x4004;
}

int main(void)
{
    int ok;

    reset();
    DAT_00118e1c = 0;
    race_finish_laps_update();
    ok = release_calls == 0
        && handler_calls == 0
        && DAT_0011df0 == 0
        && DAT_0011d268 == -1.0f
        && DAT_0011d26c == -1.0f
        && DAT_0011d270 == -1
        && DAT_00118e28 == 0x12345678
        && DAT_00118e1c == 0;
    report(ok, '0');

    reset();
    DAT_0011d260 = 1;
    race_finish_laps_update();
    ok = result_ok() && handler_calls == 1 && handler_id == 1
        && handler_order == 2;
    report(ok, '1');

    reset();
    DAT_0011d260 = 2;
    race_finish_laps_update();
    ok = result_ok() && handler_calls == 1 && handler_id == 2;
    report(ok, '2');

    reset();
    DAT_0011d260 = 4;
    race_finish_laps_update();
    ok = result_ok() && handler_calls == 1 && handler_id == 4;
    report(ok, '3');

    reset();
    DAT_0011d260 = 0x10;
    race_finish_laps_update();
    ok = result_ok() && handler_calls == 1 && handler_id == 16;
    report(ok, '4');

    reset();
    DAT_0011d260 = 0x20;
    race_finish_laps_update();
    ok = result_ok() && handler_calls == 1 && handler_id == 32;
    report(ok, '5');

    reset();
    DAT_0011d260 = 0x40;
    race_finish_laps_update();
    ok = result_ok() && handler_calls == 1 && handler_id == 64;
    report(ok, '6');

    reset();
    DAT_0011d260 = 0;
    race_finish_laps_update();
    ok = result_ok() && handler_calls == 0;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
