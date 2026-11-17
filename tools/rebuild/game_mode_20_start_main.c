/* The test harness for the rebuilt `game_mode_20_start` module.
 *
 * The harness holds the opponent count, the opponent records, the three state
 * cells, and the two callee stubs.  The stubs record the call arguments.
 *
 * The harness runs these paths:
 *
 *     0  the count 2: the first two started flags clear, the third record
 *        keeps its value, and the other record bytes keep their value
 *     1  the count 0: no started flag changes, and the state and the car and
 *        scene calls still run
 *     2  the count 1 with the mode 0: only the first started flag clears
 *     3  the count 3 with the mode -1: the first three flags clear and the
 *        fourth flag keeps its value
 *     4  the player car load fails with 9: the function returns 9 and the
 *        scene setup stays unrun
 *     5  the player car load fails with -3: the function returns -3
 *     6  the count 0 with a failed car load: the function returns 5 and the
 *        scene setup stays unrun
 *     7  the mode 0x100: the car load and the scene setup take the mode
 *     8  the count 4: the first four flags clear and the fifth flag keeps its
 *        value
 *     D  the exit marker
 */
#pragma off (check_stack)

int game_mode_20_start(int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0010d6f8;
char DAT_0010d700[0x40 * 6];
int DAT_001100d4;
int DAT_0000cf40;
int DAT_001100a4;

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

static int field(int index)
{
    return *(int *)(DAT_0010d700 + index * 0x40 + 0x3c);
}

static void set_field(int index, int value)
{
    *(int *)(DAT_0010d700 + index * 0x40 + 0x3c) = value;
}

static void reset(void)
{
    int index;

    DAT_0010d6f8 = 0;
    DAT_001100d4 = 0x11111111;
    DAT_0000cf40 = 0x22222222;
    DAT_001100a4 = 0x33333333;
    for (index = 0; index < 0x40 * 6; index = index + 1) {
        DAT_0010d700[index] = 0x5A;
    }
    init_calls = 0;
    init_car = 0;
    init_load = -1;
    init_result = 0;
    scene_calls = 0;
    scene_value = -1;
}

static int state_ok(void)
{
    return DAT_001100d4 == 0
        && DAT_0000cf40 == 1
        && DAT_001100a4 == 0;
}

int main(void)
{
    int result;
    int ok;

    reset();
    DAT_0010d6f8 = 2;
    set_field(0, -1);
    set_field(1, 0x12345678);
    set_field(2, 0x0BADF00D);
    result = game_mode_20_start(0x21);
    ok = result == 0
        && state_ok()
        && field(0) == 0
        && field(1) == 0
        && field(2) == 0x0BADF00D
        && DAT_0010d700[0] == 0x5A
        && DAT_0010d700[0x40 + 0x38] == 0x5A
        && init_calls == 1
        && init_car == (void *)0xf91c
        && init_load == 0x21
        && scene_calls == 1
        && scene_value == 0x21;
    put(ok ? '0' : 'x');

    reset();
    DAT_0010d6f8 = 0;
    set_field(0, 0x11111111);
    result = game_mode_20_start(0x22);
    ok = result == 0
        && state_ok()
        && field(0) == 0x11111111
        && init_calls == 1
        && init_car == (void *)0xf91c
        && init_load == 0x22
        && scene_calls == 1
        && scene_value == 0x22;
    put(ok ? '1' : 'x');

    reset();
    DAT_0010d6f8 = 1;
    set_field(0, 0x22222222);
    set_field(1, 0x33333333);
    result = game_mode_20_start(0);
    ok = result == 0
        && state_ok()
        && field(0) == 0
        && field(1) == 0x33333333
        && init_calls == 1
        && init_load == 0
        && scene_calls == 1
        && scene_value == 0;
    put(ok ? '2' : 'x');

    reset();
    DAT_0010d6f8 = 3;
    set_field(0, 0x01010101);
    set_field(1, 0x02020202);
    set_field(2, 0x03030303);
    set_field(3, 0x04040404);
    result = game_mode_20_start(-1);
    ok = result == 0
        && state_ok()
        && field(0) == 0
        && field(1) == 0
        && field(2) == 0
        && field(3) == 0x04040404
        && init_calls == 1
        && init_load == -1
        && scene_calls == 1
        && scene_value == -1;
    put(ok ? '3' : 'x');

    reset();
    DAT_0010d6f8 = 2;
    set_field(0, 0x55555555);
    set_field(1, 0x66666666);
    init_result = 9;
    result = game_mode_20_start(0x24);
    ok = result == 9
        && state_ok()
        && field(0) == 0
        && field(1) == 0
        && init_calls == 1
        && init_car == (void *)0xf91c
        && init_load == 0x24
        && scene_calls == 0;
    put(ok ? '4' : 'x');

    reset();
    DAT_0010d6f8 = 2;
    init_result = -3;
    result = game_mode_20_start(0x25);
    ok = result == -3
        && state_ok()
        && field(0) == 0
        && field(1) == 0
        && init_calls == 1
        && init_load == 0x25
        && scene_calls == 0;
    put(ok ? '5' : 'x');

    reset();
    DAT_0010d6f8 = 0;
    set_field(0, 0x77777777);
    init_result = 5;
    result = game_mode_20_start(0x26);
    ok = result == 5
        && state_ok()
        && field(0) == 0x77777777
        && init_calls == 1
        && init_load == 0x26
        && scene_calls == 0;
    put(ok ? '6' : 'x');

    reset();
    DAT_0010d6f8 = 2;
    result = game_mode_20_start(0x100);
    ok = result == 0
        && state_ok()
        && field(0) == 0
        && field(1) == 0
        && init_calls == 1
        && init_load == 0x100
        && scene_calls == 1
        && scene_value == 0x100;
    put(ok ? '7' : 'x');

    reset();
    DAT_0010d6f8 = 4;
    set_field(0, 0x0A0A0A0A);
    set_field(1, 0x0B0B0B0B);
    set_field(2, 0x0C0C0C0C);
    set_field(3, 0x0D0D0D0D);
    set_field(4, 0x0E0E0E0E);
    result = game_mode_20_start(0x1234);
    ok = result == 0
        && state_ok()
        && field(0) == 0
        && field(1) == 0
        && field(2) == 0
        && field(3) == 0
        && field(4) == 0x0E0E0E0E
        && init_calls == 1
        && init_load == 0x1234
        && scene_calls == 1
        && scene_value == 0x1234;
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
