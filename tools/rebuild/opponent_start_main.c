/* The test harness for the rebuilt `opponent_start` module.
 *
 * The harness holds the opponent record array, the player object, the two
 * speed scales, and the `table_lookup` stub.  The stub records the call count
 * and the last index.
 *
 * The player heading is zero on every path, so the sine is 0 and the negated
 * cosine is -1.  The player speed is 50, so the two scales give the exact
 * vertical speed 1 and the exact speed 70.  The paths cover the empty list,
 * the three range guards, the contact-distance boundary, the first and the
 * second body point, an already running record, a record that started before,
 * and the truncation of two non-square distances:
 *
 *     0  the record count is zero: the function changes nothing
 *     1  the record is 2000 units away on x: the x guard skips it
 *     2  the record is 2000 units away on z: the z guard skips it
 *     3  the record is 2000 units away on y: the y guard skips it
 *     4  the body distance is exactly 100 above the contact distance
 *     5  the first body point starts the record with the heading and speed
 *     6  the second body point starts the record
 *     7  an already running record is skipped
 *     8  a record that started before does not increment the counter
 *     9  the distances sqrt(2) and sqrt(13) truncate to 1 and 3
 *     D  the exit marker
 *
 * The float and double values arrive from their bit patterns, because a
 * literal in this minimal image reads the constant segment, which this
 * harness does not load.
 */
#pragma off (check_stack)

void opponent_start(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0010d6f8;
char DAT_0010d700[0x800];
double opponent_vertical_speed_scale;
double opponent_speed_scale;

static char text[8];
static char self[0x800];

static int table_calls;
static int table_arg;

int table_lookup(int index)
{
    table_calls = table_calls + 1;
    table_arg = index;
    return index;
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

static void set_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void set_double(double *p, unsigned int low, unsigned int high)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = low;
    value.u[1] = high;
    *p = value.d;
}

static void self_int(int offset, int value)
{
    *(int *)(self + offset) = value;
}

static int self_get(int offset)
{
    return *(int *)(self + offset);
}

static unsigned int self_float_bits(int offset)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = *(float *)(self + offset);
    return value.u;
}

static void record_int(int index, int offset, int value)
{
    *(int *)(DAT_0010d700 + index * 0x40 + offset) = value;
}

static int record_get(int index, int offset)
{
    return *(int *)(DAT_0010d700 + index * 0x40 + offset);
}

static unsigned int record_float_bits(int index, int offset)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = *(float *)(DAT_0010d700 + index * 0x40 + offset);
    return value.u;
}

static void reset_case(void)
{
    clear_bytes(self, 0x800);
    clear_bytes(DAT_0010d700, 0x800);

    DAT_0010d6f8 = 0;
    set_float((float *)(self + 0x50c), 0x42480000);
    set_float((float *)(self + 0x4cc), 0x00000000);

    table_calls = 0;
    table_arg = -1;
}

static int started(void)
{
    return record_get(0, 0x38) == 1
        && record_get(0, 0x3c) == 1
        && record_float_bits(0, 0x24) == 0x00000000
        && record_float_bits(0, 0x2c) == 0xbf800000
        && record_float_bits(0, 0x34) == 0x3f800000
        && record_float_bits(0, 0x30) == 0x428c0000;
}

int main(void)
{
    set_double(&opponent_vertical_speed_scale, 0x47ae147b, 0x3f947ae1);
    set_double(&opponent_speed_scale, 0x66666666, 0x3ff66666);

    reset_case();
    DAT_0010d6f8 = 0;
    record_int(0, 0x24, 0x11223344);
    opponent_start(self);
    put(record_get(0, 0x24) == 0x11223344 && self_get(0x560) == 0
        && self_get(0x7b8) == 0 && table_calls == 0 ? '0' : 'x');

    reset_case();
    DAT_0010d6f8 = 1;
    record_int(0, 0, -2000);
    opponent_start(self);
    put(record_get(0, 0x38) == 0 && self_get(0x560) == 0
        && table_calls == 0 ? '1' : 'x');

    reset_case();
    DAT_0010d6f8 = 1;
    record_int(0, 8, -2000);
    opponent_start(self);
    put(record_get(0, 0x38) == 0 && self_get(0x560) == 0
        && table_calls == 0 ? '2' : 'x');

    reset_case();
    DAT_0010d6f8 = 1;
    record_int(0, 4, -2000);
    opponent_start(self);
    put(record_get(0, 0x38) == 0 && self_get(0x560) == 0
        && table_calls == 0 ? '3' : 'x');

    reset_case();
    DAT_0010d6f8 = 1;
    self_int(0x544, 50);
    self_int(0x52c, 250);
    self_int(0x538, 0);
    self_int(0x540, 250);
    opponent_start(self);
    put(record_get(0, 0x38) == 0 && self_get(0x560) == 150
        && self_get(0x55c) == 250 && table_calls == 0 ? '4' : 'x');

    reset_case();
    DAT_0010d6f8 = 1;
    self_int(0x544, 50);
    self_int(0x538, 1000);
    opponent_start(self);
    put(started() && self_get(0x7b8) == 1 && self_get(0x55c) == 1000
        && table_calls == 1 && table_arg == 5 ? '5' : 'x');

    reset_case();
    DAT_0010d6f8 = 1;
    self_int(0x544, 50);
    self_int(0x52c, 1000);
    opponent_start(self);
    put(started() && self_get(0x7b8) == 1 && self_get(0x55c) == 0
        && table_calls == 1 && table_arg == 5 ? '6' : 'x');

    reset_case();
    DAT_0010d6f8 = 1;
    self_int(0x544, 50);
    record_int(0, 0x38, 1);
    record_int(0, 0x24, 0x11223344);
    record_int(0, 0x30, 0x55667788);
    opponent_start(self);
    put(record_get(0, 0x38) == 1 && record_get(0, 0x24) == 0x11223344
        && record_get(0, 0x30) == 0x55667788 && self_get(0x7b8) == 0
        && table_calls == 0 ? '7' : 'x');

    reset_case();
    DAT_0010d6f8 = 1;
    self_int(0x544, 50);
    self_int(0x7b8, 7);
    self_int(0x538, 1000);
    record_int(0, 0x3c, 1);
    opponent_start(self);
    put(started() && self_get(0x7b8) == 7 && table_calls == 1
        ? '8' : 'x');

    reset_case();
    DAT_0010d6f8 = 1;
    self_int(0x544, 50);
    self_int(0x52c, 1);
    self_int(0x534, 1);
    self_int(0x538, 2);
    self_int(0x540, 3);
    opponent_start(self);
    put(started() && self_get(0x7b8) == 1 && self_get(0x55c) == 3
        && table_calls == 2 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
