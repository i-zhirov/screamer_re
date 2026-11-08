/* The test harness for the rebuilt `name_entry_letter_update` module.
 *
 * The harness holds one 0x64-byte letter record, one 0x40-byte object block,
 * the slot table DAT_001d2d0, the entry table DAT_0010653c, the frame delta,
 * the frame scale, and the eight float constants.  The two list stubs record
 * their arguments.  The `pow_` stub lives in the module FPU stub.
 *
 * The harness runs twelve paths:
 *
 *     0  the record holds no object: the bind runs with the slot pointer
 *     1  the timer has elapsed: the object takes the frame, the positions, the
 *        angles, and the square of the progress; the progress steps to 0.5
 *     2  the progress starts at 0.5: the height interpolates to 190 and the
 *        progress reaches one
 *     3  the progress passes one and the scaled start falls below the target:
 *        the direction becomes -1 and the start takes the target
 *     4  the progress passes one and the scaled start stays above the target:
 *        the direction stays zero
 *     5  the zero direction and a progress that reaches zero from above: the
 *        progress clamps at zero
 *     6  the zero direction and a progress that passes zero: the progress
 *        clamps at zero and the direction becomes +1
 *     7  the rising angle stays below the upper bound
 *     8  the rising angle passes the upper bound and resets
 *     9  the falling angle stays above the lower bound
 *     a  the falling angle passes the lower bound and resets
 *     b  the timer is still positive: the function returns and the record and
 *        the object stay clear
 *     D  the exit marker
 *
 * The constants are exact halves and powers of two, so every check is exact.
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x64
#define OBJECT_SIZE 0x40
#define SLOT_COUNT 0x50
#define ENTRY_SIZE 0x1c

void name_entry_letter_update(char *record);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char record[RECORD_SIZE];
static char object[OBJECT_SIZE];

float DAT_0018e08;
int DAT_001d2d0[SLOT_COUNT * 2];
int DAT_001fab8;
char DAT_0010653c[SLOT_COUNT * ENTRY_SIZE];

double letter_angle_high;
double letter_angle_low;
double letter_progress_rate;
double letter_start_scale;
float letter_progress_decay;
float letter_angle_rate;
float letter_angle_scale;
float letter_pi;

static int bind_calls;
static int refresh_calls;
static void *bind_table;
static char *bind_name;
static int *bind_slot;
static void *refresh_table;
static char *refresh_name;
static int *refresh_slot;

void object_list_bind(void *table, char *name, int *slot)
{
    bind_calls = bind_calls + 1;
    bind_table = table;
    bind_name = name;
    bind_slot = slot;
}

void object_list_refresh(void *table, char *name, int *slot)
{
    refresh_calls = refresh_calls + 1;
    refresh_table = table;
    refresh_name = name;
    refresh_slot = slot;
}

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int float_bits_are(float value, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = value;
    return v.u == bits;
}

static void set_float(int offset, float value)
{
    *(float *)(record + offset) = value;
}

static void set_int(int offset, int value)
{
    *(int *)(record + offset) = value;
}

static int get_int(int offset)
{
    return *(int *)(record + offset);
}

static void reset(void)
{
    int i;

    for (i = 0; i < RECORD_SIZE; i++)
        record[i] = 0;
    for (i = 0; i < OBJECT_SIZE; i++)
        object[i] = 0;
    for (i = 0; i < SLOT_COUNT * 2; i++)
        DAT_001d2d0[i] = 0;
    for (i = 0; i < SLOT_COUNT * ENTRY_SIZE; i++)
        DAT_0010653c[i] = 0;

    DAT_0018e08 = 0.5f;
    DAT_001fab8 = 10;
    DAT_001d2d0[0] = 0;
    *(char **)(DAT_0010653c + 0) = object;

    letter_angle_high = 2.0;
    letter_angle_low = -2.0;
    letter_progress_rate = 1.0;
    letter_start_scale = 0.5;
    letter_progress_decay = 1.0f;
    letter_angle_rate = 1.0f;
    letter_angle_scale = 1024.0f;
    letter_pi = 1.0f;

    bind_calls = 0;
    refresh_calls = 0;
    bind_table = 0;
    bind_name = 0;
    bind_slot = 0;
    refresh_table = 0;
    refresh_name = 0;
    refresh_slot = 0;
}

static void set_letter(int direction_bits, float progress, float angle)
{
    set_int(8, 1);
    set_float(0x60, 0.0f);
    set_int(0, 0);
    set_int(0x40, 100);
    set_int(0x44, 200);
    set_int(0x38, 160);
    set_int(0x48, 300);
    set_float(0x10, angle);
    set_float(0x14, 0.25f);
    set_float(0x18, 0.0f);
    set_float(0x58, progress);
    set_int(0x5c, direction_bits);
}

int main(void)
{
    reset();
    set_int(8, -1);
    set_int(0, 3);
    name_entry_letter_update(record);
    put(bind_calls == 1
        && refresh_calls == 0
        && bind_table == (void *)0x6530
        && bind_name == (char *)0x6350
        && bind_slot == DAT_001d2d0 + 6 ? '0' : 'x');

    reset();
    set_letter(0x3f800000, 0.0f, 0.5f);
    set_int(0xc, 1);
    name_entry_letter_update(record);
    put(refresh_calls == 1
        && refresh_table == (void *)0x6530
        && refresh_name == (char *)0x6350
        && refresh_slot == DAT_001d2d0
        && float_bits_are(*(float *)(record + 0x60), 0x00000000)
        && *(int *)(object + 0x20) == 0x230
        && *(int *)(object + 8) == 100
        && *(int *)(object + 0xc) == 200
        && *(int *)(object + 0x10) == 300
        && *(int *)(object + 0x14) == 512
        && *(int *)(object + 0x18) == 256
        && *(int *)(object + 0x1c) == 0
        && float_bits_are(*(float *)(record + 0x58), 0x3f000000)
        && float_bits_are(*(float *)(record + 0x10), 0x3f800000)
        ? '1' : 'x');

    reset();
    set_letter(0x3f800000, 0.5f, 0.5f);
    set_int(0xc, 1);
    name_entry_letter_update(record);
    put(*(int *)(object + 0xc) == 190
        && float_bits_are(*(float *)(record + 0x58), 0x3f800000)
        ? '2' : 'x');

    reset();
    set_letter(0x3f800000, 1.0f, -2.0f);
    set_int(0xc, 0);
    name_entry_letter_update(record);
    put(get_int(0x5c) == (int)0xbf800000
        && get_int(0x44) == 160
        && float_bits_are(*(float *)(record + 0x58), 0x3f800000)
        ? '3' : 'x');

    reset();
    set_letter(0x3f800000, 1.0f, -2.0f);
    set_int(0x44, 400);
    set_int(0xc, 0);
    name_entry_letter_update(record);
    put(get_int(0x5c) == 0
        && get_int(0x44) == 200
        && float_bits_are(*(float *)(record + 0x58), 0x3f800000)
        ? '4' : 'x');

    reset();
    set_letter(0x00000000, 0.5f, -2.0f);
    set_int(0xc, 0);
    name_entry_letter_update(record);
    put(get_int(0x5c) == 0
        && float_bits_are(*(float *)(record + 0x58), 0x00000000)
        ? '5' : 'x');

    reset();
    set_letter(0x00000000, 0.25f, -2.0f);
    set_int(0xc, 0);
    name_entry_letter_update(record);
    put(get_int(0x5c) == (int)0x3f800000
        && float_bits_are(*(float *)(record + 0x58), 0x00000000)
        ? '6' : 'x');

    reset();
    set_letter(0xbf800000, 0.0f, 1.0f);
    set_int(0xc, 1);
    name_entry_letter_update(record);
    put(float_bits_are(*(float *)(record + 0x10), 0x3fc00000) ? '7' : 'x');

    reset();
    set_letter(0xbf800000, 0.0f, 1.8f);
    set_int(0xc, 1);
    name_entry_letter_update(record);
    put(float_bits_are(*(float *)(record + 0x10), 0x400cbe4d) ? '8' : 'x');

    reset();
    set_letter(0xbf800000, 0.0f, 0.0f);
    set_int(0xc, 0);
    name_entry_letter_update(record);
    put(float_bits_are(*(float *)(record + 0x10), 0xbf000000) ? '9' : 'x');

    reset();
    set_letter(0xbf800000, 0.0f, -1.8f);
    set_int(0xc, 0);
    name_entry_letter_update(record);
    put(float_bits_are(*(float *)(record + 0x10), 0xbf20d97c) ? 'a' : 'x');

    reset();
    set_letter(0x3f800000, 0.0f, 0.5f);
    set_float(0x60, 1.0f);
    name_entry_letter_update(record);
    put(refresh_calls == 0
        && float_bits_are(*(float *)(record + 0x60), 0x3f800000)
        && *(int *)(object + 0x20) == 0
        ? 'b' : 'x');

    put('D');
    quit();
    return 0;
}
