/* The test harness for the rebuilt `map_object_place` module.
 *
 * The harness holds one line descriptor, one input string, the 0x50 map object
 * records at `DAT_001db78`, and the timer step.  The `cos`, `sin`, and `__CHP`
 * stubs live in the local FPU assembly file.
 *
 * The harness runs five paths:
 *
 *     0  a centered line of three codes with angle 0: the codes 1, 2, and 3
 *        take the first three free records at x 640, 0, and -640; every record
 *        holds the line id 7, the vertical position 1696, the start z 0x100,
 *        the amplitudes 0x30 and 0x40, and the timers 0.5, 0.51, and 0.52
 *     1  a left line of two codes with the start override and angle pi/2: the
 *        first record holds the start 100 and 200, the flag 1, the angle pi,
 *        and the targets 3328 and 2080; the rotated origin starts the second
 *        record near 200 and -100 at x 2688
 *     2  the first record is occupied: the first code takes record 1 and the
 *        record 0 keeps its marker
 *     3  the first code is 0x1f: the function places no record and the timer
 *        stays at 2.0
 *     4  the second code is 0x80: the function places the first code and stops,
 *        so record 1 stays free
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because an FPU memory
 * operand of a literal reads the constant pool at a low address in this
 * toolchain.  The float checks compare the bit patterns with a small
 * tolerance.
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x64
#define RECORD_COUNT 0x50

struct map_line {
    int line;
    int field4;
    int field8;
    int centered;
    int flag10;
    int start_x;
    int start_y;
    int start_z;
    int amplitude_x;
    int amplitude_y;
    float angle;
    float timer;
};

void map_object_place(struct map_line *line, char *text);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_001db78[RECORD_COUNT * RECORD_SIZE];
float map_object_timer_step;

static char text[8];
static char line_text[8];
static struct map_line line;

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

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void reset_records(void)
{
    int index;

    clear_bytes(DAT_001db78, RECORD_COUNT * RECORD_SIZE);
    for (index = 0; index < RECORD_COUNT; index++) {
        *(int *)(DAT_001db78 + index * RECORD_SIZE + 8) = -1;
    }
}

static void reset_case(void)
{
    reset_records();
    clear_bytes((char *)&line, sizeof(line));
    clear_bytes(line_text, 8);
    set_bits_float(&map_object_timer_step, 0x3c23d70a);
}

static int record_int(int index, int offset)
{
    return *(int *)(DAT_001db78 + index * RECORD_SIZE + offset);
}

static unsigned int record_bits(int index, int offset)
{
    return *(unsigned int *)(DAT_001db78 + index * RECORD_SIZE + offset);
}

static int near_int(int value, int expected)
{
    int difference;

    difference = value - expected;
    if (difference < 0) {
        difference = -difference;
    }
    return difference <= 2;
}

static int near_bits(unsigned int value, unsigned int expected)
{
    unsigned int difference;

    difference = value > expected ? value - expected : expected - value;
    return difference <= 0x10000;
}

static int record_zero(int index)
{
    return record_int(index, 0x14) == 0
        && record_int(index, 0x18) == 0
        && record_int(index, 0x20) == 0
        && record_int(index, 0x24) == 0
        && record_int(index, 0x28) == 0
        && record_int(index, 0x30) == 0
        && record_bits(index, 0x1c) == 0x3f800000
        && record_bits(index, 0x2c) == 0x3f800000
        && record_bits(index, 0x58) == 0x3f800000
        && record_bits(index, 0x5c) == 0x3f800000;
}

int main(void)
{
    reset_case();
    line.line = 7;
    line.field4 = 1;
    line.centered = 1;
    line.start_z = 0x100;
    line.amplitude_x = 0x30;
    line.amplitude_y = 0x40;
    set_bits_float(&line.timer, 0x3f000000);
    line_text[0] = 0x21;
    line_text[1] = 0x22;
    line_text[2] = 0x23;
    map_object_place(&line, line_text);
    put(record_int(0, 8) == 1
        && record_int(0, 4) == 7
        && record_int(0, 0x40) == 640
        && record_int(0, 0x34) == 640
        && record_int(0, 0x44) == 1696
        && record_int(0, 0x38) == 1696
        && record_int(0, 0x3c) == 0x100
        && record_int(0, 0x48) == 0
        && record_int(0, 0x10) == 0
        && record_int(0, 0x0c) == 0
        && record_int(0, 0x4c) == 0x30
        && record_int(0, 0x50) == 0x40
        && record_bits(0, 0x60) == 0x3f000000
        && record_zero(0)
        && record_int(1, 8) == 2
        && record_int(1, 0x40) == 0
        && record_int(1, 0x34) == 0
        && near_bits(record_bits(1, 0x60), 0x3f028f5c)
        && record_int(2, 8) == 3
        && record_int(2, 0x40) == -640
        && record_int(2, 0x34) == -640
        && near_bits(record_bits(2, 0x60), 0x3f051eb8)
        && record_int(3, 8) == -1
        && near_bits(*(unsigned int *)&line.timer, 0x3f07ae14) ? '0' : 'x');

    reset_case();
    line.line = 9;
    line.field8 = 2;
    line.flag10 = 1;
    line.start_x = 100;
    line.start_y = 200;
    line.start_z = 300;
    line.amplitude_x = 5;
    line.amplitude_y = 6;
    set_bits_float(&line.angle, 0x3fc90fdb);
    line_text[0] = 0x41;
    line_text[1] = 0x42;
    map_object_place(&line, line_text);
    put(record_int(0, 8) == 0x21
        && record_int(0, 4) == 9
        && record_int(0, 0x40) == 3328
        && record_int(0, 0x34) == 100
        && record_int(0, 0x44) == 2080
        && record_int(0, 0x38) == 200
        && record_int(0, 0x3c) == 300
        && record_int(0, 0x10) == 0x40490fdb
        && record_int(0, 0x0c) == 1
        && record_int(0, 0x4c) == 5
        && record_int(0, 0x50) == 6
        && record_bits(0, 0x60) == 0
        && record_int(1, 8) == 0x22
        && record_int(1, 0x40) == 2688
        && near_int(record_int(1, 0x34), 200)
        && near_int(record_int(1, 0x38), -100)
        && near_bits(record_bits(1, 0x60), 0x3c23d70a)
        && record_int(2, 8) == -1
        && near_bits(*(unsigned int *)&line.timer, 0x3ca3d70a) ? '1' : 'x');

    reset_case();
    *(int *)(DAT_001db78 + 8) = 5;
    line.line = 1;
    set_bits_float(&line.timer, 0x3f800000);
    line_text[0] = 0x41;
    map_object_place(&line, line_text);
    put(record_int(0, 8) == 5
        && record_int(1, 8) == 0x21
        && record_int(1, 0x40) == 4608
        && record_int(1, 0x34) == 4608
        && record_int(1, 0x44) == 2080
        && record_bits(1, 0x60) == 0x3f800000
        && record_int(2, 8) == -1
        && near_bits(*(unsigned int *)&line.timer, 0x3f8147ae) ? '2' : 'x');

    reset_case();
    set_bits_float(&line.timer, 0x40000000);
    line_text[0] = 0x1f;
    line_text[1] = 0x41;
    map_object_place(&line, line_text);
    put(record_int(0, 8) == -1
        && line.start_x == 0
        && line.start_y == 0
        && record_bits(0, 0x60) == 0
        && near_bits(*(unsigned int *)&line.timer, 0x40000000) ? '3' : 'x');

    reset_case();
    line_text[0] = 0x41;
    line_text[1] = (char)0x80;
    line_text[2] = 0x42;
    map_object_place(&line, line_text);
    put(record_int(0, 8) == 0x21
        && record_bits(0, 0x60) == 0
        && record_int(1, 8) == -1
        && near_bits(*(unsigned int *)&line.timer, 0x3c23d70a) ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
