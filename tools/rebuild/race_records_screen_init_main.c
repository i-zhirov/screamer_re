/* The test harness for the rebuilt `race_records_screen_init` module.
 *
 * The harness holds the menu index, the entry table, the base table, the
 * selected track and row, the two float fields, the two buffers, the palette,
 * the format, and the archive.  Each path sets the menu index, the row, and the
 * track field, and then checks the selected track, the member name, the four
 * callee flags, the two presented scales and offsets, the copy source, the
 * target and the count, and the cleared highlight row.
 *
 * The harness runs three paths:
 *
 *     0  the index 0, the row 1, and the track 5: the member is load6H.pcx and
 *        no cleanup runs
 *     1  the index 1, the row 2, and the track 9: the member is load10H.pcx
 *        and the failed load runs the cleanup
 *     2  the index 2, the row 0, and the track 99: the member is load100H.pcx
 *        and the multi-digit number is correct
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_records_screen_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011fb60;
int DAT_0011fb08[3];
int DAT_00102de8[3];
int DAT_0012098c;
float DAT_00120378;
float DAT_00120380;
int DAT_0011fac8;
void *DAT_0011fad0;
void *DAT_00121528;
unsigned char DAT_0010cf5c[0x300];
float DAT_00118e08;
int DAT_000007f9;
int DAT_00120990;
char race_records_format[] = "load%d%c.pcx";
char race_records_archive[] = "gfx\\graf.dat";

static char text[8];
static unsigned char track_table[3][0x30];
static char pcx_file[32];
static char pcx_member[32];
static int pcx_calls;
static int pcx_result;
static void *pcx_pixels;
static char *pcx_palette;

static int run_calls;
static int cleanup_calls;
static int palette_calls;
static unsigned char *palette_pointer;
static int present_calls;
static float present_scale[4];
static int present_offset[4];
static int copy_calls;
static void *copy_target;
static void *copy_source;
static unsigned int copy_count;
static int fill_calls;

static char pixel_buffer[0x400];
static char screen_buffer[0x400];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0 && index < 0x1f; index++) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; index++) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

static int near_value(float left, float right, float tolerance)
{
    float difference;

    difference = left - right;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference < tolerance;
}

static void set_track(int index, int row, int value)
{
    DAT_00102de8[index] = (int)track_table[index];
    DAT_0011fb08[index] = row;
    *(int *)(track_table[index] + row * 16 + 0xc) = value;
}

static void reset(void)
{
    int index;

    DAT_0011fb60 = 0;
    DAT_0012098c = -1;
    DAT_00120378 = 0.0f;
    DAT_00120380 = 0.0f;
    DAT_0011fac8 = 0;
    DAT_00118e08 = 0.0f;
    DAT_000007f9 = 0;
    DAT_00120990 = 7;
    for (index = 0; index < 3; index++) {
        DAT_0011fb08[index] = 0;
        DAT_00102de8[index] = 0;
    }
    DAT_0011fad0 = pixel_buffer;
    DAT_00121528 = screen_buffer;

    pcx_calls = 0;
    pcx_result = 0;
    pcx_pixels = 0;
    pcx_palette = 0;
    pcx_file[0] = 0;
    pcx_member[0] = 0;
    run_calls = 0;
    cleanup_calls = 0;
    palette_calls = 0;
    palette_pointer = 0;
    present_calls = 0;
    copy_calls = 0;
    copy_target = 0;
    copy_source = 0;
    copy_count = 0;
    fill_calls = 0;
    for (index = 0; index < 4; index++) {
        present_scale[index] = 0.0f;
        present_offset[index] = 0;
    }
}

void sub_2dff4(void)
{
    run_calls = run_calls + 1;
}

void sub_15150(void)
{
    cleanup_calls = cleanup_calls + 1;
}

void palette_shade_build(unsigned char *palette)
{
    palette_calls = palette_calls + 1;
    palette_pointer = palette;
}

void video_present(float scale, int offset)
{
    if (present_calls < 4) {
        present_scale[present_calls] = scale;
        present_offset[present_calls] = offset;
    }
    present_calls = present_calls + 1;
}

void *memcpy(void *target, const void *source, unsigned int count)
{
    copy_calls = copy_calls + 1;
    copy_target = target;
    copy_source = (void *)source;
    copy_count = count;
    return target;
}

void video_fill(void)
{
    fill_calls = fill_calls + 1;
}

int pcx_load(char *file, char *member, void *pixels, char *palette)
{
    pcx_calls = pcx_calls + 1;
    copy(pcx_file, file);
    copy(pcx_member, member);
    pcx_pixels = pixels;
    pcx_palette = palette;
    return pcx_result;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *out;
    char *string;
    char digits[12];
    int index;
    int count;
    int value;
    char character;

    out = buffer;
    index = 0;
    va_start(arguments, format);
    while (format[index] != 0) {
        if (format[index] != '%') {
            *out = format[index];
            out = out + 1;
            index = index + 1;
            continue;
        }
        index = index + 1;
        if (format[index] == 'd') {
            value = va_arg(arguments, int);
            if (value < 0) {
                *out = '-';
                out = out + 1;
                value = -value;
            }
            count = 0;
            if (value == 0) {
                digits[count] = '0';
                count = count + 1;
            }
            while (value > 0) {
                digits[count] = (char)('0' + value % 10);
                count = count + 1;
                value = value / 10;
            }
            while (count > 0) {
                count = count - 1;
                *out = digits[count];
                out = out + 1;
            }
            index = index + 1;
        } else if (format[index] == 'c') {
            character = (char)va_arg(arguments, int);
            *out = character;
            out = out + 1;
            index = index + 1;
        } else if (format[index] == 's') {
            string = va_arg(arguments, char *);
            while (*string != 0) {
                *out = *string;
                out = out + 1;
                string = string + 1;
            }
            index = index + 1;
        } else {
            *out = format[index];
            out = out + 1;
            index = index + 1;
        }
    }
    va_end(arguments);
    *out = 0;
    return out - buffer;
}

static int common_ok(void)
{
    return near_value(DAT_00120378, 1.5f, 0.001f)
        && near_value(DAT_00120380, 8.0f, 0.001f)
        && DAT_0011fac8 == 1
        && run_calls == 1
        && same(pcx_file, "gfx\\graf.dat")
        && pcx_pixels == DAT_0011fad0
        && pcx_palette == (char *)DAT_0010cf5c
        && near_value(DAT_00118e08, 0.08f, 0.001f)
        && DAT_000007f9 == 8
        && palette_calls == 1
        && palette_pointer == DAT_0010cf5c
        && present_calls == 2
        && near_value(present_scale[0], 0.0f, 0.001f)
        && near_value(present_scale[1], 0.0f, 0.001f)
        && present_offset[0] == 0
        && present_offset[1] == 0
        && copy_calls == 1
        && copy_target == DAT_00121528
        && copy_source == DAT_0011fad0
        && copy_count == 0x3e800
        && fill_calls == 1
        && DAT_00120990 == 0;
}

int main(void)
{
    reset();
    DAT_0011fb60 = 0;
    set_track(0, 1, 5);
    race_records_screen_init();
    put(DAT_0012098c == 5
            && pcx_calls == 1
            && same(pcx_member, "load6H.pcx")
            && cleanup_calls == 0
            && common_ok()
        ? '0' : 'x');

    reset();
    DAT_0011fb60 = 1;
    set_track(1, 2, 9);
    pcx_result = 1;
    race_records_screen_init();
    put(DAT_0012098c == 9
            && pcx_calls == 1
            && same(pcx_member, "load10H.pcx")
            && cleanup_calls == 1
            && common_ok()
        ? '1' : 'x');

    reset();
    DAT_0011fb60 = 2;
    set_track(2, 0, 99);
    race_records_screen_init();
    put(DAT_0012098c == 99
            && pcx_calls == 1
            && same(pcx_member, "load100H.pcx")
            && cleanup_calls == 0
            && common_ok()
        ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
