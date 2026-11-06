/* The test harness for the rebuilt `car_select_screen` module.
 *
 * The harness holds the 14 car display records at `DAT_0011d550`, the object
 * pointer table at `DAT_0010653c`, the display objects, the pixel and video
 * buffers, the palette, and the float constants.  The stubs record the call
 * counts and the arguments.
 *
 * The harness runs five paths:
 *
 *     0  the index 0 path: the init stores every menu constant, builds the 14
 *        records at x = 0, -4000, ..., -52000, and converts every pi/2 angle to
 *        512
 *     1  the index 3 path: the offset cells take -12000 and the display records
 *        stay in the same order
 *     2  the failed load: the cleanup runs once and the rest of the setup still
 *        runs
 *     3  the table order: the record i selects the object 13 - i, so the object
 *        order follows the table and the record at index 14 stays clear
 *     4  the index 7 path: the offset cells take -28000 and the fixed constants
 *        keep their values
 *     D  the exit marker
 *
 * The float checks compare the bit patterns exactly.
 */
#pragma off (check_stack)

void car_select_screen(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct car_record {
    int object_index;
    int field_04;
    int angle_x;
    int angle_y;
    int angle_z;
    char rest[0x30];
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

int DAT_0011d240;
float DAT_00120378;
float DAT_00120380;
int DAT_0011fac8;
void *DAT_0011fad0;
void *DAT_0021528;
int DAT_001203c4;
int DAT_00203b0;
int DAT_00203a4;
float DAT_0020384;
int DAT_00203a8;
int DAT_002039c;
int DAT_00203ac;
int DAT_00203a0;
int DAT_00109a0;
float DAT_0020388;
float DAT_0020394;
int DAT_00109a4;
float DAT_002038c;
int DAT_00109a8;
float DAT_002037c;
float DAT_0018e08;
int DAT_000007f9;
float DAT_0010112c;
float DAT_00101130;

struct car_record DAT_0011d550[15];
struct display_object *DAT_0010653c[16][7];

static struct display_object objects[16];
static char text[8];
static char pixel_buffer[0x3e800];
static char video_buffer[0x3e800];

static int pcx_calls;
static char *pcx_file;
static char *pcx_member;
static void *pcx_pixels;
static char *pcx_palette;
static int pcx_result;
static int cleanup_calls;
static int present_calls;
static unsigned int present_scale[4];
static int present_offset[4];
static int memcpy_calls;
static void *memcpy_target;
static void *memcpy_source;
static unsigned int memcpy_count;
static int fill_calls;
static int init_calls;
static int refresh_calls;
static void *refresh_table[16];
static char *refresh_name[16];
static void *refresh_record[16];
static int heading_calls;

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
    union {
        float f;
        unsigned int u;
    } number;

    number.f = value;
    return number.u;
}

int pcx_load(char *file, char *member, void *pixels, char *palette)
{
    pcx_calls = pcx_calls + 1;
    pcx_file = file;
    pcx_member = member;
    pcx_pixels = pixels;
    pcx_palette = palette;
    return pcx_result;
}

void sub_15150(void) { cleanup_calls = cleanup_calls + 1; }

void video_present(float scale, int offset)
{
    if (present_calls < 4) {
        present_scale[present_calls] = bits(scale);
        present_offset[present_calls] = offset;
    }
    present_calls = present_calls + 1;
}

void *memcpy(void *target, const void *source, unsigned int count)
{
    memcpy_calls = memcpy_calls + 1;
    memcpy_target = target;
    memcpy_source = (void *)source;
    memcpy_count = count;
    return target;
}

void video_fill(void) { fill_calls = fill_calls + 1; }

void sub_2dff4(void) { init_calls = init_calls + 1; }

void object_list_refresh(void *table, char *name, int *record)
{
    if (refresh_calls < 16) {
        refresh_table[refresh_calls] = table;
        refresh_name[refresh_calls] = name;
        refresh_record[refresh_calls] = record;
    }
    refresh_calls = refresh_calls + 1;
}

void race_heading_update(void) { heading_calls = heading_calls + 1; }

static void reset(void)
{
    int i;
    int j;

    for (i = 0; i < 15; ++i) {
        DAT_0011d550[i].object_index = i;
        DAT_0011d550[i].field_04 = 0x11111111;
        DAT_0011d550[i].angle_x = 0x11111111;
        DAT_0011d550[i].angle_y = 0x11111111;
        DAT_0011d550[i].angle_z = 0x11111111;
        for (j = 0; j < 0x30; ++j) {
            DAT_0011d550[i].rest[j] = 0x11;
        }
    }
    for (i = 0; i < 16; ++i) {
        objects[i].field_00 = 0x22222222;
        objects[i].field_04 = 0x22222222;
        objects[i].x = 0x22222222;
        objects[i].y = 0x22222222;
        objects[i].z = 0x22222222;
        objects[i].angle_x = 0x22222222;
        objects[i].angle_y = 0x22222222;
        objects[i].angle_z = 0x22222222;
        for (j = 0; j < 7; ++j) {
            DAT_0010653c[i][j] = 0;
        }
        DAT_0010653c[i][0] = &objects[i];
    }
    DAT_0011d240 = 0x11111111;
    DAT_00120378 = 1.0f;
    DAT_00120380 = 1.0f;
    DAT_0011fac8 = 0x11111111;
    DAT_0011fad0 = pixel_buffer;
    DAT_0021528 = video_buffer;
    DAT_001203c4 = 0;
    DAT_00203b0 = 0x11111111;
    DAT_00203a4 = 0x11111111;
    DAT_0020384 = 1.0f;
    DAT_00203a8 = 0x11111111;
    DAT_002039c = 0x11111111;
    DAT_00203ac = 0x11111111;
    DAT_00203a0 = 0x11111111;
    DAT_00109a0 = 0x11111111;
    DAT_0020388 = 1.0f;
    DAT_0020394 = 1.0f;
    DAT_00109a4 = 0x11111111;
    DAT_002038c = 1.0f;
    DAT_00109a8 = 0x11111111;
    DAT_002037c = 1.0f;
    DAT_0018e08 = 1.0f;
    DAT_000007f9 = 0x11111111;
    DAT_0010112c = 1024.0f;
    DAT_00101130 = 3.1415927410125732f;
    pcx_calls = 0;
    pcx_file = 0;
    pcx_member = 0;
    pcx_pixels = 0;
    pcx_palette = 0;
    pcx_result = 0;
    cleanup_calls = 0;
    present_calls = 0;
    memcpy_calls = 0;
    memcpy_target = 0;
    memcpy_source = 0;
    memcpy_count = 0;
    fill_calls = 0;
    init_calls = 0;
    refresh_calls = 0;
    heading_calls = 0;
}

static int records_ok(int permutation)
{
    int i;
    int j;

    for (i = 0; i < 14; ++i) {
        if (DAT_0011d550[i].angle_x != 0) {
            return 0;
        }
        if (DAT_0011d550[i].angle_y != 0x3fc90fdb) {
            return 0;
        }
        if (DAT_0011d550[i].angle_z != 0) {
            return 0;
        }
        if (DAT_0011d550[i].field_04 != 0x11111111) {
            return 0;
        }
        for (j = 0; j < 0x30; ++j) {
            if (DAT_0011d550[i].rest[j] != 0x11) {
                return 0;
            }
        }
        if (permutation) {
            if (DAT_0011d550[i].object_index != 13 - i) {
                return 0;
            }
        } else if (DAT_0011d550[i].object_index != i) {
            return 0;
        }
    }
    if (DAT_0011d550[14].angle_x != 0x11111111) {
        return 0;
    }
    if (DAT_0011d550[14].angle_y != 0x11111111) {
        return 0;
    }
    if (DAT_0011d550[14].angle_z != 0x11111111) {
        return 0;
    }
    return 1;
}

static int objects_ok(int permutation)
{
    int i;
    int j;

    for (i = 0; i < 14; ++i) {
        if (permutation) {
            j = 13 - i;
        } else {
            j = i;
        }
        if (objects[j].x != -4000 * i) {
            return 0;
        }
        if (objects[j].y != 0) {
            return 0;
        }
        if (objects[j].z != 0xb54) {
            return 0;
        }
        if (objects[j].angle_x != 0) {
            return 0;
        }
        if (objects[j].angle_y != 512) {
            return 0;
        }
        if (objects[j].angle_z != 0) {
            return 0;
        }
        if (objects[j].field_00 != 0x22222222) {
            return 0;
        }
        if (objects[j].field_04 != 0x22222222) {
            return 0;
        }
    }
    if (objects[14].x != 0x22222222) {
        return 0;
    }
    if (objects[14].angle_y != 0x22222222) {
        return 0;
    }
    return 1;
}

static int calls_ok(void)
{
    int i;

    if (pcx_calls != 1 || cleanup_calls != 0) {
        return 0;
    }
    if (pcx_file != (char *)0x111f || pcx_member != (char *)0x1114) {
        return 0;
    }
    if (pcx_pixels != DAT_0011fad0 || pcx_palette != (char *)0xcf5c) {
        return 0;
    }
    if (present_calls != 2) {
        return 0;
    }
    for (i = 0; i < 2; ++i) {
        if (present_scale[i] != 0 || present_offset[i] != 0) {
            return 0;
        }
    }
    if (memcpy_calls != 1) {
        return 0;
    }
    if (memcpy_target != DAT_0021528 || memcpy_source != DAT_0011fad0) {
        return 0;
    }
    if (memcpy_count != 0x3e800) {
        return 0;
    }
    if (fill_calls != 1 || init_calls != 1 || heading_calls != 1) {
        return 0;
    }
    if (refresh_calls != 14) {
        return 0;
    }
    for (i = 0; i < 14; ++i) {
        if (refresh_table[i] != (void *)0x6530) {
            return 0;
        }
        if (refresh_name[i] != (char *)0x6350) {
            return 0;
        }
        if (refresh_record[i] != (void *)&DAT_0011d550[i]) {
            return 0;
        }
    }
    return 1;
}

static int constants_ok(int offset)
{
    if (DAT_0011d240 != 1) {
        return 0;
    }
    if (bits(DAT_00120378) != 0x3fc00000) {
        return 0;
    }
    if (bits(DAT_00120380) != 0x3f733333) {
        return 0;
    }
    if (DAT_0011fac8 != 1) {
        return 0;
    }
    if (DAT_00203b0 != 0x1004 || DAT_00203a4 != 0x1004) {
        return 0;
    }
    if (bits(DAT_0020384) != 0xbec90fdb) {
        return 0;
    }
    if (DAT_00203a8 != offset || DAT_002039c != offset) {
        return 0;
    }
    if (DAT_00203ac != 0x1f4 || DAT_00203a0 != 0x1f4) {
        return 0;
    }
    if (DAT_00109a0 != 0x781) {
        return 0;
    }
    if (bits(DAT_0020388) != 0 || bits(DAT_0020394) != 0) {
        return 0;
    }
    if (DAT_00109a4 != 0 || DAT_00109a8 != 0) {
        return 0;
    }
    if (bits(DAT_002038c) != 0 || bits(DAT_002037c) != 0) {
        return 0;
    }
    if (bits(DAT_0018e08) != 0x3da3d70a) {
        return 0;
    }
    if (DAT_000007f9 != 8) {
        return 0;
    }
    return 1;
}

int main(void)
{
    reset();
    car_select_screen();
    put(constants_ok(0)
        && records_ok(0)
        && objects_ok(0)
        && calls_ok()
        ? '0' : 'x');

    reset();
    DAT_001203c4 = 3;
    car_select_screen();
    put(constants_ok(-12000)
        && records_ok(0)
        && objects_ok(0)
        && calls_ok()
        ? '1' : 'x');

    reset();
    pcx_result = 1;
    car_select_screen();
    put(cleanup_calls == 1
        && records_ok(0)
        && objects_ok(0)
        && pcx_calls == 1
        && refresh_calls == 14
        && heading_calls == 1
        ? '2' : 'x');

    reset();
    {
        int i;

        for (i = 0; i < 14; ++i) {
            DAT_0011d550[i].object_index = 13 - i;
        }
    }
    car_select_screen();
    put(constants_ok(0)
        && records_ok(1)
        && objects_ok(1)
        && calls_ok()
        ? '3' : 'x');

    reset();
    DAT_001203c4 = 7;
    car_select_screen();
    put(constants_ok(-28000)
        && records_ok(0)
        && objects_ok(0)
        && calls_ok()
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
