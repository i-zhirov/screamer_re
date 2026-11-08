/* The test harness for the rebuilt `body_point_collision_detect` module.
 *
 * The harness holds the player object with the four 0x2c-byte body slots, the
 * two position globals, the two cosines, the yaw, the two view offsets, and the
 * two callee stubs.  The rotate stub records the input pair and the angle of
 * every call and writes a chosen result.  The collision stub records its call
 * and returns a scripted value.
 *
 * The harness runs six paths:
 *
 *     0  no contact: the two cosines, the four slots, the four rotations, the
 *        cleared body flag, the four screen positions, and the clear slot flags
 *     1  a contact on every slot: each slot flag takes 1 and the body flag
 *        takes 1
 *     2  a contact on the first and the third slot: the two clear flags stay 0
 *        and the body flag takes 1
 *     3  the rotation angle: every call takes the negative yaw
 *     4  the absolute cosine: the negative cosine of pi scales the offset by
 *        1, not by -1
 *     5  the loop bound: the four slots change and the bytes at +0x104 stay
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void body_point_collision_detect(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define SLOT_BASE 0x54
#define SLOT_STRIDE 0x2c
#define SLOT_COUNT 4

int DAT_00108038;
int DAT_00108040;

struct rotate_point {
    int x;
    int pad;
    int z;
};

static char text[8];
static char object[0x200];
static int rotate_calls;
static int rotate_in_x[SLOT_COUNT];
static int rotate_in_z[SLOT_COUNT];
static unsigned int rotate_angle[SLOT_COUNT];
static int rotate_out_x;
static int rotate_out_z;
static int collision_calls;
static int collision_mode[SLOT_COUNT];
static int collision_result[SLOT_COUNT];

void rotate_point(struct rotate_point *out, struct rotate_point *in,
    float angle)
{
    rotate_in_x[rotate_calls] = in->x;
    rotate_in_z[rotate_calls] = in->z;
    rotate_angle[rotate_calls] = *(unsigned int *)&angle;
    rotate_calls = rotate_calls + 1;
    out->x = rotate_out_x;
    out->z = rotate_out_z;
}

int map_collision_check(int mode)
{
    collision_mode[collision_calls] = mode;
    collision_calls = collision_calls + 1;
    return collision_result[collision_calls - 1];
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

static char *slot_at(int index)
{
    return object + SLOT_BASE + index * SLOT_STRIDE;
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

    clear_bytes(object, 0x200);
    rotate_calls = 0;
    collision_calls = 0;
    rotate_out_x = 10;
    rotate_out_z = 20;
    for (index = 0; index < SLOT_COUNT; index++) {
        char *slot;

        rotate_in_x[index] = 0;
        rotate_in_z[index] = 0;
        rotate_angle[index] = 0;
        collision_mode[index] = 0;
        collision_result[index] = 1;
        slot = slot_at(index);
        set_int(slot, 0x00, index + 1);
        set_int(slot, 0x08, index + 2);
        set_int(slot, 0x0c, 0x0c0c0c0c);
        set_int(slot, 0x14, 0x14141414);
        set_int(slot, 0x18, 0x99);
    }
    set_bits(object, 0x4d0, 0x00000000);
    set_bits(object, 0x4c8, 0x00000000);
    set_bits(object, 0x4cc, 0x00000000);
    set_int(object, 0x4b0, 100);
    set_int(object, 0x4b8, 200);
    set_int(object, 0x40, 0x0badf00d);
    DAT_00108038 = 0x11111111;
    DAT_00108040 = 0x22222222;
}

int main(void)
{
    int index;
    int ok;

    reset();
    body_point_collision_detect(object);
    ok = rotate_calls == SLOT_COUNT
        && collision_calls == SLOT_COUNT
        && get_int(object, 0x40) == 0
        && DAT_00108038 == 110
        && DAT_00108040 == 220;
    for (index = 0; index < SLOT_COUNT; index++) {
        char *slot;

        slot = slot_at(index);
        ok = ok
            && collision_mode[index] == 1
            && rotate_in_x[index] == index + 1
            && rotate_in_z[index] == index + 2
            && get_int(slot, 0x0c) == 110
            && get_int(slot, 0x14) == 220
            && get_int(slot, 0x18) == 0;
    }
    put(ok ? '0' : 'x');

    reset();
    for (index = 0; index < SLOT_COUNT; index++) {
        collision_result[index] = 0;
    }
    body_point_collision_detect(object);
    ok = get_int(object, 0x40) == 1;
    for (index = 0; index < SLOT_COUNT; index++) {
        ok = ok && get_int(slot_at(index), 0x18) == 1;
    }
    put(ok ? '1' : 'x');

    reset();
    collision_result[0] = 0;
    collision_result[2] = 0;
    body_point_collision_detect(object);
    ok = get_int(object, 0x40) == 1
        && get_int(slot_at(0), 0x18) == 1
        && get_int(slot_at(1), 0x18) == 0
        && get_int(slot_at(2), 0x18) == 1
        && get_int(slot_at(3), 0x18) == 0;
    put(ok ? '2' : 'x');

    reset();
    set_bits(object, 0x4cc, 0x3fc00000);
    body_point_collision_detect(object);
    ok = rotate_calls == SLOT_COUNT;
    for (index = 0; index < SLOT_COUNT; index++) {
        ok = ok && rotate_angle[index] == 0xbfc00000;
    }
    put(ok ? '3' : 'x');

    reset();
    set_bits(object, 0x4d0, 0x40490fdb);
    set_int(slot_at(0), 0x00, 7);
    set_int(slot_at(0), 0x08, 9);
    body_point_collision_detect(object);
    put(rotate_in_x[0] == 7 && rotate_in_z[0] == 9 ? '4' : 'x');

    reset();
    set_int(object, 0x104, 0xaaaaaaaa);
    set_int(object, 0x108, 0xbbbbbbbb);
    set_int(object, 0x10c, 0xcccccccc);
    body_point_collision_detect(object);
    ok = rotate_calls == SLOT_COUNT
        && get_int(object, 0x104) == 0xaaaaaaaa
        && get_int(object, 0x108) == 0xbbbbbbbb
        && get_int(object, 0x10c) == 0xcccccccc;
    put(ok ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
