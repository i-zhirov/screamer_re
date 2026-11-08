/* The test harness for the rebuilt `player_car_wheel_effect_start` module.
 *
 * The harness holds one player object, the four 0x54-byte wheel units at
 * +0x194, the eight 0x28-byte effect slots at +0x308 through +0x448, the rate
 * timer at +0x304, the frame counter, the player position, the frame delta,
 * the 0.05 limit, and the two helper stubs.  The helper stubs record the call
 * arguments.
 *
 * The harness runs six paths:
 *
 *     0  a timer below 0.05 takes the frame delta 0.02 and returns, so the
 *        timer becomes 0.03 and no slot and no helper runs
 *     1  a timer at the limit clears to zero, fills the first two slots, and
 *        runs both helpers on each slot; the zero dither keeps the wheel
 *        offsets 0x101, 0x201 and 0x103, 0x203
 *     2  a timer above the limit with the frame counter 2 gives the dither
 *        10 and 4, so the four positions become 0x10b, 0x205 and 0xf9, 0x207
 *     3  the slots 0 through 6 active fill only slot 7 once
 *     4  the slots 0 and 2 through 7 active fill only slot 1 once
 *     5  all eight slots active fill nothing and only clear the timer
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void player_car_wheel_effect_start(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define SLOT_BASE 0x308
#define SLOT_STRIDE 0x28
#define SLOT_COUNT 8
#define UNIT_BASE 0x194
#define UNIT_STRIDE 0x54

double player_car_wheel_effect_interval;

int DAT_000007fd;
int DAT_0010fdcc;
int DAT_0010fdd4;
float DAT_00118e08;

static char text[8];
static char object[0x600];

static int frame_calls;
static unsigned int frame_table[4];
static unsigned int frame_slot[4];
static unsigned int frame_value[4];
static int refresh_calls;
static unsigned int refresh_table[4];
static unsigned int refresh_name[4];
static unsigned int refresh_slot[4];

void object_frame_set(void *table, void *slot, float value)
{
    frame_table[frame_calls] = (unsigned int)table;
    frame_slot[frame_calls] = (unsigned int)slot;
    frame_value[frame_calls] = *(unsigned int *)&value;
    frame_calls = frame_calls + 1;
}

void object_list_refresh(void *table, char *name, void *slot)
{
    refresh_table[refresh_calls] = (unsigned int)table;
    refresh_name[refresh_calls] = (unsigned int)name;
    refresh_slot[refresh_calls] = (unsigned int)slot;
    refresh_calls = refresh_calls + 1;
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

static void clear_bytes(char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static void set_int(char *base, int offset, int value)
{
    *(int *)(base + offset) = value;
}

static int get_int(char *base, int offset)
{
    return *(int *)(base + offset);
}

static void set_bits(char *base, int offset, unsigned int bits)
{
    *(unsigned int *)(base + offset) = bits;
}

static char *slot_at(int index)
{
    return object + SLOT_BASE + index * SLOT_STRIDE;
}

static char *unit_at(int index)
{
    return object + UNIT_BASE + index * UNIT_STRIDE;
}

static void set_double_bits(double *p, unsigned int lo, unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = lo;
    value.u[1] = hi;
    *p = value.d;
}

static void reset(void)
{
    int index;

    clear_bytes(object, 0x600);
    frame_calls = 0;
    refresh_calls = 0;
    for (index = 0; index < 4; index++) {
        frame_table[index] = 0;
        frame_slot[index] = 0;
        frame_value[index] = 0;
        refresh_table[index] = 0;
        refresh_name[index] = 0;
        refresh_slot[index] = 0;
    }
    DAT_000007fd = 0;
    DAT_0010fdcc = 13;
    DAT_0010fdd4 = 10;
    set_bits((char *)&DAT_00118e08, 0, 0x3ca3d70a);
    for (index = 0; index < 4; index++) {
        set_int(unit_at(index), 0x18, 0x100 + index);
        set_int(unit_at(index), 0x20, 0x200 + index);
    }
}

int main(void)
{
    int index;
    int ok;

    set_double_bits(&player_car_wheel_effect_interval, 0x9999999a, 0x3fa99999);

    reset();
    set_bits(object, 0x304, 0x3c23d70a);
    player_car_wheel_effect_start(object);
    ok = get_int(object, 0x304) == 0x3cf5c28f
        && frame_calls == 0
        && refresh_calls == 0;
    for (index = 0; index < SLOT_COUNT; index++) {
        ok = ok && get_int(slot_at(index), 0x08) == 0;
    }
    put(ok ? '0' : 'x');

    reset();
    set_bits(object, 0x304, 0x3d4ccccd);
    player_car_wheel_effect_start(object);
    ok = get_int(object, 0x304) == 0
        && frame_calls == 2
        && refresh_calls == 2
        && get_int(slot_at(0), 0x08) == 1
        && get_int(slot_at(1), 0x08) == 1
        && get_int(slot_at(0), 0x18) == 0x101
        && get_int(slot_at(0), 0x20) == 0x201
        && get_int(slot_at(1), 0x18) == 0x103
        && get_int(slot_at(1), 0x20) == 0x203
        && frame_table[0] == 0x6530
        && frame_table[1] == 0x6530
        && frame_slot[0] == (unsigned int)slot_at(0)
        && frame_slot[1] == (unsigned int)slot_at(1)
        && frame_value[0] == 0
        && frame_value[1] == 0
        && refresh_table[0] == 0x6530
        && refresh_name[0] == 0x6350
        && refresh_slot[0] == (unsigned int)slot_at(0)
        && refresh_slot[1] == (unsigned int)slot_at(1);
    put(ok ? '1' : 'x');

    reset();
    set_bits(object, 0x304, 0x40a00000);
    DAT_000007fd = 2;
    player_car_wheel_effect_start(object);
    ok = get_int(object, 0x304) == 0
        && frame_calls == 2
        && refresh_calls == 2
        && get_int(slot_at(0), 0x18) == 0x10b
        && get_int(slot_at(0), 0x20) == 0x205
        && get_int(slot_at(1), 0x18) == 0xf9
        && get_int(slot_at(1), 0x20) == 0x207;
    put(ok ? '2' : 'x');

    reset();
    set_bits(object, 0x304, 0x3d4ccccd);
    for (index = 0; index < 7; index++) {
        set_int(slot_at(index), 0x08, 1);
    }
    player_car_wheel_effect_start(object);
    ok = frame_calls == 1
        && refresh_calls == 1
        && get_int(slot_at(7), 0x08) == 1
        && get_int(slot_at(7), 0x18) == 0x101
        && get_int(slot_at(7), 0x20) == 0x201;
    put(ok ? '3' : 'x');

    reset();
    set_bits(object, 0x304, 0x3d4ccccd);
    set_int(slot_at(0), 0x08, 1);
    for (index = 2; index < SLOT_COUNT; index++) {
        set_int(slot_at(index), 0x08, 1);
    }
    player_car_wheel_effect_start(object);
    ok = frame_calls == 1
        && refresh_calls == 1
        && get_int(slot_at(1), 0x08) == 1
        && get_int(slot_at(1), 0x18) == 0x101
        && get_int(slot_at(1), 0x20) == 0x201;
    put(ok ? '4' : 'x');

    reset();
    set_bits(object, 0x304, 0x3d4ccccd);
    for (index = 0; index < SLOT_COUNT; index++) {
        set_int(slot_at(index), 0x08, 1);
    }
    player_car_wheel_effect_start(object);
    ok = get_int(object, 0x304) == 0
        && frame_calls == 0
        && refresh_calls == 0;
    for (index = 0; index < SLOT_COUNT; index++) {
        ok = ok && get_int(slot_at(index), 0x08) == 1;
    }
    put(ok ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
