/* The test harness for the rebuilt `car_collision_push` module.
 *
 * The harness holds the scanning car record, the candidate car record, and the
 * double constant 0.5.  The module has no callee, so the harness supplies no
 * stub.  Every test resets the two records, the constant, and the fields, calls
 * the module, and prints one marker.
 *
 * The harness runs five paths:
 *
 *     0  the scanning car is faster: its speed becomes the average 3.0, and
 *        the candidate positions move to 103 and 204
 *     1  the candidate is faster: its speed becomes the average 3.0, and the
 *        candidate positions move to 96 and 195 with the negative deltas
 *     2  the distance is zero: the module sets it to 1 and moves the
 *        candidate positions to 23 and 24
 *     3  the positions are negative: the two results -1.7 truncate toward
 *        zero to -1
 *     4  the speeds are fractional: the candidate speed becomes 2.25
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because an FPU memory
 * operand of a literal reads the constant pool at a low address in this
 * toolchain.  The float checks compare the bit patterns exactly.
 */
#pragma off (check_stack)

void car_collision_push(char *self, char *other);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

double car_collision_push_half;

static char self_object[0x900];
static char other_object[0x900];
static char text[8];

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
    clear_bytes(self_object, 0x900);
    clear_bytes(other_object, 0x900);
    set_bits_double(&car_collision_push_half, 0x00000000, 0x3fe00000);
}

int main(void)
{
    reset();
    set_bits(self_object, 0x418, 0x40800000);
    set_bits(other_object, 0x418, 0x40000000);
    set_int(self_object, 0x484, 10);
    set_int(self_object, 0x488, 4);
    set_int(self_object, 0x48c, 6);
    set_int(self_object, 0x494, 8);
    set_int(other_object, 0x454, 100);
    set_int(other_object, 0x45c, 200);
    car_collision_push(self_object, other_object);
    put(float_is(self_object, 0x418, 0x40400000)
        && float_is(other_object, 0x418, 0x40000000)
        && int_is(self_object, 0x484, 10)
        && int_is(other_object, 0x454, 103)
        && int_is(other_object, 0x45c, 204) ? '0' : 'x');

    reset();
    set_bits(self_object, 0x418, 0x3f800000);
    set_bits(other_object, 0x418, 0x40a00000);
    set_int(self_object, 0x484, 10);
    set_int(self_object, 0x488, 4);
    set_int(self_object, 0x48c, -6);
    set_int(self_object, 0x494, -8);
    set_int(other_object, 0x454, 100);
    set_int(other_object, 0x45c, 200);
    car_collision_push(self_object, other_object);
    put(float_is(self_object, 0x418, 0x3f800000)
        && float_is(other_object, 0x418, 0x40400000)
        && int_is(other_object, 0x454, 96)
        && int_is(other_object, 0x45c, 195) ? '1' : 'x');

    reset();
    set_bits(self_object, 0x418, 0x40000000);
    set_bits(other_object, 0x418, 0x40000000);
    set_int(self_object, 0x484, 0);
    set_int(self_object, 0x488, 10);
    set_int(self_object, 0x48c, 3);
    set_int(self_object, 0x494, 4);
    set_int(other_object, 0x454, 50);
    set_int(other_object, 0x45c, 60);
    car_collision_push(self_object, other_object);
    put(float_is(self_object, 0x418, 0x40000000)
        && int_is(self_object, 0x484, 1)
        && int_is(other_object, 0x454, 23)
        && int_is(other_object, 0x45c, 24) ? '2' : 'x');

    reset();
    set_bits(self_object, 0x418, 0x40000000);
    set_bits(other_object, 0x418, 0x40000000);
    set_int(self_object, 0x484, 10);
    set_int(self_object, 0x488, 7);
    set_int(self_object, 0x48c, 1);
    set_int(self_object, 0x494, 1);
    set_int(other_object, 0x454, -2);
    set_int(other_object, 0x45c, -2);
    car_collision_push(self_object, other_object);
    put(float_is(self_object, 0x418, 0x40000000)
        && int_is(other_object, 0x454, -1)
        && int_is(other_object, 0x45c, -1) ? '3' : 'x');

    reset();
    set_bits(self_object, 0x418, 0x3fc00000);
    set_bits(other_object, 0x418, 0x40400000);
    set_int(self_object, 0x484, 1);
    set_int(other_object, 0x454, 100);
    set_int(other_object, 0x45c, 200);
    car_collision_push(self_object, other_object);
    put(float_is(self_object, 0x418, 0x3fc00000)
        && float_is(other_object, 0x418, 0x40100000)
        && int_is(other_object, 0x454, 100)
        && int_is(other_object, 0x45c, 200) ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
