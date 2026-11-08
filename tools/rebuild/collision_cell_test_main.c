/* The test harness for the rebuilt `collision_cell_test` module.
 *
 * The harness holds one collision cell with up to four 0x40-byte records, four
 * objects with a 0x100-byte body and the shape at offset 0x40, the handle
 * table, the world position, the two float constants, and the three callee
 * stubs.  The record probe stubs follow a small script.  The rotation stub
 * records the input pair and the angle and writes a chosen output pair.
 *
 * The harness runs ten paths:
 *
 *     0  the empty cell: the function returns 0 and calls no stub
 *     1  the record stays outside the radius: the record is skipped
 *     2  the mode 0 misses all three probes: the position advances by 30
 *     3  the mode 1 misses all three probes: the position advances by 30
 *     4  the mode 0 hits the first probe: the angle rotates back, the pair
 *        takes the second output, and the shape byte lands in the surface
 *     5  the mode 1 hits the first probe: the function returns 1 at once and
 *        keeps the pair and the surface byte
 *     6  the mode 0 misses twice and hits the third probe: the position
 *        advances by 20 before the hit
 *     7  the squared offset equals the squared radius: the record is tested
 *     8  the first record is skipped and the second record hits: the loop
 *        advances and the object globals follow the second record
 *     9  the negative angle hits: the two rotations take pi and minus pi
 *     D  the exit marker
 *
 * The float bits are set exactly, because a float literal in this minimal image
 * reads the constant segment, which this harness does not load.
 */
#pragma off (check_stack)

