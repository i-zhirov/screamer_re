/* The test harness for the rebuilt `player_car_wheel_effect_update` module.
 *
 * The harness holds one player object, the eight 0x28-byte effect slots at
 * +0x308 through +0x448, the object entry table, the frame delta, the double
 * rate, and the four callee stubs.  The rotation stubs record the input pair
 * and the angle and write chosen values eight bytes apart.  The advance stub
 * records the table, the slot, and the delta and returns a chosen result.  The
 * bind stub records the table, the name, and the slot.
 *
 * The harness runs six paths:
 *
 *     0  no active slot: no callee runs and no slot changes
 *     1  one active slot: the two rotations fill the slot position, the object
 *        position adds the player position, and the advance result keeps the
 *        slot active
 *     2  the first and the last slot active: both objects take their own
 *        position and the inactive slots stay clear
 *     3  an advance result of -1: the slot clears and the bind helper runs
 *     4  the frame delta times the rate: the advance helper takes 3000.0
 *     5  the first advance keeps the slot and the second advance releases it
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void player_car_wheel_effect_update(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define SLOT_BASE 0x308
#define SLOT_STRIDE 0x28
#define SLOT_COUNT 8
#define ENTRY_STRIDE 28
#define ENTRY_COUNT 3

struct point3 {
    int x;
    int y;
    int z;
};

char DAT_0010653c[ENTRY_COUNT * ENTRY_STRIDE];
float DAT_00118e04;
double player_car_wheel_effect_rate;

static char text[8];
static char object[0x600];
static char display[ENTRY_COUNT][0x40];

static int heading_calls;
static int heading_x;
static int heading_z;
static unsigned int heading_angle;
static int pitch_calls;
static int pitch_y;
static int pitch_angle;
static unsigned int pitch_angle_bits;
static int advance_calls;
static int advance_result;
static unsigned int advance_table[SLOT_COUNT];
static unsigned int advance_slot[SLOT_COUNT];
static unsigned int advance_delta[SLOT_COUNT];
static int bind_calls;
static unsigned int bind_table[SLOT_COUNT];
static unsigned int bind_name[SLOT_COUNT];
static unsigned int bind_slot[SLOT_COUNT];

void rotate_heading(struct point3 *out, struct point3 *in, float angle)
{
    heading_calls = heading_calls + 1;
    heading_x = in->x;
    heading_z = in->z;
    heading_angle = *(unsigned int *)&angle;
    out->x = in->x + 1;
    out->z = in->z + 2;
}

void rotate_pitch(struct point3 *out, struct point3 *in, float angle)
{
    pitch_calls = pitch_calls + 1;
    pitch_y = in->y;
    pitch_angle_bits = *(unsigned int *)&angle;
    out->x = in->x;
    out->y = in->y + 3;
    out->z = in->z + 4;
}

int effect_slot_advance(void *table, void *slot, float delta)
{
    advance_table[advance_calls] = (unsigned int)table;
    advance_slot[advance_calls] = (unsigned int)slot;
    advance_delta[advance_calls] = *(unsigned int *)&delta;
    advance_calls = advance_calls + 1;
    return advance_result;
}

int object_list_bind(void *table, char *name, void *slot)
{
    bind_table[bind_calls] = (unsigned int)table;
    bind_name[bind_calls] = (unsigned int)name;
    bind_slot[bind_calls] = (unsigned int)slot;
    bind_calls = bind_calls + 1;
    return 0;
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

static unsigned int get_bits(char *base, int offset)
{
    return *(unsigned int *)(base + offset);
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

static char *slot_at(int index)
{
    return object + SLOT_BASE + index * SLOT_STRIDE;
}

static char *display_at(int index)
{
    return display[index];
}

static void reset(void)
{
    int index;

    clear_bytes(object, 0x600);
    for (index = 0; index < ENTRY_COUNT; index++) {
        clear_bytes(display[index], 0x40);
    }
    clear_bytes(DAT_0010653c, ENTRY_COUNT * ENTRY_STRIDE);
    for (index = 0; index < ENTRY_COUNT; index++) {
        *(char **)(DAT_0010653c + index * ENTRY_STRIDE) = display[index];
    }
    heading_calls = 0;
    heading_x = 0;
    heading_z = 0;
    heading_angle = 0;
    pitch_calls = 0;
    pitch_y = 0;
    pitch_angle_bits = 0;
    advance_calls = 0;
    advance_result = 0;
    bind_calls = 0;
    for (index = 0; index < SLOT_COUNT; index++) {
        advance_table[index] = 0;
        advance_slot[index] = 0;
        advance_delta[index] = 0;
        bind_table[index] = 0;
        bind_name[index] = 0;
        bind_slot[index] = 0;
    }
    set_bits((char *)&DAT_00118e04, 0, 0x3ca3d70a);
    set_double_bits(&player_car_wheel_effect_rate, 0, 0x3ff00000);
    set_bits(object, 0x4cc, 0x00000000);
    set_bits(object, 0x128, 0x00000000);
    set_bits(object, 0x4c8, 0x00000000);
    set_int(object, 0x4b0, 0);
    set_int(object, 0x4b4, 0);
    set_int(object, 0x4b8, 0);
}

int main(void)
{
    int index;
    int ok;

    reset();
    player_car_wheel_effect_update(object);
    ok = heading_calls == 0
        && pitch_calls == 0
        && advance_calls == 0
        && bind_calls == 0;
    for (index = 0; index < SLOT_COUNT; index++) {
        ok = ok && get_int(slot_at(index), 0x08) == 0;
    }
    put(ok ? '0' : 'x');

    reset();
    set_bits(object, 0x4cc, 0x3f800000);
    set_bits(object, 0x128, 0x3f000000);
    set_bits(object, 0x4c8, 0x40000000);
    set_int(object, 0x4b0, 0x1000);
    set_int(object, 0x4b4, 0x2000);
    set_int(object, 0x4b8, 0x3000);
    set_int(slot_at(0), 0x00, 0);
    set_int(slot_at(0), 0x08, 1);
    set_int(slot_at(0), 0x18, 100);
    set_int(slot_at(0), 0x1c, 0x7777);
    set_int(slot_at(0), 0x20, 200);
    player_car_wheel_effect_update(object);
    ok = heading_calls == 1
        && heading_x == 100
        && heading_z == 200
        && heading_angle == 0xbfc00000
        && pitch_calls == 1
        && pitch_y == 0
        && pitch_angle_bits == 0x40000000
        && get_int(slot_at(0), 0x0c) == 101
        && get_int(slot_at(0), 0x10) == 3
        && get_int(slot_at(0), 0x14) == 206
        && get_int(display_at(0), 0x08) == 101 + 0x1000
        && get_int(display_at(0), 0x0c) == 3 + 0x2000
        && get_int(display_at(0), 0x10) == 206 + 0x3000
        && advance_calls == 1
        && advance_table[0] == 0x6530
        && advance_slot[0] == (unsigned int)slot_at(0)
        && advance_delta[0] == 0x3ca3d70a
        && get_int(slot_at(0), 0x08) == 1
        && bind_calls == 0;
    put(ok ? '1' : 'x');

    reset();
    set_int(object, 0x4b0, 0x10);
    set_int(object, 0x4b4, 0x20);
    set_int(object, 0x4b8, 0x30);
    set_int(slot_at(0), 0x00, 1);
    set_int(slot_at(0), 0x08, 1);
    set_int(slot_at(0), 0x18, 10);
    set_int(slot_at(0), 0x20, 20);
    set_int(slot_at(7), 0x00, 2);
    set_int(slot_at(7), 0x08, 1);
    set_int(slot_at(7), 0x18, 30);
    set_int(slot_at(7), 0x20, 40);
    player_car_wheel_effect_update(object);
    ok = heading_calls == 2
        && pitch_calls == 2
        && advance_calls == 2
        && get_int(display_at(1), 0x08) == 11 + 0x10
        && get_int(display_at(1), 0x0c) == 3 + 0x20
        && get_int(display_at(1), 0x10) == 26 + 0x30
        && get_int(display_at(2), 0x08) == 31 + 0x10
        && get_int(display_at(2), 0x0c) == 3 + 0x20
        && get_int(display_at(2), 0x10) == 46 + 0x30
        && advance_slot[0] == (unsigned int)slot_at(0)
        && advance_slot[1] == (unsigned int)slot_at(7)
        && get_int(slot_at(1), 0x08) == 0
        && get_int(slot_at(6), 0x08) == 0
        && bind_calls == 0;
    put(ok ? '2' : 'x');

    reset();
    advance_result = -1;
    set_int(slot_at(2), 0x00, 0);
    set_int(slot_at(2), 0x08, 1);
    set_int(slot_at(2), 0x18, 5);
    set_int(slot_at(2), 0x20, 6);
    player_car_wheel_effect_update(object);
    ok = advance_calls == 1
        && bind_calls == 1
        && bind_table[0] == 0x6530
        && bind_name[0] == 0x6350
        && bind_slot[0] == (unsigned int)slot_at(2)
        && get_int(slot_at(2), 0x08) == 0
        && get_int(display_at(0), 0x08) == 6;
    put(ok ? '3' : 'x');

    reset();
    set_bits((char *)&DAT_00118e04, 0, 0x40400000);
    set_double_bits(&player_car_wheel_effect_rate, 0, 0x408f4000);
    set_int(slot_at(0), 0x00, 0);
    set_int(slot_at(0), 0x08, 1);
    player_car_wheel_effect_update(object);
    ok = advance_calls == 1
        && advance_delta[0] == 0x453b8000;
    put(ok ? '4' : 'x');

    reset();
    set_int(slot_at(0), 0x00, 1);
    set_int(slot_at(0), 0x08, 1);
    set_int(slot_at(3), 0x00, 2);
    set_int(slot_at(3), 0x08, 1);
    advance_result = 0;
    player_car_wheel_effect_update(object);
    ok = advance_calls == 2
        && bind_calls == 0
        && get_int(slot_at(0), 0x08) == 1
        && get_int(slot_at(3), 0x08) == 1;
    put(ok ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
