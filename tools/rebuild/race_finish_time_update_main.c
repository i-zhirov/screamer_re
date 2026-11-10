/* The test harness for the rebuilt `race_finish_time_update` module.
 *
 * The harness holds the control word, the finish timer, the sound handle, the
 * player slot, the mode, and the state flag.  The stubs record the sound
 * release, the effect push, and the selected finish handler.
 *
 * The harness runs eight paths:
 *
 *     0  the gate bit is clear: the function returns and changes nothing
 *     1  mode 1: the release and the effect run, then the mode 1 handler runs
 *     2  mode 2: the mode 2 handler runs
 *     3  mode 4: the mode 4 handler runs and sets the player slot to 2
 *     4  mode 0x10: the mode 0x10 handler runs
 *     5  mode 0x20: the mode 0x20 handler runs
 *     6  mode 0x40: the mode 0x40 handler runs
 *     7  mode 0: the default path runs the release, the effect, and the update
 *     D  the exit marker
 *
 * The control word holds the preserved bits 6 and 12, so the check confirms the
 * mask and the set value.
 */
#pragma off (check_stack)

void race_finish_time_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];

int DAT_00100e8;
int DAT_0011d260;
int DAT_0011df0;
unsigned short DAT_00118e1c;
int DAT_00118e28;
int DAT_000ffa4;

static int release_calls;
static int release_arg;
static int release_order;
static int effect_calls;
static int effect_type;
static int effect_subtype;
static int effect_value;
static int effect_order;
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

void effect_push(int type, int subtype, int value)
{
    effect_calls++;
    effect_type = type;
    effect_subtype = subtype;
    effect_value = value;
    effect_order = ++call_order;
}

void finish_time_mode_1(void)
{
    handler_calls++;
    handler_id = 1;
    handler_order = ++call_order;
}

void finish_time_mode_2(void)
{
    handler_calls++;
    handler_id = 2;
    handler_order = ++call_order;
}

void finish_time_mode_4(void)
{
    handler_calls++;
    handler_id = 4;
    handler_order = ++call_order;
    DAT_000ffa4 = 2;
}

void finish_time_mode_16(void)
{
    handler_calls++;
    handler_id = 16;
    handler_order = ++call_order;
}

void finish_time_mode_32(void)
{
    handler_calls++;
    handler_id = 32;
    handler_order = ++call_order;
}

void finish_time_mode_64(void)
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
    DAT_00118e1c = 0x1050;
    DAT_00118e28 = 0x12345678;
    DAT_00100e8 = 0x1234;
    DAT_0011d260 = 1;
    DAT_0011df0 = 0;
    DAT_000ffa4 = 3;
    release_calls = 0;
    release_arg = 0;
    release_order = 0;
    effect_calls = 0;
    effect_type = 0;
    effect_subtype = 0;
    effect_value = -1;
    effect_order = 0;
    handler_calls = 0;
    handler_id = 0;
    handler_order = 0;
    call_order = 0;
}

static int update_ok(void)
{
    return DAT_0011df0 == 1
        && DAT_00118e28 == 0
        && DAT_00118e1c == 0x1044;
}

static int effect_ok(void)
{
    return effect_calls == 1
        && effect_type == 0x19
        && effect_subtype == 3
        && effect_value == 0
        && effect_order == 2;
}

int main(void)
{
    int ok;

    reset();
    DAT_00118e1c = 0;
    race_finish_time_update();
    ok = release_calls == 0
        && effect_calls == 0
        && handler_calls == 0
        && DAT_0011df0 == 0
        && DAT_00118e28 == 0x12345678
        && DAT_000ffa4 == 3
        && DAT_00118e1c == 0;
    report(ok, '0');

    reset();
    DAT_0011d260 = 1;
    race_finish_time_update();
    ok = release_calls == 1
        && release_arg == 0x1234
        && release_order == 1
        && effect_ok()
        && update_ok()
        && handler_calls == 1
        && handler_id == 1
        && handler_order == 3;
    report(ok, '1');

    reset();
    DAT_0011d260 = 2;
    race_finish_time_update();
    ok = effect_ok() && update_ok()
        && handler_calls == 1 && handler_id == 2;
    report(ok, '2');

    reset();
    DAT_0011d260 = 4;
    race_finish_time_update();
    ok = effect_ok() && update_ok()
        && handler_calls == 1 && handler_id == 4
        && DAT_000ffa4 == 2;
    report(ok, '3');

    reset();
    DAT_0011d260 = 0x10;
    race_finish_time_update();
    ok = effect_ok() && update_ok()
        && handler_calls == 1 && handler_id == 16;
    report(ok, '4');

    reset();
    DAT_0011d260 = 0x20;
    race_finish_time_update();
    ok = effect_ok() && update_ok()
        && handler_calls == 1 && handler_id == 32;
    report(ok, '5');

    reset();
    DAT_0011d260 = 0x40;
    race_finish_time_update();
    ok = effect_ok() && update_ok()
        && handler_calls == 1 && handler_id == 64;
    report(ok, '6');

    reset();
    DAT_0011d260 = 0;
    race_finish_time_update();
    ok = update_ok() && handler_calls == 0;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
