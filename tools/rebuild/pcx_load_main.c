/* The test harness for the rebuilt `pcx_load` module.
 *
 * The harness holds the base path, the simulated PCX member, the stream stubs,
 * the open stub, the error stub, and the close stub.  The stream stubs serve
 * the member from a buffer and record the seek calls.  The harness runs four
 * paths:
 *
 *     0  a 3x2 image with two RLE runs and one literal byte.  The path is
 *        `ROOT\GFX\GRAF.DAT` and the member is `menu1h.pcx`.  The palette
 *        becomes the 6-bit form.
 *     1  a 63x1 image with one run of 0x3f bytes.
 *     2  a 1x1 image with the literal byte 0xbf.
 *     3  the open fails, so `file_error_report` runs and the decode continues
 *     D  the exit marker
 *
 * The input strings live in the data segment and the harness fills them with
 * single-character stores, because this harness does not load the constant
 * segment.  The markers go through the data buffer `text`, because the `print`
 * helper of this harness writes garbage for a string literal (the same fault
 * shows in the output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

int pcx_load(char *file, char *member, unsigned char *pixels,
             unsigned char *palette);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007edc[0x80];

static char text[8];
static unsigned char file_data[0x800];
static unsigned char pixels[0x200];
static unsigned char palette[0x300];
static char file_name[0x40];
static char member_name[0x40];
static char expected_path[0x80];
static int file_size;
static int position;
static int open_result;
static int open_calls;
static int close_calls;
static int error_calls;
static int seek_calls;
static int seek_offset[4];
static int seek_origin[4];
static char path_text[0x100];
static char member_text[0x80];
static char category_text[8];

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

static void fill(unsigned char *buffer, int size, int value)
{
    int index;

    for (index = 0; index < size; ++index) {
        buffer[index] = (unsigned char)value;
    }
}

static void set_short(unsigned char *buffer, int offset, int value)
{
    *(short *)(buffer + offset) = (short)value;
}

static void set_names(void)
{
    DAT_00007edc[0] = 'R';
    DAT_00007edc[1] = 'O';
    DAT_00007edc[2] = 'O';
    DAT_00007edc[3] = 'T';
    DAT_00007edc[4] = 0;

    file_name[0] = 'G';
    file_name[1] = 'F';
    file_name[2] = 'X';
    file_name[3] = '\\';
    file_name[4] = 'G';
    file_name[5] = 'R';
    file_name[6] = 'A';
    file_name[7] = 'F';
    file_name[8] = '.';
    file_name[9] = 'D';
    file_name[10] = 'A';
    file_name[11] = 'T';
    file_name[12] = 0;

    member_name[0] = 'm';
    member_name[1] = 'e';
    member_name[2] = 'n';
    member_name[3] = 'u';
    member_name[4] = '1';
    member_name[5] = 'h';
    member_name[6] = '.';
    member_name[7] = 'p';
    member_name[8] = 'c';
    member_name[9] = 'x';
    member_name[10] = 0;

    expected_path[0] = 'R';
    expected_path[1] = 'O';
    expected_path[2] = 'O';
    expected_path[3] = 'T';
    expected_path[4] = '\\';
    expected_path[5] = 'G';
    expected_path[6] = 'F';
    expected_path[7] = 'X';
    expected_path[8] = '\\';
    expected_path[9] = 'G';
    expected_path[10] = 'R';
    expected_path[11] = 'A';
    expected_path[12] = 'F';
    expected_path[13] = '.';
    expected_path[14] = 'D';
    expected_path[15] = 'A';
    expected_path[16] = 'T';
    expected_path[17] = 0;
}

static void set_palette(void)
{
    int base;
    int index;

    base = file_size - 0x300;
    for (index = 0; index < 0x300; ++index) {
        file_data[base + index] = (unsigned char)((index * 7 + 3) & 0xff);
    }
}

static void setup_file(int width, int height, int payload)
{
    fill(file_data, 0x800, 0);
    set_short(file_data, 0x04, 0);
    set_short(file_data, 0x06, 0);
    set_short(file_data, 0x08, width - 1);
    set_short(file_data, 0x0a, height - 1);
    file_size = 0x80 + payload + 0x300;
    set_palette();
}

static int palette_is_shifted(void)
{
    int index;

    for (index = 0; index < 0x300; ++index) {
        if (palette[index] != (unsigned char)(((index * 7 + 3) & 0xff) >> 2)) {
            return 0;
        }
    }
    return 1;
}

static int run_is(int first, int count, int value)
{
    int index;

    for (index = 0; index < count; ++index) {
        if (pixels[first + index] != value) {
            return 0;
        }
    }
    return 1;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *first;
    char *second;
    int index;

    (void)format;
    va_start(arguments, format);
    first = va_arg(arguments, char *);
    second = va_arg(arguments, char *);
    index = 0;
    while (*first) {
        buffer[index++] = *first++;
    }
    buffer[index++] = '\\';
    while (*second) {
        buffer[index++] = *second++;
    }
    buffer[index] = 0;
    va_end(arguments);
    return index;
}

int fopen(char *stream, char *path, char *name, char *category)
{
    (void)stream;
    copy(path_text, path);
    copy(member_text, name);
    copy(category_text, category);
    open_calls = open_calls + 1;
    return open_result;
}

int stream_seek(char *stream, int offset, int origin)
{
    (void)stream;
    seek_offset[seek_calls] = offset;
    seek_origin[seek_calls] = origin;
    seek_calls = seek_calls + 1;
    if (origin == 2) {
        position = file_size + offset;
    } else if (origin == 1) {
        position = position + offset;
    } else {
        position = offset;
    }
    return 0;
}

int stream_read(char *stream, void *buffer, int count)
{
    unsigned char *out;
    int index;

    (void)stream;
    out = buffer;
    for (index = 0; index < count && position < file_size; ++index) {
        out[index] = file_data[position++];
    }
    return index;
}

int stream_close(char *stream)
{
    (void)stream;
    close_calls = close_calls + 1;
    return 0;
}

void file_error_report(void *stream)
{
    (void)stream;
    error_calls = error_calls + 1;
}

static void reset(void)
{
    position = 0;
    open_result = 0;
    open_calls = 0;
    close_calls = 0;
    error_calls = 0;
    seek_calls = 0;
    seek_offset[0] = 0;
    seek_origin[0] = 0;
    path_text[0] = 0;
    member_text[0] = 0;
    category_text[0] = 0;
    fill(pixels, 0x200, 0xaa);
    fill(palette, 0x300, 0);
    set_names();
}

int main(void)
{
    int result;

    reset();
    setup_file(3, 2, 5);
    file_data[0x80] = 0xc3;
    file_data[0x81] = 5;
    file_data[0x82] = 1;
    file_data[0x83] = 0xc2;
    file_data[0x84] = 7;
    result = pcx_load(file_name, member_name, pixels, palette);
    put(result == 0
        && same(path_text, expected_path)
        && same(member_text, member_name)
        && open_calls == 1
        && close_calls == 1
        && error_calls == 0
        && seek_calls == 2
        && seek_offset[0] == -0x300 && seek_origin[0] == 2
        && seek_offset[1] == 0 && seek_origin[1] == 0
        && run_is(0, 3, 5) && pixels[3] == 1 && run_is(4, 2, 7)
        && pixels[6] == 0xaa && palette_is_shifted() ? '0' : 'x');

    reset();
    setup_file(63, 1, 2);
    file_data[0x80] = 0xff;
    file_data[0x81] = 9;
    result = pcx_load(file_name, member_name, pixels, palette);
    put(result == 0 && run_is(0, 63, 9) && pixels[63] == 0xaa
        && palette_is_shifted() ? '1' : 'x');

    reset();
    setup_file(1, 1, 1);
    file_data[0x80] = 0xbf;
    result = pcx_load(file_name, member_name, pixels, palette);
    put(result == 0 && pixels[0] == 0xbf && pixels[1] == 0xaa
        && palette_is_shifted() ? '2' : 'x');

    reset();
    setup_file(1, 1, 1);
    file_data[0x80] = 1;
    open_result = 1;
    result = pcx_load(file_name, member_name, pixels, palette);
    put(result == 0 && error_calls == 1 && open_calls == 1
        && close_calls == 1 && pixels[0] == 1 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
