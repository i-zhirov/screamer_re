/* The test harness for the rebuilt `track_timer_update` module.
 *
 * The harness holds the object timer at `+0x790`, the lap index at `+0x684`,
 * the race position, the mode byte, the divisor, and the two steps.  The
 * `effect_timer_clear` stub records the index and the call count.  Every
 * expected timer is an exact binary value, because each timer is a multiple of
 * the divisor and each step is an integer.  The harness compares the float bit
 * patterns through a union, because this toolchain reads an FPU literal memory
 * operand from a low address.
 *
 * The harness runs these paths:
 *
 *     0  the race position plus one equals the lap index: the timer stays, and
 *        the effect timer clear does not run
 *     1  the clear mode bits select the default step 3: the timer 10.0 becomes
 *        5.0, and the clear runs with the index 0x16
 *     2  the mode 0x60 selects the mode step 7: the timer 10.0 becomes 9.0
 *     3  the mode bit 0x20 selects the mode step 7: the timer 15.0 becomes 10.0
 *     4  the mode bit 0x40 selects the mode step 7: the timer 5.0 becomes 8.0
 *     5  the mode bit 0x10 alone keeps the default step 3: the timer 20.0
 *        becomes 7.0
 *     6  the negative default step -2: the timer 10.0 becomes 0.0
 *     7  the mode 0x50 selects the mode step 7: the timer 5.0 becomes 8.0
 *     8  two normal calls then one early return: the clear count reaches 2, and
 *        the early return keeps the timer and the count
 *     D  the exit marker
 */
#pragma off (check_stack)

void track_timer_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int race_position;
unsigned char DAT_0011d260;
float track_timer_divisor;
int track_timer_step_mode;
int track_timer_step_default;

static unsigned char object[0x800];
static char text[8];
static int clear_count;
static int clear_index;

void effect_timer_clear(int index)
{
    clear_index = index;
    clear_count = clear_count + 1;
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

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void reset(void)
{
    int index;

    for (index = 0; index < (int)sizeof(object); index++) {
        object[index] = 0;
    }
    clear_count = 0;
    clear_index = 0;
}

static void set_int(int offset, int value)
{
    *(int *)((char *)object + offset) = value;
}

static int get_int(int offset)
{
    return *(int *)((char *)object + offset);
}

static void set_float_bits(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *(float *)((char *)object + offset) = v.f;
}

static int is_float_bits(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = *(float *)((char *)object + offset);
    return v.u == bits;
}

static void set_global_bits(float *value, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *value = v.f;
}

int main(void)
{
    set_global_bits(&track_timer_divisor, 0x40A00000);
    track_timer_step_mode = 7;
    track_timer_step_default = 3;

    reset();
    race_position = 4;
    DAT_0011d260 = 0;
    set_int(0x684, 5);
    set_float_bits(0x790, 0x41200000);
    track_timer_update(object);
    check('0',
          is_float_bits(0x790, 0x41200000)
          && clear_count == 0);

    reset();
    race_position = 0;
    DAT_0011d260 = 0;
    set_int(0x684, 2);
    set_float_bits(0x790, 0x41200000);
    track_timer_update(object);
    check('1',
          is_float_bits(0x790, 0x40A00000)
          && clear_count == 1
          && clear_index == 0x16);

    reset();
    race_position = 0;
    DAT_0011d260 = 0x60;
    set_int(0x684, 2);
    set_float_bits(0x790, 0x41200000);
    track_timer_update(object);
    check('2',
          is_float_bits(0x790, 0x41100000)
          && clear_count == 1
          && clear_index == 0x16);

    reset();
    race_position = 0;
    DAT_0011d260 = 0x20;
    set_int(0x684, 2);
    set_float_bits(0x790, 0x41700000);
    track_timer_update(object);
    check('3',
          is_float_bits(0x790, 0x41200000)
          && clear_count == 1);

    reset();
    race_position = 0;
    DAT_0011d260 = 0x40;
    set_int(0x684, 2);
    set_float_bits(0x790, 0x40A00000);
    track_timer_update(object);
    check('4',
          is_float_bits(0x790, 0x41000000)
          && clear_count == 1);

    reset();
    race_position = 0;
    DAT_0011d260 = 0x10;
    set_int(0x684, 2);
    set_float_bits(0x790, 0x41A00000);
    track_timer_update(object);
    check('5',
          is_float_bits(0x790, 0x40E00000)
          && clear_count == 1);

    reset();
    race_position = 0;
    DAT_0011d260 = 0;
    track_timer_step_default = -2;
    set_int(0x684, 2);
    set_float_bits(0x790, 0x41200000);
    track_timer_update(object);
    check('6',
          is_float_bits(0x790, 0x00000000)
          && clear_count == 1);
    track_timer_step_default = 3;

    reset();
    race_position = 0;
    DAT_0011d260 = 0x50;
    set_int(0x684, 2);
    set_float_bits(0x790, 0x40A00000);
    track_timer_update(object);
    check('7',
          is_float_bits(0x790, 0x41000000)
          && clear_count == 1);

    reset();
    race_position = 0;
    DAT_0011d260 = 0;
    set_int(0x684, 2);
    set_float_bits(0x790, 0x41200000);
    track_timer_update(object);
    track_timer_update(object);
    race_position = 1;
    track_timer_update(object);
    check('8',
          is_float_bits(0x790, 0x40800000)
          && clear_count == 2
          && clear_index == 0x16);

    put('D');
    quit();
    return 0;
}
