/* The test harness for the rebuilt `race_data_load` module.
 *
 * The harness holds every global of the module and a stub for every call
 * target.  The `sprintf` stub formats `%s`, `%c`, and `%d` into the buffer, so
 * each check compares the built name.  The other stubs record their arguments
 * and count their calls.
 *
 * The program runs these paths:
 *
 *     0  the level 0 returns at once and calls nothing
 *     1  the high letter, the plain track name, the non-0x100 mode, and no
 *        music: the graphics path, the five list loads, the material calls,
 *        the `.smg` open, the screen mode, the color copy, the two name
 *        builds, the two table inits, the font and the sprite load, the member
 *        list, and the three cockpit resources
 *     2  the low letter, the replay flag, the `i` suffix, the level 0 letter
 *        `A`, the mode 0x100, and the music flag: the replay open, the
 *        `graf.dat` font and sprite load, and the music name `gameH.spr`
 *     3  the non-0x100 mode and the music flag: the music name `music5`
 *     4  the mode 0x100 and no music: the `attrH.fnt` font and the `logoH.spr`
 *        sprite load, and no music call
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

int race_data_load(int level);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007edc[0x20];
char DAT_00007f44[0x34];
int DAT_00001f00;
int DAT_00001f08[8];
int DAT_000203bc;
int DAT_0010cf44[4];
int DAT_0010d25c[8];
int DAT_0010d6dc;
int DAT_0010f000[8];
int DAT_0010f17c;
int DAT_0010f180[8];
int DAT_0010f8ec;
char DAT_0011d278[8];
char DAT_0011d294[8];
int DAT_00110d9c[8];
int DAT_00118e30;
char DAT_00118f10[8];
char DAT_00118f14[8];
char DAT_00118f18[8];
int DAT_00118f94[8];
int DAT_0011905c;
int DAT_0011d260;
int DAT_0011d274;
int DAT_0011d2b4;
int DAT_0011d2b8;
int DAT_0011d2cc;
char DAT_001062b0[8];
unsigned int *PTR_DAT_0008a844;

static unsigned int color_target[8];
static char text[8];

static int graphics_calls;
static char *graphics_a;
static char *graphics_b;
static int replay_calls;
static char *replay_name;
static int replay_value;
static int list_calls;
static char *list_type[8];
static char *list_name[8];
static char *list_list[8];
static void *list_table[8];
static int list_stride[8];
static int list_count[8];
static int list_result[8];
static int material_calls;
static char *material_name;
static int material_name_set_calls;
static char *material_name_set_name;
static int stream_calls;
static char *stream_path;
static char *stream_member;
static int stream_value;
static int screen_calls;
static unsigned int screen_width;
static unsigned int screen_height;
static unsigned int screen_value;
static int names_build_calls;
static char *names_build_name;
static int resource_calls;
static char *resource_name;
static int font_table_calls;
static int sound_slot_calls;
static int font_calls;
static char *font_file;
static char *font_member;
static int sprite_calls;
static char *sprite_file;
static char *sprite_member;
static int member_list_calls;
static char *member_list_name;
static void *member_list_array;
static int member_list_result;
static int cockpit_calls;
static char *cockpit_name[4];
static void *cockpit_out[4];
static int music_calls;
static void *music_record;
static char *music_name;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static char *append_text(char *out, char *value)
{
    while (*value != 0) {
        *out = *value;
        ++out;
        ++value;
    }
    return out;
}

static char *append_char(char *out, char value)
{
    *out = value;
    ++out;
    return out;
}

static char *append_number(char *out, int value)
{
    char digits[12];
    int count;

    count = 0;
    do {
        digits[count] = (char)('0' + value % 10);
        value = value / 10;
        ++count;
    } while (value != 0);
    while (count > 0) {
        --count;
        *out = digits[count];
        ++out;
    }
    return out;
}

int sprintf(char *buffer, char *format, ...)
{
    va_list arguments;
    char *out;
    char *scan;
    char *value;

    va_start(arguments, format);
    out = buffer;
    scan = format;
    while (*scan != 0) {
        if (*scan == '%') {
            ++scan;
            if (*scan == 's') {
                value = va_arg(arguments, char *);
                out = append_text(out, value);
            } else if (*scan == 'c') {
                out = append_char(out, (char)va_arg(arguments, int));
            } else if (*scan == 'd') {
                out = append_number(out, va_arg(arguments, int));
            }
            ++scan;
        } else {
            *out = *scan;
            ++out;
            ++scan;
        }
    }
    *out = 0;
    va_end(arguments);
    return (int)(out - buffer);
}

void graphics_load(char *first, char *second)
{
    graphics_a = first;
    graphics_b = second;
    graphics_calls = graphics_calls + 1;
}

void replay_open(char *name, int value)
{
    replay_name = name;
    replay_value = value;
    replay_calls = replay_calls + 1;
}

int list_file_load(char *type, char *name, char *list, void *table,
                   int stride, int count)
{
    list_type[list_calls] = type;
    list_name[list_calls] = name;
    list_list[list_calls] = list;
    list_table[list_calls] = table;
    list_stride[list_calls] = stride;
    list_count[list_calls] = count;
    list_calls = list_calls + 1;
    return list_result[list_calls - 1];
}

void material_load(char *name)
{
    material_name = name;
    material_calls = material_calls + 1;
}

void material_name_set(char *name)
{
    material_name_set_name = name;
    material_name_set_calls = material_name_set_calls + 1;
}

void track_stream_open(char *path, char *member, int value)
{
    stream_path = path;
    stream_member = member;
    stream_value = value;
    stream_calls = stream_calls + 1;
}

void screen_mode_set(unsigned int width, unsigned int height,
                     unsigned int value)
{
    screen_width = width;
    screen_height = height;
    screen_value = value;
    screen_calls = screen_calls + 1;
}

void track_names_build(char *name)
{
    names_build_name = name;
    names_build_calls = names_build_calls + 1;
}

void track_resource_load(char *name)
{
    resource_name = name;
    resource_calls = resource_calls + 1;
}

void font_table_init(void)
{
    font_table_calls = font_table_calls + 1;
}

void sound_slot_init(void)
{
    sound_slot_calls = sound_slot_calls + 1;
}

int font_load(char *file, char *member)
{
    font_file = file;
    font_member = member;
    font_calls = font_calls + 1;
    return 0;
}

int sprite_load(char *file, char *member)
{
    sprite_file = file;
    sprite_member = member;
    sprite_calls = sprite_calls + 1;
    return 0;
}

int member_list_load(char *name, void *array)
{
    member_list_name = name;
    member_list_array = array;
    member_list_calls = member_list_calls + 1;
    return member_list_result;
}

void cockpit_resource_load(char *name, void *out)
{
    cockpit_name[cockpit_calls] = name;
    cockpit_out[cockpit_calls] = out;
    cockpit_calls = cockpit_calls + 1;
}

void music_load(void *record, char *name)
{
    music_record = record;
    music_name = name;
    music_calls = music_calls + 1;
}

static int same(char *left, char *right)
{
    while (*left != 0 && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void copy(char *target, char *source)
{
    while (*source != 0) {
        *target = *source;
        ++target;
        ++source;
    }
    *target = 0;
}

static void reset(void)
{
    copy(DAT_00007f44, "BASE");
    copy(DAT_00007edc, "ROOT");
    copy(DAT_0011d294, "MAP1");
    copy(DAT_0011d278, "GHOST");
    DAT_00001f00 = 0x48;
    DAT_00001f08[2] = 5;
    DAT_000203bc = 2;
    DAT_0010cf44[0] = 7;
    DAT_0010d6dc = 0;
    DAT_0010f17c = 0;
    DAT_0010f8ec = 0;
    DAT_00118e30 = 0;
    DAT_00118f10[0] = 1;
    DAT_00118f14[0] = 2;
    DAT_00118f18[0] = 3;
    DAT_0011905c = 0;
    DAT_0011d260 = 0;
    DAT_0011d274 = 0;
    DAT_0011d2b4 = 1;
    DAT_0011d2b8 = 1;
    DAT_0011d2cc = 0;
    PTR_DAT_0008a844 = color_target;
    color_target[0] = 0;
    color_target[1] = 0;
    color_target[2] = 0;
    color_target[3] = 0;
    color_target[4] = 0;
    list_result[0] = 0x11;
    list_result[1] = 0x12;
    list_result[2] = 0x13;
    list_result[3] = 0x14;
    list_result[4] = 0x15;
    member_list_result = 0x77;
    graphics_calls = 0;
    replay_calls = 0;
    list_calls = 0;
    material_calls = 0;
    material_name_set_calls = 0;
    stream_calls = 0;
    screen_calls = 0;
    names_build_calls = 0;
    resource_calls = 0;
    font_table_calls = 0;
    sound_slot_calls = 0;
    font_calls = 0;
    sprite_calls = 0;
    member_list_calls = 0;
    cockpit_calls = 0;
    music_calls = 0;
}

static int level_zero(void)
{
    reset();
    race_data_load(0);
    return graphics_calls == 0
        && replay_calls == 0
        && list_calls == 0
        && material_calls == 0
        && stream_calls == 0
        && screen_calls == 0
        && font_table_calls == 0
        && sound_slot_calls == 0
        && font_calls == 0
        && sprite_calls == 0
        && member_list_calls == 0
        && cockpit_calls == 0
        && music_calls == 0;
}

static int normal_path(void)
{
    reset();
    race_data_load(1);
    return graphics_calls == 1
        && same(graphics_a, "BASE\\gfx\\MAP1H.dat")
        && same(graphics_b, "BASE\\gfx\\MAP1H.dat")
        && replay_calls == 0
        && list_calls == 5
        && same(list_type[0], "MAP1")
        && same(list_name[0], "MAP1dat")
        && same(list_list[0], "OSTACLES")
        && list_table[0] == DAT_0010f000
        && list_stride[0] == 0x24
        && list_count[0] == 0x24
        && same(list_type[1], "MAP1")
        && same(list_name[1], "MAP1B")
        && same(list_list[1], "TRACK_DATA")
        && list_table[1] == DAT_0010cf44
        && list_stride[1] == 0x10
        && list_count[1] == 0x10
        && same(list_type[2], "MAP1")
        && same(list_name[2], "MAP1B")
        && same(list_list[2], "OPPONENTS")
        && list_table[2] == DAT_00110d9c
        && list_stride[2] == 0x590
        && list_count[2] == 0x14
        && same(list_type[3], "MAP1")
        && same(list_name[3], "MAP1dat")
        && same(list_list[3], "ANIMATIONS")
        && list_table[3] == DAT_0010f180
        && list_stride[3] == 0xbc
        && list_count[3] == 0x18
        && same(list_type[4], "GEN")
        && same(list_name[4], "CARS")
        && same(list_list[4], "CARS_DATA")
        && list_table[4] == DAT_0010d25c
        && list_stride[4] == 0x40
        && list_count[4] == 0x40
        && DAT_0010f8ec == 0x13
        && DAT_0010f17c == 0x14
        && DAT_0010d6dc == 0x15
        && material_calls == 1
        && same(material_name, "MAP1")
        && material_name_set_calls == 1
        && same(material_name_set_name, "MAP1")
        && stream_calls == 1
        && same(stream_path, "BASE\\gfx\\MAP1H.dat")
        && same(stream_member, "MAP1.smg")
        && stream_value == 0x104
        && screen_calls == 1
        && screen_width == 0x140
        && screen_height == 0x190
        && screen_value == 0x104
        && color_target[4] == 0x07070707
        && names_build_calls == 1
        && same(names_build_name, "MAP1")
        && resource_calls == 1
        && same(resource_name, "MAP1")
        && font_table_calls == 1
        && sound_slot_calls == 1
        && font_calls == 1
        && same(font_file, "BASE\\gfx\\game.dat")
        && same(font_member, "gameH.fnt")
        && sprite_calls == 1
        && same(sprite_file, "BASE\\gfx\\game.dat")
        && same(sprite_member, "gameH.spr")
        && member_list_calls == 1
        && same(member_list_name, "game")
        && member_list_array == DAT_00118f94
        && DAT_0011905c == 0x77
        && cockpit_calls == 3
        && same(cockpit_name[0], "impres_")
        && cockpit_out[0] == DAT_00118f10
        && same(cockpit_name[1], "implet_")
        && cockpit_out[1] == DAT_00118f18
        && same(cockpit_name[2], "cdfront")
        && cockpit_out[2] == DAT_00118f14
        && music_calls == 0;
}

static int mode_100_path(void)
{
    reset();
    DAT_0011d2b8 = 0;
    DAT_00118e30 = 1;
    DAT_0011d274 = 8;
    DAT_0011d2b4 = 0;
    DAT_0011d260 = 0x100;
    DAT_0011d2cc = 1;
    race_data_load(1);
    return graphics_calls == 1
        && same(graphics_a, "BASE\\gfx\\MAP1L.dat")
        && replay_calls == 1
        && replay_name == DAT_0011d278
        && replay_value == 0
        && list_calls == 5
        && same(list_type[0], "MAP1i")
        && same(list_name[0], "MAP1dat")
        && same(list_name[1], "MAP1A")
        && same(list_name[2], "MAP1A")
        && same(list_name[3], "MAP1dat")
        && font_calls == 1
        && same(font_file, "ROOT\\gfx\\graf.dat")
        && same(font_member, "attrH.fnt")
        && sprite_calls == 1
        && same(sprite_file, "ROOT\\gfx\\graf.dat")
        && same(sprite_member, "logoH.spr")
        && member_list_calls == 1
        && same(member_list_name, "game")
        && DAT_0011905c == 0x77
        && cockpit_calls == 0
        && music_calls == 1
        && music_record == DAT_001062b0
        && same(music_name, "gameH.spr");
}

static int music_path(void)
{
    reset();
    DAT_0011d2cc = 1;
    DAT_000203bc = 2;
    DAT_00001f08[2] = 5;
    race_data_load(1);
    return same(font_member, "gameH.fnt")
        && same(sprite_member, "gameH.spr")
        && cockpit_calls == 3
        && music_calls == 1
        && music_record == DAT_001062b0
        && same(music_name, "music5");
}

static int no_music_path(void)
{
    reset();
    DAT_0011d260 = 0x100;
    DAT_0011d2cc = 0;
    race_data_load(1);
    return same(font_file, "ROOT\\gfx\\graf.dat")
        && same(font_member, "attrH.fnt")
        && same(sprite_member, "logoH.spr")
        && member_list_calls == 1
        && cockpit_calls == 0
        && music_calls == 0;
}

int main(void)
{
    put(level_zero() ? '0' : 'x');
    put(normal_path() ? '1' : 'x');
    put(mode_100_path() ? '2' : 'x');
    put(music_path() ? '3' : 'x');
    put(no_music_path() ? '4' : 'x');
    put('D');
    quit();
    return 0;
}
