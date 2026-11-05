/* The test harness for the rebuilt `race_results_screen` module.
 *
 * The harness holds the chosen player index, the 0x38-byte player records, the
 * 0x44-byte car records, the 0x50-byte lap records, the 680-byte result
 * records, the race context, the two screen buffers, and the palette.  The
 * `sprintf` stub selects the pattern by the call order, because this minimal
 * image does not load the constant segment, so the module literals are not
 * readable here.  For the same reason the harness builds every compared string
 * from characters, not from a literal.
 *
 * The harness runs six paths:
 *
 *     0  the full path: two finishers, the sort, the two result lines, both
 *        video swaps, the frame step, and the menu frame
 *     1  the input word 0x1020 skips the results screen and the wait loop
 *     2  the first image load fails: the cleanup runs and the path continues
 *     3  the flag 0x10 runs the first video swap only
 *     4  the track 7 drops to 1, sets the flag 0x08 during the run, and
 *        restores the saved dword at the end
 *     5  three finishers sort by time and the hundredths reach the time text
 *     D  the exit marker
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_results_screen(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

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
    int field_28;
    int field_2c;
    int field_30;
    int field_34;
};

unsigned char DAT_00000003;
unsigned char DAT_0000001e;
unsigned char DAT_00113cd8[8 * 680];
char DAT_0011d550[8 * 0x44];
char DAT_00120430[6 * 0x50];
char DAT_0011d294[0x40];
char DAT_0010cf5c[0x300];
char *DAT_0011fc3c;
char *DAT_0011fc48;
int DAT_001021d8[8];
int DAT_00118e1c;
int DAT_0011d250;
int DAT_0011d254;
int DAT_0011d2a0;
int DAT_0011d2b0;
int DAT_0011d2b4;
int DAT_0011d2b8;
int DAT_0011d2c0;
int DAT_0011d2c8;
int DAT_0011d2cc;
int DAT_001203bc;
int DAT_001203c4;
int DAT_00120b5c;
unsigned int DAT_0011d274;
float DAT_0010108b;
float DAT_0011d268;
float DAT_0011d26c;
struct net_player DAT_0012099c[8];
void *DAT_00102dec;
void *DAT_0011fad0;
void *DAT_00121528;

static char text[8];
static char title[8];
static char result_title[8];
static int context[0x60];
static unsigned char video_buffer[0x3e800];
static unsigned char pixel_buffer[0x3e800];
static char name_a[4];
static char name_b[4];
static char name_c[4];
static char car_2[8];
static char car_3[8];
static char car_4[8];
static char member_expected[0x20];
static char map_expected[8];
static char time_expected[4][12];
static char member_text[0x40];
static int pcx_calls;
static int pcx_results[4];
static void *pcx_pixels;
static void *pcx_palette;
static int palette_calls;
static char *palette_pointer;
static int message_calls;
static char *message_pointer;
static int memset_calls;
static void *memset_target;
static unsigned int memset_count;
static int memset_value;
static int memcpy_calls;
static void *memcpy_target;
static void *memcpy_source;
static unsigned int memcpy_count;
static int video_fill_calls;
static int swap_a_calls;
static int swap_b_calls;
static int panel_calls;
static char *panel_text[4];
static int panel_y[4];
static int panel_font[4];
static int panel_color[4];
static int text_calls;
static int text_x[8];
static int text_y[8];
static int text_font[8];
static int text_color[8];
static char text_value[8][0x100];
static int format_calls;
static int line_calls;
static char line_text[8][0x100];
static int race_cleanup_calls;
static int run_calls;
static int music_calls;
static int menu_space_calls;
static int frame_step_calls;
static int input_poll_calls;
static int menu_frame_calls;
static int state_calls;
static int state_value;
static int cleanup_calls;
static int count_value;
static int local_value;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
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

static char *copy_end(char *destination, char *source)
{
    while (*source != 0) {
        *destination++ = *source++;
    }
    return destination;
}

static void copy(char *destination, char *source)
{
    while (*source != 0) {
        *destination++ = *source++;
    }
    *destination = 0;
}

static void set_text(char *out, char first, char second, char third, char fourth)
{
    out[0] = first;
    out[1] = second;
    out[2] = third;
    out[3] = fourth;
    out[4] = 0;
}

static char *put_decimal(char *out, int value)
{
    char digits[12];
    unsigned int number;
    int count;

    count = 0;
    if (value < 0) {
        *out++ = '-';
        number = (unsigned int)(-value);
    } else {
        number = (unsigned int)value;
    }
    do {
        digits[count] = (char)('0' + (number % 10));
        number = number / 10;
        ++count;
    } while (number != 0);
    while (count > 0) {
        --count;
        *out++ = digits[count];
    }
    return out;
}

static char *put_two(char *out, int value)
{
    if (value < 10) {
        *out++ = '0';
    }
    return put_decimal(out, value);
}

static void build_member(char *out, int value)
{
    char *end;

    out[0] = 'l';
    out[1] = 'o';
    out[2] = 'a';
    out[3] = 'd';
    end = put_decimal(out + 4, value);
    *end++ = 'H';
    *end++ = '.';
    *end++ = 'p';
    *end++ = 'c';
    *end++ = 'x';
    *end = 0;
}

static void build_map(char *out, int value)
{
    char *end;

    out[0] = 'm';
    out[1] = 'a';
    out[2] = 'p';
    end = put_decimal(out + 3, value);
    *end = 0;
}

static void build_time(char *out, int first, int second, int third)
{
    char *end;

    end = put_two(out, first);
    *end++ = '.';
    end = put_two(end, second);
    *end++ = '.';
    end = put_two(end, third);
    *end = 0;
}

static void build_line(char *out, int position, char *who, char *when,
                       char *car)
{
    *out++ = 0x01;
    out = put_decimal(out, position);
    *out++ = '.';
    *out++ = ' ';
    out = copy_end(out, who);
    *out++ = ' ';
    out = copy_end(out, when);
    *out++ = ' ';
    *out++ = 0x02;
    out = copy_end(out, car);
    *out = 0;
}

static float float_of(unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    return v.f;
}

void *memset(void *target, unsigned int count, int value)
{
    volatile unsigned char *out;
    unsigned int index;

    memset_calls = memset_calls + 1;
    memset_target = target;
    memset_count = count;
    memset_value = value;
    out = target;
    for (index = 0; index < count; ++index) {
        out[index] = (unsigned char)value;
    }
    return target;
}

void *memcpy(void *target, const void *source, unsigned int count)
{
    volatile unsigned char *out;
    const volatile unsigned char *in;
    unsigned int index;

    memcpy_calls = memcpy_calls + 1;
    memcpy_target = target;
    memcpy_source = (void *)source;
    memcpy_count = count;
    out = target;
    in = source;
    for (index = 0; index < count; ++index) {
        out[index] = in[index];
    }
    return target;
}

int pcx_load(char *file, char *member, void *pixels, char *palette)
{
    (void)file;
    if (pcx_calls == 0) {
        copy(member_text, member);
    }
    pcx_pixels = pixels;
    pcx_palette = palette;
    pcx_calls = pcx_calls + 1;
    return pcx_results[pcx_calls - 1];
}

void palette_set(char *palette)
{
    palette_pointer = palette;
    palette_calls = palette_calls + 1;
}

void message_show(char *buffer)
{
    message_pointer = buffer;
    message_calls = message_calls + 1;
}

void video_fill(void)
{
    video_fill_calls = video_fill_calls + 1;
}

void video_block_swap_a(void)
{
    swap_a_calls = swap_a_calls + 1;
}

void video_block_swap_b(void)
{
    swap_b_calls = swap_b_calls + 1;
}

void panel_draw_centered(char *text_pointer, int y, int font, int color)
{
    if (panel_calls < 4) {
        panel_text[panel_calls] = text_pointer;
        panel_y[panel_calls] = y;
        panel_font[panel_calls] = font;
        panel_color[panel_calls] = color;
    }
    panel_calls = panel_calls + 1;
}

void text_draw(int x, int y, char *value, int font, int color)
{
    if (text_calls < 8) {
        text_x[text_calls] = x;
        text_y[text_calls] = y;
        text_font[text_calls] = font;
        text_color[text_calls] = color;
        copy(text_value[text_calls], value);
    }
    text_calls = text_calls + 1;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *out;
    int number;
    int letter;
    int first;
    int second;
    int third;

    (void)format;
    va_start(arguments, format);
    out = buffer;
    if (format_calls == 0) {
        out[0] = 'l';
        out[1] = 'o';
        out[2] = 'a';
        out[3] = 'd';
        out = out + 4;
        number = va_arg(arguments, int);
        out = put_decimal(out, number);
        letter = va_arg(arguments, int);
        *out++ = (char)letter;
        *out++ = '.';
        *out++ = 'p';
        *out++ = 'c';
        *out++ = 'x';
    } else if (format_calls == 1) {
        out[0] = 'm';
        out[1] = 'a';
        out[2] = 'p';
        out = out + 3;
        number = va_arg(arguments, int);
        out = put_decimal(out, number);
    } else if (((format_calls - 2) % 2) == 0) {
        first = va_arg(arguments, int);
        second = va_arg(arguments, int);
        third = va_arg(arguments, int);
        out = put_two(out, first);
        *out++ = '.';
        out = put_two(out, second);
        *out++ = '.';
        out = put_two(out, third);
    } else {
        first = va_arg(arguments, int);
        {
            char *who = va_arg(arguments, char *);
            char *when = va_arg(arguments, char *);
            char *car = va_arg(arguments, char *);

            *out++ = 0x01;
            out = put_decimal(out, first);
            *out++ = '.';
            *out++ = ' ';
            out = copy_end(out, who);
            *out++ = ' ';
            out = copy_end(out, when);
            *out++ = ' ';
            *out++ = 0x02;
            out = copy_end(out, car);
        }
        if (line_calls < 8) {
            *out = 0;
            copy(line_text[line_calls], buffer);
        }
        line_calls = line_calls + 1;
    }
    *out = 0;
    va_end(arguments);
    format_calls = format_calls + 1;
    return 0;
}

void race_cleanup(void)
{
    race_cleanup_calls = race_cleanup_calls + 1;
}

int sub_1fd70(void)
{
    run_calls = run_calls + 1;
    return 0;
}

void music_step(void)
{
    music_calls = music_calls + 1;
}

void menu_space_init(void)
{
    menu_space_calls = menu_space_calls + 1;
}

void frame_step(void)
{
    frame_step_calls = frame_step_calls + 1;
}

void input_poll(void)
{
    input_poll_calls = input_poll_calls + 1;
}

void menu_frame(void)
{
    menu_frame_calls = menu_frame_calls + 1;
}

void state_cells_set(int value)
{
    state_value = value;
    state_calls = state_calls + 1;
}

void sub_15150(void)
{
    cleanup_calls = cleanup_calls + 1;
}

int race_count(void)
{
    return count_value;
}

int race_local_index(void)
{
    return local_value;
}

static void zero(void *target, unsigned int count)
{
    volatile unsigned char *out;
    unsigned int index;

    out = target;
    for (index = 0; index < count; ++index) {
        out[index] = 0;
    }
}

static void set_player(int index, char *name, int car, int track, int value_18,
                       int value_1c, int flags)
{
    DAT_0012099c[index].field_14 = track;
    DAT_0012099c[index].field_18 = value_18;
    DAT_0012099c[index].field_1c = value_1c;
    DAT_0012099c[index].field_20 = flags;
    DAT_0012099c[index].field_10 = car;
    copy(DAT_0012099c[index].name, name);
}

static void set_car(int index, char *name)
{
    copy(DAT_0011d550 + index * 0x44 + 0x14, name);
}

static void set_result(int index, int status, unsigned int time_bits)
{
    *(int *)(DAT_00113cd8 + index * 680) = status;
    *(float *)(DAT_00113cd8 + index * 680 + 0x2c) = float_of(time_bits);
}

static void setup_text(void)
{
    set_text(name_a, 'A', 'A', 'A', 0);
    set_text(name_b, 'B', 'B', 'B', 0);
    set_text(name_c, 'C', 'C', 'C', 0);
    set_text(car_2, 'C', 'A', 'R', '2');
    set_text(car_3, 'C', 'A', 'R', '3');
    set_text(car_4, 'C', 'A', 'R', '4');
}

static void reset(void)
{
    int index;

    title[0] = 'T';
    title[1] = 'I';
    title[2] = 'T';
    title[3] = 'L';
    title[4] = 'E';
    title[5] = 0;
    result_title[0] = 'R';
    result_title[1] = 'E';
    result_title[2] = 'S';
    result_title[3] = 'U';
    result_title[4] = 'L';
    result_title[5] = 'T';
    result_title[6] = 0;
    setup_text();
    zero(DAT_0012099c, sizeof(DAT_0012099c));
    zero(DAT_00113cd8, sizeof(DAT_00113cd8));
    zero(DAT_0011d550, sizeof(DAT_0011d550));
    zero(DAT_00120430, sizeof(DAT_00120430));
    zero(DAT_0010cf5c, sizeof(DAT_0010cf5c));
    zero(context, sizeof(context));
    *(int *)((char *)context + 8) = 2;
    *(int *)((char *)context + 0x38) = 0x3333;
    *(int *)((char *)context + 0x48) = 0x4444;
    for (index = 0; index < 8; ++index) {
        DAT_001021d8[index] = 0x6100 + index;
    }
    DAT_0011fc3c = title;
    DAT_0011fc48 = result_title;
    DAT_0011d274 = 0;
    DAT_00118e1c = 0;
    DAT_001203bc = 0;
    DAT_001203c4 = 0x5555;
    DAT_00120b5c = 0;
    DAT_0011d250 = 0;
    DAT_0011d254 = 0;
    DAT_0011d268 = float_of(0);
    DAT_0011d26c = float_of(0);
    DAT_0011d2a0 = 0;
    DAT_0011d2b0 = 0;
    DAT_0011d2b4 = 0;
    DAT_0011d2b8 = 0;
    DAT_0011d2c0 = 0;
    DAT_0011d2c8 = 0;
    DAT_0011d2cc = 0;
    DAT_0011d294[0] = 0;
    DAT_00102dec = context;
    DAT_00121528 = video_buffer;
    DAT_0011fad0 = pixel_buffer;
    DAT_0010108b = float_of(0x42c80000);
    DAT_00000003 = 0;
    DAT_0000001e = 0;
    pcx_calls = 0;
    pcx_results[0] = 0;
    pcx_results[1] = 0;
    pcx_results[2] = 0;
    pcx_results[3] = 0;
    pcx_pixels = 0;
    pcx_palette = 0;
    palette_calls = 0;
    palette_pointer = 0;
    message_calls = 0;
    message_pointer = 0;
    memset_calls = 0;
    memset_target = 0;
    memset_count = 0;
    memset_value = 0;
    memcpy_calls = 0;
    memcpy_target = 0;
    memcpy_source = 0;
    memcpy_count = 0;
    video_fill_calls = 0;
    swap_a_calls = 0;
    swap_b_calls = 0;
    panel_calls = 0;
    text_calls = 0;
    format_calls = 0;
    line_calls = 0;
    race_cleanup_calls = 0;
    run_calls = 0;
    music_calls = 0;
    menu_space_calls = 0;
    frame_step_calls = 0;
    input_poll_calls = 0;
    menu_frame_calls = 0;
    state_calls = 0;
    state_value = 0;
    cleanup_calls = 0;
    count_value = 0;
    local_value = 0;
}

int main(void)
{
    char expected0[0x100];
    char expected1[0x100];
    char expected2[0x100];

    reset();
    set_player(0, name_a, 2, 1, 0x1818, 0x1c1c, 0x30);
    set_player(1, name_b, 3, 1, 0x1819, 0x1c1d, 0);
    set_player(2, name_c, 4, 1, 0x181a, 0x1c1e, 0);
    set_car(2, car_2);
    set_car(3, car_3);
    set_car(4, car_4);
    count_value = 3;
    local_value = 0;
    DAT_0011d250 = 0;
    DAT_0011d268 = float_of(0x42b40000);
    set_result(1, -3, 0x42700000);
    set_result(2, 0, 0x00000000);
    DAT_00000003 = 1;
    build_member(member_expected, 2);
    build_map(map_expected, 2);
    build_time(time_expected[0], 1, 0, 0);
    build_time(time_expected[1], 1, 30, 0);
    build_line(expected0, 1, name_b, time_expected[0], car_3);
    build_line(expected1, 2, name_a, time_expected[1], car_2);
    race_results_screen();
    put(DAT_001203bc == 1 && same(DAT_0011d294, map_expected)
        && same(member_text, member_expected)
        && pcx_calls == 2 && pcx_pixels == DAT_0011fad0
        && pcx_palette == DAT_0010cf5c
        && palette_calls == 2 && palette_pointer == DAT_0010cf5c
        && message_calls == 1 && message_pointer == DAT_0010cf5c
        && memset_calls == 1 && memset_target == DAT_00121528
        && memset_count == 0x3e800 && memset_value == 0
        && memcpy_calls == 2 && memcpy_target == DAT_00121528
        && memcpy_source == DAT_0011fad0 && memcpy_count == 0x3e800
        && video_fill_calls == 3 && swap_a_calls == 2 && swap_b_calls == 2
        && panel_calls == 2 && panel_text[0] == DAT_0011fc3c
        && panel_y[0] == 0x50 && panel_font[0] == 3 && panel_color[0] == 0
        && panel_text[1] == DAT_0011fc48 && panel_y[1] == 2
        && panel_font[1] == 3 && panel_color[1] == 2
        && text_calls == 2 && line_calls == 2
        && same(line_text[0], expected0) && same(line_text[1], expected1)
        && text_x[0] == 0x40 && text_y[0] == 0x80
        && text_x[1] == 0x40 && text_y[1] == 0xa0
        && text_font[0] == 0 && text_color[0] == 2
        && DAT_0011d2a0 == 0x5555 && DAT_0011d2b0 == 0x1818
        && DAT_0011d2b4 == 0x1c1c && DAT_0011d2b8 == 2
        && state_calls == 1 && state_value == DAT_001021d8[2]
        && DAT_0011d2c8 == 0x3333 && DAT_0011d2cc == 0x4444
        && race_cleanup_calls == 1 && run_calls == 1 && music_calls == 1
        && menu_space_calls == 1 && frame_step_calls == 1
        && input_poll_calls == 0 && menu_frame_calls == 1
        && DAT_0011d274 == 0 ? '0' : 'x');

    reset();
    set_player(0, name_a, 2, 1, 0x1818, 0x1c1c, 0);
    count_value = 3;
    local_value = 0;
    DAT_00118e1c = 0x1020;
    race_results_screen();
    put(pcx_calls == 1 && panel_calls == 1 && text_calls == 0
        && frame_step_calls == 0 && menu_frame_calls == 1
        && menu_space_calls == 1 && DAT_0011d274 == 0 ? '1' : 'x');

    reset();
    set_player(0, name_a, 2, 1, 0x1818, 0x1c1c, 0);
    set_car(2, car_2);
    count_value = 1;
    local_value = 0;
    DAT_0011d250 = 0;
    DAT_0011d268 = float_of(0x42700000);
    pcx_results[0] = 5;
    DAT_00000003 = 1;
    build_time(time_expected[0], 1, 0, 0);
    build_line(expected0, 1, name_a, time_expected[0], car_2);
    race_results_screen();
    put(cleanup_calls == 1 && pcx_calls == 2 && palette_calls == 2
        && text_calls == 1 && same(line_text[0], expected0)
        && frame_step_calls == 1 && menu_frame_calls == 1 ? '2' : 'x');

    reset();
    set_player(0, name_a, 2, 1, 0x1818, 0x1c1c, 0x10);
    count_value = 1;
    local_value = 0;
    DAT_0011d250 = 0;
    DAT_0011d268 = float_of(0x42700000);
    DAT_00000003 = 1;
    race_results_screen();
    put(swap_a_calls == 2 && swap_b_calls == 0 && pcx_calls == 2
        && DAT_0011d274 == 0 ? '3' : 'x');

    reset();
    set_player(0, name_a, 2, 7, 0x1818, 0x1c1c, 0);
    count_value = 1;
    local_value = 0;
    DAT_0011d250 = 0;
    DAT_0011d268 = float_of(0x42700000);
    DAT_00000003 = 1;
    build_member(member_expected, 2);
    build_map(map_expected, 2);
    race_results_screen();
    put(DAT_001203bc == 1 && same(member_text, member_expected)
        && same(DAT_0011d294, map_expected)
        && DAT_0012099c[0].field_14 == 1
        && DAT_0011d274 == 0 ? '4' : 'x');

    reset();
    set_player(0, name_a, 2, 1, 0x1818, 0x1c1c, 0);
    set_player(1, name_b, 3, 1, 0x1819, 0x1c1d, 0);
    set_player(2, name_c, 4, 1, 0x181a, 0x1c1e, 0);
    set_car(2, car_2);
    set_car(3, car_3);
    set_car(4, car_4);
    count_value = 4;
    local_value = 0;
    DAT_0011d250 = 0;
    DAT_0011d268 = float_of(0x41f20000);
    set_result(1, -3, 0x42f00000);
    set_result(2, -3, 0x42360000);
    set_result(3, 0, 0x00000000);
    DAT_00000003 = 1;
    build_time(time_expected[0], 0, 30, 25);
    build_time(time_expected[1], 0, 45, 50);
    build_time(time_expected[2], 2, 0, 0);
    build_line(expected0, 1, name_a, time_expected[0], car_2);
    build_line(expected1, 2, name_c, time_expected[1], car_4);
    build_line(expected2, 3, name_b, time_expected[2], car_3);
    race_results_screen();
    put(text_calls == 3 && line_calls == 3
        && same(line_text[0], expected0) && same(line_text[1], expected1)
        && same(line_text[2], expected2) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
