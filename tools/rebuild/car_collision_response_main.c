/* The test harness for the rebuilt `car_collision_response` module.
 *
 * The harness holds the player object, the other car record, the filter table,
 * the two position globals, the six constants, and three callee stubs.  The
 * `table_lookup` stub records its index, the `collision_check` stub records
 * its value and returns the configured result, and the `angle_flags` stub
 * records both points and the angle and returns the configured flags for each
 * call.  A zero collision result takes the player move path, and a non-zero
 * result takes the other car path.
 *
 * The harness runs eight paths:
 *
 *     0  the map cell is clear with positive deltas: the player position adds
 *        the penetration 1 and 1, and the probe positions fall by the same
 *        amounts
 *     1  the map cell is clear with negative deltas: the two negative
 *        penetrations truncate toward zero, so the player position falls by 1
 *        and 1
 *     2  the distance count is zero: the function sets it to 1, forms the
 *        unit vector from the raw deltas, and moves the player position by 36
 *        and 54
 *     3  the first flag has bit 1: the record rotation scales by 0.7 and the
 *        function skips the rate and heading stores
 *     4  the two flags are 3: the record takes the positive heading, the
 *        positive rate 0.2, the speed factor 500, and the player speed scales
 *        by 0.5
 *     5  the second flag is 2: the record takes the negative heading -0.1 and
 *        the negative rate -0.2
 *     6  the second flag is 1: the record takes the negative heading -0.1 and
 *        the positive rate 0.2
 *     7  the speed is 20000: the angle step clamps to pi / 2, the rate is 4,
 *        and the speed factor is 10000
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because an FPU memory
 * operand of a literal reads the constant pool at a low address in this
 * toolchain.  The float checks compare the bit patterns exactly.
 */
#pragma off (check_stack)

void car_collision_response(char *self, char *target);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00108038;
int DAT_00108040;
float *DAT_0010cf08;

double car_collision_slow;
double car_collision_half;
double car_collision_angle_scale;
float car_collision_angle_limit;
double car_collision_rate_scale;
double car_collision_heading_step;

static char self_object[0x900];
static char target_object[0x900];
static float table[0x100];
static char text[8];

static int lookup_calls;
static int lookup_arg;
static int lookup_result;
static int collision_calls;
static int collision_arg;
static int collision_result;
static int angle_calls;
static char *angle_first[4];
static char *angle_second[4];
static unsigned int angle_bits[4];
static int angle_result[4];

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

static unsigned int float_bits(char *object, int offset)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = *(float *)(object + offset);
    return value.u;
}

static int float_is(char *object, int offset, unsigned int bits)
{
    return float_bits(object, offset) == bits;
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

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void set_bits_double(double *p, unsigned int lo, unsigned int hi)
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

    clear_bytes(self_object, 0x900);
    clear_bytes(target_object, 0x900);
    for (index = 0; index < 0x100; index++) {
        table[index] = 0.0f;
    }
    DAT_00108038 = 0;
    DAT_00108040 = 0;
    DAT_0010cf08 = table;
    lookup_calls = 0;
    lookup_arg = 0;
    lookup_result = 100;
    collision_calls = 0;
    collision_arg = 0;
    collision_result = 1;
    angle_calls = 0;
    for (index = 0; index < 4; index++) {
        angle_first[index] = 0;
        angle_second[index] = 0;
        angle_bits[index] = 0;
        angle_result[index] = 0;
    }
    set_bits_float(&car_collision_angle_limit, 0x3fc90fdb);
    set_bits_double(&car_collision_slow, 0x66666666, 0x3fe66666);
    set_bits_double(&car_collision_half, 0x00000000, 0x3fe00000);
    set_bits_double(&car_collision_angle_scale, 0xeb1c432d, 0x3f1a36e2);
    set_bits_double(&car_collision_rate_scale, 0xae147ae1, 0x3fcae147);
    set_bits_double(&car_collision_heading_step, 0xeb1c432d, 0x3f2a36e2);
    set_bits_float(&table[0x168 / 4], 0x3f000000);
}

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_arg = index;
    return lookup_result;
}

int collision_check(int value)
{
    collision_calls = collision_calls + 1;
    collision_arg = value;
    return collision_result;
}

int angle_flags(void *first, void *second, float angle)
{
    union {
        float f;
        unsigned int u;
    } value;
    int index;

    index = angle_calls;
    value.f = angle;
    if (index < 4) {
        angle_first[index] = (char *)first;
        angle_second[index] = (char *)second;
        angle_bits[index] = value.u;
    }
    angle_calls = angle_calls + 1;
    return angle_result[index];
}

