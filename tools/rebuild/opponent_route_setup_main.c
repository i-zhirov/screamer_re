/* The test harness for the rebuilt `opponent_route_setup` module.
 *
 * The harness holds the spline table, the point records, the opponent record
 * array, the two position globals, the height and the heading outputs, the
 * rotation pair, and the four callee stubs.  The rotation stub records the input
 * pair and the angle and writes two chosen values 8 bytes apart.  The collision
 * stub sets the height.  The track stub sets the heading.  The object stub
 * records the table, the name, and the slot and returns a chosen result.
 *
 * The harness runs nine paths:
 *
 *     0  a zero spline count: the record count clears and no callee runs
 *     1  one point without the load flag: the record takes the position, the
 *        height, the heading, and the two scaled angles
 *     2  one point with the load flag: the object name is formed from the model
 *        index and the state mask, and the slot takes the object handle
 *     3  one point with a load error: the function returns the error and fills
 *        no record
 *     4  three points along the next chain: the records follow the chain order
 *        and the count stays the spline count
 *     5  the state mask 3: the name offset takes the third variant
 *     6  negative positions and negative rotation outputs: the angles keep the
 *        sign bits
 *     7  an error on the second record: the first record fills and the second
 *        returns the error
 *     D  the exit marker
 *
 * The constants are set from their exact bit patterns, because a float literal
 * in this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

int opponent_route_setup(int load);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x40
#define MAX_RECORDS 4
#define POINT_STRIDE 0x38

struct rotate_pair {
    int first;
    int pad;
    int second;
};

void *DAT_0010d6f4;
int DAT_0010d6fc;
int DAT_0010d6f8;
char DAT_0010d700[STRIDE * MAX_RECORDS];
int DAT_0011d274;
int DAT_00108038;
int DAT_00108040;
float DAT_00108048;
int DAT_0010c72c;
int DAT_0010c734;
int DAT_0010c73c;
float DAT_00100633;
float DAT_00100637;
float DAT_0010063b;

static char route[0x28];
static char points[4][POINT_STRIDE];
static char text[8];
static int collision_calls;
static int collision_height;
static int collision_mode;
static int track_calls;
static int track_mode;
static float track_heading;
static int lookup_calls;
static int lookup_result;
static int lookup_result_count;
static int lookup_results[8];
static void *lookup_table;
static char *lookup_name;
static int rotate_calls;
static int rotate_in0;
static int rotate_in1;
static unsigned int rotate_angle;
static int rotate_out0;
static int rotate_out1;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits(unsigned int *p, unsigned int bits)
{
    *p = bits;
}

static void set_float(float *p, unsigned int bits)
{
    *(unsigned int *)p = bits;
}

static void clear_bytes(char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static char *record_at(int index)
{
    return DAT_0010d700 + index * STRIDE;
}

static int rec_int(int index, int offset)
{
    return *(int *)(record_at(index) + offset);
}

static unsigned int rec_bits(int index, int offset)
{
    return *(unsigned int *)(record_at(index) + offset);
}

static void set_point(int index, int x, int z, int model, int next)
{
    char *p;

    p = points[index];
    *(int *)(p + 4) = x;
    *(int *)(p + 8) = z;
    *(int *)(p + 0xc) = model;
    *(int *)(p + 0x10) = next;
}

static void set_route(int count, int first)
{
    *(int *)(route + 0x1c) = count;
    *(int *)(route + 0x20) = first;
}

int object_load_named(void *table, char *name, int *slot)
{
    int result;

    lookup_table = table;
    lookup_name = name;
    result = lookup_result;
    if (lookup_calls < lookup_result_count) {
        result = lookup_results[lookup_calls];
    }
    lookup_calls = lookup_calls + 1;
    if (result == 0) {
        *slot = 0x1234;
    }
    return result;
}

int map_collision(int mode)
{
    collision_calls = collision_calls + 1;
    collision_mode = mode;
    DAT_0010c72c = collision_height;
    return 0;
}

int track_nearest_node(int mode)
{
    track_calls = track_calls + 1;
    track_mode = mode;
    DAT_00108048 = track_heading;
    return 0;
}

void rotate_pair(struct rotate_pair *out, struct rotate_pair *in, float angle)
{
    rotate_calls = rotate_calls + 1;
    rotate_in0 = in->first;
    rotate_in1 = in->second;
    rotate_angle = *(unsigned int *)&angle;
    out->first = rotate_out0;
    out->second = rotate_out1;
}

static void reset(void)
{
    int index;

    clear_bytes(DAT_0010d700, STRIDE * MAX_RECORDS);
    clear_bytes(route, 0x28);
    clear_bytes((char *)points, POINT_STRIDE * 4);
    DAT_0010d6f4 = route;
    *(char **)route = (char *)points;
    set_route(0, 0);
    DAT_0010d6fc = 1;
    DAT_0010d6f8 = 0;
    DAT_0011d274 = 0;
    DAT_00108038 = 0;
    DAT_00108040 = 0;
    DAT_00108048 = 0;
    DAT_0010c72c = 0;
    DAT_0010c734 = 30;
    DAT_0010c73c = -40;
    set_float(&DAT_00100633, 0xbfc90fdb);
    set_float(&DAT_00100637, 0x40490fdb);
    set_float(&DAT_0010063b, 0x47340000);
    collision_calls = 0;
    collision_height = 400;
    collision_mode = -1;
    track_calls = 0;
    track_mode = -1;
    set_float(&track_heading, 0x3fc90fdb);
    lookup_calls = 0;
    lookup_result = 0;
    lookup_result_count = 0;
    lookup_table = 0;
    lookup_name = 0;
    for (index = 0; index < 8; index++) {
        lookup_results[index] = 0;
    }
    rotate_calls = 0;
    rotate_in0 = 0;
    rotate_in1 = 0;
    rotate_angle = 0;
    rotate_out0 = 23040;
    rotate_out1 = 46080;
}

int main(void)
{
    int result;

    reset();
    set_route(3, 0);
    DAT_0010d6fc = 0;
    set_point(0, 10, 20, 0, 0);
    result = opponent_route_setup(0);
    put(result == 0 && DAT_0010d6f8 == 0 && collision_calls == 0
        && track_calls == 0 && rotate_calls == 0 && lookup_calls == 0
        && rec_int(0, 0) == 0 ? '0' : 'x');

    reset();
    set_route(1, 0);
    set_point(0, 1000, 2000, 1, 0);
    result = opponent_route_setup(0);
    put(result == 0 && DAT_0010d6f8 == 1
        && rec_int(0, 0) == 1000 && rec_int(0, 4) == 400
        && rec_int(0, 8) == 2000
        && rec_bits(0, 0x10) == 0x3fc90fdb
        && rec_bits(0, 0xc) == 0x3fc90fdb
        && rec_bits(0, 0x14) == 0x40490fdb
        && rec_int(0, 0x20) == 1 && rec_int(0, 0x38) == 0
        && rec_int(0, 0x3c) == 0
        && DAT_00108038 == 1000 && DAT_00108040 == 2000
        && collision_calls == 1 && collision_mode == 0
        && track_calls == 1 && track_mode == 0
        && rotate_calls == 1 && rotate_in0 == 30 && rotate_in1 == -40
        && rotate_angle == 0 && lookup_calls == 0 ? '1' : 'x');

    reset();
    set_route(1, 0);
    set_point(0, 1000, 2000, 2, 0);
    DAT_0011d274 = 1;
    result = opponent_route_setup(1);
    put(result == 0 && lookup_calls == 1
        && lookup_table == (void *)0x6530
        && lookup_name == (char *)0xf054
        && rec_int(0, 0x18) == 0x1234
        && rec_int(0, 0) == 1000 && collision_calls == 1 ? '2' : 'x');

    reset();
    set_route(1, 0);
    set_point(0, 1000, 2000, 0, 0);
    lookup_result = 5;
    result = opponent_route_setup(1);
    put(result == 5 && lookup_calls == 1
        && lookup_name == (char *)0xf000
        && rec_int(0, 0) == 0 && rec_int(0, 0x18) == 0
        && collision_calls == 0 && track_calls == 0
        && rotate_calls == 0 ? '3' : 'x');

    reset();
    set_route(3, 2);
    set_point(0, 10, 20, 0, 1);
    set_point(1, 30, 40, 0, 2);
    set_point(2, 50, 60, 0, 0);
    result = opponent_route_setup(0);
    put(result == 0 && DAT_0010d6f8 == 3
        && rec_int(0, 0) == 50 && rec_int(1, 0) == 10
        && rec_int(2, 0) == 30
        && rec_int(0, 8) == 60 && rec_int(1, 8) == 20
        && rec_int(2, 8) == 40
        && collision_calls == 3 && track_calls == 3
        && rotate_calls == 3 ? '4' : 'x');

    reset();
    set_route(1, 0);
    set_point(0, 1, 2, 1, 0);
    DAT_0011d274 = 3;
    lookup_result = 5;
    result = opponent_route_setup(1);
    put(result == 5 && lookup_calls == 1
        && lookup_name == (char *)0xf048 ? '5' : 'x');

    reset();
    set_route(1, 0);
    set_point(0, -1000, -2000, 0, 0);
    rotate_out0 = -23040;
    rotate_out1 = -46080;
    result = opponent_route_setup(0);
    put(result == 0 && rec_int(0, 0) == -1000
        && rec_int(0, 8) == -2000
        && rec_bits(0, 0xc) == 0xbfc90fdb
        && rec_bits(0, 0x14) == 0xc0490fdb
        && DAT_00108038 == -1000 && DAT_00108040 == -2000 ? '6' : 'x');

    reset();
    set_route(2, 0);
    set_point(0, 10, 20, 0, 1);
    set_point(1, 30, 40, 0, 0);
    lookup_result_count = 2;
    lookup_results[0] = 0;
    lookup_results[1] = 7;
    result = opponent_route_setup(1);
    put(result == 7 && lookup_calls == 2
        && rec_int(0, 0) == 10 && rec_int(0, 0x18) == 0x1234
        && rec_int(1, 0) == 0
        && collision_calls == 1 && track_calls == 1
        && rotate_calls == 1 ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