int collision_cell_test(int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define RECORD_STRIDE 0x40
#define MAX_RECORDS 4
#define OBJECT_SIZE 0x100

struct rotate_pair {
    int first;
    int pad;
    int second;
};

int *DAT_00021540;
int *DAT_00108034;
int DAT_00108038;
int DAT_00108040;
int DAT_00108024;
char *DAT_0010c740;
char *DAT_0010c744;
char *DAT_0010c748;
int DAT_0010c728;
int DAT_0010c730;
int DAT_0010c734;
int DAT_0010c73c;
float DAT_00100404;
float DAT_00100408;

static char text[8];
static unsigned char cell_buf[0x20];
static unsigned char records[RECORD_STRIDE * MAX_RECORDS];
static unsigned char objects[MAX_RECORDS][OBJECT_SIZE];
static int object_handles[MAX_RECORDS];
static int cell_slot;
static int shape_calls;
static int shape_return[8];
static int plain_calls;
static int plain_return[8];
static int rotate_calls;
static int rotate_in0[8];
static int rotate_in1[8];
static unsigned int rotate_angle[8];
static int rotate_out_first[8];
static int rotate_out_second[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_float(float *p, unsigned int bits)
{
    *(unsigned int *)p = bits;
}

static void clear_bytes(unsigned char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static void set_cell(int count)
{
    *(int *)(cell_buf + 0x14) = count;
}

static void set_object(int index, int radius, int a, int b, int surface)
{
    unsigned char *object;

    object = objects[index];
    *(int *)(object + 0x18) = 0x40;
    *(int *)(object + 0x40) = radius;
    *(short *)(object + 0x44) = (short)a;
    *(short *)(object + 0x48) = (short)b;
    object[0x40 + 0x6b] = (unsigned char)surface;
}

static void set_record(int index, int handle, int x, int y, int angle)
{
    char *record;

    record = (char *)records + index * RECORD_STRIDE;
    *(int *)(record + 0x00) = handle;
    *(int *)(record + 0x08) = x;
    *(int *)(record + 0x10) = y;
    *(int *)(record + 0x18) = angle;
}

int sub_1bfd4(void)
{
    int result;

    result = 0;
    if (shape_calls < 8) {
        result = shape_return[shape_calls];
    }
    shape_calls = shape_calls + 1;
    return result;
}

int sub_1c0ac(void)
{
    int result;

    result = 0;
    if (plain_calls < 8) {
        result = plain_return[plain_calls];
    }
    plain_calls = plain_calls + 1;
    return result;
}

void rotate_pair(struct rotate_pair *out, struct rotate_pair *in, float angle)
{
    int index;

    index = rotate_calls;
    if (index < 8) {
        rotate_in0[index] = in->first;
        rotate_in1[index] = in->second;
        rotate_angle[index] = *(unsigned int *)&angle;
        out->first = rotate_out_first[index];
        out->second = rotate_out_second[index];
    } else {
        out->first = 0;
        out->second = 0;
    }
    rotate_calls = rotate_calls + 1;
}

static void reset(void)
{
    int index;

    clear_bytes(cell_buf, 0x20);
    clear_bytes(records, RECORD_STRIDE * MAX_RECORDS);
    clear_bytes((unsigned char *)objects, MAX_RECORDS * OBJECT_SIZE);
    cell_slot = (int)cell_buf;
    DAT_00108034 = &cell_slot;
    DAT_00021540 = object_handles;
    for (index = 0; index < MAX_RECORDS; index++) {
        object_handles[index] = (int)objects[index];
    }
    *(char **)cell_buf = (char *)records;
    set_cell(0);
    DAT_00108038 = 0;
    DAT_00108040 = 0;
    DAT_00108024 = 0x55;
    DAT_0010c728 = 0;
    DAT_0010c730 = 0;
    DAT_0010c734 = 30;
    DAT_0010c73c = -40;
    DAT_0010c740 = 0;
    DAT_0010c744 = 0;
    DAT_0010c748 = 0;
    set_float(&DAT_00100404, 0x3a800000);
    set_float(&DAT_00100408, 0x40490fdb);
    shape_calls = 0;
    plain_calls = 0;
    rotate_calls = 0;
    for (index = 0; index < 8; index++) {
        shape_return[index] = 0;
        plain_return[index] = 0;
        rotate_in0[index] = 0;
        rotate_in1[index] = 0;
        rotate_angle[index] = 0;
        rotate_out_first[index] = 100;
        rotate_out_second[index] = -50;
    }
}

int main(void)
{
    int result;

    reset();
    result = collision_cell_test(0);
    put(result == 0 && shape_calls == 0 && plain_calls == 0
        && rotate_calls == 0 && DAT_0010c728 == 0 && DAT_0010c730 == 0
        ? '0' : 'x');

    reset();
    set_cell(1);
    set_object(0, 5, 0, 0, 0x11);
    set_record(0, 0, 0, 0, 1024);
    DAT_00108038 = 6;
    DAT_00108040 = 8;
    result = collision_cell_test(0);
    put(result == 0 && shape_calls == 0 && plain_calls == 0
        && rotate_calls == 0 && DAT_00108024 == 0x55 ? '1' : 'x');

    reset();
    set_cell(1);
    set_object(0, 100, 10, 20, 0x22);
    set_record(0, 0, 1000, 2000, 1024);
    DAT_00108038 = 1000;
    DAT_00108040 = 2000;
    result = collision_cell_test(0);
    put(result == 0 && shape_calls == 3 && plain_calls == 0
        && rotate_calls == 1 && rotate_in0[0] == 0 && rotate_in1[0] == 0
        && rotate_angle[0] == 0x40490fdb
        && DAT_0010c728 == 1130 && DAT_0010c730 == 1980
        && DAT_00108024 == 0x55 ? '2' : 'x');

    reset();
    set_cell(1);
    set_object(0, 100, 10, 20, 0x33);
    set_record(0, 0, 1000, 2000, 1024);
    DAT_00108038 = 1000;
    DAT_00108040 = 2000;
    result = collision_cell_test(1);
    put(result == 0 && shape_calls == 0 && plain_calls == 3
        && rotate_calls == 1 && DAT_0010c728 == 1130
        && DAT_0010c730 == 1980 ? '3' : 'x');

    reset();
    set_cell(1);
    set_object(0, 100, 10, 20, 0xab);
    set_record(0, 0, 1000, 2000, 1024);
    DAT_00108038 = 1000;
    DAT_00108040 = 2000;
    shape_return[0] = 1;
    rotate_out_first[1] = 7;
    rotate_out_second[1] = 9;
    result = collision_cell_test(0);
    put(result == 1 && shape_calls == 1 && plain_calls == 0
        && rotate_calls == 2 && DAT_0010c728 == 1100
        && DAT_0010c730 == 1950 && rotate_angle[1] == 0xc0490fdb
        && rotate_in0[1] == 30 && rotate_in1[1] == -40
        && DAT_0010c734 == 7 && DAT_0010c73c == 9
        && DAT_00108024 == 0xab ? '4' : 'x');

    reset();
    set_cell(1);
    set_object(0, 100, 10, 20, 0xcd);
    set_record(0, 0, 1000, 2000, 1024);
    DAT_00108038 = 1000;
    DAT_00108040 = 2000;
    DAT_00108024 = 0x55;
    plain_return[0] = 1;
    result = collision_cell_test(1);
    put(result == 1 && shape_calls == 0 && plain_calls == 1
        && rotate_calls == 1 && DAT_0010c728 == 1100
        && DAT_0010c730 == 1950 && DAT_0010c734 == 30
        && DAT_0010c73c == -40 && DAT_00108024 == 0x55 ? '5' : 'x');

    reset();
    set_cell(1);
    set_object(0, 100, 10, 20, 0xef);
    set_record(0, 0, 1000, 2000, 1024);
    DAT_00108038 = 1000;
    DAT_00108040 = 2000;
    shape_return[0] = 0;
    shape_return[1] = 0;
    shape_return[2] = 1;
    rotate_out_first[1] = 7;
    rotate_out_second[1] = 9;
    result = collision_cell_test(0);
    put(result == 1 && shape_calls == 3 && rotate_calls == 2
        && DAT_0010c728 == 1120 && DAT_0010c730 == 1970
        && DAT_0010c734 == 7 && DAT_0010c73c == 9
        && DAT_00108024 == 0xef ? '6' : 'x');

    reset();
    set_cell(1);
    set_object(0, 5, 0, 0, 0x77);
    set_record(0, 0, 0, 0, 0);
    DAT_00108038 = 3;
    DAT_00108040 = 4;
    result = collision_cell_test(0);
    put(result == 0 && shape_calls == 3 && rotate_calls == 1
        && DAT_0010c728 == 130 && DAT_0010c730 == -20 ? '7' : 'x');

    reset();
    set_cell(2);
    set_object(0, 5, 0, 0, 0x10);
    set_object(1, 100, 0, 0, 0x21);
    set_record(0, 0, 0, 0, 0);
    set_record(1, 1, 1000, 2000, 1024);
    DAT_00108038 = 1000;
    DAT_00108040 = 2000;
    shape_return[0] = 1;
    rotate_out_first[1] = 7;
    rotate_out_second[1] = 9;
    result = collision_cell_test(0);
    put(result == 1 && shape_calls == 1 && rotate_calls == 2
        && DAT_0010c734 == 7 && DAT_0010c73c == 9
        && DAT_00108024 == 0x21
        && DAT_0010c748 == (char *)objects[1]
        && DAT_0010c740 == (char *)objects[1] + 0x40 ? '8' : 'x');

    reset();
    set_cell(1);
    set_object(0, 100, 10, 20, 0xff);
    set_record(0, 0, 1000, 2000, -1024);
    DAT_00108038 = 1000;
    DAT_00108040 = 2000;
    shape_return[0] = 1;
    rotate_out_first[1] = 7;
    rotate_out_second[1] = 9;
    result = collision_cell_test(0);
    put(result == 1 && shape_calls == 1 && rotate_calls == 2
        && rotate_angle[0] == 0xc0490fdb
        && rotate_angle[1] == 0x40490fdb
        && DAT_00108024 == 0xff ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
