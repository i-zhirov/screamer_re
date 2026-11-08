/* The test harness for the rebuilt `terrain_contact_update` module.
 *
 * The harness holds the object record, the position and height fields, the two
 * terrain angle fields, the speed field, the map globals, the five constants,
 * and the two callee stubs.  The rotate stub records the input pair and the
 * angle of every call and writes a chosen result.  The collision stub records
 * its call and returns a chosen value.
 *
 * The harness runs six paths:
 *
 *     0  no collision with the speed at the 3000.0 boundary: the world
 *        position globals take the sums, the speed stays, and the height and
 *        the angle fields stay
 *     1  no collision with the speed 4000.0: the speed takes zero
 *     2  no collision with the speed 100.0: the speed stays
 *     3  a collision with the point 46080 and 23040: the ground height and the
 *        center pair, the angle from the heading plus -pi/2, and the two
 *        angles pi and pi/2
 *     4  a collision with the point -46080 and -1: the angle pi after one
 *        2*pi add and the small negative angle after one 2*pi add
 *     5  a collision with the point -1 and -46080: the two fields normalize
 *        in the module order
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void terrain_contact_update(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct rotate_point {
    int x;
    int pad;
    int y;
};

int DAT_00108038;
int DAT_00108040;
int DAT_0010c72c;
int DAT_0010c734;
int DAT_0010c73c;
float DAT_00100b04;
float DAT_00100b08;
float DAT_00100b0c;
float DAT_00100b10;
float DAT_00100b14;

static char text[8];
static char object[0x600];

static int rotate_calls;
static int rotate_in0[4];
static int rotate_in1[4];
static unsigned int rotate_angle[4];
static int rotate_out0;
static int rotate_out1;

static int collision_calls;
static int collision_arg;
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
    collision_arg = value;
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

static void set_float_bits(float *value, unsigned int bits)
{
    *(unsigned int *)value = bits;
}

static void reset(void)
{
    int index;

    clear_bytes(object, 0x600);

    set_int(object, 0x258, 1000);
    set_int(object, 0x454, 234);
    set_int(object, 0x260, 2000);
    set_int(object, 0x45c, 345);
    set_int(object, 0x238, 0x0bad);
    set_bits(object, 0x4bc, 0x3f000000);
    set_bits(object, 0x23c, 0x11111111);
    set_bits(object, 0x244, 0x22222222);
    set_bits(object, 0x418, 0x453b8000);

    DAT_00108038 = 0x11111111;
    DAT_00108040 = 0x22222222;
    DAT_0010c72c = 0x4321;
    DAT_0010c734 = 111;
    DAT_0010c73c = 222;

    set_float_bits(&DAT_00100b04, 0xbfc90fdb);
    set_float_bits(&DAT_00100b08, 0x40490fdb);
    set_float_bits(&DAT_00100b0c, 0x47340000);
    set_float_bits(&DAT_00100b10, 0x40c90fdb);
    set_float_bits(&DAT_00100b14, 0x453b8000);

    rotate_calls = 0;
    rotate_out0 = 0;
    rotate_out1 = 0;
    for (index = 0; index < 4; index++) {
        rotate_in0[index] = 0;
        rotate_in1[index] = 0;
        rotate_angle[index] = 0;
    }

    collision_calls = 0;
    collision_arg = -1;
    collision_result = 0;
}

int main(void)
{
    reset();
    terrain_contact_update(object);
    put(collision_calls == 1
        && collision_arg == 0
        && rotate_calls == 0
        && DAT_00108038 == 1234
        && DAT_00108040 == 2345
        && get_int(object, 0x238) == 0x0bad
        && get_bits(object, 0x23c) == 0x11111111
        && get_bits(object, 0x244) == 0x22222222
        && get_bits(object, 0x418) == 0x453b8000
        ? '0' : 'x');

    reset();
    set_bits(object, 0x418, 0x457a0000);
    terrain_contact_update(object);
    put(collision_calls == 1
        && collision_arg == 0
        && rotate_calls == 0
        && get_bits(object, 0x418) == 0
        && get_int(object, 0x238) == 0x0bad
        && get_bits(object, 0x23c) == 0x11111111
        && get_bits(object, 0x244) == 0x22222222
        ? '1' : 'x');

    reset();
    set_bits(object, 0x418, 0x42c80000);
    terrain_contact_update(object);
    put(collision_calls == 1
        && rotate_calls == 0
        && get_bits(object, 0x418) == 0x42c80000
        ? '2' : 'x');

    reset();
    collision_result = 1;
    rotate_out0 = 46080;
    rotate_out1 = 23040;
    terrain_contact_update(object);
    put(collision_calls == 1
        && collision_arg == 0
        && rotate_calls == 1
        && rotate_in0[0] == 111
        && rotate_in1[0] == 222
        && rotate_angle[0] == 0xbf890fdb
        && get_int(object, 0x238) == 0x4321
        && get_bits(object, 0x23c) == 0x40490fdb
        && get_bits(object, 0x244) == 0x3fc90fdb
        && get_bits(object, 0x418) == 0x453b8000
        && DAT_00108038 == 1234
        && DAT_00108040 == 2345
        ? '3' : 'x');

    reset();
    collision_result = 1;
    rotate_out0 = -46080;
    rotate_out1 = -1;
    terrain_contact_update(object);
    put(collision_calls == 1
        && rotate_calls == 1
        && get_int(object, 0x238) == 0x4321
        && get_bits(object, 0x23c) == 0x40490fdb
        && get_bits(object, 0x244) == 0x40c90f4c
        ? '4' : 'x');

    reset();
    collision_result = 1;
    rotate_out0 = -1;
    rotate_out1 = -46080;
    terrain_contact_update(object);
    put(collision_calls == 1
        && rotate_calls == 1
        && get_bits(object, 0x23c) == 0x40c90f4c
        && get_bits(object, 0x244) == 0x40490fdb
        ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
