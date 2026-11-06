/* The test harness for the rebuilt `ai_update` module.
 *
 * The harness holds the record count, the record storage, the three-position
 * ground and height values, the pointer table, the collision outputs, the
 * frame delta, the constants, and the two callee stubs.  The rotate stub
 * records the input pair and the angle and writes a chosen result.  The
 * collision stub records its call and returns a chosen value.
 *
 * The harness runs eleven paths:
 *
 *     0  a zero count: the loop runs no pass and touches no record
 *     1  a stopped record with no publish flag: no callee runs
 *     2  a stopped record with the publish flag: the target record takes the
 *        position, the height, and the three zero angles
 *     3  a running record without a collision: the position advances, the
 *        angles turn, the speed falls, the target takes the moved position,
 *        and the published angle masks to 0x345
 *     4  a running record with a collision: the position restores, the speed
 *        scales by 0.3, the two sign bits flip, the height clamps to the saved
 *        ground value, and the vertical speed reverses to 200.0
 *     5  a running record that stops without a collision: the pair rotates,
 *        the angle at +0xc takes pi / 2, the run flag clears, and the target
 *        takes the angle 512
 *     6  a running record that stops with a collision: the height takes
 *        -0x4e20, the run flag stays set, and the pair does not rotate
 *     7  the height clamp with a small vertical speed: the vertical speed
 *        clears, because its magnitude stays below 50.0
 *     8  a negative fractional sum: the truncation gives -499 and the second
 *        position gives 2500
 *     9  the publish of a negative angle: the mask gives 0x600
 *     D  the exit marker
 *
 * The constants and the expected values are set from their exact bit patterns,
 * because a float literal in this minimal image reads the constant segment,
 * which this harness does not load.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void ai_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x40
#define MAX_RECORDS 4

int DAT_0010d6f8;
char DAT_0010d700[STRIDE * MAX_RECORDS];
float DAT_00100cc5;
float DAT_00100cc9;
float DAT_00100ccd;
float DAT_00100cd1;
float DAT_00100cd5;
float DAT_00100ca5;
float DAT_00100cb1;
double DAT_00100ca9;
double DAT_00100cb5;
double DAT_00100cbd;
double DAT_00100c8d;
double DAT_00100c95;
double DAT_00100c9d;
int DAT_0010c734;
int DAT_0010c73c;
int DAT_0010c72c;
unsigned int DAT_0010653c[64];
int DAT_00108038;
int DAT_00108040;
float DAT_00118e04;

static char text[8];
static int targets[4][16];
static int rotate_calls;
static int rotate_in0;
static int rotate_in1;
static unsigned int rotate_angle;
static int rotate_out0;
static int collision_calls;
static int collision_result;

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

static void set_double_bits(double *p, unsigned int lo, unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.u[0] = lo;
    v.u[1] = hi;
    *p = v.d;
}

static char *record_at(int index)
{
    return DAT_0010d700 + index * STRIDE;
}

static void set_int(int index, int offset, int value)
{
    *(int *)(record_at(index) + offset) = value;
}

static void set_float(int index, int offset, unsigned int bits)
{
    *(unsigned int *)(record_at(index) + offset) = bits;
}

static int get_int(int index, int offset)
{
    return *(int *)(record_at(index) + offset);
}

static int target_int(int index, int offset)
{
    return targets[index][offset / 4];
}

static void clear_bytes(char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

double fabs(double value)
{
    return value < 0.0 ? -value : value;
}

void ai_rotate(int *out, int *in, float angle)
{
    rotate_calls = rotate_calls + 1;
    rotate_in0 = in[0];
    rotate_in1 = in[1];
    rotate_angle = *(unsigned int *)&angle;
    out[0] = rotate_out0;
}

int ai_collision(int value)
{
    collision_calls = collision_calls + 1;
    return collision_result;
}

static void reset(void)
{
    int index;

    clear_bytes(DAT_0010d700, STRIDE * MAX_RECORDS);
    clear_bytes((char *)targets, (int)sizeof(targets));
    DAT_0010d6f8 = 0;
    DAT_0010c734 = 30;
    DAT_0010c73c = -40;
    DAT_0010c72c = 0;
    DAT_00108038 = 0;
    DAT_00108040 = 0;
    set_bits((unsigned int *)&DAT_00100cc5, 0xbfc90fdb);
    set_bits((unsigned int *)&DAT_00100cc9, 0x40490fdb);
    set_bits((unsigned int *)&DAT_00100ccd, 0x47340000);
    set_bits((unsigned int *)&DAT_00100cd1, 0x3fc90fdb);
    set_bits((unsigned int *)&DAT_00100cd5, 0x44800000);
    set_bits((unsigned int *)&DAT_00100ca5, 0x44fa0000);
    set_bits((unsigned int *)&DAT_00100cb1, 0x43fa0000);
    set_double_bits(&DAT_00100ca9, 0x33333333, 0x3fd33333);
    set_double_bits(&DAT_00100cb5, 0x9999999a, 0x3fe99999);
    set_double_bits(&DAT_00100cbd, 0x00000000, 0x40490000);
    set_double_bits(&DAT_00100c8d, 0xd2f1a9fc, 0x3f80624d);
    set_double_bits(&DAT_00100c95, 0xd2f1a9fc, 0x3f70624d);
    set_double_bits(&DAT_00100c9d, 0xbc6a7efa, 0x3f589374);
    set_bits((unsigned int *)&DAT_00118e04, 0x3f000000);
    for (index = 0; index < 4; index++) {
        DAT_0010653c[index * 7] = (unsigned int)targets[index];
    }
    rotate_calls = 0;
    rotate_in0 = 0;
    rotate_in1 = 0;
    rotate_angle = 0;
    rotate_out0 = 0;
    collision_calls = 0;
    collision_result = 1;
}

static void stopped_record(int index)
{
    set_int(index, 0x00, 10);
    set_int(index, 0x04, 20);
    set_int(index, 0x08, 30);
    set_float(index, 0x0c, 0x00000000);
    set_float(index, 0x10, 0x00000000);
    set_float(index, 0x14, 0x00000000);
    set_int(index, 0x18, index);
    set_int(index, 0x20, 0);
    set_int(index, 0x38, 0);
}

static void running_record(int index, int x, int y, int z)
{
    set_int(index, 0x00, x);
    set_int(index, 0x04, y);
    set_int(index, 0x08, z);
    set_float(index, 0x0c, 0x00000000);
    set_float(index, 0x10, 0x00000000);
    set_float(index, 0x14, 0x00000000);
    set_int(index, 0x18, index);
    set_int(index, 0x20, 0);
    set_float(index, 0x24, 0x3f800000);
    set_float(index, 0x28, 0x00000000);
    set_float(index, 0x2c, 0x00000000);
    set_float(index, 0x30, 0x451c4000);
    set_float(index, 0x34, 0x447a0000);
    set_int(index, 0x38, 1);
}

int main(void)
{
    reset();
    DAT_0010d6f8 = 0;
    set_int(0, 0x00, 10);
    set_int(0, 0x38, 1);
    ai_update();
    put(collision_calls == 0 && rotate_calls == 0
        && get_int(0, 0x00) == 10 ? '0' : 'x');

    reset();
    DAT_0010d6f8 = 1;
    stopped_record(0);
    ai_update();
    put(collision_calls == 0 && rotate_calls == 0
        && get_int(0, 0x00) == 10 && target_int(0, 8) == 0 ? '1' : 'x');

    reset();
    DAT_0010d6f8 = 1;
    stopped_record(0);
    set_int(0, 0x20, 1);
    ai_update();
    put(collision_calls == 0 && rotate_calls == 0
        && get_int(0, 0x20) == 0
        && target_int(0, 8) == 10
        && target_int(0, 12) == 20
        && target_int(0, 16) == 30
        && target_int(0, 20) == 0
        && target_int(0, 24) == 0
        && target_int(0, 28) == 0 ? '2' : 'x');

    reset();
    DAT_0010d6f8 = 1;
    running_record(0, 1000, 1000, 2000);
    set_float(0, 0x30, 0x451c4000);
    set_float(0, 0x34, 0x447a0000);
    ai_update();
    put(collision_calls == 1 && rotate_calls == 0
        && get_int(0, 0x00) == 2250
        && get_int(0, 0x04) == 1750
        && get_int(0, 0x08) == 2000
        && get_int(0, 0x38) == 1
        && DAT_00108038 == 2250
        && DAT_00108040 == 2000
        && target_int(0, 8) == 2250
        && target_int(0, 12) == 1750
        && target_int(0, 16) == 2000
        && target_int(0, 20) == 0x345
        && target_int(0, 24) == 0
        && target_int(0, 28) == 0 ? '3' : 'x');

    reset();
    DAT_0010d6f8 = 1;
    running_record(0, 1000, 1000, 2000);
    collision_result = 0;
    set_float(0, 0x34, 0x00000000);
    ai_update();
    put(collision_calls == 1 && rotate_calls == 0
        && get_int(0, 0x00) == 1000
        && get_int(0, 0x04) == 1000
        && get_int(0, 0x08) == 2000
        && DAT_0010c72c == 1000
        && (get_int(0, 0x30) == 0x43e10000)
        && (get_int(0, 0x34) == 0x43480000)
        && (*(unsigned char *)(record_at(0) + 0x27) == 0xbf)
        && (*(unsigned char *)(record_at(0) + 0x2f) == 0x80)
        && target_int(0, 8) == 1000
        && target_int(0, 12) == 1000
        && target_int(0, 16) == 2000 ? '4' : 'x');

    reset();
    DAT_0010d6f8 = 1;
    running_record(0, 1000, 1000, 2000);
    set_float(0, 0x24, 0x00000000);
    set_float(0, 0x30, 0x447a0000);
    set_float(0, 0x34, 0x437a0000);
    ai_update();
    put(collision_calls == 1 && rotate_calls == 1
        && rotate_in0 == 30 && rotate_in1 == -40
        && rotate_angle == 0xbfc90fdb
        && get_int(0, 0x38) == 0
        && (get_int(0, 0x0c) == 0x3fc90fdb)
        && get_int(0, 0x04) == 1000
        && target_int(0, 20) == 512 ? '5' : 'x');

    reset();
    DAT_0010d6f8 = 1;
    running_record(0, 1000, 1000, 2000);
    collision_result = 0;
    set_float(0, 0x24, 0x00000000);
    set_float(0, 0x30, 0x447a0000);
    set_float(0, 0x34, 0x437a0000);
    ai_update();
    put(collision_calls == 1 && rotate_calls == 0
        && get_int(0, 0x38) == 1
        && get_int(0, 0x04) == -0x4e20
        && DAT_0010c72c == 1000
        && target_int(0, 12) == -0x4e20 ? '6' : 'x');

    reset();
    DAT_0010d6f8 = 1;
    running_record(0, 1000, 1000, 2000);
    set_float(0, 0x24, 0x00000000);
    set_float(0, 0x30, 0x451c4000);
    set_float(0, 0x34, 0x43660000);
    DAT_0010c72c = 1000;
    ai_update();
    put(collision_calls == 1 && rotate_calls == 0
        && get_int(0, 0x04) == 1000
        && get_int(0, 0x34) == 0
        && get_int(0, 0x38) == 1
        && target_int(0, 12) == 1000 ? '7' : 'x');

    reset();
    DAT_0010d6f8 = 1;
    running_record(0, -3000, 1000, 0);
    set_float(0, 0x2c, 0x3f800000);
    set_float(0, 0x30, 0x459c4800);
    ai_update();
    put(collision_calls == 1 && rotate_calls == 0
        && get_int(0, 0x00) == -499
        && get_int(0, 0x08) == 2500
        && target_int(0, 8) == -499
        && target_int(0, 16) == 2500 ? '8' : 'x');

    reset();
    DAT_0010d6f8 = 1;
    stopped_record(0);
    set_int(0, 0x20, 1);
    set_float(0, 0x0c, 0xbfc90fdb);
    ai_update();
    put(collision_calls == 0 && rotate_calls == 0
        && target_int(0, 20) == 0x600
        && target_int(0, 24) == 0
        && target_int(0, 28) == 0 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
