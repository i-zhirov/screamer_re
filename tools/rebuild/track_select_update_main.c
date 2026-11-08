/* The test harness for the rebuilt `track_select_update` module.
 *
 * The harness holds the 13 map records at `DAT_0011d908`, the 13 display
 * objects, the object pointer table `DAT_0010653c`, the frame step
 * `DAT_0018e08`, the four object-3 float constants, the mode cell
 * `DAT_0011d260`, the two selection cells `DAT_001203c8` and
 * `DAT_00120b5c`, and the network stub.
 *
 * The harness runs these paths:
 *
 *     0  the normal path: 12 records rotate and write their objects, and the
 *        record 12 sentinel stays unchanged
 *     1  the network mode 0x10: the calls carry the event 3 and 0x8844, and
 *        the selection cell takes the lobby value only during the second call
 *     2  the frame step 1.0 adds pi/4 to the heading
 *     3  a heading at 2*pi resets to 0
 *     4  a heading below 2*pi stays unchanged
 *     5  a positive angle 1.0 truncates to 325
 *     6  a negative angle -1.0 truncates to -325 and masks to 0x6bb
 *     7  the record object index selects the object pointer
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because an FPU memory
 * operand of a literal reads the constant pool at a low address in this
 * toolchain.
 */
#pragma off (check_stack)

#define RECORD_COUNT 13
#define ACTIVE_COUNT 12

struct track_record {
    int object_index;
    int field_04;
    float angle_x;
    float angle_y;
    float angle_z;
    char rest[0x20];
};

struct display_object {
    int field_00;
    int field_04;
    int x;
    int y;
    int z;
    int angle_x;
    int angle_y;
    int angle_z;
};

void track_select_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct track_record DAT_0011d908[RECORD_COUNT];
struct display_object display_objects[RECORD_COUNT];
struct display_object *DAT_0010653c[RECORD_COUNT][7];

int DAT_0011d260;
int DAT_001203c8;
int DAT_00120b5c;
float DAT_0018e08;
float DAT_001011e3;
float DAT_001011e7;
float DAT_001011eb;
float DAT_001011ef;

static char text[8];
static int net_calls;
static int net_ids[4];
static int net_selection[4];

int net_player_add(int id)
{
    if (net_calls < 4) {
        net_ids[net_calls] = id;
        net_selection[net_calls] = DAT_001203c8;
    }
    net_calls = net_calls + 1;
    return 0;
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

static unsigned int bits(float value)
{
    return *(unsigned int *)&value;
}

static void set_bits_float(float *value, unsigned int pattern)
{
    *(unsigned int *)value = pattern;
}

static void clear_bytes(void *target, unsigned int count)
{
    unsigned char *bytes;
    unsigned int index;

    bytes = (unsigned char *)target;
    for (index = 0; index < count; index++) {
        bytes[index] = 0;
    }
}

static void set_record(int index, int object_index, float x, float y, float z)
{
    DAT_0011d908[index].object_index = object_index;
    DAT_0011d908[index].angle_x = x;
    DAT_0011d908[index].angle_y = y;
    DAT_0011d908[index].angle_z = z;
}

static void set_object(int index, int angle)
{
    display_objects[index].angle_x = angle;
    display_objects[index].angle_y = angle;
    display_objects[index].angle_z = angle;
}

static int object_is(int index, int angle)
{
    return display_objects[index].angle_x == angle
        && display_objects[index].angle_y == angle
        && display_objects[index].angle_z == angle;
}

static void reset(void)
{
    int index;
    int slot;

    for (index = 0; index < RECORD_COUNT; index++) {
        clear_bytes(&DAT_0011d908[index], sizeof(DAT_0011d908[index]));
        clear_bytes(&display_objects[index], sizeof(display_objects[index]));
        DAT_0010653c[index][0] = &display_objects[index];
        for (slot = 1; slot < 7; slot++) {
            DAT_0010653c[index][slot] = 0;
        }
    }
    DAT_0011d260 = 0;
    DAT_001203c8 = 0;
    DAT_00120b5c = 0;
    DAT_0018e08 = 0.0f;
    set_bits_float(&DAT_001011e3, 0x3f490fdb);
    set_bits_float(&DAT_001011e7, 0x40c90fdb);
    set_bits_float(&DAT_001011eb, 0x44800000);
    set_bits_float(&DAT_001011ef, 0x40490fdb);
    net_calls = 0;
    net_ids[0] = 0;
    net_ids[1] = 0;
    net_ids[2] = 0;
    net_ids[3] = 0;
    net_selection[0] = 0;
    net_selection[1] = 0;
    net_selection[2] = 0;
    net_selection[3] = 0;
}

static void prepare_records(void)
{
    int index;

    for (index = 0; index < ACTIVE_COUNT; index++) {
        set_record(index, index, 0.0f, 0.0f, 0.0f);
        set_object(index, 0x123);
    }
    set_record(ACTIVE_COUNT, ACTIVE_COUNT, 1.0f, 2.0f, 3.0f);
    set_object(ACTIVE_COUNT, 0x456);
}

int main(void)
{
    int index;
    int ok;

    reset();
    prepare_records();
    track_select_update();
    ok = net_calls == 0;
    for (index = 0; index < ACTIVE_COUNT; index++) {
        ok = ok && object_is(index, 0);
    }
    ok = ok && object_is(ACTIVE_COUNT, 0x456)
        && bits(DAT_0011d908[ACTIVE_COUNT].angle_y) == 0x40000000;
    put(ok ? '0' : 'x');

    reset();
    prepare_records();
    DAT_0011d260 = 0x10;
    DAT_001203c8 = 5;
    DAT_00120b5c = 9;
    track_select_update();
    ok = net_calls == 2 && net_ids[0] == 3 && net_ids[1] == 0x8844
        && net_selection[0] == 5 && net_selection[1] == 9
        && DAT_001203c8 == 5;
    put(ok ? '1' : 'x');

    reset();
    prepare_records();
    DAT_0018e08 = 1.0f;
    track_select_update();
    put(bits(DAT_0011d908[0].angle_y) == 0x3f490fdb ? '2' : 'x');

    reset();
    prepare_records();
    set_record(0, 0, 0.0f, DAT_001011e7, 0.0f);
    track_select_update();
    put(bits(DAT_0011d908[0].angle_y) == 0 ? '3' : 'x');

    reset();
    prepare_records();
    set_record(0, 0, 0.0f, 6.0f, 0.0f);
    track_select_update();
    put(bits(DAT_0011d908[0].angle_y) == 0x40c00000 ? '4' : 'x');

    reset();
    prepare_records();
    set_record(0, 0, 1.0f, 1.0f, 1.0f);
    track_select_update();
    put(object_is(0, 325) ? '5' : 'x');

    reset();
    prepare_records();
    set_record(0, 0, -1.0f, -1.0f, -1.0f);
    track_select_update();
    put(object_is(0, 0x6bb) ? '6' : 'x');

    reset();
    prepare_records();
    DAT_0011d908[0].object_index = 3;
    DAT_0011d908[3].object_index = 4;
    DAT_0011d908[11].object_index = 7;
    set_object(0, 0x111);
    set_object(3, 0x222);
    set_object(4, 0x333);
    set_object(7, 0x444);
    track_select_update();
    ok = object_is(0, 0x111) && object_is(3, 0) && object_is(4, 0)
        && object_is(7, 0);
    put(ok ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
