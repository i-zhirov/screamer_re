/* The test harness for the rebuilt `map_object_finish` module.
 *
 * The harness holds one 0x64-byte map object record, four object blocks, the
 * 0x1c-byte entry table, the slot table, the selected-line table, and the two
 * float constants.  The two list stubs record their arguments.
 *
 * The harness runs six paths:
 *
 *     0  the full path with the record index not selected: the refresh runs,
 *        the object takes the id and the three positions, the three angles
 *        scale by 1024.0 / pi, and the progress and the timer reset to zero
 *     1  the record index equals the selected line: the record angle takes
 *        pi/2 before the scale, so the first object angle is 512
 *     2  the record holds no object: the bind runs and the record fields stay
 *        unchanged
 *     3  a negative angle: the truncate makes -488 and the mask gives 1560;
 *        round-to-nearest would give 1559
 *     4  a large angle: the scale gives 2281 and the mask gives 233
 *     5  a different record index: the slot table selects the third object and
 *        the id scale gives 0x246
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x64
#define OBJECT_SIZE 0x40
#define SLOT_COUNT 8
#define ENTRY_SIZE 0x1c

void map_object_finish(char *record);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char record[RECORD_SIZE];
static char objects[4][OBJECT_SIZE];
static int refresh_calls;
static int bind_calls;
static void *refresh_table;
static char *refresh_name;
static int *refresh_slot;
static void *bind_table;
static char *bind_name;
static int *bind_slot;

int DAT_001d2d0[SLOT_COUNT * 2];
int DAT_001fab8;
int DAT_001fb60;
int DAT_001fb08[4];
char DAT_0010653c[SLOT_COUNT * ENTRY_SIZE];

float map_object_finish_angle_scale;
float map_object_finish_pi;

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

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_bytes(char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static void set_int(char *base, int offset, int value)
{
    *(int *)(base + offset) = value;
}

static int get_int(char *base, int offset)
{
    return *(int *)(base + offset);
}

static void set_float_bits(char *base, int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(base + offset) = value.f;
}

static unsigned int get_float_bits(char *base, int offset)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = *(float *)(base + offset);
    return value.u;
}

static void set_pointer(char *base, int offset, void *pointer)
{
    *(void **)(base + offset) = pointer;
}

static void link_object(int slot, int object_index)
{
    set_pointer(DAT_0010653c, slot * ENTRY_SIZE, objects[object_index]);
}

static void reset(void)
{
    int index;

    clear_bytes(record, RECORD_SIZE);
    for (index = 0; index < 4; index++) {
        clear_bytes(objects[index], OBJECT_SIZE);
        set_int(objects[index], 0x14, 0x99);
        set_int(objects[index], 0x18, 0x99);
        set_int(objects[index], 0x1c, 0x99);
    }
    clear_bytes((char *)DAT_001d2d0, sizeof(DAT_001d2d0));
    clear_bytes(DAT_0010653c, SLOT_COUNT * ENTRY_SIZE);
    refresh_calls = 0;
    bind_calls = 0;
    refresh_table = 0;
    refresh_name = 0;
    refresh_slot = 0;
    bind_table = 0;
    bind_name = 0;
    bind_slot = 0;
    DAT_001fab8 = 0x100;
    DAT_001fb60 = 0;
    for (index = 0; index < 4; index++) {
        DAT_001fb08[index] = 0x7fffffff;
    }
    set_int(record, 4, 0x12345678);
    set_int(record, 0x58, 0x12345678);
    set_float_bits(record, 0x60, 0x40a00000);
}

int main(void)
{
    int index;
    int ok;

    set_float_bits((char *)&map_object_finish_angle_scale, 0, 0x44800000);
    set_float_bits((char *)&map_object_finish_pi, 0, 0x40490fdb);

    reset();
    index = 2;
    set_int(record, 0, index);
    set_int(record, 8, 5);
    set_int(record, 0x40, 100);
    set_int(record, 0x44, 200);
    set_int(record, 0x48, 300);
    set_float_bits(record, 0x10, 0x3f800000);
    set_float_bits(record, 0x14, 0x40000000);
    set_float_bits(record, 0x18, 0x40400000);
    DAT_001d2d0[index * 2] = 1;
    link_object(1, 1);
    map_object_finish(record);
    ok = refresh_calls == 1
        && bind_calls == 0
        && refresh_table == (void *)0x6530
        && refresh_name == (char *)0x6350
        && refresh_slot == &DAT_001d2d0[index * 2]
        && get_int(objects[1], 0x20) == 0x100 * 5 + 0x226
        && get_int(objects[1], 8) == 100
        && get_int(objects[1], 0x0c) == 200
        && get_int(objects[1], 0x10) == 300
        && get_int(objects[1], 0x14) == 325
        && get_int(objects[1], 0x18) == 651
        && get_int(objects[1], 0x1c) == 977
        && get_int(record, 0x58) == 0
        && get_float_bits(record, 0x60) == 0;
    put(ok ? '0' : 'x');

    reset();
    index = 1;
    set_int(record, 0, index);
    set_int(record, 8, 0);
    set_int(record, 4, 3);
    set_float_bits(record, 0x10, 0);
    set_float_bits(record, 0x14, 0x40000000);
    set_float_bits(record, 0x18, 0x40400000);
    DAT_001d2d0[index * 2] = 2;
    link_object(2, 2);
    DAT_001fb60 = 1;
    DAT_001fb08[1] = 3;
    map_object_finish(record);
    ok = get_int(objects[2], 0x14) == 512
        && get_int(objects[2], 0x18) == 651
        && get_int(objects[2], 0x1c) == 977;
    put(ok ? '1' : 'x');

    reset();
    index = 6;
    set_int(record, 0, index);
    set_int(record, 8, -1);
    set_int(record, 4, 3);
    set_int(record, 0x58, 0x1234);
    set_float_bits(record, 0x60, 0x40a00000);
    DAT_001d2d0[index * 2] = 0;
    link_object(0, 0);
    DAT_001fb60 = 1;
    DAT_001fb08[1] = 3;
    map_object_finish(record);
    ok = bind_calls == 1
        && refresh_calls == 0
        && bind_table == (void *)0x6530
        && bind_name == (char *)0x6350
        && bind_slot == &DAT_001d2d0[index * 2]
        && get_int(record, 0x58) == 0x1234
        && get_float_bits(record, 0x60) == 0x40a00000
        && get_int(objects[0], 0x14) == 0x99;
    put(ok ? '2' : 'x');

    reset();
    index = 0;
    set_int(record, 0, index);
    set_int(record, 8, 1);
    set_float_bits(record, 0x10, 0xbfc00000);
    set_float_bits(record, 0x14, 0);
    set_float_bits(record, 0x18, 0);
    DAT_001d2d0[index * 2] = 0;
    link_object(0, 0);
    map_object_finish(record);
    put(get_int(objects[0], 0x14) == 1560 ? '3' : 'x');

    reset();
    index = 0;
    set_int(record, 0, index);
    set_int(record, 8, 1);
    set_float_bits(record, 0x10, 0x40e00000);
    set_float_bits(record, 0x14, 0);
    set_float_bits(record, 0x18, 0);
    DAT_001d2d0[index * 2] = 0;
    link_object(0, 0);
    map_object_finish(record);
    put(get_int(objects[0], 0x14) == 233 ? '4' : 'x');

    reset();
    index = 4;
    set_int(record, 0, index);
    set_int(record, 8, 2);
    set_float_bits(record, 0x10, 0);
    set_float_bits(record, 0x14, 0);
    set_float_bits(record, 0x18, 0);
    DAT_001d2d0[index * 2] = 3;
    link_object(3, 3);
    DAT_001fab8 = 0x10;
    map_object_finish(record);
    ok = get_int(objects[3], 0x20) == 0x10 * 2 + 0x226
        && refresh_slot == &DAT_001d2d0[index * 2]
        && refresh_calls == 1;
    put(ok ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
