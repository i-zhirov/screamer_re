/* The test harness for the rebuilt `game_mode_100_start` module.
 *
 * The harness holds the credits state cells, the twenty line positions and
 * cells, the credit page table, the car records, the car count and slot
 * counter, the frame flags, the frame counter, the two float cells, and the
 * three callee stubs.  The stubs record the call arguments.
 *
 * The harness runs four paths:
 *
 *     0  three cars load with the load flag 1: every state cell clears, the
 *        two float cells take -320.0 and 1.0, the twenty positions take 0x15e,
 *        the twenty cells take 0x64, the car records walk by 0x590, the credit
 *        member loads once, the frame flags set, and the scene setup takes the
 *        mode argument
 *     1  the second car fails: the function returns the error at once, the
 *        counter holds 1, and the credit member, the frame flags, and the
 *        scene setup stay unrun
 *     2  the car count is zero: no car loads, but the credit member, the frame
 *        flags, and the scene setup still run
 *     3  the input word holds 0x0300: the function sets 0x2001 and clears bit
 *        1 of the high byte, so the word becomes 0x2101
 *     D  the exit marker
 *
 * The float cells take their bit patterns, because a float literal in this
 * minimal image reads the constant segment, which this harness does not load.
 */
#pragma off (check_stack)

typedef union {
    unsigned short bits;
    unsigned char bytes[2];
} control_flags;

int game_mode_100_start(int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011ce3c;
int DAT_0011ce40;
int DAT_0011ce44;
unsigned int DAT_0011ce48;
unsigned int DAT_0011ce4c;
int DAT_0011ce50;
int DAT_0011ce58[24];
int DAT_0011cea8[24];
char *DAT_0011cef8[8];
int DAT_0011d218;
int DAT_0011d21c;
int DAT_0011d220;
unsigned int DAT_0011d224;
int DAT_0011d228;
control_flags DAT_00118e1c;
int DAT_00001e04;
int DAT_000007fd;
int DAT_0000634c;
char DAT_00010ab8[0x590 * 4];
int DAT_0000f8ec;
int DAT_0000f8f0;
int DAT_0010a18;
int DAT_0010a1c;
int DAT_0010a80;
int DAT_0010a94;
int DAT_0010a9c;

static char text[8];

static int car_calls;
static unsigned int car_record[8];
static int car_index[8];
static int car_load[8];
static int car_fail_index;
static int car_fail_value;
static int member_calls;
static char *member_name;
static int *member_array;
static void *member_result;
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

void *member_list_load(char *name, int *array)
{
    member_name = name;
    member_array = array;
    member_calls = member_calls + 1;
    return member_result;
}

void scene_setup(int value)
{
    scene_calls = scene_calls + 1;
    scene_value = value;
}

static void reset(void)
{
    int index;

    DAT_0011ce3c = 0x11111111;
    DAT_0011ce40 = 0x11111111;
    DAT_0011ce44 = 0x11111111;
    DAT_0011ce48 = 0x11111111u;
    DAT_0011ce4c = 0x11111111u;
    DAT_0011ce50 = 0x11111111;
    DAT_0011d218 = 0x11111111;
    DAT_0011d21c = 0x11111111;
    DAT_0011d220 = 0x11111111;
    DAT_0011d224 = 0x11111111u;
    DAT_0011d228 = 0x11111111;
    DAT_00118e1c.bits = 0;
    DAT_00001e04 = 0x22222222;
    DAT_000007fd = 0x13579bdf;
    DAT_0000634c = 0x22222222;
    DAT_0000f8ec = 0;
    DAT_0000f8f0 = 0x22222222;
    DAT_0010a18 = 0x22222222;
    DAT_0010a1c = 0x22222222;
    DAT_0010a80 = 0x22222222;
    DAT_0010a94 = 0x22222222;
    DAT_0010a9c = 0x22222222;
    for (index = 0; index < 24; index++) {
        DAT_0011ce58[index] = -1;
        DAT_0011cea8[index] = -1;
    }
    for (index = 0; index < 8; index++) {
        DAT_0011cef8[index] = (char *)0xdeadbeef;
    }
    car_calls = 0;
    car_fail_index = -1;
    car_fail_value = 0;
    member_calls = 0;
    member_name = 0;
    member_array = 0;
    member_result = (void *)0x12345678;
    scene_calls = 0;
    scene_value = -1;
    for (index = 0; index < 8; index++) {
        car_record[index] = 0;
        car_index[index] = 0;
        car_load[index] = 0;
    }
}

static int check_initial(void)
{
    int index;
    int ok;

    ok = DAT_0011ce44 == 0
        && DAT_0011ce48 == 0
        && DAT_0011ce40 == 0
        && DAT_0011ce3c == 0
        && DAT_0011d220 == 0
        && DAT_0011d21c == 0
        && DAT_0011ce4c == 0xc3a00000u
        && DAT_0011ce50 == 0
        && DAT_0011d224 == 0x3f800000u
        && DAT_0000f8f0 == 0;
    for (index = 0; index < 0x14; index++) {
        ok = ok && DAT_0011ce58[index] == 0x15e;
        ok = ok && DAT_0011cea8[index] == 0x64;
    }
    return ok;
}

static int check_values(void)
{
    return DAT_00001e04 == 1
        && DAT_0010a9c == 1
        && DAT_0010a80 == -1
        && DAT_0010a18 == 0
        && DAT_0010a1c == -1
        && DAT_0010a94 == 0
        && DAT_0011d228 == 0x13579bdf;
}

int main(void)
{
    int index;
    int result;
    int ok;

    reset();
    DAT_0000f8ec = 3;
    result = game_mode_100_start(0x55);
    ok = result == 0
        && check_initial()
        && check_values()
        && DAT_0000634c == 3
        && DAT_00118e1c.bits == 0x2001
        && car_calls == 3
        && member_calls == 1
        && scene_calls == 1
        && scene_value == 0x55
        && DAT_0011d218 == (int)member_result
        && member_name == (char *)0x47b
        && member_array == (int *)DAT_0011cef8;
    for (index = 0; index < 3; index++) {
        ok = ok
            && car_record[index]
                == (unsigned int)(DAT_00010ab8 + index * 0x590)
            && car_index[index] == index
            && car_load[index] == 1;
    }
    put(ok ? '0' : 'x');

    reset();
    DAT_0000f8ec = 3;
    car_fail_index = 1;
    car_fail_value = 7;
    result = game_mode_100_start(9);
    ok = result == 7
        && check_initial()
        && DAT_0000634c == 1
        && car_calls == 2
        && member_calls == 0
        && scene_calls == 0
        && DAT_0011d218 == 0x11111111
        && DAT_0010a9c == 0x22222222
        && DAT_00001e04 == 0x22222222
        && DAT_00118e1c.bits == 0;
    put(ok ? '1' : 'x');

    reset();
    DAT_0000f8ec = 0;
    result = game_mode_100_start(0);
    ok = result == 0
        && check_initial()
        && check_values()
        && DAT_0000634c == 0
        && DAT_00118e1c.bits == 0x2001
        && car_calls == 0
        && member_calls == 1
        && scene_calls == 1
        && scene_value == 0
        && DAT_0011d218 == (int)member_result;
    put(ok ? '2' : 'x');

    reset();
    DAT_0000f8ec = 1;
    DAT_00118e1c.bits = 0x0300;
    result = game_mode_100_start(0x2a);
    ok = result == 0
        && DAT_00118e1c.bits == 0x2101
        && car_calls == 1
        && member_calls == 1
        && scene_calls == 1
        && scene_value == 0x2a;
    put(ok ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
