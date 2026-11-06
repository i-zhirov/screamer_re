/* The test harness for the rebuilt `traffic_collision_pass` module.
 *
 * The harness holds the player object, the traffic table, the traffic count,
 * the player index byte, and two callee stubs.  The `traffic_push_response`
 * stub records the call count and the flag.  The `filter_update` stub records
 * the pointer offset, the float value, and the rate.
 *
 * The harness runs ten paths:
 *
 *     0  the state field +0x148 is set: the function clears the mask and
 *        returns before the scan
 *     1  the traffic count is zero: the scan runs no pass
 *     2  the first field is negative: the function skips the record
 *     3  the flag field holds the player bit: the function takes the velocity
 *        branch and copies the negated velocity, even when the record is near
 *     4  the nearby mask does not hold the index: the function skips the
 *        record before the distance block
 *     5  the record flag +0x16 bit 0 is set: the function skips the record
 *     6  the distance equals the contact distance: the strict compare keeps
 *        the contact clear
 *     7  the four body pairs sit inside the contact distance: every pair
 *        pushes and the contact mask takes the record bit
 *     8  the raw distance is 2.828: the truncating helper stores 2, so the
 *        pair pushes against the contact distance 3
 *     9  two records stay inside the contact distance: the mask takes both
 *        record bits
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void traffic_collision_pass(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00115218;
unsigned char DAT_00113cd8[0xaa0];
unsigned char DAT_0011d250;

static char self_object[0x900];
static char text[8];

static int push_calls;
static int push_last;
static char *push_self;
static int filter_calls;
static int filter_offset;
static unsigned int filter_bits;
static int filter_rate;

void traffic_push_response(char *self, int push)
{
    push_calls = push_calls + 1;
    push_last = push;
    push_self = self;
}

void filter_update(float *p, float f, int i)
{
    union {
        float f;
        unsigned int u;
    } value;

    filter_calls = filter_calls + 1;
    filter_offset = (char *)p - self_object;
    value.f = f;
    filter_bits = value.u;
    filter_rate = i;
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

static void set_bits(char *object, int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(object + offset) = value.f;
}

static unsigned int bits_at(char *object, int offset)
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
    return (char *)DAT_00113cd8 + index * 0x2a8;
}

static void reset(void)
{
    clear_bytes(self_object, 0x900);
    clear_bytes((char *)DAT_00113cd8, 0xaa0);
    DAT_00115218 = 0;
    DAT_0011d250 = 0;
    push_calls = 0;
    push_last = -1;
    push_self = 0;
    filter_calls = 0;
    filter_offset = -1;
    filter_bits = 0;
    filter_rate = 0;
}

static void set_player_points(int x, int z)
{
    set_int(self_object, 0x52c, x);
    set_int(self_object, 0x534, z);
    set_int(self_object, 0x538, x);
    set_int(self_object, 0x540, z);
}

static void set_record_points(char *record, int x, int z)
{
    set_int(record, 0x28c, x);
    set_int(record, 0x294, z);
    set_int(record, 0x298, x);
    set_int(record, 0x2a0, z);
}

int main(void)
{
    char *record0;
    char *record1;

    reset();
    set_int(self_object, 0x148, 1);
    set_int(self_object, 0x2fc, 0x1234);
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_player_points(3, 4);
    DAT_00115218 = 1;
    record0 = record_at(0);
    set_int(record0, 0x00, 0);
    set_int(record0, 0x2a4, 4);
    set_record_points(record0, 0, 0);
    traffic_collision_pass(self_object);
    put(int_is(self_object, 0x2fc, 0) && push_calls == 0
        && filter_calls == 1 && filter_offset == 0x7c4
        && filter_bits == 0x00000000 && filter_rate == 50 ? '0' : 'x');

    reset();
    set_int(self_object, 0x2fc, 0x1234);
    DAT_00115218 = 0;
    traffic_collision_pass(self_object);
    put(int_is(self_object, 0x2fc, 0) && push_calls == 0
        && filter_calls == 1 && filter_offset == 0x7c4
        && filter_bits == 0x00000000 && filter_rate == 50 ? '1' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    DAT_00115218 = 1;
    record0 = record_at(0);
    set_int(record0, 0x00, -1);
    traffic_collision_pass(self_object);
    put(int_is(self_object, 0x2fc, 0) && push_calls == 0
        && filter_bits == 0x00000000 ? '2' : 'x');

    reset();
    DAT_0011d250 = 3;
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_player_points(3, 4);
    DAT_00115218 = 1;
    record0 = record_at(0);
    set_int(record0, 0x00, 2);
    set_int(record0, 0x14, 1 << 3);
    set_bits(record0, 0x30, 0x40000000);
    set_bits(record0, 0x38, 0xc0400000);
    set_int(record0, 0x2a4, 4);
    set_record_points(record0, 0, 0);
    traffic_collision_pass(self_object);
    put(push_calls == 1 && push_last == 0 && push_self == self_object
        && bits_at(self_object, 0x550) == 0xc0000000
        && bits_at(self_object, 0x558) == 0x40400000
        && int_is(self_object, 0x2fc, 0)
        && filter_bits == 0x00000000 ? '3' : 'x');

    reset();
    set_int(self_object, 0x104, 0);
    set_int(self_object, 0x544, 2);
    set_int(self_object, 0x560, 0x1234);
    set_player_points(3, 4);
    DAT_00115218 = 1;
    record0 = record_at(0);
    set_int(record0, 0x00, 0);
    set_int(record0, 0x2a4, 4);
    set_record_points(record0, 0, 0);
    traffic_collision_pass(self_object);
    put(push_calls == 0 && int_is(self_object, 0x2fc, 0)
        && int_is(self_object, 0x560, 0x1234)
        && filter_bits == 0x00000000 ? '4' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_int(self_object, 0x560, 0x1234);
    set_player_points(3, 4);
    DAT_00115218 = 1;
    record0 = record_at(0);
    set_int(record0, 0x00, 0);
    set_int(record0, 0x16, 1);
    set_int(record0, 0x2a4, 4);
    set_record_points(record0, 0, 0);
    traffic_collision_pass(self_object);
    put(push_calls == 0 && int_is(self_object, 0x2fc, 0)
        && int_is(self_object, 0x560, 0x1234)
        && filter_bits == 0x00000000 ? '5' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 90);
    set_player_points(100, 0);
    DAT_00115218 = 1;
    record0 = record_at(0);
    set_int(record0, 0x00, 0);
    set_int(record0, 0x2a4, 10);
    set_record_points(record0, 0, 0);
    traffic_collision_pass(self_object);
    put(push_calls == 0 && int_is(self_object, 0x2fc, 0)
        && int_is(self_object, 0x560, 100)
        && int_is(self_object, 0x564, 100)
        && int_is(self_object, 0x56c, 0)
        && int_is(self_object, 0x55c, 100)
        && filter_bits == 0x00000000 ? '6' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 2);
    set_player_points(3, 4);
    DAT_00115218 = 1;
    record0 = record_at(0);
    set_int(record0, 0x00, 0);
    set_int(record0, 0x2a4, 4);
    set_record_points(record0, 0, 0);
    traffic_collision_pass(self_object);
    put(push_calls == 4 && push_last == 1
        && int_is(self_object, 0x2fc, 1)
        && int_is(self_object, 0x560, 6)
        && int_is(self_object, 0x55c, 5)
        && filter_bits == 0x3f800000 && filter_rate == 50 ? '7' : 'x');

    reset();
    set_int(self_object, 0x104, 1);
    set_int(self_object, 0x544, 1);
    set_player_points(2, 2);
    DAT_00115218 = 1;
    record0 = record_at(0);
    set_int(record0, 0x00, 0);
    set_int(record0, 0x2a4, 2);
    set_record_points(record0, 0, 0);
    traffic_collision_pass(self_object);
    put(push_calls == 4 && push_last == 1
        && int_is(self_object, 0x2fc, 1)
        && int_is(self_object, 0x560, 3)
        && int_is(self_object, 0x55c, 2)
        && filter_bits == 0x3f800000 ? '8' : 'x');

    reset();
    set_int(self_object, 0x104, 3);
    set_int(self_object, 0x544, 2);
    set_player_points(3, 4);
    DAT_00115218 = 2;
    record0 = record_at(0);
    set_int(record0, 0x00, 0);
    set_int(record0, 0x2a4, 4);
    set_record_points(record0, 0, 0);
    record1 = record_at(1);
    set_int(record1, 0x00, 3);
    set_int(record1, 0x2a4, 4);
    set_record_points(record1, 0, 0);
    traffic_collision_pass(self_object);
    put(push_calls == 8 && push_last == 1
        && int_is(self_object, 0x2fc, 9)
        && filter_bits == 0x3f800000 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
