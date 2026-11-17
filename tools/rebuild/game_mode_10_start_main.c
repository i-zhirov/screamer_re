/* The test harness for the rebuilt `game_mode_10_start` module.
 *
 * The harness holds the player index, the traffic count, the two float cells,
 * the traffic table, the shared counter, the three timing constants, and the
 * callee stubs.  The stubs record the call arguments.  The clear stub fills the
 * table the same way the original memset helper does.
 *
 * The harness runs four paths:
 *
 *     0  three traffic records: the player index plus one lands at
 *        DAT_0000cf40, the player car loads with 0xf91c and the mode argument,
 *        the two float cells take the count formula, the table clears with
 *        3 * 0x2a8 bytes, the step takes 0x50, the wait runs once, the three
 *        record pointers walk by 0x2a8, and the scene setup takes the mode
 *        argument
 *     1  the player car load fails: the function returns the error at once and
 *        every later step stays unrun
 *     2  the second record fails: the function returns the error at once, the
 *        counter holds 1, and the scene setup stays unrun
 *     3  the traffic count is zero: the table clears with a zero byte count, no
 *        record prepares, and the scene setup still runs
 *     D  the exit marker
 *
 * The float comparisons use a small tolerance, because the single-precision
 * cell need not land on the same bit pattern as the stated decimal value.
 */
#pragma off (check_stack)