int main(void)
{
    reset();
    set_int(self_object, 0x55c, 8);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_int(self_object, 0x4b0, 100);
    set_int(self_object, 0x4b8, 100);
    set_int(target_object, 0x498, 100);
    set_int(target_object, 0x4a0, 200);
    collision_result = 0;
    car_collision_response(self_object, target_object);
    put(int_is(self_object, 0x4b0, 101)
        && int_is(self_object, 0x4b8, 101)
        && int_is(self_object, 0x55c, 8)
        && DAT_00108038 == 99 && DAT_00108040 == 198
        && lookup_calls == 1 && lookup_arg == 1
        && collision_calls == 1 && collision_arg == 1
        && angle_calls == 0 ? '0' : 'x');

    reset();
    set_int(self_object, 0x55c, 8);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, -4);
    set_int(self_object, 0x56c, -6);
    set_int(self_object, 0x4b0, 100);
    set_int(self_object, 0x4b8, 100);
    set_int(target_object, 0x498, 100);
    set_int(target_object, 0x4a0, 200);
    collision_result = 0;
    car_collision_response(self_object, target_object);
    put(int_is(self_object, 0x4b0, 99)
        && int_is(self_object, 0x4b8, 98)
        && int_is(self_object, 0x55c, 8)
        && DAT_00108038 == 101 && DAT_00108040 == 201
        && lookup_calls == 1 && lookup_arg == 1
        && collision_calls == 1 && collision_arg == 1
        && angle_calls == 0 ? '1' : 'x');

    reset();
    set_int(self_object, 0x55c, 0);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_int(target_object, 0x498, 100);
    set_int(target_object, 0x4a0, 200);
    collision_result = 0;
    car_collision_response(self_object, target_object);
    put(int_is(self_object, 0x4b0, 36)
        && int_is(self_object, 0x4b8, 54)
        && int_is(self_object, 0x55c, 1)
        && DAT_00108038 == 64 && DAT_00108040 == 146
        && lookup_calls == 1 && lookup_arg == 1
        && collision_calls == 1 && collision_arg == 1
        && angle_calls == 0 ? '2' : 'x');

    reset();
    set_int(self_object, 0x55c, 8);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_bits(self_object, 0x4cc, 0x3f800000);
    set_int(target_object, 0x498, 100);
    set_int(target_object, 0x4a0, 200);
    set_int(target_object, 0x454, 200);
    set_int(target_object, 0x45c, 300);
    set_bits(target_object, 0x418, 0x3f800000);
    collision_result = 1;
    angle_result[0] = 2;
    car_collision_response(self_object, target_object);
    put(float_is(target_object, 0x418, 0x3f333333)
        && int_is(target_object, 0x450, 0)
        && int_is(target_object, 0x4c8, 0)
        && int_is(target_object, 0x54c, 0)
        && int_is(target_object, 0x548, 0)
        && int_is(self_object, 0x50c, 0)
        && float_is(target_object, 0x460, 0xbf000000)
        && float_is(target_object, 0x468, 0xbf400000)
        && int_is(target_object, 0x454, 199)
        && int_is(target_object, 0x45c, 298)
        && DAT_00108038 == 99 && DAT_00108040 == 198
        && angle_calls == 1
        && angle_first[0] == self_object + 0x4b0
        && angle_second[0] == target_object + 0x498
        && angle_bits[0] == 0x3f800000
        && collision_calls == 1 && collision_arg == 1
        ? '3' : 'x');

    reset();
    set_int(self_object, 0x55c, 8);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_bits(self_object, 0x50c, 0x447a0000);
    set_bits(self_object, 0x4cc, 0x3f800000);
    set_int(target_object, 0x498, 100);
    set_int(target_object, 0x4a0, 200);
    set_int(target_object, 0x454, 200);
    set_int(target_object, 0x45c, 300);
    set_bits(target_object, 0x418, 0x3f800000);
    set_bits(target_object, 0x4a8, 0x3f000000);
    collision_result = 1;
    angle_result[0] = 0;
    angle_result[1] = 3;
    car_collision_response(self_object, target_object);
    put(float_is(target_object, 0x450, 0x43fa0000)
        && float_is(target_object, 0x4c8, 0x3dcccccd)
        && float_is(target_object, 0x54c, 0x43520000)
        && float_is(target_object, 0x548, 0x3e4ccccd)
        && float_is(self_object, 0x50c, 0x43fa0000)
        && float_is(self_object, 0x550, 0x3f000000)
        && float_is(self_object, 0x558, 0x3f400000)
        && float_is(target_object, 0x418, 0x3f800000)
        && float_is(target_object, 0x460, 0xbf000000)
        && float_is(target_object, 0x468, 0xbf400000)
        && int_is(target_object, 0x454, 199)
        && int_is(target_object, 0x45c, 298)
        && DAT_00108038 == 99 && DAT_00108040 == 198
        && angle_calls == 2
        && angle_first[0] == self_object + 0x4b0
        && angle_second[0] == target_object + 0x498
        && angle_bits[0] == 0x3f800000
        && angle_first[1] == target_object + 0x498
        && angle_second[1] == self_object + 0x4b0
        && angle_bits[1] == 0x3f000000
        ? '4' : 'x');

    reset();
    set_int(self_object, 0x55c, 8);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_bits(self_object, 0x50c, 0x447a0000);
    set_bits(self_object, 0x4cc, 0x3f800000);
    set_int(target_object, 0x498, 100);
    set_int(target_object, 0x4a0, 200);
    set_int(target_object, 0x454, 200);
    set_int(target_object, 0x45c, 300);
    set_bits(target_object, 0x4a8, 0x3f000000);
    collision_result = 1;
    angle_result[0] = 0;
    angle_result[1] = 2;
    car_collision_response(self_object, target_object);
    put(float_is(target_object, 0x450, 0x43fa0000)
        && float_is(target_object, 0x4c8, 0xbdcccccd)
        && float_is(target_object, 0x54c, 0x43520000)
        && float_is(target_object, 0x548, 0xbe4ccccd)
        && float_is(self_object, 0x50c, 0x43fa0000)
        && int_is(target_object, 0x454, 199)
        && int_is(target_object, 0x45c, 298)
        && DAT_00108038 == 99 && DAT_00108040 == 198
        && angle_calls == 2 ? '5' : 'x');

    reset();
    set_int(self_object, 0x55c, 8);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_bits(self_object, 0x50c, 0x447a0000);
    set_bits(self_object, 0x4cc, 0x3f800000);
    set_int(target_object, 0x498, 100);
    set_int(target_object, 0x4a0, 200);
    set_int(target_object, 0x454, 200);
    set_int(target_object, 0x45c, 300);
    set_bits(target_object, 0x4a8, 0x3f000000);
    collision_result = 1;
    angle_result[0] = 0;
    angle_result[1] = 1;
    car_collision_response(self_object, target_object);
    put(float_is(target_object, 0x450, 0x43fa0000)
        && float_is(target_object, 0x4c8, 0xbdcccccd)
        && float_is(target_object, 0x54c, 0x43520000)
        && float_is(target_object, 0x548, 0x3e4ccccd)
        && float_is(self_object, 0x50c, 0x43fa0000)
        && int_is(target_object, 0x454, 199)
        && int_is(target_object, 0x45c, 298)
        && DAT_00108038 == 99 && DAT_00108040 == 198
        && angle_calls == 2 ? '6' : 'x');

    reset();
    set_int(self_object, 0x55c, 8);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_bits(self_object, 0x50c, 0x469c4000);
    set_bits(self_object, 0x4cc, 0x3f800000);
    set_int(target_object, 0x498, 100);
    set_int(target_object, 0x4a0, 200);
    set_int(target_object, 0x454, 200);
    set_int(target_object, 0x45c, 300);
    set_bits(target_object, 0x4a8, 0x3f000000);
    collision_result = 1;
    angle_result[0] = 0;
    angle_result[1] = 3;
    car_collision_response(self_object, target_object);
    put(float_is(target_object, 0x450, 0x461c4000)
        && float_is(target_object, 0x4c8, 0x3fc90fdb)
        && float_is(target_object, 0x54c, 0x45834000)
        && float_is(target_object, 0x548, 0x40800000)
        && float_is(self_object, 0x50c, 0x461c4000)
        && float_is(self_object, 0x550, 0x3f000000)
        && float_is(self_object, 0x558, 0x3f400000)
        && int_is(target_object, 0x454, 199)
        && int_is(target_object, 0x45c, 298)
        && DAT_00108038 == 99 && DAT_00108040 == 198
        && angle_calls == 2 ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
