/* The test harness for the rebuilt `loading_screen` module.
 *
 * The harness holds the screen title pointer, the two flag words, the race
 * context, the record table, the pixel and video buffers, and the resource
 * record.  The stubs record their arguments and the `sprintf` stub formats the
 * two patterns `load%d%c.pcx` and `map%d`.  The stub selects the pattern by the
 * call order, because this minimal image does not load the constant segment, so
 * the module literals are not readable here.  The pixel and palette stubs
 * capture the member name and the palette pointer.  The harness runs nine
 * paths:
 *
 *     0  the normal path with the screen 3: the resource, the screen index,
 *        the context word, the clear, the fill, the load, the palette, the
 *        message, the copy, the panel, the map name, the published fields, and
 *        the table record all run
 *     1  the screen 9 drops to 3, sets the flag bit 0x08 during the run, and
 *        restores the saved dword at the end
 *     2  the load fails: the cleanup at 0x15150 runs and the path continues
 *     3  the flag 0x10 runs the first video swap only
 *     4  the flag 0x20 runs the second video swap only
 *     5  the mode word 0x40 skips the table and stores 0
 *     6  the race reports the code 7 through the video, the error, and the
 *        exit calls
 *     7  the screen 7 drops to 1, so the index 0 and the first record run
 *     8  the screen 6 stays, so the index 5 and the sixth record run
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void loading_screen(int arg1, int arg2, int arg3);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char *DAT_0001fc3c;
int DAT_0001d260;
unsigned int DAT_0001d274;
void *DAT_00002dec;
int DAT_000203bc;
int DAT_000203c4;
char DAT_0001d294[0x40];
int DAT_0001d2a0;
int DAT_0001d2b0;
int DAT_0001d2b4;
int DAT_0001d2b8;
int DAT_0001d2c8;
int DAT_0001d2cc;
int DAT_0001d26c;
char DAT_00020430[0x300];
void *DAT_00021528;
void *DAT_0001fad0;
char DAT_0000cf5c[0x300];
void *DAT_000062ac;
int DAT_000021d8[8];

static char text[8];
static char title[8];
static int resource[8];
static int context[0x40];
static unsigned char video_buffer[0x3e800];
static unsigned char pixel_buffer[0x3e800];
static char resource_name[0x20];
static char file_text[0x20];
static char member_text[0x40];
static char expected_member[0x40];
static char expected_map[0x20];
static char *format_buffer[4];
static int format_value[4];
static int format_calls;
static int resource_calls;
static void *resource_record;
static int index_calls;
static int index_value;
static int memset_calls;
static void *memset_target;
static unsigned int memset_count;
static int memset_value;
static int video_fill_calls;
static int pcx_calls;
static void *pcx_pixels;
static void *pcx_palette;
static int pcx_result;
static unsigned int observed_flags;
static int cleanup_calls;
static int palette_calls;
static char *palette_pointer;
static int message_calls;
static char *message_pointer;
static int memcpy_calls;
static void *memcpy_target;
static void *memcpy_source;
static unsigned int memcpy_count;
static int swap_a_calls;
static int swap_b_calls;
static int panel_calls;
static char *panel_text;
static int panel_y;
static int panel_font;
static int panel_color;
static int race_cleanup_calls;
static int run_calls;
static int run_result;
static int bios_calls;
static int bios_mode;
static int error_calls;
static int error_code;
static int exit_calls;
static int exit_code;
static int final_calls;

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

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
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

void sub_2f326(void *record, char *name)
{
    resource_record = record;
    copy(resource_name, name);
    resource_calls = resource_calls + 1;
}

void sub_8bd54(int value)
{
    index_value = value;
    index_calls = index_calls + 1;
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

void video_fill(void)
{
    video_fill_calls = video_fill_calls + 1;
}

int pcx_load(char *file, char *member, void *pixels, char *palette)
{
    copy(file_text, file);
    copy(member_text, member);
    pcx_pixels = pixels;
    pcx_palette = palette;
    observed_flags = DAT_0001d274;
    palette[0] = 0x5a;
    palette[0x2ff] = 0xa5;
    pcx_calls = pcx_calls + 1;
    return pcx_result;
}

void sub_15150(void)
{
    cleanup_calls = cleanup_calls + 1;
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

void video_block_swap_a(void)
{
    swap_a_calls = swap_a_calls + 1;
}

void video_block_swap_b(void)
{
    swap_b_calls = swap_b_calls + 1;
}

void panel_draw_centered(char *text, int y, int font, int color)
{
    panel_text = text;
    panel_y = y;
    panel_font = font;
    panel_color = color;
    panel_calls = panel_calls + 1;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *out;
    int number;
    int letter;

    (void)format;
    va_start(arguments, format);
    out = buffer;
    if (format_calls == 0) {
        *out++ = 'l';
        *out++ = 'o';
        *out++ = 'a';
        *out++ = 'd';
        number = va_arg(arguments, int);
        out = put_decimal(out, number);
        letter = va_arg(arguments, int);
        *out++ = (char)letter;
        *out++ = '.';
        *out++ = 'p';
        *out++ = 'c';
        *out++ = 'x';
    } else {
        *out++ = 'm';
        *out++ = 'a';
        *out++ = 'p';
        number = va_arg(arguments, int);
        out = put_decimal(out, number);
    }
    *out = 0;
    va_end(arguments);
    if (format_calls < 4) {
        format_buffer[format_calls] = buffer;
        format_value[format_calls] = number;
    }
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
    return run_result;
}

void bios_video(int mode)
{
    bios_mode = mode;
    bios_calls = bios_calls + 1;
}

void error_report(int code)
{
    error_code = code;
    error_calls = error_calls + 1;
}

void exit(int code)
{
    exit_code = code;
    exit_calls = exit_calls + 1;
}

void sub_1be6c(void)
{
    final_calls = final_calls + 1;
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
    for (index = 0; index < 8; ++index) {
        resource[index] = 0x5000 + index;
        DAT_000021d8[index] = 0x6100 + index;
    }
    for (index = 0; index < 0x40; ++index) {
        context[index] = 0;
    }
    *(int *)((char *)context + 8) = 2;
    *(int *)((char *)context + 0x38) = 0x3333;
    *(int *)((char *)context + 0x48) = 0x4444;
    for (index = 0; index < 0x300; ++index) {
        DAT_00020430[index] = 0;
    }
    for (index = 0; index < 8; ++index) {
        *(int *)(DAT_00020430 + index * 0x50) = 0x7000 + index * 0x100;
    }
    DAT_0001fc3c = title;
    DAT_0001d260 = 0;
    DAT_0001d274 = 0;
    DAT_00002dec = context;
    DAT_000203bc = 0;
    DAT_000203c4 = 0x5555;
    DAT_0001d294[0] = 0;
    DAT_0001d2a0 = 0;
    DAT_0001d2b0 = 0;
    DAT_0001d2b4 = 0;
    DAT_0001d2b8 = 0;
    DAT_0001d2c8 = 0;
    DAT_0001d2cc = 0;
    DAT_0001d26c = 0;
    DAT_00021528 = video_buffer;
    DAT_0001fad0 = pixel_buffer;
    DAT_0000cf5c[0] = 0;
    DAT_0000cf5c[0x2ff] = 0;
    DAT_000062ac = resource;
    resource_name[0] = 0;
    file_text[0] = 0;
    member_text[0] = 0;
    format_calls = 0;
    resource_calls = 0;
    resource_record = 0;
    index_calls = 0;
    index_value = 0;
    memset_calls = 0;
    memset_target = 0;
    memset_count = 0;
    memset_value = -1;
    video_fill_calls = 0;
    pcx_calls = 0;
    pcx_pixels = 0;
    pcx_palette = 0;
    pcx_result = 0;
    observed_flags = 0;
    cleanup_calls = 0;
    palette_calls = 0;
    palette_pointer = 0;
    message_calls = 0;
    message_pointer = 0;
    memcpy_calls = 0;
    memcpy_target = 0;
    memcpy_source = 0;
    memcpy_count = 0;
    swap_a_calls = 0;
    swap_b_calls = 0;
    panel_calls = 0;
    panel_text = 0;
    panel_y = 0;
    panel_font = 0;
    panel_color = 0;
    race_cleanup_calls = 0;
    run_calls = 0;
    run_result = 0;
    bios_calls = 0;
    bios_mode = 0;
    error_calls = 0;
    error_code = 0;
    exit_calls = 0;
    exit_code = 0;
    final_calls = 0;
}

int main(void)
{
    reset();
    build_member(expected_member, 3);
    build_map(expected_map, 3);
    loading_screen(0x111, 3, 0x222);
    put(resource_calls == 1 && resource_record == DAT_000062ac
        && DAT_000203bc == 2
        && index_calls == 1 && index_value == DAT_000021d8[2]
        && DAT_0001d2b8 == 2
        && memset_calls == 1 && memset_target == DAT_00021528
        && memset_count == 0x3e800 && memset_value == 0
        && video_fill_calls == 2
        && format_calls == 2
        && format_buffer[0] != 0 && format_buffer[1] == DAT_0001d294
        && format_value[0] == 3 && format_value[1] == 3
        && same(member_text, expected_member)
        && pcx_calls == 1
        && pcx_pixels == DAT_0001fad0 && pcx_palette == DAT_0000cf5c
        && cleanup_calls == 0
        && palette_calls == 1 && palette_pointer == DAT_0000cf5c
        && message_calls == 1 && message_pointer == DAT_0000cf5c
        && memcpy_calls == 1 && memcpy_target == DAT_00021528
        && memcpy_source == DAT_0001fad0 && memcpy_count == 0x3e800
        && swap_a_calls == 0 && swap_b_calls == 0
        && panel_calls == 1 && panel_text == DAT_0001fc3c
        && panel_y == 0x50 && panel_font == 3 && panel_color == 0
        && same(DAT_0001d294, expected_map)
        && DAT_0001d2a0 == 0x5555 && DAT_0001d2b0 == 0x111
        && DAT_0001d2b4 == 0x222
        && DAT_0001d2c8 == 0x3333 && DAT_0001d2cc == 0x4444
        && DAT_0001d26c == 0x7200
        && race_cleanup_calls == 1 && run_calls == 1 && final_calls == 1
        && bios_calls == 0 && error_calls == 0 && exit_calls == 0
        && DAT_0001d274 == 0 ? '0' : 'x');

    reset();
    build_member(expected_member, 3);
    build_map(expected_map, 3);
    DAT_0001d274 = 0x100;
    loading_screen(0x111, 9, 0x222);
    put(same(member_text, expected_member) && same(DAT_0001d294, expected_map)
        && DAT_000203bc == 2 && observed_flags == 0x108
        && DAT_0001d274 == 0x100 && DAT_0001d26c == 0x7200 ? '1' : 'x');

    reset();
    pcx_result = 5;
    loading_screen(0x111, 3, 0x222);
    put(cleanup_calls == 1 && pcx_calls == 1 && palette_calls == 1
        && message_calls == 1 && race_cleanup_calls == 1
        && run_calls == 1 && final_calls == 1 ? '2' : 'x');

    reset();
    DAT_0001d274 = 0x10;
    loading_screen(0x111, 3, 0x222);
    put(swap_a_calls == 1 && swap_b_calls == 0
        && DAT_0001d274 == 0x10 ? '3' : 'x');

    reset();
    DAT_0001d274 = 0x20;
    loading_screen(0x111, 3, 0x222);
    put(swap_a_calls == 0 && swap_b_calls == 1
        && DAT_0001d274 == 0x20 ? '4' : 'x');

    reset();
    DAT_0001d260 = 0x40;
    loading_screen(0x111, 3, 0x222);
    put(DAT_0001d26c == 0 && run_calls == 1 ? '5' : 'x');

    reset();
    run_result = 7;
    loading_screen(0x111, 3, 0x222);
    put(bios_calls == 1 && bios_mode == 3
        && error_calls == 1 && error_code == 7
        && exit_calls == 1 && exit_code == 7
        && final_calls == 1 ? '6' : 'x');

    reset();
    build_member(expected_member, 1);
    build_map(expected_map, 1);
    loading_screen(0x111, 7, 0x222);
    put(DAT_000203bc == 0 && same(member_text, expected_member)
        && same(DAT_0001d294, expected_map) && observed_flags == 0x08
        && DAT_0001d274 == 0 && DAT_0001d26c == 0x7000 ? '7' : 'x');

    reset();
    build_member(expected_member, 6);
    build_map(expected_map, 6);
    loading_screen(0x111, 6, 0x222);
    put(DAT_000203bc == 5 && same(member_text, expected_member)
        && same(DAT_0001d294, expected_map) && DAT_0001d274 == 0
        && DAT_0001d26c == 0x7500 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