int game_mode_10_start(int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011d250;
int DAT_0011d254;
int DAT_0000cf40;
int DAT_001100a4;
int DAT_00115218;
float DAT_0000d6e0;
float DAT_0000d6e4;
char DAT_00113cd8[0x2a8 * 16];
int DAT_0000634c;

double traffic_time_scale;
double traffic_time_bias;
double traffic_time_factor;

static char text[8];

static int init_calls;
static int init_offset;
static int init_index;
static int init_result;
static int clear_calls;
static void *clear_dest;
static unsigned int clear_count;
static int clear_value;
static int step_calls;
static int step_limit;
static int wait_calls;
static int record_calls;
static void *record_address[16];
static int record_fail_index;
static int record_fail_value;
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

int player_car_init(int offset, int index)
{
    init_calls = init_calls + 1;
    init_offset = offset;
    init_index = index;
    return init_result;
}

void memory_clear(void *dest, unsigned int count, int value)
{
    unsigned int *words;
    unsigned int pattern;
    unsigned int index;

    words = (unsigned int *)dest;
    pattern = ((unsigned int)(value & 0xff)) * 0x01010101u;
    for (index = 0; index < (count >> 2); index = index + 1) {
        words[index] = pattern;
    }
    clear_calls = clear_calls + 1;
    clear_dest = dest;
    clear_count = count;
    clear_value = value;
}

int traffic_step(int limit)
{
    step_calls = step_calls + 1;
    step_limit = limit;
    return 0;
}

void traffic_wait(void)
{
    wait_calls = wait_calls + 1;
}

void scene_setup(int value)
{
    scene_calls = scene_calls + 1;
    scene_value = value;
}

int traffic_record_init(void *record)
{
    record_address[record_calls] = record;
    record_calls = record_calls + 1;
    if (record_calls - 1 == record_fail_index) {
        return record_fail_value;
    }
    return 0;
}

static int almost(float value, float expected, float tolerance)
{
    float difference;

    difference = value - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference < tolerance;
}

static void reset(void)
{
    unsigned int index;

    DAT_0011d250 = 0;
    DAT_0011d254 = 0;
    DAT_0000cf40 = 0x11111111;
    DAT_001100a4 = 0x11111111;
    DAT_00115218 = 0x11111111;
    DAT_0000d6e0 = 1234.5f;
    DAT_0000d6e4 = 1234.5f;
    DAT_0000634c = 0x22222222;
    for (index = 0; index < 0x2a8 * 16; index = index + 1) {
        DAT_00113cd8[index] = 0x5a;
    }
    traffic_time_scale = 0.1;
    traffic_time_bias = -0.3;
    traffic_time_factor = 0.7;
    init_calls = 0;
    init_offset = 0;
    init_index = 0;
    init_result = 0;
    clear_calls = 0;
    clear_dest = 0;
    clear_count = 0;
    clear_value = -1;
    step_calls = 0;
    step_limit = 0;
    wait_calls = 0;
    record_calls = 0;
    record_fail_index = -1;
    record_fail_value = 0;
    scene_calls = 0;
    scene_value = -1;
    for (index = 0; index < 16; index = index + 1) {
        record_address[index] = 0;
    }
}

int main(void)
{
    int index;
    int result;
    int ok;

    reset();
    DAT_0011d250 = 5;
    DAT_0011d254 = 3;
    result = game_mode_10_start(0x22);
    ok = result == 0
        && DAT_0000cf40 == 6
        && init_calls == 1
        && init_offset == 0xf91c
        && init_index == 0x22
        && almost(DAT_0000d6e0, 0.17340579f, 0.001f)
        && almost(DAT_0000d6e4, 0.82659421f, 0.001f)
        && DAT_001100a4 == 3
        && DAT_00115218 == 3
        && clear_calls == 1
        && clear_dest == (void *)DAT_00113cd8
        && clear_count == 3 * 0x2a8
        && clear_value == 0
        && step_calls == 1
        && step_limit == 0x50
        && wait_calls == 1
        && DAT_0000634c == 3
        && record_calls == 3
        && scene_calls == 1
        && scene_value == 0x22
        && DAT_00113cd8[0] == 0
        && DAT_00113cd8[3 * 0x2a8 - 1] == 0
        && DAT_00113cd8[3 * 0x2a8] == 0x5a;
    for (index = 0; index < 3; index = index + 1) {
        ok = ok
            && record_address[index]
                == (void *)(DAT_00113cd8 + index * 0x2a8);
    }
    put(ok ? '0' : 'x');

    reset();
    DAT_0011d250 = 7;
    DAT_0011d254 = 3;
    init_result = 9;
    result = game_mode_10_start(0x33);
    ok = result == 9
        && DAT_0000cf40 == 8
        && init_calls == 1
        && init_offset == 0xf91c
        && init_index == 0x33
        && DAT_0000d6e0 == 1234.5f
        && DAT_0000d6e4 == 1234.5f
        && DAT_001100a4 == 0x11111111
        && DAT_00115218 == 0x11111111
        && clear_calls == 0
        && step_calls == 0
        && wait_calls == 0
        && record_calls == 0
        && scene_calls == 0;
    put(ok ? '1' : 'x');

    reset();
    DAT_0011d250 = 1;
    DAT_0011d254 = 3;
    record_fail_index = 1;
    record_fail_value = -1;
    result = game_mode_10_start(0x44);
    ok = result == -1
        && DAT_0000cf40 == 2
        && init_calls == 1
        && DAT_0000d6e0 != 1234.5f
        && clear_calls == 1
        && step_calls == 1
        && wait_calls == 1
        && DAT_0000634c == 1
        && record_calls == 2
        && record_address[0] == (void *)DAT_00113cd8
        && record_address[1] == (void *)(DAT_00113cd8 + 0x2a8)
        && scene_calls == 0;
    put(ok ? '2' : 'x');

    reset();
    DAT_0011d250 = 2;
    DAT_0011d254 = 0;
    result = game_mode_10_start(0x55);
    ok = result == 0
        && DAT_0000cf40 == 3
        && init_calls == 1
        && almost(DAT_0000d6e0, -0.21f, 0.001f)
        && almost(DAT_0000d6e4, 1.21f, 0.001f)
        && DAT_001100a4 == 0
        && DAT_00115218 == 0
        && clear_calls == 1
        && clear_count == 0
        && step_calls == 1
        && wait_calls == 1
        && DAT_0000634c == 0
        && record_calls == 0
        && scene_calls == 1
        && scene_value == 0x55;
    put(ok ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
