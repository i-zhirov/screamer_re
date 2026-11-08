/* The test harness for the rebuilt `player_segment_update` module.
 *
 * The harness holds one player object, the four 0x2c-byte slots at `+0x54`
 * through `+0x104`, the two position globals, the nearest-segment pointer,
 * one segment entry, and the two callee stubs.  The rotation stub records the
 * input pair and the angle and writes chosen values eight bytes apart.  The
 * nearest-segment stub records the mode and returns a chosen result.
 *
 * The harness runs six paths:
 *
 *     0  the flag +0x40 is clear: no callee runs and no slot changes
 *     1  the flag is set and every clear flag is clear: the slots clear the
 *        correction fields and the search does not run
 *     2  one active slot and a search miss: the position reaches the globals
 *        and the slot correction stays
 *     3  one active slot and a search hit with a non-negative local x: the
 *        side term is -30 and the slot takes the two rotations
 *     4  the last slot and a negative local x: the side term is +30 and the
 *        last slot takes the two rotations
 *     5  the first and the last slot active with a hit and a miss: the two
 *        searches run and the second slot correction stays
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.  The float checks compare the bit patterns exactly.
 */
#pragma off (check_stack)

void player_segment_update(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define SLOT_BASE 0x54
#define SLOT_STRIDE 0x2c
#define SLOT_COUNT 4

struct point3 {
    int x;
    int y;
    int z;
};

struct segment {
    float angle;
    int x;
    int y;
};

int DAT_00108038;
int DAT_00108040;
char *DAT_00108044;

static char text[8];
static char object[0x200];
static struct segment segment;

static int nearest_calls;
static int nearest_mode[SLOT_COUNT];
static int nearest_result[SLOT_COUNT];
static int rotate_calls;
static int rotate_in_x[SLOT_COUNT * 2];
static int rotate_in_z[SLOT_COUNT * 2];
static unsigned int rotate_angle[SLOT_COUNT * 2];

int track_nearest_node(int mode)
{
    nearest_mode[nearest_calls] = mode;
    DAT_00108044 = (char *)&segment;
    return nearest_result[nearest_calls++];
}

void rotate_point(struct point3 *out, struct point3 *in, float angle)
{
    rotate_in_x[rotate_calls] = in->x;
    rotate_in_z[rotate_calls] = in->z;
    rotate_angle[rotate_calls] = *(unsigned int *)&angle;
    if ((rotate_calls & 1) == 0) {
        out->x = in->x;
        out->z = in->z + 200;
    } else {
        out->x = in->x + 10;
        out->z = in->z + 20;
    }
    rotate_calls = rotate_calls + 1;
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

static char *slot_at(int index)
{
    return object + SLOT_BASE + index * SLOT_STRIDE;
}

static void reset(void)
{
    int index;

    clear_bytes(object, 0x200);
    DAT_00108038 = 0;
    DAT_00108040 = 0;
    DAT_00108044 = 0;
    set_bits((char *)&segment.angle, 0, 0);
    segment.x = 0;
    segment.y = 0;
    nearest_calls = 0;
    rotate_calls = 0;
    for (index = 0; index < SLOT_COUNT; index++) {
        nearest_mode[index] = 0;
        nearest_result[index] = 0;
    }
    for (index = 0; index < SLOT_COUNT * 2; index++) {
        rotate_in_x[index] = 0;
        rotate_in_z[index] = 0;
        rotate_angle[index] = 0;
    }
}

int main(void)
{
    int ok;

    reset();
    set_int(object, 0x40, 0);
    set_int(slot_at(0), 0x18, 1);
    set_int(slot_at(0), 0x1c, 0x1111);
    player_segment_update(object);
    ok = nearest_calls == 0
        && rotate_calls == 0
        && get_int(slot_at(0), 0x1c) == 0x1111;
    put(ok ? '0' : 'x');

    reset();
    set_int(object, 0x40, 1);
    set_int(slot_at(0), 0x1c, 0x2222);
    set_int(slot_at(0), 0x24, 0x3333);
    set_bits(slot_at(0), 0x28, 0x44444444);
    player_segment_update(object);
    ok = nearest_calls == 0
        && rotate_calls == 0
        && get_int(slot_at(0), 0x1c) == 0
        && get_int(slot_at(0), 0x24) == 0
        && get_bits(slot_at(0), 0x28) == 0;
    put(ok ? '1' : 'x');

    reset();
    set_int(object, 0x40, 1);
    set_int(slot_at(0), 0x0c, 1000);
    set_int(slot_at(0), 0x14, 2000);
    set_int(slot_at(0), 0x18, 1);
    set_int(slot_at(0), 0x1c, 0x5555);
    nearest_result[0] = 0;
    player_segment_update(object);
    ok = nearest_calls == 1
        && nearest_mode[0] == 1
        && DAT_00108038 == 1000
        && DAT_00108040 == 2000
        && rotate_calls == 0
        && get_int(slot_at(0), 0x1c) == 0x5555
        && get_int(slot_at(0), 0x24) == 0
        && get_bits(slot_at(0), 0x28) == 0;
    put(ok ? '2' : 'x');

    reset();
    set_int(object, 0x40, 1);
    set_int(slot_at(0), 0x0c, 1000);
    set_int(slot_at(0), 0x14, 2000);
    set_int(slot_at(0), 0x18, 1);
    set_bits((char *)&segment.angle, 0, 0x3f000000);
    segment.x = 100;
    segment.y = 200;
    nearest_result[0] = 1;
    player_segment_update(object);
    ok = nearest_calls == 1
        && nearest_mode[0] == 1
        && DAT_00108038 == 1000
        && DAT_00108040 == 2000
        && rotate_calls == 2
        && rotate_in_x[0] == 900
        && rotate_in_z[0] == 1800
        && rotate_angle[0] == 0xbf000000
        && rotate_in_x[1] == -30
        && rotate_in_z[1] == 2000
        && rotate_angle[1] == 0x3f000000
        && get_int(slot_at(0), 0x1c) == -920
        && get_int(slot_at(0), 0x24) == 220
        && get_bits(slot_at(0), 0x28) == 0x3f000000;
    put(ok ? '3' : 'x');

    reset();
    set_int(object, 0x40, 1);
    set_int(slot_at(3), 0x0c, 0);
    set_int(slot_at(3), 0x14, 300);
    set_int(slot_at(3), 0x18, 1);
    set_bits((char *)&segment.angle, 0, 0x3f000000);
    segment.x = 100;
    segment.y = 200;
    nearest_result[0] = 1;
    player_segment_update(object);
    ok = nearest_calls == 1
        && rotate_calls == 2
        && rotate_in_x[0] == -100
        && rotate_in_z[0] == 100
        && rotate_angle[0] == 0xbf000000
        && rotate_in_x[1] == 30
        && rotate_in_z[1] == 300
        && rotate_angle[1] == 0x3f000000
        && get_int(slot_at(3), 0x1c) == 140
        && get_int(slot_at(3), 0x24) == 220
        && get_bits(slot_at(3), 0x28) == 0x3f000000;
    put(ok ? '4' : 'x');

    reset();
    set_int(object, 0x40, 1);
    set_int(slot_at(0), 0x0c, 1000);
    set_int(slot_at(0), 0x14, 2000);
    set_int(slot_at(0), 0x18, 1);
    set_int(slot_at(3), 0x0c, 500);
    set_int(slot_at(3), 0x14, 600);
    set_int(slot_at(3), 0x18, 1);
    set_int(slot_at(3), 0x1c, 0x6666);
    set_int(slot_at(3), 0x24, 0x7777);
    set_bits(slot_at(3), 0x28, 0x88888888);
    set_bits((char *)&segment.angle, 0, 0x3f000000);
    segment.x = 100;
    segment.y = 200;
    nearest_result[0] = 1;
    nearest_result[1] = 0;
    player_segment_update(object);
    ok = nearest_calls == 2
        && nearest_mode[0] == 1
        && nearest_mode[1] == 1
        && rotate_calls == 2
        && get_int(slot_at(0), 0x1c) == -920
        && get_int(slot_at(0), 0x24) == 220
        && DAT_00108038 == 500
        && DAT_00108040 == 600
        && get_int(slot_at(3), 0x1c) == 0x6666
        && get_int(slot_at(3), 0x24) == 0x7777
        && get_bits(slot_at(3), 0x28) == 0x88888888;
    put(ok ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
