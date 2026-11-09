/* The test harness for the rebuilt `game_mode_1_start` module.
 *
 * The harness holds the car count, the car records, the three start cells, the
 * two replay flags, the frame buffer cell, the replay name buffer, and the
 * seven callee stubs.  The stubs record the call arguments.
 *
 * The harness runs these paths:
 *
 *     0  two cars load with the mode 0x21 and both replay flags clear: the
 *        three start cells take the count, every car record walks by 0x590
 *        with the mode argument, the player car loads with 0xf91c, the scene
 *        setup takes the mode argument, the frame buffer prepares once, and
 *        the clear flag path runs
 *     1  the second car fails: the function returns the error at once, the
 *        shared counter holds 1, and every later step stays unrun
 *     2  the first car fails: the function returns the error, the counter
 *        holds 0, and every later step stays unrun
 *     3  the player car load fails: the function returns the error, the scene
 *        setup and the tail stay unrun
 *     4  the replay flag is set with the mode 0x25: the frame buffer prepares
 *        once, the replay name loads with the flag 1, and the replay path runs
 *     5  the replay flag is set with the mode 0: the frame buffer stays
 *        unchanged, and the replay name still loads
 *     6  the replay flag is clear and the control flag is set: the frame
 *        buffer stays unchanged, the replay name stays unloaded, and the
 *        replay path runs
 *     7  both flags are clear with the mode 0: the frame buffer stays
 *        unchanged and the clear flag path runs
 *     8  the car count is zero: no car loads, but the player car, the scene
 *        setup, the frame buffer, and the tail still run
 *     D  the exit marker
 */
#pragma off (check_stack)

