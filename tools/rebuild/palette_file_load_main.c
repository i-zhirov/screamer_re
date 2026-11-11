/* The test harness for the rebuilt `palette_file_load` module.
 *
 * The harness holds the level name and the palette buffer, and a stub for every
 * call target.  The `sprintf` stub builds the member name from the base name and
 * records the format and the output buffer.  The other stubs record their
 * arguments and return the configured results.
 *
 * The harness runs three paths:
 *
 *     0  the base name `MAP1` and a successful member load: the member name
 *        `MAP1.COL`, the empty path, and the palette buffer
 *     1  the member load fails: the file error report runs with the same
 *        stream, and the exit runs with the code 0
 *     2  the empty base name: the member name `.COL`
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void palette_file_load(char *name, char *palette);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char name_text[0x40];
static char palette[0x300];
static char format_expected[0x10];
static char path_expected[0x10];
static char name_expected[0x40];

static char format_seen[0x10];
static char buffer_seen[0x40];
static char *buffer_ptr_seen;
static int format_calls;

static char *load_stream;
static char load_path[0x10];
static char *load_name_ptr;
static char load_name[0x40];
static char *load_palette;
static int load_result;
static int load_calls;

static char *report_stream;
static int report_calls;

static int exit_code;
static int exit_calls;

static void put(char marker)
{
    text[0] = marker;
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

static void fill(char *target, char a, char b, char c, char d,
                 char e, char f, char g, char h)
{
    target[0] = a;
    target[1] = b;
    target[2] = c;
    target[3] = d;
    target[4] = e;
    target[5] = f;
    target[6] = g;
    target[7] = h;
    target[8] = 0;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *value;

    va_start(arguments, format);
    value = va_arg(arguments, char *);
    va_end(arguments);
    copy(buffer, value);
    {
        int index;

        for (index = 0; buffer[index] != 0; ++index) {
        }
        buffer[index] = '.';
        buffer[index + 1] = 'C';
        buffer[index + 2] = 'O';
        buffer[index + 3] = 'L';
        buffer[index + 4] = 0;
    }
    copy(format_seen, (char *)format);
    copy(buffer_seen, buffer);
    buffer_ptr_seen = buffer;
    format_calls = format_calls + 1;
    return 0;
}

int member_load(char *stream, char *path, char *name, char *buffer)
{
    load_stream = stream;
    copy(load_path, path);
    load_name_ptr = name;
    copy(load_name, name);
    load_palette = buffer;
    load_calls = load_calls + 1;
    return load_result;
}

void file_error_report(char *stream)
{
    report_stream = stream;
    report_calls = report_calls + 1;
}

void exit(int status)
{
    exit_code = status;
    exit_calls = exit_calls + 1;
}

static void build_texts(void)
{
    fill(format_expected, '%', 's', '.', 'C', 'O', 'L', 0, 0);
    fill(path_expected, 0, 0, 0, 0, 0, 0, 0, 0);
    fill(name_text, 'M', 'A', 'P', '1', 0, 0, 0, 0);
}

static void reset(void)
{
    format_seen[0] = 0;
    buffer_seen[0] = 0;
    buffer_ptr_seen = 0;
    format_calls = 0;

    load_stream = 0;
    load_path[0] = 0;
    load_name_ptr = 0;
    load_name[0] = 0;
    load_palette = 0;
    load_calls = 0;

    report_stream = 0;
    report_calls = 0;

    exit_code = -1;
    exit_calls = 0;
}

static int full_path(void)
{
    return format_calls == 1
        && same(format_seen, format_expected)
        && same(buffer_seen, name_expected)
        && buffer_ptr_seen == load_name_ptr
        && load_calls == 1
        && same(load_path, path_expected)
        && same(load_name, name_expected)
        && load_stream != 0
        && load_palette == palette
        && report_calls == 0
        && exit_calls == 0;
}

static int failed_load(void)
{
    return format_calls == 1
        && load_calls == 1
        && same(load_name, name_expected)
        && load_palette == palette
        && report_calls == 1
        && report_stream == load_stream
        && exit_calls == 1
        && exit_code == 0;
}

static int empty_name(void)
{
    return format_calls == 1
        && load_calls == 1
        && same(load_name, name_expected)
        && load_palette == palette
        && report_calls == 0
        && exit_calls == 0;
}

int main(void)
{
    build_texts();

    fill(name_expected, 'M', 'A', 'P', '1', '.', 'C', 'O', 'L');
    reset();
    load_result = 0;
    palette_file_load(name_text, palette);
    put(full_path() ? '0' : 'x');

    fill(name_expected, 'M', 'A', 'P', '1', '.', 'C', 'O', 'L');
    reset();
    load_result = 1;
    palette_file_load(name_text, palette);
    put(failed_load() ? '1' : 'x');

    name_text[0] = 0;
    fill(name_expected, '.', 'C', 'O', 'L', 0, 0, 0, 0);
    reset();
    load_result = 0;
    palette_file_load(name_text, palette);
    put(empty_name() ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
