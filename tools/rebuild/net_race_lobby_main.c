/* The test harness for the rebuilt `net_race_lobby` module.
 *
 * The harness holds the player record array, the title pointers, the buffers,
 * and the network event queue.  The stubs record their arguments and count
 * their calls.
 *
 * The program runs these paths:
 *
 *     0  the lobby without a message: three players, the mask, the local name,
 *        and the two remote names in two columns
 *     1  the join event: the node add, the sort, the index lookup, and the
 *        0x38-byte record copy
 *     2  the leave event: the node index and the queue remove
 *     3  the start event with the chosen player: the add call and the race
 *        state 3
 *     4  the start event with another chosen player: the loading transition
 *     5  the client wait: the 0x8844 message gives the chosen player
 *     6  the active mode with the event 3 record copy and the chosen draw
 *     7  the active mode with the event 4 menu transition
 *     8  the exit event with one player: the lobby exit
 *     9  the quit event: the lobby exit
 *     D  the done marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, so the test needs no
 * initialized data page.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

struct net_player {
    int field_00;
    int field_04;
    int field_08;
    char name[4];
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    int field_24;
    char node[16];
};

void net_race_lobby(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct net_player DAT_0012099c[4];
int DAT_00120994;
int DAT_00120b60;
int DAT_00120b5c;
int DAT_001203c8;
int DAT_0011d224;
int DAT_000007fd;
char *DAT_0011fc34;
char *DAT_0011fc40;
char *DAT_0011fc44;
char *DAT_0011fc4c;
char *DAT_0011fc50;
void *DAT_0011fad0;
void *DAT_0021528;

static char text[8];
static char title_34[8];
static char title_40[8];
static char title_44[8];
static char title_4c[8];
static char title_50[8];
static char buffer_a[0x30];
static char buffer_b[0x30];
static char car_text[8];
static char pixel_buffer[8];
static char video_buffer[8];

static unsigned char queue[8][0x38];
static unsigned char message_copy[0x38];
static int queue_count;
static int queue_pos;

static int count_value;
static int local_value;
static int node_index_value;
static int pcx_result;

static int panel_calls;
static char *panel_text[8];
static char panel_copy[8][0x20];
static int panel_y[8];
static int panel_font[8];
static int panel_color[8];
static int text_calls;
static int text_x[8];
static int text_y[8];
static char *text_text[8];
static char text_copy[8][0x20];
static int text_font[8];
static int text_color[8];
static int show_calls;
static int show_x[8];
static int show_y[8];
static int show_data[8];
static int receive_calls;
static int queue_add_calls;
static int queue_sort_calls;
static int node_index_calls;
static int queue_remove_calls;
static int queue_remove_index;
static int poll_calls;
static int player_add_calls;
static int player_add_id;
static int race_begin_calls;
static int race_begin_state;
static int present_calls;
static unsigned int present_scale;
static int present_offset;
static int fill_calls;
static int pcx_calls;
static char *pcx_file;
static char *pcx_member;
static void *pcx_pixels;
static char *pcx_palette;
static int cleanup_calls;
static int memcpy_calls;
static void *memcpy_target;
static void *memcpy_source;
static unsigned int memcpy_count;
static int palette_calls;
static char *palette_arg;
static int lookup_calls;
static int lookup_index;
static int wait_calls;
static int menu_calls;
static int exit_calls;
static int format_calls;
static unsigned int format_pointer[8];
static char *format_source[8];
static unsigned int car_pointer;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

static void fill(char *target, int count, char a, char b, char c)
{
    int index;

    for (index = 0; index < count; ++index) {
        target[index] = 0;
    }
    target[0] = a;
    target[1] = b;
    if (count > 2) {
        target[2] = c;
    }
    if (count > 3) {
        target[3] = 0;
    }
}

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; ++index) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

void panel_draw_centered(char *text, int y, int font, int color)
{
    if (panel_calls < 8) {
        panel_text[panel_calls] = text;
        panel_y[panel_calls] = y;
        panel_font[panel_calls] = font;
        panel_color[panel_calls] = color;
        copy(panel_copy[panel_calls], text);
    }
    panel_calls = panel_calls + 1;
}

void text_draw(int x, int y, char *text, int font, int color)
{
    if (text_calls < 8) {
        text_x[text_calls] = x;
        text_y[text_calls] = y;
        text_text[text_calls] = text;
        text_font[text_calls] = font;
        text_color[text_calls] = color;
        copy(text_copy[text_calls], text);
    }
    text_calls = text_calls + 1;
}

void panel_show(int x, int y, int data)
{
    if (show_calls < 8) {
        show_x[show_calls] = x;
        show_y[show_calls] = y;
        show_data[show_calls] = data;
    }
    show_calls = show_calls + 1;
}

int net_receive(void *dest, int count, unsigned char **out)
{
    int index;
    unsigned char *target;

    receive_calls = receive_calls + 1;
    if (queue_pos >= queue_count) {
        return 0;
    }
    target = (unsigned char *)dest;
    for (index = 0; index < count; ++index) {
        target[index] = queue[queue_pos][index];
    }
    for (index = 0; index < 0x38; ++index) {
        message_copy[index] = queue[queue_pos][index];
    }
    if (out != 0) {
        *out = message_copy + 0xc;
    }
    queue_pos = queue_pos + 1;
    return 1;
}

void net_copy(unsigned char *source, unsigned char *dest, int count)
{
    int index;

    for (index = 0; index < count; ++index) {
        dest[index] = source[index];
    }
}

void net_queue_add(void *node)
{
    (void)node;
    queue_add_calls = queue_add_calls + 1;
}

void net_queue_sort(void) { queue_sort_calls = queue_sort_calls + 1; }

int net_node_index(void *node)
{
    (void)node;
    node_index_calls = node_index_calls + 1;
    return node_index_value;
}

void net_queue_remove(int index)
{
    queue_remove_calls = queue_remove_calls + 1;
    queue_remove_index = index;
}

void net_poll(void) { poll_calls = poll_calls + 1; }

int net_count(void) { return count_value; }

int net_local_index(void) { return local_value; }

int net_player_add(int id)
{
    player_add_calls = player_add_calls + 1;
    player_add_id = id;
    return 0;
}

void race_state_begin(int state)
{
    race_begin_calls = race_begin_calls + 1;
    race_begin_state = state;
}

void video_present(int scale, int offset)
{
    present_calls = present_calls + 1;
    present_scale = (unsigned int)scale;
    present_offset = offset;
}

void video_fill(void) { fill_calls = fill_calls + 1; }

int pcx_load(char *file, char *member, void *pixels, char *palette)
{
    pcx_calls = pcx_calls + 1;
    pcx_file = file;
    pcx_member = member;
    pcx_pixels = pixels;
    pcx_palette = palette;
    return pcx_result;
}

void cleanup(void) { cleanup_calls = cleanup_calls + 1; }

void *memcpy(void *target, const void *source, unsigned int count)
{
    memcpy_calls = memcpy_calls + 1;
    memcpy_target = target;
    memcpy_source = (void *)source;
    memcpy_count = count;
    return target;
}

void palette_set(char *palette)
{
    palette_calls = palette_calls + 1;
    palette_arg = palette;
}

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_index = index;
    return 0;
}

void wait_for_input(void) { wait_calls = wait_calls + 1; }

void menu_transition(void) { menu_calls = menu_calls + 1; }

void net_lobby_exit(void) { exit_calls = exit_calls + 1; }

int __cdecl sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *source;

    va_start(arguments, format);
    source = va_arg(arguments, char *);
    va_end(arguments);
    if (format_calls < 8) {
        format_pointer[format_calls] = (unsigned int)format;
        format_source[format_calls] = source;
    }
    format_calls = format_calls + 1;
    if ((unsigned int)source >= 0x1d908
        && (unsigned int)source < 0x1e000) {
        car_pointer = (unsigned int)source;
        copy(buffer, car_text);
    } else {
        copy(buffer, source);
    }
    return 0;
}

static void setup_text(void)
{
    fill(title_34, 8, 'T', '4', 0);
    fill(title_40, 8, 'T', '0', 0);
    fill(title_44, 8, 'T', '5', 0);
    fill(title_4c, 8, 'T', '1', 0);
    fill(title_50, 8, 'T', '2', 0);
    fill(car_text, 8, 'C', 'A', 'R');
    fill(pixel_buffer, 8, 'P', 'X', 0);
    fill(video_buffer, 8, 'V', 'D', 0);
}

static void set_record(int slot, int event, int field_08, char *name,
                       int field_10, int field_14, int field_24, char node)
{
    struct net_player *record;
    int index;

    record = &DAT_0012099c[slot];
    record->field_00 = 0;
    record->field_04 = event;
    record->field_08 = field_08;
    fill(record->name, 4, name[0], name[1], name[2]);
    record->field_10 = field_10;
    record->field_14 = field_14;
    record->field_18 = 0;
    record->field_1c = 0;
    record->field_20 = 0;
    record->field_24 = field_24;
    for (index = 0; index < 16; ++index) {
        record->node[index] = node;
    }
}

static void queue_record(int event, int field_08, char *name, int field_10,
                         int field_14, int field_24, char node)
{
    int index;

    for (index = 0; index < 0x38; ++index) {
        queue[queue_count][index] = 0;
    }
    queue[queue_count][4] = (unsigned char)event;
    queue[queue_count][5] = (unsigned char)(event >> 8);
    queue[queue_count][6] = (unsigned char)(event >> 16);
    queue[queue_count][7] = (unsigned char)(event >> 24);
    queue[queue_count][8] = (unsigned char)field_08;
    queue[queue_count][9] = (unsigned char)(field_08 >> 8);
    queue[queue_count][10] = (unsigned char)(field_08 >> 16);
    queue[queue_count][11] = (unsigned char)(field_08 >> 24);
    queue[queue_count][0xc] = (unsigned char)name[0];
    queue[queue_count][0xd] = (unsigned char)name[1];
    queue[queue_count][0xe] = (unsigned char)name[2];
    queue[queue_count][0xf] = 0;
    queue[queue_count][0x10] = (unsigned char)field_10;
    queue[queue_count][0x14] = (unsigned char)field_14;
    queue[queue_count][0x24] = (unsigned char)field_24;
    for (index = 0; index < 16; ++index) {
        queue[queue_count][0x28 + index] = (unsigned char)node;
    }
    queue_count = queue_count + 1;
}

static void reset(void)
{
    int index;

    queue_count = 0;
    queue_pos = 0;
    count_value = 3;
    local_value = 0;
    node_index_value = 1;
    pcx_result = 0;
    DAT_00120994 = 0;
    DAT_00120b60 = 0;
    DAT_00120b5c = 0;
    DAT_001203c8 = 0x12345678;
    DAT_0011d224 = 0x11111111;
    DAT_000007fd = 0;
    DAT_0011fc34 = title_34;
    DAT_0011fc40 = title_40;
    DAT_0011fc44 = title_44;
    DAT_0011fc4c = title_4c;
    DAT_0011fc50 = title_50;
    DAT_0011fad0 = pixel_buffer;
    DAT_0021528 = video_buffer;
    panel_calls = 0;
    text_calls = 0;
    show_calls = 0;
    receive_calls = 0;
    queue_add_calls = 0;
    queue_sort_calls = 0;
    node_index_calls = 0;
    queue_remove_calls = 0;
    queue_remove_index = -1;
    poll_calls = 0;
    player_add_calls = 0;
    player_add_id = 0;
    race_begin_calls = 0;
    race_begin_state = 0;
    present_calls = 0;
    present_scale = 1;
    present_offset = -1;
    fill_calls = 0;
    pcx_calls = 0;
    pcx_file = 0;
    pcx_member = 0;
    pcx_pixels = 0;
    pcx_palette = 0;
    cleanup_calls = 0;
    memcpy_calls = 0;
    memcpy_target = 0;
    memcpy_source = 0;
    memcpy_count = 0;
    palette_calls = 0;
    palette_arg = 0;
    lookup_calls = 0;
    lookup_index = -1;
    wait_calls = 0;
    menu_calls = 0;
    exit_calls = 0;
    format_calls = 0;
    car_pointer = 0;
    for (index = 0; index < 4; ++index) {
        set_record(index, 0, index, "PLY", 0x100, 0, 0, 'A');
    }
    for (index = 0; index < 8; ++index) {
        panel_text[index] = 0;
        panel_y[index] = 0;
        panel_font[index] = 0;
        panel_color[index] = 0;
        text_text[index] = 0;
        text_x[index] = 0;
        text_y[index] = 0;
        text_font[index] = 0;
        text_color[index] = 0;
        show_x[index] = 0;
        show_y[index] = 0;
        show_data[index] = 0;
        format_pointer[index] = 0;
        format_source[index] = 0;
    }
}

static int run_zero(void)
{
    reset();
    set_record(0, 0, 0, "LOC", 0x100, 0, 1, 'A');
    set_record(1, 0, 1, "AAA", 0x110, 0, 2, 'B');
    set_record(2, 0, 2, "BBB", 0x120, 0, 4, 'C');
    net_race_lobby();
    return panel_calls == 3
        && panel_text[0] == title_40 && panel_y[0] == 4
        && panel_font[0] == 2 && panel_color[0] == 2
        && panel_text[1] == title_4c && panel_y[1] == 0x14
        && panel_font[1] == 0 && panel_color[1] == 2
        && panel_text[2] == title_50 && panel_y[2] == 0xba
        && DAT_00120b60 == 7
        && text_calls == 3
        && same(text_copy[0], DAT_0012099c[0].name)
        && same(text_copy[1], DAT_0012099c[1].name)
        && same(text_copy[2], DAT_0012099c[2].name)
        && text_x[0] == 0x18 && text_y[0] == 0x68
        && text_x[1] == 0x150 && text_y[1] == 0x68
        && text_x[2] == 0x18 && text_y[2] == 0xb4
        && show_calls == 3
        && show_data[0] == 0x110
        && show_x[0] == 0x2d && show_y[0] == 0x25
        && format_calls == 3
        && format_pointer[0] == 0x12be
        && format_pointer[1] == 0x12c1
        && format_pointer[2] == 0x12c1;
}

static int run_join(void)
{
    reset();
    set_record(0, 0, 0, "LOC", 0x100, 0, 1, 'A');
    set_record(1, 0, 1, "AAA", 0x110, 0, 2, 'B');
    set_record(2, 0, 2, "BBB", 0x120, 0, 4, 'C');
    node_index_value = 2;
    queue_record(0, 5, "REM", 0x130, 3, 8, 'Z');
    net_race_lobby();
    return queue_add_calls == 1
        && queue_sort_calls == 1
        && node_index_calls == 1
        && queue_remove_calls == 0
        && DAT_0012099c[2].field_08 == 5
        && same(DAT_0012099c[2].name, "REM")
        && DAT_0012099c[2].field_14 == 3
        && DAT_0012099c[2].node[0] == 'Z';
}

static int run_leave(void)
{
    reset();
    node_index_value = 1;
    queue_record(1, 1, "AAA", 0x110, 0, 2, 'B');
    net_race_lobby();
    return queue_remove_calls == 1
        && queue_remove_index == 1
        && queue_sort_calls == 1
        && queue_add_calls == 0;
}

static int run_start_local(void)
{
    reset();
    count_value = 1;
    local_value = 0;
    DAT_00120b60 = 1;
    queue_record(2, 0, "LOC", 0x100, 0, 1, 'A');
    net_race_lobby();
    return player_add_calls == 1
        && player_add_id == 0x8844
        && DAT_001203c8 == 0x12345678
        && DAT_00120b5c == 0
        && race_begin_calls == 1
        && race_begin_state == 3
        && DAT_0011d224 == 0;
}

static int run_start_other(void)
{
    reset();
    count_value = 2;
    local_value = 0;
    DAT_00120b60 = 1;
    DAT_000007fd = 1;
    queue_record(2, 0, "LOC", 0x100, 0, 1, 'A');
    net_race_lobby();
    return player_add_calls == 1
        && player_add_id == 0x8844
        && DAT_00120b5c == 1
        && race_begin_calls == 0
        && present_calls == 1
        && present_scale == 0
        && present_offset == 0
        && fill_calls == 2
        && pcx_calls == 1
        && pcx_file == (char *)0x12b1
        && pcx_member == (char *)0x12a6
        && pcx_pixels == DAT_0011fad0
        && pcx_palette == (char *)0xcf5c
        && cleanup_calls == 0
        && memcpy_calls == 1
        && memcpy_target == DAT_0021528
        && memcpy_source == DAT_0011fad0
        && memcpy_count == 0x3e800
        && palette_calls == 1
        && palette_arg == (char *)0xcf5c
        && DAT_00120994 == 1
        && lookup_calls == 1
        && lookup_index == 7
        && wait_calls == 1;
}

static int run_client(void)
{
    reset();
    count_value = 2;
    local_value = 1;
    DAT_00120b60 = 1;
    queue_record(2, 0, "LOC", 0x100, 0, 1, 'A');
    queue_record(0x8844, 7, "HST", 0x140, 0, 9, 'H');
    net_race_lobby();
    return player_add_calls == 0
        && DAT_00120b5c == 0
        && present_calls == 1
        && DAT_00120994 == 1
        && lookup_index == 7;
}

static int run_active_copy(void)
{
    reset();
    DAT_00120994 = 1;
    DAT_00120b5c = 1;
    queue_record(3, 1, "NEW", 0x150, 2, 0x10, 'N');
    net_race_lobby();
    return player_add_calls == 1
        && player_add_id == 0x8844
        && DAT_001203c8 == 0x12345678
        && DAT_0012099c[1].field_08 == 1
        && same(DAT_0012099c[1].name, "NEW")
        && DAT_0012099c[1].field_14 == 2
        && panel_calls == 4
        && panel_text[0] == title_34 && panel_y[0] == 2
        && panel_font[0] == 3 && panel_color[0] == 2
        && panel_y[1] == 0x50 && panel_font[1] == 2
        && same(panel_copy[1], "NEW")
        && panel_text[2] == title_44 && panel_y[2] == 0x6e
        && panel_y[3] == 0x96 && panel_font[3] == 3
        && format_calls == 2
        && format_pointer[0] == 0x12c4
        && format_pointer[1] == 0x12c7
        && car_pointer == 0x1d908 + 2 * 0x34 + 0x14;
}

static int run_active_menu(void)
{
    reset();
    DAT_00120994 = 1;
    DAT_00120b5c = 0;
    queue_record(4, 0, "WIN", 0x100, 0, 1, 'A');
    net_race_lobby();
    return player_add_calls == 1
        && menu_calls == 1
        && panel_calls == 4
        && format_calls == 2;
}

static int run_exit_one(void)
{
    reset();
    count_value = 1;
    local_value = 0;
    queue_record(3, 0, "LOC", 0x100, 0, 1, 'A');
    net_race_lobby();
    return exit_calls == 1
        && race_begin_calls == 0
        && player_add_calls == 0;
}

static int run_quit(void)
{
    reset();
    count_value = 1;
    local_value = 0;
    queue_record(0x1234, 0, "LOC", 0x100, 0, 1, 'A');
    net_race_lobby();
    return exit_calls == 1
        && race_begin_calls == 0
        && player_add_calls == 0;
}

int main(void)
{
    setup_text();

    put(run_zero() ? '0' : 'x');
    put(run_join() ? '1' : 'x');
    put(run_leave() ? '2' : 'x');
    put(run_start_local() ? '3' : 'x');
    put(run_start_other() ? '4' : 'x');
    put(run_client() ? '5' : 'x');
    put(run_active_copy() ? '6' : 'x');
    put(run_active_menu() ? '7' : 'x');
    put(run_exit_one() ? '8' : 'x');
    put(run_quit() ? '9' : 'x');
    put('D');
    quit();
    return 0;
}
