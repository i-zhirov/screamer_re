/* The test harness for the rebuilt `wheel_contact_update` module.
 *
 * The harness holds the player object with the four 0x54-byte wheel units, the
 * two contact globals, the ground height, the surface value, and the two callee
 * stubs.  The rotate stub records the input pair and the angle of every call
 * and writes a chosen result.  The collision stub records its call and returns
 * a chosen value.
 *
 * The harness runs six paths:
 *
 *     0  no collision: the two cosines, the four units, the eight rotations,
 *        the cleared wheel flag, the four contact pairs, and the unchanged
 *        ground and surface fields
 *     1  a collision with the surface value 0x65: the two ground heights and
 *        the even-unit surface bit set the wheel flag to 1
 *     2  a collision with another surface value: the surface flag stays 0 and
 *        the wheel flag stays 0
 *     3  a preset wheel flag 0x40: the clear discards it and the even-unit
 *        surface bit gives 1
 *     4  the two angles: the first rotation takes the negative yaw plus the
 *        delta and the second rotation takes the negative yaw
 *     5  the absolute cosine: the negative cosine of pi scales the offset by
 *        1, not by -1
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void wheel_contact_update(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define UNIT_BASE 0x194
#define UNIT_STRIDE 0x54
#define UNIT_COUNT 4

int DAT_0010c72c;
int DAT_00108024;
int DAT_00108038;
int DAT_00108040;

struct rotate_point {
    int x;
    int pad;
    int y;
};

static char text[8];
static char object[0x900];
static int rotate_calls;
static int rotate_in0[16];
static int rotate_in1[16];
static unsigned int rotate_angle[16];
static int rotate_out0;
static int rotate_out1;
static int collision_calls;
static int collision_result;

void rotate_pair(struct rotate_point *out, struct rotate_point *in,
    float angle)
{
    rotate_in0[rotate_calls] = in->x;
    rotate_in1[rotate_calls] = in->y;
    rotate_angle[rotate_calls] = *(unsigned int *)&angle;
    rotate_calls = rotate_calls + 1;
    out->x = rotate_out0;
    out->y = rotate_out1;
}

int map_collision_check(int value)
{
    collision_calls = collision_calls + 1;
    return collision_result;
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

static char *unit_at(int index)
{
    return object + UNIT_BASE + index * UNIT_STRIDE;
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

static void reset(void)
{
    int index;

    clear_bytes(object, 0x900);
    rotate_calls = 0;
    collision_calls = 0;
    rotate_out0 = 10;
    rotate_out1 = 20;
    collision_result = 0;
    for (index = 0; index < 16; index++) {
        rotate_in0[index] = 0;
        rotate_in1[index] = 0;
        rotate_angle[index] = 0;
    }
    set_bits(object, 0x4d0, 0x00000000);
    set_bits(object, 0x4c8, 0x00000000);
    set_bits(object, 0x4cc, 0x00000000);
    set_bits(object, 0x128, 0x00000000);
    set_int(object, 0x4b0, 100);
    set_int(object, 0x4b8, 200);
    set_int(object, 0x2f0, 0x0badf00d);
    DAT_0010c72c = 0x1234;
    DAT_00108024 = 0x65;
    DAT_00108038 = 0x11111111;
    DAT_00108040 = 0x22222222;
    for (index = 0; index < UNIT_COUNT; index++) {
        char *unit;

        unit = unit_at(index);
        set_int(unit, 0x18, index + 1);
        set_int(unit, 0x20, index + 2);
        set_int(unit, 0x00, 0x01010101);
        set_int(unit, 0x08, 0x08080808);
        set_int(unit, 0x0c, 0x0c0c0c0c);
        set_int(unit, 0x14, 0x14141414);
        set_int(unit, 0x24, 0x1111);
        set_int(unit, 0x28, 0x2222);
        set_int(unit, 0x2c, 0x99);
    }
}

int main(void)
{
    int index;
    int ok;

    reset();
    wheel_contact_update(object);
    ok = rotate_calls == 8
        && collision_calls == 8
        && get_int(object, 0x2f0) == 0
        && DAT_00108038 == 110
        && DAT_00108040 == 220
        && rotate_in0[6] == 4
        && rotate_in1[6] == 5
        && rotate_in0[7] == 4
        && rotate_in1[7] == 5;
    for (index = 0; index < UNIT_COUNT; index++) {
        char *unit;

        unit = unit_at(index);
        ok = ok
            && get_int(unit, 0x00) == 110
            && get_int(unit, 0x08) == 220
            && get_int(unit, 0x0c) == 110
            && get_int(unit, 0x14) == 220
            && get_int(unit, 0x24) == 0x1111
            && get_int(unit, 0x28) == 0x2222
            && get_int(unit, 0x2c) == 0x99;
    }
    put(ok ? '0' : 'x');

    reset();
    collision_result = 1;
    wheel_contact_update(object);
    ok = get_int(object, 0x2f0) == 1;
    for (index = 0; index < UNIT_COUNT; index++) {
        char *unit;

        unit = unit_at(index);
        ok = ok
            && get_int(unit, 0x24) == 0x1234
            && get_int(unit, 0x28) == 0x1234
            && get_int(unit, 0x2c) == 1;
    }
    put(ok ? '1' : 'x');

    reset();
    collision_result = 1;
    DAT_00108024 = 0x11;
    wheel_contact_update(object);
    ok = get_int(object, 0x2f0) == 0;
    for (index = 0; index < UNIT_COUNT; index++) {
        char *unit;

        unit = unit_at(index);
        ok = ok
            && get_int(unit, 0x24) == 0x1234
            && get_int(unit, 0x28) == 0x1234
            && get_int(unit, 0x2c) == 0;
    }
    put(ok ? '2' : 'x');

    reset();
    collision_result = 1;
    set_int(object, 0x2f0, 0x40);
    wheel_contact_update(object);
    put(get_int(object, 0x2f0) == 1 ? '3' : 'x');

    reset();
    set_bits(object, 0x4cc, 0x3fc00000);
    set_bits(object, 0x128, 0x3e800000);
    wheel_contact_update(object);
    ok = rotate_angle[0] == 0xbfe00000
        && rotate_angle[1] == 0xbfc00000
        && rotate_angle[6] == 0xbfe00000
        && rotate_angle[7] == 0xbfc00000;
    put(ok ? '4' : 'x');

    reset();
    set_bits(object, 0x4d0, 0x40490fdb);
    set_int(unit_at(0), 0x18, 7);
    set_int(unit_at(0), 0x20, 9);
    wheel_contact_update(object);
    put(rotate_in0[0] == 7 && rotate_in1[0] == 9 ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