int game_mode_1_start(int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0000f8ec;
int DAT_0000f8f0;
int DAT_0000634c;
int DAT_0000cf40;
int DAT_001100a4;
int DAT_0000cf54;
int DAT_0018e30;
int DAT_0018e60;
char DAT_00010ab8[0x590 * 4];
char DAT_0001d278[0x20];

static char text[8];

static int car_calls;
static unsigned int car_record[8];
static int car_index[8];
static int car_load[8];
static int car_fail_index;
static int car_fail_value;
static int init_calls;
static void *init_car;
static int init_load;
static int init_result;
static int scene_calls;
static int scene_value;
static int replay_calls;
static char *replay_name;
static int replay_load_frames;
static int buffer_calls;
static int buffer_result;
static int clear_calls;
static int flush_calls;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int car_setup(void *record, int index, int load)
{
    car_record[car_calls] = (unsigned int)record;
    car_index[car_calls] = index;
    car_load[car_calls] = load;
    car_calls = car_calls + 1;
    if (index == car_fail_index) {
        return car_fail_value;
    }
    return 0;
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

int replay_load(char *name, int load_frames)
{
    replay_calls = replay_calls + 1;
    replay_name = name;
    replay_load_frames = load_frames;
    return 0;
}

int func_22c30(void)
{
    buffer_calls = buffer_calls + 1;
    return buffer_result;
}

void func_22d88(void)
{
    clear_calls = clear_calls + 1;
}

void func_22db8(void)
{
    flush_calls = flush_calls + 1;
}

static void reset(void)
{
    int index;

    DAT_0000f8ec = 0;
    DAT_0000f8f0 = 0x11111111;
    DAT_0000634c = 0x22222222;
    DAT_0000cf40 = 0x33333333;
    DAT_001100a4 = 0x44444444;
    DAT_0000cf54 = 0;
    DAT_0018e30 = 0;
    DAT_0018e60 = 0x55555555;
    for (index = 0; index < 0x590 * 4; index = index + 1) {
        DAT_00010ab8[index] = 0;
    }
    DAT_0001d278[0] = 'R';
    DAT_0001d278[1] = 0;
    car_calls = 0;
    car_fail_index = -1;
    car_fail_value = 0;
    init_calls = 0;
    init_car = 0;
    init_load = -1;
    init_result = 0;
    scene_calls = 0;
    scene_value = -1;
    replay_calls = 0;
    replay_name = 0;
    replay_load_frames = -1;
    buffer_calls = 0;
    buffer_result = 7;
    clear_calls = 0;
    flush_calls = 0;
    for (index = 0; index < 8; index = index + 1) {
        car_record[index] = 0;
        car_index[index] = 0;
        car_load[index] = 0;
    }
}

static int check_start(int count, int mode)
{
    return DAT_0000634c == count
        && DAT_0000cf40 == count + 1
        && DAT_0000f8f0 == 0
        && DAT_001100a4 == count
        && car_calls == count
        && init_calls == 1
        && init_car == (void *)0xf91c
        && init_load == mode
        && scene_calls == 1
        && scene_value == mode;
}

int main(void)
{
    int result;
    int ok;

    reset();
    DAT_0000f8ec = 2;
    result = game_mode_1_start(0x21);
    ok = result == 0
        && check_start(2, 0x21)
        && car_record[0] == (unsigned int)DAT_00010ab8
        && car_index[0] == 0
        && car_load[0] == 0x21
        && car_record[1] == (unsigned int)(DAT_00010ab8 + 0x590)
        && car_index[1] == 1
        && car_load[1] == 0x21
        && buffer_calls == 1
        && DAT_0018e60 == 7
        && replay_calls == 0
        && flush_calls == 0
        && clear_calls == 1;
    put(ok ? '0' : 'x');

    reset();
    DAT_0000f8ec = 3;
    car_fail_index = 1;
    car_fail_value = -3;
    result = game_mode_1_start(0x22);
    ok = result == -3
        && DAT_0000cf40 == 4
        && DAT_001100a4 == 3
        && DAT_0000634c == 1
        && car_calls == 2
        && init_calls == 0
        && scene_calls == 0
        && buffer_calls == 0
        && DAT_0018e60 == 0x55555555
        && replay_calls == 0
        && flush_calls == 0
        && clear_calls == 0;
    put(ok ? '1' : 'x');

    reset();
    DAT_0000f8ec = 1;
    car_fail_index = 0;
    car_fail_value = 5;
    result = game_mode_1_start(0x23);
    ok = result == 5
        && DAT_0000634c == 0
        && car_calls == 1
        && init_calls == 0
        && scene_calls == 0
        && buffer_calls == 0
        && clear_calls == 0;
    put(ok ? '2' : 'x');

    reset();
    DAT_0000f8ec = 1;
    init_result = 9;
    result = game_mode_1_start(0x24);
    ok = result == 9
        && DAT_0000634c == 1
        && DAT_0000cf40 == 2
        && DAT_0000f8f0 == 0
        && DAT_001100a4 == 1
        && car_calls == 1
        && init_calls == 1
        && init_car == (void *)0xf91c
        && init_load == 0x24
        && scene_calls == 0
        && buffer_calls == 0
        && replay_calls == 0
        && flush_calls == 0
        && clear_calls == 0;
    put(ok ? '3' : 'x');

    reset();
    DAT_0000f8ec = 1;
    DAT_0018e30 = 1;
    result = game_mode_1_start(0x25);
    ok = result == 0
        && check_start(1, 0x25)
        && buffer_calls == 1
        && DAT_0018e60 == 7
        && replay_calls == 1
        && replay_name == DAT_0001d278
        && replay_load_frames == 1
        && flush_calls == 1
        && clear_calls == 0;
    put(ok ? '4' : 'x');

    reset();
    DAT_0000f8ec = 1;
    DAT_0018e30 = 1;
    result = game_mode_1_start(0);
    ok = result == 0
        && check_start(1, 0)
        && buffer_calls == 0
        && DAT_0018e60 == 0x55555555
        && replay_calls == 1
        && replay_load_frames == 1
        && flush_calls == 1
        && clear_calls == 0;
    put(ok ? '5' : 'x');

    reset();
    DAT_0000f8ec = 1;
    DAT_0000cf54 = 1;
    result = game_mode_1_start(0x26);
    ok = result == 0
        && check_start(1, 0x26)
        && buffer_calls == 0
        && DAT_0018e60 == 0x55555555
        && replay_calls == 0
        && flush_calls == 1
        && clear_calls == 0;
    put(ok ? '6' : 'x');

    reset();
    DAT_0000f8ec = 1;
    result = game_mode_1_start(0);
    ok = result == 0
        && check_start(1, 0)
        && buffer_calls == 0
        && DAT_0018e60 == 0x55555555
        && replay_calls == 0
        && flush_calls == 0
        && clear_calls == 1;
    put(ok ? '7' : 'x');

    reset();
    DAT_0000f8ec = 0;
    result = game_mode_1_start(0x27);
    ok = result == 0
        && check_start(0, 0x27)
        && car_calls == 0
        && buffer_calls == 1
        && DAT_0018e60 == 7
        && replay_calls == 0
        && flush_calls == 0
        && clear_calls == 1;
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
