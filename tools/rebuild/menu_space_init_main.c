/* The test harness for the rebuilt `menu_space_init` module.
 *
 * The harness holds every global of the module and a stub for every call
 * target.  The `sprintf` stub records the buffer, the format, and the one
 * variadic argument of each call.  The other stubs record their arguments and
 * count their calls.
 *
 * The harness runs four paths:
 *
 *     0  the normal path: both resources load, the sprite and the font load,
 *        the rectangle and the menu constants run, the four lists load, the two
 *        list files load, the sound bank and the objects init run, the view
 *        moves, and the music path stays off
 *     1  the sprite and the font loads fail: the cleanup runs twice and the
 *        path continues through the same calls
 *     2  the flag `DAT_0001d2cc` is set: the input apply and the two music
 *        calls run with the track from `DAT_00001f04`
 *     3  both resource requests return zero: the memory error report runs
 *        twice and the path continues
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void menu_space_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007edc[0x20];
void *DAT_000062ac;
int DAT_0002082c;
int DAT_0002087c;
int DAT_00015cf0;
int DAT_00015ce4;
int DAT_00015ce8;
int DAT_00015cec;
void *DAT_00021528;
void *DAT_0001fad0;
void *DAT_0001fb70;
void *DAT_0001fb74;
void *DAT_0001fb78;
void *DAT_0001fb7c;
int DAT_0001fb80[8];
int DAT_0001fd10[8];
int DAT_0001fea0[8];
int DAT_00020030[8];
int DAT_0001fb64;
int DAT_0001fb60;
int DAT_00018d84;
int DAT_0002e04[8];
int DAT_0015224[8];
int DAT_0001d2c0;
int DAT_0001d2bc;
int DAT_0001d2c4;
int DAT_0001d2cc;
int DAT_00001f04;

static char text[8];

static char *sprintf_buffer[8];
static char *sprintf_format[8];
static unsigned int sprintf_arg[8];
static int sprintf_calls;

static int resource_calls;
static void *resource_resource[4];
static int resource_size[4];
static void *resource_result[4];
static int report_calls;
static void *report_resource;
static int choice_calls;
static int state_cells_calls;
static int state_cells_value;
static int font_table_calls;
static int sound_slot_calls;
static int sprite_calls;
static char *sprite_file;
static char *sprite_member;
static int sprite_result;
static int cleanup_calls;
static int font_calls;
static char *font_file;
static char *font_member;
static int font_result;
static int graphics_calls;
static char *graphics_a;
static char *graphics_b;
static int material_calls;
static char *material_name;
static int screen_calls;
static unsigned int screen_width;
static unsigned int screen_height;
static unsigned int screen_value;
static int memset_calls;
static void *memset_target;
static unsigned int memset_count;
static int memset_value;
static int video_fill_calls;
static int member_list_calls;
static char *member_list_name[8];
static void *member_list_array[8];
static void *member_list_result[8];
static int list_calls;
static void *list_table[4];
static int list_stride[4];
static int list_count[4];
static int list_result[4];
static int sound_bank_calls;
static int objects_calls;
static int view_calls;
static int view_a;
static int view_b;
static int view_c;
static int input_calls;
static unsigned int input_state;
static void *input_device;
static int music_load_calls;
static char *music_load_record;
static char *music_load_name;
static int music_select_calls;
static char *music_select_record;
static int music_select_value;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    sprintf_arg[sprintf_calls] = va_arg(arguments, unsigned int);
    va_end(arguments);
    sprintf_buffer[sprintf_calls] = buffer;
    sprintf_format[sprintf_calls] = (char *)format;
    sprintf_calls = sprintf_calls + 1;
    return 0;
}

void *resource_get(void *resource, char *name, int size)
{
    resource_resource[resource_calls] = resource;
    resource_size[resource_calls] = size;
    resource_calls = resource_calls + 1;
    return resource_result[resource_calls - 1];
}

void memory_error_report(void *resource)
{
    report_resource = resource;
    report_calls = report_calls + 1;
}

void choice_load(void) { choice_calls = choice_calls + 1; }

void state_cells_set(int value)
{
    state_cells_value = value;
    state_cells_calls = state_cells_calls + 1;
}

void font_table_init(void) { font_table_calls = font_table_calls + 1; }
void sound_slot_init(void) { sound_slot_calls = sound_slot_calls + 1; }

int sprite_load(char *file, char *member)
{
    sprite_file = file;
    sprite_member = member;
    sprite_calls = sprite_calls + 1;
    return sprite_result;
}

void cleanup(void) { cleanup_calls = cleanup_calls + 1; }

int font_load(char *file, char *member)
{
    font_file = file;
    font_member = member;
    font_calls = font_calls + 1;
    return font_result;
}

void graphics_load(char *name_a, char *name_b)
{
    graphics_a = name_a;
    graphics_b = name_b;
    graphics_calls = graphics_calls + 1;
}

void material_load(char *name)
{
    material_name = name;
    material_calls = material_calls + 1;
}

void screen_mode_set(unsigned int width, unsigned int height,
                     unsigned int value)
{
    screen_width = width;
    screen_height = height;
    screen_value = value;
    screen_calls = screen_calls + 1;
}

void *memset(void *target, unsigned int count, int value)
{
    memset_target = target;
    memset_count = count;
    memset_value = value;
    memset_calls = memset_calls + 1;
    return target;
}

void video_fill(void) { video_fill_calls = video_fill_calls + 1; }

void *member_list_load(char *name, void *array)
{
    member_list_name[member_list_calls] = name;
    member_list_array[member_list_calls] = array;
    member_list_calls = member_list_calls + 1;
    return member_list_result[member_list_calls - 1];
}

int list_file_load(char *type, char *name, char *list, void *table,
                   int stride, int count)
{
    list_table[list_calls] = table;
    list_stride[list_calls] = stride;
    list_count[list_calls] = count;
    list_calls = list_calls + 1;
    return list_result[list_calls - 1];
}

void sound_bank_load(void) { sound_bank_calls = sound_bank_calls + 1; }
void menu_objects_init(void) { objects_calls = objects_calls + 1; }

void view_move(int a, int b, int c)
{
    view_a = a;
    view_b = b;
    view_c = c;
    view_calls = view_calls + 1;
}

void input_apply(unsigned int state, void *device)
{
    input_state = state;
    input_device = device;
    input_calls = input_calls + 1;
}

void music_load(char *record, char *name)
{
    music_load_record = record;
    music_load_name = name;
    music_load_calls = music_load_calls + 1;
}

void music_select(char *record, int value)
{
    music_select_record = record;
    music_select_value = value;
    music_select_calls = music_select_calls + 1;
}

static void reset(void)
{
    int index;

    DAT_00007edc[0] = 0;
    DAT_000062ac = (void *)0x1000;
    DAT_0002082c = 0x11111111;
    DAT_0002087c = 0x11111111;
    DAT_00015cf0 = 0x11111111;
    DAT_00015ce4 = 0x11111111;
    DAT_00015ce8 = 0x11111111;
    DAT_00015cec = 0x11111111;
    DAT_00021528 = (void *)0x2000;
    DAT_0001fad0 = (void *)0x11111111;
    DAT_0001fb70 = (void *)0x11111111;
    DAT_0001fb74 = (void *)0x11111111;
    DAT_0001fb78 = (void *)0x11111111;
    DAT_0001fb7c = (void *)0x11111111;
    DAT_0001fb64 = 0x11111111;
    DAT_0001fb60 = 0x11111111;
    DAT_00018d84 = 0x11111111;
    DAT_0001d2c0 = 0x11;
    DAT_0001d2bc = 0x22;
    DAT_0001d2c4 = 0x33;
    DAT_0001d2cc = 0;
    DAT_00001f04 = 7;
    for (index = 0; index < 8; ++index) {
        DAT_0001fb80[index] = 0;
        DAT_0001fd10[index] = 0;
        DAT_0001fea0[index] = 0;
        DAT_00020030[index] = 0;
    }
    sprintf_calls = 0;
    for (index = 0; index < 8; ++index) {
        sprintf_buffer[index] = 0;
        sprintf_format[index] = 0;
        sprintf_arg[index] = 0x11111111;
        member_list_name[index] = 0;
        member_list_array[index] = 0;
        member_list_result[index] = 0;
    }
    resource_calls = 0;
    for (index = 0; index < 4; ++index) {
        resource_resource[index] = 0;
        resource_size[index] = 0;
        resource_result[index] = 0;
        list_table[index] = 0;
        list_stride[index] = 0;
        list_count[index] = 0;
        list_result[index] = 0;
    }
    resource_result[0] = (void *)0x4000;
    resource_result[1] = (void *)0x5000;
    report_calls = 0;
    report_resource = 0;
    choice_calls = 0;
    state_cells_calls = 0;
    state_cells_value = 0;
    font_table_calls = 0;
    sound_slot_calls = 0;
    sprite_calls = 0;
    sprite_file = 0;
    sprite_member = 0;
    sprite_result = 0;
    cleanup_calls = 0;
    font_calls = 0;
    font_file = 0;
    font_member = 0;
    font_result = 0;
    graphics_calls = 0;
    graphics_a = 0;
    graphics_b = 0;
    material_calls = 0;
    material_name = 0;
    screen_calls = 0;
    screen_width = 0;
    screen_height = 0;
    screen_value = 0;
    memset_calls = 0;
    memset_target = 0;
    memset_count = 0;
    memset_value = 0;
    video_fill_calls = 0;
    member_list_calls = 0;
    member_list_result[0] = (void *)0x6000;
    member_list_result[1] = (void *)0x7000;
    member_list_result[2] = (void *)0x8000;
    member_list_result[3] = (void *)0x9000;
    list_calls = 0;
    list_result[0] = 0x11;
    list_result[1] = 0x12;
    sound_bank_calls = 0;
    objects_calls = 0;
    view_calls = 0;
    view_a = 0;
    view_b = 0;
    view_c = 0;
    input_calls = 0;
    input_state = 0;
    input_device = 0;
    music_load_calls = 0;
    music_load_record = 0;
    music_load_name = 0;
    music_select_calls = 0;
    music_select_record = 0;
    music_select_value = 0;
}

static int common(void)
{
    return resource_calls == 2
        && resource_resource[0] == DAT_000062ac
        && resource_resource[1] == DAT_000062ac
        && resource_size[0] == 0x2000
        && resource_size[1] == 0x3e800
        && choice_calls == 1
        && state_cells_calls == 1
        && state_cells_value == 0xffff54e0
        && font_table_calls == 1
        && sound_slot_calls == 1
        && DAT_0002082c == 0
        && DAT_0002087c == 0
        && DAT_00015cf0 == 0x190
        && DAT_00015ce4 == 0
        && DAT_00015ce8 == 0x280
        && DAT_00015cec == 0
        && sprite_calls == 1
        && font_calls == 1
        && graphics_calls == 1
        && material_calls == 1
        && screen_calls == 1
        && screen_width == 0x140
        && screen_height == 0x190
        && screen_value == 0xc8
        && memset_calls == 1
        && memset_target == DAT_00021528
        && memset_count == 0x3e800
        && memset_value == 0
        && video_fill_calls == 1
        && member_list_calls == 4
        && member_list_array[0] == (void *)DAT_0001fb80
        && member_list_array[1] == (void *)DAT_0001fd10
        && member_list_array[2] == (void *)DAT_0001fea0
        && member_list_array[3] == (void *)DAT_00020030
        && DAT_0001fb70 == (void *)0x6000
        && DAT_0001fb74 == (void *)0x7000
        && DAT_0001fb78 == (void *)0x8000
        && DAT_0001fb7c == (void *)0x9000
        && DAT_0001fb64 == -1
        && DAT_0001fb60 == 0
        && list_calls == 2
        && list_table[0] == (void *)DAT_0002e04
        && list_stride[0] == 0x18
        && list_count[0] == 0xc
        && list_table[1] == (void *)DAT_0015224
        && list_stride[1] == 0x24
        && list_count[1] == 0x24
        && DAT_00018d84 == 0x12
        && sound_bank_calls == 1
        && objects_calls == 1
        && view_calls == 1
        && view_a == 0x11
        && view_b == 0x22
        && view_c == 0x33;
}

static int normal(void)
{
    return common()
        && report_calls == 0
        && cleanup_calls == 0
        && sprintf_calls == 4
        && sprintf_arg[0] == (unsigned int)DAT_00007edc
        && sprintf_arg[1] == 0x48
        && sprintf_arg[2] == 0x48
        && sprintf_arg[3] == (unsigned int)DAT_00007edc
        && sprite_file == sprintf_buffer[0]
        && sprite_member == sprintf_buffer[1]
        && font_file == sprintf_buffer[0]
        && font_member == sprintf_buffer[2]
        && graphics_a == sprintf_buffer[3]
        && graphics_b == sprintf_buffer[3]
        && DAT_0001fad0 == (void *)0x5000
        && input_calls == 0
        && music_load_calls == 0
        && music_select_calls == 0;
}

static int failures(void)
{
    return common()
        && report_calls == 0
        && cleanup_calls == 2
        && sprite_result == 1
        && font_result == 1
        && DAT_0001fad0 == (void *)0x5000;
}

static int music(void)
{
    return common()
        && report_calls == 0
        && cleanup_calls == 0
        && sprintf_calls == 5
        && sprintf_arg[4] == 7
        && music_load_name == sprintf_buffer[4]
        && input_calls == 1
        && input_state == 8
        && input_device == (void *)1
        && music_load_calls == 1
        && music_load_record == (char *)0x62b0
        && music_select_calls == 1
        && music_select_record == (char *)0x62b0
        && music_select_value == 1;
}

static int zero_resources(void)
{
    return common()
        && report_calls == 2
        && report_resource == DAT_000062ac
        && DAT_0001fad0 == 0
        && cleanup_calls == 0;
}

int main(void)
{
    reset();
    menu_space_init();
    put(normal() ? '0' : 'x');

    reset();
    sprite_result = 1;
    font_result = 1;
    menu_space_init();
    put(failures() ? '1' : 'x');

    reset();
    DAT_0001d2cc = 1;
    menu_space_init();
    put(music() ? '2' : 'x');

    reset();
    resource_result[0] = 0;
    resource_result[1] = 0;
    menu_space_init();
    put(zero_resources() ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
