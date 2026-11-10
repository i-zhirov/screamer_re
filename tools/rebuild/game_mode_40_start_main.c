/* The test harness for the rebuilt `game_mode_40_start` module.
 *
 * The harness holds the marker count, the per-marker hit flags, the four state
 * cells, and the two callee stubs.  The stubs record the call arguments.  The
 * reset case fills every flag and every state cell with a sentinel, so a missed
 * write shows.
 *
 * The paths:
 *
 *     0  the count 2: the first two flags clear, the rest keep the sentinel,
 *        the state cells reset, and the car and scene calls run
 *     1  the count 0: no flag changes and the state and the calls still run
 *     2  the count 1 with the mode 0
 *     3  the count 3 with the mode -1: the first three flags clear
 *     4  the count -1: the signed guard clears no flag
 *     5  the player car load fails with 9: the function returns 9 and the
 *        scene setup stays unrun
 *     6  the player car load fails with -3: the function returns -3
 *     7  the count 0 with a failed car load: the function returns 5 and the
 *        scene setup stays unrun
 *     8  the count 4 with the mode 0x100: the car load and the scene setup
 *        take the mode
 *     D  the exit marker
 */
#pragma off (check_stack)

#define FLAG_COUNT 6
#define SENTINEL 0x7e7e7e7e

int game_mode_40_start(int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int sla_marker_count;
int sla_marker_flags[FLAG_COUNT];
int DAT_001100a4;
int DAT_001100dc;
int DAT_001100d4;
int DAT_0000cf40;

static char text[8];

static int init_calls;
static void *init_car;
static int init_load;
static int init_result;
static int scene_calls;
static int scene_value;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int player_car_init(void *car, int load)
{
    init_calls = init_calls + 1;
    init_car = car;
    init_load = load;
    return init_result;
}

void scene_setup(int value)
{
    scene_calls = scene_calls + 1;
    scene_value = value;
}

static void reset(void)
{
    int index;

    sla_marker_count = 0;
    for (index = 0; index < FLAG_COUNT; index++) {
        sla_marker_flags[index] = SENTINEL;
    }
    DAT_001100a4 = 0x11111111;
    DAT_001100dc = 0x22222222;
    DAT_001100d4 = 0x33333333;
    DAT_0000cf40 = 0x44444444;
    init_calls = 0;
    init_car = 0;
    init_load = -1;
    init_result = 0;
    scene_calls = 0;
    scene_value = -1;
}

static int state_ok(void)
{
    return DAT_001100a4 == 0
        && DAT_001100dc == 0
        && DAT_001100d4 == 0
        && DAT_0000cf40 == 1;
}

static int flags_ok(int count)
{
    int index;

    for (index = 0; index < count; index++) {
        if (sla_marker_flags[index] != 0) {
            return 0;
        }
    }
    for (; index < FLAG_COUNT; index++) {
        if (sla_marker_flags[index] != SENTINEL) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int result;
    int ok;

    reset();
    sla_marker_count = 2;
    result = game_mode_40_start(0x21);
    ok = result == 0
        && state_ok()
        && flags_ok(2)
        && init_calls == 1
        && init_car == (void *)0xf91c
        && init_load == 0x21
        && scene_calls == 1
        && scene_value == 0x21;
    put(ok ? '0' : 'x');

    reset();
    sla_marker_count = 0;
    result = game_mode_40_start(0x22);
    ok = result == 0
        && state_ok()
        && flags_ok(0)
        && init_calls == 1
        && init_car == (void *)0xf91c
        && init_load == 0x22
        && scene_calls == 1
        && scene_value == 0x22;
    put(ok ? '1' : 'x');

    reset();
    sla_marker_count = 1;
    result = game_mode_40_start(0);
    ok = result == 0
        && state_ok()
        && flags_ok(1)
        && init_calls == 1
        && init_load == 0
        && scene_calls == 1
        && scene_value == 0;
    put(ok ? '2' : 'x');

    reset();
    sla_marker_count = 3;
    result = game_mode_40_start(-1);
    ok = result == 0
        && state_ok()
        && flags_ok(3)
        && init_calls == 1
        && init_load == -1
        && scene_calls == 1
        && scene_value == -1;
    put(ok ? '3' : 'x');

    reset();
    sla_marker_count = -1;
    result = game_mode_40_start(0x1234);
    ok = result == 0
        && state_ok()
        && flags_ok(0)
        && init_calls == 1
        && init_load == 0x1234
        && scene_calls == 1
        && scene_value == 0x1234;
    put(ok ? '4' : 'x');

    reset();
    sla_marker_count = 2;
    init_result = 9;
    result = game_mode_40_start(0x24);
    ok = result == 9
        && state_ok()
        && flags_ok(2)
        && init_calls == 1
        && init_car == (void *)0xf91c
        && init_load == 0x24
        && scene_calls == 0;
    put(ok ? '5' : 'x');

    reset();
    sla_marker_count = 2;
    init_result = -3;
    result = game_mode_40_start(0x25);
    ok = result == -3
        && state_ok()
        && flags_ok(2)
        && init_calls == 1
        && init_load == 0x25
        && scene_calls == 0;
    put(ok ? '6' : 'x');

    reset();
    sla_marker_count = 0;
    init_result = 5;
    result = game_mode_40_start(0x26);
    ok = result == 5
        && state_ok()
        && flags_ok(0)
        && init_calls == 1
        && init_load == 0x26
        && scene_calls == 0;
    put(ok ? '7' : 'x');

    reset();
    sla_marker_count = 4;
    result = game_mode_40_start(0x100);
    ok = result == 0
        && state_ok()
        && flags_ok(4)
        && init_calls == 1
        && init_load == 0x100
        && scene_calls == 1
        && scene_value == 0x100;
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
