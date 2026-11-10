/* The test harness for the rebuilt `mmp_file_load` module.
 *
 * The harness holds the four module globals and a stub for every call target.
 * The `sprintf` stub builds the member name from the base name and records the
 * format and the output buffer.  The other stubs record their arguments and
 * return the configured results.
 *
 * The harness runs four paths:
 *
 *     0  the base name `MAP1` and a successful member load: the name
 *        `MAP1.MMP`, the empty path, the out cell, the resource head, the grid
 *        copy, and the relocate call
 *     1  the member load fails: the file error report runs with the same
 *        stream, the grid copy still runs, and the relocate still runs
 *     2  the base name `MENU`: the name `MENU.MMP`
 *     3  the empty base name: the name `.MMP`
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void mmp_file_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_002154c[0x40];
void *DAT_0021468;
void *DAT_0021574;
unsigned int DAT_00102fa4;

static char text[8];
static char base_text[0x40];
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
static void **load_out_ptr;
static void *load_out_value;
static void *load_resource;
static int load_result;
static int load_calls;

static char *report_stream;
static int report_calls;

static int relocate_calls;

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

static void append(char *destination, char a, char b, char c, char d)
{
    int index;

    for (index = 0; destination[index] != 0; ++index) {
    }
    destination[index] = a;
    destination[index + 1] = b;
    destination[index + 2] = c;
    destination[index + 3] = d;
    destination[index + 4] = 0;
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
    append(buffer, '.', 'M', 'M', 'P');
    if (format_calls == 0) {
        copy(format_seen, (char *)format);
        copy(buffer_seen, buffer);
        buffer_ptr_seen = buffer;
    }
    format_calls = format_calls + 1;
    return 0;
}

int alfile_load(char *stream, char *path, char *name, void **out,
                void *resource)
{
    load_stream = stream;
    copy(load_path, path);
    load_name_ptr = name;
    copy(load_name, name);
    load_out_ptr = out;
    load_out_value = *out;
    load_resource = resource;
    load_calls = load_calls + 1;
    *out = (void *)0x22220000;
    return load_result;
}

void file_error_report(char *stream)
{
    report_stream = stream;
    report_calls = report_calls + 1;
}

void spatial_grid_relocate(void)
{
    relocate_calls = relocate_calls + 1;
}

static void build_texts(void)
{
    fill(format_expected, '%', 's', '.', 'M', 'M', 'P', 0, 0);
    fill(path_expected, 0, 0, 0, 0, 0, 0, 0, 0);
    fill(name_expected, 'M', 'A', 'P', '1', '.', 'M', 'M', 'P');
    name_expected[8] = 0;
    fill(base_text, 'M', 'A', 'P', '1', 0, 0, 0, 0);
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x40; index++) {
        DAT_002154c[index] = 0;
    }
    copy(DAT_002154c, base_text);
    DAT_0021468 = (void *)0x11110000;
    DAT_0021574 = (void *)0x33330000;
    DAT_00102fa4 = 0x44440000;

    format_seen[0] = 0;
    buffer_seen[0] = 0;
    buffer_ptr_seen = 0;
    format_calls = 0;

    load_stream = 0;
    load_path[0] = 0;
    load_name_ptr = 0;
    load_name[0] = 0;
    load_out_ptr = 0;
    load_out_value = 0;
    load_resource = 0;
    load_result = 0;
    load_calls = 0;

    report_stream = 0;
    report_calls = 0;

    relocate_calls = 0;
}

static int common_ok(void)
{
    return format_calls == 1
        && same(format_seen, format_expected)
        && buffer_ptr_seen != 0
        && buffer_ptr_seen == load_name_ptr
        && load_calls == 1
        && load_stream != 0
        && same(load_path, path_expected)
        && load_out_ptr == &DAT_0021468
        && load_out_value == (void *)0x11110000
        && load_resource == DAT_0021574
        && DAT_0021468 == (void *)0x22220000
        && DAT_00102fa4 == (unsigned int)(void *)0x22220000
        && relocate_calls == 1;
}

int main(void)
{
    build_texts();

    reset();
    load_result = 0;
    mmp_file_load();
    put(common_ok()
        && same(buffer_seen, name_expected)
        && same(load_name, name_expected)
        && report_calls == 0
        ? '0' : 'x');

    reset();
    load_result = 1;
    mmp_file_load();
    put(common_ok()
        && same(load_name, name_expected)
        && report_calls == 1
        && report_stream == load_stream
        ? '1' : 'x');

    fill(base_text, 'M', 'E', 'N', 'U', 0, 0, 0, 0);
    fill(name_expected, 'M', 'E', 'N', 'U', '.', 'M', 'M', 'P');
    name_expected[8] = 0;
    reset();
    load_result = 0;
    mmp_file_load();
    put(common_ok()
        && same(buffer_seen, name_expected)
        && same(load_name, name_expected)
        && report_calls == 0
        ? '2' : 'x');

    fill(base_text, 0, 0, 0, 0, 0, 0, 0, 0);
    fill(name_expected, '.', 'M', 'M', 'P', 0, 0, 0, 0);
    reset();
    load_result = 0;
    mmp_file_load();
    put(common_ok()
        && same(buffer_seen, name_expected)
        && same(load_name, name_expected)
        && report_calls == 0
        ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
