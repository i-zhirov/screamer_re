/* The test harness for the rebuilt `car_proximity_pass` module.
 *
 * The harness holds the player object, five 0x590-byte car records at
 * `DAT_00110ab8`, the record count `DAT_0010f8ec`, the filter coefficient
 * table, and two callee stubs.  The `car_collision_response` stub records the
 * call count, the two pointers, and the distance, the contact distance, and the
 * two deltas at the call.  The `filter_update` stub applies the one-pole
 * filter and records the pointer offset, the target, and the rate.
 *
 * The harness runs these paths:
 *
 *     0  the state field +0x148 is set: the scan does not run and the contact
 *        distance keeps its value
 *     1  the record count is zero: the scan runs no pass
 *     2  the nearby mask leaves the record out: the contact distance is set
 *        and no response runs
 *     3  the physics gate is clear: the contact distance is set and no
 *        response runs
 *     4  the record state +0x2f8 is not zero: the contact distance is set and
 *        no response runs
 *     5  the first player point sits inside the contact distance: the response
 *        runs once and the filter takes the set flag
 *     6  every point pair sits inside the contact distance: the response runs
 *        once for each player point
 *     7  the distance equals the contact distance: the strict compare runs no
 *        response
 *     8  a squared distance of 8 truncates to 2, below the contact distance 3
 *     9  the mask selects the second record: the response runs once with that
 *        record and the filter takes the set flag
 *     A  the filter coefficient 0.5 moves 200 toward the clear flag to 100
 *     B  the filter coefficient 0.5 moves 200 toward the set flag to 100.5
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void car_proximity_pass(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x590
#define CARS 5

int DAT_0010f8ec;
unsigned char DAT_00110ab8[STRIDE * CARS];

float *DAT_0010cf08;

static float coefficients[256];
static char self_object[0x900];
static char text[8];

static int response_calls;
static char *response_self;
static char *response_other;
static int response_distance;
static int response_threshold;
static int response_dx;
static int response_dz;

static int filter_calls;
static int filter_offset;
static unsigned int filter_target_bits;
static int filter_index;

void car_collision_response(char *self, char *other)
{
    response_calls = response_calls + 1;
    response_self = self;
    response_other = other;
    response_distance = *(int *)(self + 0x55c);
    response_threshold = *(int *)(self + 0x560);
    response_dx = *(int *)(self + 0x564);
    response_dz = *(int *)(self + 0x56c);
}

void filter_update(float *value, float target, int index)
{
    union {
        float f;
        unsigned int u;
    } bits;

    filter_calls = filter_calls + 1;
    filter_offset = (char *)value - self_object;
    bits.f = target;
    filter_target_bits = bits.u;
    filter_index = index;
    *value = (*value - target) * DAT_0010cf08[index] + target;
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

static void clear_bytes(char *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0;
    }
}

static void set_int(char *object, int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int int_is(char *object, int offset, int value)
{
    return *(int *)(object + offset) == value;
}

static void set_float_bits(char *object, int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(object + offset) = value.f;
}

static unsigned int float_bits_at(char *object, int offset)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = *(float *)(object + offset);
    return value.u;
}

static char *record_at(int index)
{
    return (char *)DAT_00110ab8 + index * STRIDE;
}

static void set_player_point(int point, int x, int z)
{
    set_int(self_object, 0x52c + point * 12, x);
    set_int(self_object, 0x534 + point * 12, z);
}

static void set_record_point(char *record, int point, int x, int z)
{
    set_int(record, 0x434 + point * 12, x);
    set_int(record, 0x43c + point * 12, z);
}

static void reset(void)
{
    int index;

    clear_bytes(self_object, 0x900);
    clear_bytes((char *)DAT_00110ab8, STRIDE * CARS);
    DAT_0010f8ec = 0;
    DAT_0010cf08 = coefficients;
    for (index = 0; index < 256; index++) {
        set_float_bits((char *)coefficients, index * 4, 0x3f800000);
    }
    response_calls = 0;
    response_self = 0;
    response_other = 0;
    response_distance = -1;
    response_threshold = -1;
    response_dx = 0;
    response_dz = 0;
    filter_calls = 0;
    filter_offset = -1;
    filter_target_bits = 0xffffffff;
    filter_index = -1;
}

int main(void)
{
    char *record0;
    char *record1;


    reset();
    set_int(self_object, 0x148, 1);
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_int(self_object, 0x560, 0x1111);
    set_player_point(0, 3, 4);
    set_player_point(1, 3, 4);
    set_float_bits(self_object, 0x7c4, 0);
    DAT_0010f8ec = 1;
    record0 = record_at(0);
    set_int(record0, 0x414, 1);
    set_int(record0, 0x2f8, 0);
    set_int(record0, 0x44c, 4);
    set_record_point(record0, 0, 0, 0);
    set_record_point(record0, 1, 0, 0);
    car_proximity_pass(self_object);
    put(response_calls == 0 && int_is(self_object, 0x560, 0x1111)
        && filter_calls == 1 && filter_offset == 0x7c4
        && filter_target_bits == 0 && filter_index == 0x32 ? '0' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_int(self_object, 0x560, 0x1111);
    set_float_bits(self_object, 0x7c4, 0);
    DAT_0010f8ec = 0;
    car_proximity_pass(self_object);
    put(response_calls == 0 && int_is(self_object, 0x560, 0x1111)
        && filter_calls == 1 && filter_offset == 0x7c4
        && filter_target_bits == 0 && filter_index == 0x32 ? '1' : 'x');

    reset();
    set_int(self_object, 0x104, 0);
    set_int(self_object, 0x544, 2);
    set_float_bits(self_object, 0x7c4, 0);
    DAT_0010f8ec = 1;
    record0 = record_at(0);
    set_int(record0, 0x414, 1);
    set_int(record0, 0x2f8, 0);
    set_int(record0, 0x44c, 4);
    car_proximity_pass(self_object);
    put(response_calls == 0 && int_is(self_object, 0x560, 6)
        && filter_calls == 1 && filter_target_bits == 0 ? '2' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_float_bits(self_object, 0x7c4, 0);
    DAT_0010f8ec = 1;
    record0 = record_at(0);
    set_int(record0, 0x414, 0);
    set_int(record0, 0x2f8, 0);
    set_int(record0, 0x44c, 4);
    car_proximity_pass(self_object);
    put(response_calls == 0 && int_is(self_object, 0x560, 6)
        && filter_calls == 1 && filter_target_bits == 0 ? '3' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_float_bits(self_object, 0x7c4, 0);
    DAT_0010f8ec = 1;
    record0 = record_at(0);
    set_int(record0, 0x414, 1);
    set_int(record0, 0x2f8, 5);
    set_int(record0, 0x44c, 4);
    car_proximity_pass(self_object);
    put(response_calls == 0 && int_is(self_object, 0x560, 6)
        && filter_calls == 1 && filter_target_bits == 0 ? '4' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_float_bits(self_object, 0x7c4, 0);
    set_player_point(0, 3, 4);
    set_player_point(1, 100, 100);
    DAT_0010f8ec = 1;
    record0 = record_at(0);
    set_int(record0, 0x414, 1);
    set_int(record0, 0x2f8, 0);
    set_int(record0, 0x44c, 4);
    set_record_point(record0, 0, 0, 0);
    set_record_point(record0, 1, 0, 0);
    car_proximity_pass(self_object);
    put(response_calls == 1 && response_self == self_object
        && response_other == record0 && response_distance == 5
        && response_threshold == 6 && response_dx == 3 && response_dz == 4
        && filter_calls == 1 && filter_target_bits == 0x3f800000
        && filter_index == 0x32 ? '5' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_float_bits(self_object, 0x7c4, 0);
    set_player_point(0, 3, 4);
    set_player_point(1, 3, 4);
    DAT_0010f8ec = 1;
    record0 = record_at(0);
    set_int(record0, 0x414, 1);
    set_int(record0, 0x2f8, 0);
    set_int(record0, 0x44c, 4);
    set_record_point(record0, 0, 0, 0);
    set_record_point(record0, 1, 0, 0);
    car_proximity_pass(self_object);
    put(response_calls == 2 && response_other == record0
        && response_distance == 5 && filter_target_bits == 0x3f800000
        ? '6' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 1);
    set_float_bits(self_object, 0x7c4, 0);
    set_player_point(0, 0, 0);
    set_player_point(1, 0, 0);
    DAT_0010f8ec = 1;
    record0 = record_at(0);
    set_int(record0, 0x414, 1);
    set_int(record0, 0x2f8, 0);
    set_int(record0, 0x44c, 4);
    set_record_point(record0, 0, 3, 4);
    set_record_point(record0, 1, 3, 4);
    car_proximity_pass(self_object);
    put(response_calls == 0 && int_is(self_object, 0x55c, 5)
        && int_is(self_object, 0x560, 5) && filter_target_bits == 0
        ? '7' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 1);
    set_float_bits(self_object, 0x7c4, 0);
    set_player_point(0, 2, 2);
    set_player_point(1, 2, 2);
    DAT_0010f8ec = 1;
    record0 = record_at(0);
    set_int(record0, 0x414, 1);
    set_int(record0, 0x2f8, 0);
    set_int(record0, 0x44c, 2);
    set_record_point(record0, 0, 0, 0);
    set_record_point(record0, 1, 0, 0);
    car_proximity_pass(self_object);
    put(response_calls == 2 && response_distance == 2
        && response_threshold == 3 && response_dx == 2 && response_dz == 2
        && filter_target_bits == 0x3f800000 ? '8' : 'x');

    reset();
    set_int(self_object, 0x104, 2);
    set_int(self_object, 0x544, 2);
    set_float_bits(self_object, 0x7c4, 0);
    set_player_point(0, 3, 4);
    set_player_point(1, 100, 100);
    DAT_0010f8ec = 2;
    record0 = record_at(0);
    record1 = record_at(1);
    set_int(record0, 0x414, 1);
    set_int(record0, 0x2f8, 0);
    set_int(record0, 0x44c, 4);
    set_int(record1, 0x414, 1);
    set_int(record1, 0x2f8, 0);
    set_int(record1, 0x44c, 4);
    set_record_point(record0, 0, 0, 0);
    set_record_point(record1, 0, 0, 0);
    set_record_point(record1, 1, 0, 0);
    car_proximity_pass(self_object);
    put(response_calls == 1 && response_other == record1
        && response_distance == 5 && response_threshold == 6
        && filter_target_bits == 0x3f800000 ? '9' : 'x');

    reset();
    set_float_bits(self_object, 0x7c4, 0x43480000);
    set_float_bits((char *)coefficients, 0x32 * 4, 0x3f000000);
    car_proximity_pass(self_object);
    put(filter_calls == 1 && filter_offset == 0x7c4
        && filter_target_bits == 0 && filter_index == 0x32
        && float_bits_at(self_object, 0x7c4) == 0x42c80000 ? 'A' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_float_bits(self_object, 0x7c4, 0x43480000);
    set_float_bits((char *)coefficients, 0x32 * 4, 0x3f000000);
    set_player_point(0, 3, 4);
    set_player_point(1, 100, 100);
    DAT_0010f8ec = 1;
    record0 = record_at(0);
    set_int(record0, 0x414, 1);
    set_int(record0, 0x2f8, 0);
    set_int(record0, 0x44c, 4);
    set_record_point(record0, 0, 0, 0);
    set_record_point(record0, 1, 0, 0);
    car_proximity_pass(self_object);
    put(response_calls == 1 && filter_calls == 1
        && filter_target_bits == 0x3f800000 && filter_index == 0x32
        && float_bits_at(self_object, 0x7c4) == 0x42c90000 ? 'B' : 'x');

    put('D');
    quit();
    return 0;
}
