/* The test harness for the rebuilt `race_clock_update` module.
 *
 * The harness holds the player object, the traffic records, the input bytes,
 * the mode word, the saved word, the record count, the player index, and the
 * frame delta.  It runs these paths:
 *
 *     0  input bit 0 clear: the function returns at once
 *     1  input bit 2 set: the function returns at once
 *     2  the +0x784 gate clear: the two clocks stay, the remaining time
 *        counts down, and the control bit 0x40 clears
 *     3  the +0x784 gate set: the total time and the lap time both advance
 *     4  the saved word bit 4 set: the remaining time stays
 *     5  the traffic scan skips a negative record and copies the time of the
 *        next open record to the total time
 *     6  the traffic scan stops at the player index and copies no time
 *     7  the remaining time below zero with a road speed of 1.0 keeps the
 *        control bit 0x40
 *     8  the remaining time below zero with a road speed of 0.5 clears the
 *        control bit and sets the input bit 0x10
 *     9  the remaining time that stops at zero clears the control bit
 *     D  the exit marker
 *
 * The harness assigns the float values at run time, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void race_clock_update(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_00118e1c;
unsigned char DAT_00118e1d;
float DAT_00118e08;
int DAT_0011d260;
int DAT_0011d274;
int DAT_00115218;
int DAT_0011d250;
char DAT_00113cd8[3 * 680];

static char text[8];
static char object[0x900];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_object(void)
{
    int index;

    for (index = 0; index < 0x900; index++) {
        object[index] = 0;
    }
    for (index = 0; index < 3 * 680; index++) {
        DAT_00113cd8[index] = 0;
    }
    DAT_00118e1c = 1;
    DAT_00118e1d = 0;
    DAT_00118e08 = 1.0f;
    DAT_0011d260 = 0;
    DAT_0011d274 = 0;
    DAT_00115218 = 3;
    DAT_0011d250 = 2;
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static void set_float(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(object + offset) = value.f;
}

static int int_is(int offset, int value)
{
    return *(int *)(object + offset) == value;
}

static int near_float(int offset, float expected)
{
    float actual;
    float difference;

    actual = *(float *)(object + offset);
    difference = actual - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference <= 0.001f;
}

static void set_record(int index, int status)
{
    *(int *)(DAT_00113cd8 + index * 680) = status;
}

static void set_record_time(int index, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(DAT_00113cd8 + index * 680 + 0x2c) = value.f;
}

int main(void)
{
    clear_object();
    DAT_00118e1c = 0;
    set_int(0x784, 1);
    set_float(0x78c, 0x3f800000);
    set_float(0x794, 0x40000000);
    set_float(0x790, 0x40a00000);
    race_clock_update(object);
    put(near_float(0x78c, 1.0f) && near_float(0x794, 2.0f)
        && near_float(0x790, 5.0f) && DAT_00118e1d == 0 ? '0' : 'x');

    clear_object();
    DAT_00118e1c = 5;
    race_clock_update(object);
    put(near_float(0x78c, 0.0f) && near_float(0x794, 0.0f)
        && near_float(0x790, 0.0f) && DAT_00118e1d == 0 ? '1' : 'x');

    clear_object();
    set_float(0x78c, 0x3f800000);
    set_float(0x794, 0x40000000);
    set_float(0x790, 0x40a00000);
    DAT_00118e1d = 0x40;
    race_clock_update(object);
    put(near_float(0x78c, 1.0f) && near_float(0x794, 2.0f)
        && near_float(0x790, 4.0f) && DAT_00118e1d == 0 ? '2' : 'x');

    clear_object();
    set_int(0x784, 1);
    set_float(0x78c, 0x3f800000);
    set_float(0x794, 0x40000000);
    set_float(0x790, 0x40a00000);
    race_clock_update(object);
    put(near_float(0x78c, 2.0f) && near_float(0x794, 3.0f)
        && near_float(0x790, 4.0f) ? '3' : 'x');

    clear_object();
    DAT_0011d274 = 4;
    set_int(0x784, 1);
    set_float(0x78c, 0x3f800000);
    set_float(0x794, 0x40000000);
    set_float(0x790, 0x40a00000);
    race_clock_update(object);
    put(near_float(0x78c, 2.0f) && near_float(0x794, 3.0f)
        && near_float(0x790, 5.0f) ? '4' : 'x');

    clear_object();
    DAT_0011d260 = 0x10;
    set_float(0x78c, 0x40e00000);
    set_float(0x794, 0x40000000);
    set_float(0x790, 0x40a00000);
    set_record(0, -1);
    set_record(1, 0);
    set_record_time(1, 0x422a0000);
    race_clock_update(object);
    put(near_float(0x78c, 42.5f) && near_float(0x794, 2.0f)
        && near_float(0x790, 4.0f) ? '5' : 'x');

    clear_object();
    DAT_0011d260 = 0x10;
    DAT_0011d250 = 0;
    set_float(0x78c, 0x40e00000);
    set_float(0x794, 0x40000000);
    set_float(0x790, 0x40a00000);
    set_record(0, 0);
    set_record_time(0, 0x422a0000);
    race_clock_update(object);
    put(near_float(0x78c, 7.0f) && near_float(0x794, 2.0f)
        && near_float(0x790, 4.0f) ? '6' : 'x');

    clear_object();
    set_float(0x790, 0x3f000000);
    set_float(0x50c, 0x3f800000);
    race_clock_update(object);
    put(int_is(0x790, 0) && DAT_00118e1d == 0x40
        && (DAT_00118e1c & 0x10) == 0 ? '7' : 'x');

    clear_object();
    set_float(0x790, 0x3f000000);
    set_float(0x50c, 0x3f000000);
    race_clock_update(object);
    put(int_is(0x790, 0) && DAT_00118e1d == 0
        && (DAT_00118e1c & 0x10) != 0 ? '8' : 'x');

    clear_object();
    set_float(0x790, 0x3f800000);
    DAT_00118e1d = 0x40;
    race_clock_update(object);
    put(near_float(0x790, 0.0f) && DAT_00118e1d == 0 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
