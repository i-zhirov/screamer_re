/* The test harness for the rebuilt `map_object_update` module.
 *
 * The harness holds one 0x64-byte map object record, one object block, the
 * 0x1c-byte entry table, the slot table, the frame delta, the wave phase, and
 * the float constants.  The two list stubs record their arguments.
 *
 * The harness runs nine paths:
 *
 *     0  the record holds no object: the bind runs and the update returns
 *     1  the full path with progress 0: the timer falls, the object takes the
 *        target position, the wave lowers the height, and the angles scale
 *     2  the timer and the progress clamp at zero, the record flag is zero, and
 *        the falling angle clamps at zero
 *     3  the record angle is already pi: the angle block returns
 *     4  the overlay word is set and the angle is above pi/2: the mode becomes
 *        -2 and the angle clamps at pi
 *     5  the progress falls without a clamp and the object interpolates to the
 *        first position
 *     6  the record flag is zero but the overlay word is set: the rising angle
 *        takes the mode path
 *     7  the record angle is zero: the falling angle block returns
 *     D  the exit marker
 *
 * A position check tolerates one unit and a height check tolerates three units,
 * because the x87 ring and the C ring differ in the last bit.  The float
 * constants arrive from their bit patterns, because the minimal image does not
 * load the constant segment.
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x64
#define OBJECT_SIZE 0x40
#define SLOT_COUNT 0x50
#define ENTRY_SIZE 0x1c

void map_object_update(char *record);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char record[RECORD_SIZE];
static char object[OBJECT_SIZE];
static int refresh_calls;
static int bind_calls;
static void *refresh_table;
static char *refresh_name;
static int *refresh_slot;
static void *bind_table;
static char *bind_name;
static int *bind_slot;

float DAT_0018e08;
float DAT_001d248;
int DAT_001d2d0[SLOT_COUNT * 2];
int DAT_001fab8;
int DAT_001fac0;
int DAT_001fb64;
char DAT_0010653c[SLOT_COUNT * ENTRY_SIZE];

float map_object_pi_half;
float map_object_two_pi;
float map_object_pi;
float map_object_quarter;
float map_object_angle_scale;
float map_object_wave_rate;
float map_object_bias_x;
float map_object_bias_y;
double map_object_scale_x;
double map_object_scale_y;
double map_object_wave_height;
double map_object_min_angle;

static float expect_timer;
static float expect_angle;
static float expect_angle_six;
static float expect_progress;
static float expect_zero;
static float expect_tolerance;

void object_list_refresh(void *table, char *name, int *slot)
{
    refresh_calls = refresh_calls + 1;
    refresh_table = table;
    refresh_name = name;
    refresh_slot = slot;
}

void object_list_bind(void *table, char *name, int *slot)
{
    bind_calls = bind_calls + 1;
    bind_table = table;
    bind_name = name;
    bind_slot = slot;
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

static void set_bits_double(double *p, unsigned int low, unsigned int high)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = low;
    value.u[1] = high;
    *p = value.d;
}

static void set_record_int(int offset, int value)
{
    *(int *)(record + offset) = value;
}

static int record_int(int offset)
{
    return *(int *)(record + offset);
}

static unsigned int record_bits(int offset)
{
    return *(unsigned int *)(record + offset);
}

static int object_int(int offset)
{
    return *(int *)(object + offset);
}

static void set_slot(int index, int slot)
{
    DAT_001d2d0[index * 2] = slot;
    *(char **)(DAT_0010653c + slot * ENTRY_SIZE) = object;
}

static void reset_case(void)
{
    clear_bytes(record, RECORD_SIZE);
    clear_bytes(object, OBJECT_SIZE);
    clear_bytes((char *)DAT_001d2d0, sizeof(DAT_001d2d0));
    clear_bytes(DAT_0010653c, SLOT_COUNT * ENTRY_SIZE);

    set_bits_float(&DAT_0018e08, 0x3dcccccd);
    set_bits_float(&DAT_001d248, 0);
    DAT_001fab8 = 0x20;
    DAT_001fac0 = 0;
    DAT_001fb64 = 0;

    set_bits_float(&map_object_pi_half, 0x3fc90fdb);
    set_bits_float(&map_object_two_pi, 0x40c90fdb);
    set_bits_float(&map_object_pi, 0x40490fdb);
    set_bits_float(&map_object_quarter, 0x3e800000);
    set_bits_float(&map_object_angle_scale, 0x44800000);
    set_bits_float(&map_object_wave_rate, 0x41490fdb);
    set_bits_float(&map_object_bias_x, 0x45900000);
    set_bits_float(&map_object_bias_y, 0x45020000);
    set_bits_double(&map_object_scale_x, 0xc71c71c7, 0x3f1c71c7);
    set_bits_double(&map_object_scale_y, 0x1f81f820, 0x3f2f81f8);
    set_bits_double(&map_object_wave_height, 0x00000000, 0x4062c000);
    set_bits_double(&map_object_min_angle, 0x9999999a, 0x3fa99999);

    set_bits_float(&expect_timer, 0x411e6666);
    set_bits_float(&expect_angle, 0x3fa0d97c);
    set_bits_float(&expect_angle_six, 0x3fa88791);
    set_bits_float(&expect_progress, 0x3f733333);
    set_bits_float(&expect_zero, 0);
    set_bits_float(&expect_tolerance, 0x3a83126f);

    refresh_calls = 0;
    bind_calls = 0;
    refresh_table = 0;
    refresh_name = 0;
    refresh_slot = 0;
    bind_table = 0;
    bind_name = 0;
    bind_slot = 0;
}

static int near_int(int value, int expected)
{
    int difference;

    difference = value - expected;
    if (difference < 0) {
        difference = -difference;
    }
    return difference <= 1;
}

static int near_float(float value, float expected)
{
    float difference;

    difference = value - expected;
    if (difference < expect_zero) {
        difference = -difference;
    }
    return difference < expect_tolerance;
}

static int near_height(int value, int expected)
{
    int difference;

    difference = value - expected;
    if (difference < 0) {
        difference = -difference;
    }
    return difference <= 3;
}

static void base_case(int flag)
{
    reset_case();
    set_record_int(0x00, 0);
    set_record_int(0x08, 5);
    set_record_int(0x0c, flag);
    set_record_int(0x34, 0);
    set_record_int(0x38, 0);
    set_record_int(0x3c, 0);
    set_record_int(0x40, 0);
    set_record_int(0x44, 0);
    set_record_int(0x48, 0);
    set_record_int(0x4c, 0);
    set_record_int(0x50, 0);
    set_bits_float((float *)(record + 0x58), 0);
    set_bits_float((float *)(record + 0x5c), 0x3f800000);
    set_bits_float((float *)(record + 0x60), 0x41200000);
    set_slot(0, 7);
}

int main(void)
{
    reset_case();
    set_record_int(0x00, 3);
    set_record_int(0x08, -1);
    DAT_001d2d0[3 * 2] = 7;
    map_object_update(record);
    put(bind_calls == 1 && refresh_calls == 0
        && bind_table == (void *)0x6530
        && bind_name == (char *)0x6350
        && bind_slot == (int *)((char *)DAT_001d2d0 + 3 * 8) ? '0' : 'x');

    base_case(1);
    set_record_int(0x10, 0);
    set_record_int(0x14, 0x3fc90fdb);
    set_record_int(0x18, 0x40490fdb);
    set_record_int(0x48, 1000);
    map_object_update(record);
    put(bind_calls == 0 && refresh_calls == 1
        && refresh_table == (void *)0x6530
        && refresh_name == (char *)0x6350
        && refresh_slot == (int *)((char *)DAT_001d2d0 + 0)
        && object_int(0x20) == 0x20 * 5 + 0x226
        && near_int(object_int(0x08), 0)
        && near_int(object_int(0x0c), 0)
        && near_height(object_int(0x10), 850)
        && near_int(object_int(0x14) & 0x7ff, 0)
        && near_int(object_int(0x18) & 0x7ff, 512)
        && near_int(object_int(0x1c) & 0x7ff, 1024)
        && near_float(*(float *)(record + 0x60), expect_timer)
        && (record_bits(0x58) & 0x7fffffff) == 0
        && near_float(*(float *)(record + 0x10), expect_angle) ? '1' : 'x');

    base_case(0);
    set_record_int(0x10, 0x3d75c28f);
    set_record_int(0x48, 500);
    set_bits_float((float *)(record + 0x58), 0x3dcccccd);
    set_bits_float((float *)(record + 0x5c), 0x3f800000);
    set_bits_float((float *)(record + 0x60), 0x3d4ccccd);
    map_object_update(record);
    put(refresh_calls == 1
        && (record_bits(0x60) & 0x7fffffff) == 0
        && (record_bits(0x58) & 0x7fffffff) == 0
        && (record_bits(0x10) & 0x7fffffff) == 0
        && near_height(object_int(0x10), 343) ? '2' : 'x');

    base_case(1);
    set_record_int(0x10, 0x40490fdb);
    map_object_update(record);
    put(refresh_calls == 1
        && record_int(0x10) == 0x40490fdb
        && near_float(*(float *)(record + 0x60), expect_timer)
        && near_height(object_int(0x10), -150) ? '3' : 'x');

    base_case(1);
    set_record_int(0x10, 0x40000000);
    DAT_001fac0 = 1;
    map_object_update(record);
    put(refresh_calls == 1
        && DAT_001fb64 == -2
        && record_int(0x10) == 0x40490fdb ? '4' : 'x');

    base_case(1);
    set_record_int(0x10, 0);
    set_record_int(0x34, 100);
    set_record_int(0x40, 0);
    set_bits_float((float *)(record + 0x58), 0x3f800000);
    set_bits_float((float *)(record + 0x5c), 0x3f000000);
    map_object_update(record);
    put(refresh_calls == 1
        && near_int(object_int(0x08), 100)
        && near_int(object_int(0x0c), 0)
        && near_height(object_int(0x10), -149)
        && near_float(*(float *)(record + 0x58), expect_progress)
        && near_float(*(float *)(record + 0x10), expect_angle) ? '5' : 'x');

    base_case(0);
    set_record_int(0x10, 0x3d75c28f);
    DAT_001fac0 = 1;
    map_object_update(record);
    put(refresh_calls == 1
        && DAT_001fb64 == 0
        && near_float(*(float *)(record + 0x10), expect_angle_six) ? '6' : 'x');

    base_case(0);
    set_record_int(0x10, 0);
    map_object_update(record);
    put(refresh_calls == 1
        && (record_bits(0x10) & 0x7fffffff) == 0
        && near_float(*(float *)(record + 0x60), expect_timer) ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
