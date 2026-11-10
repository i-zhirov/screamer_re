/* The test harness for the rebuilt `cockpit_load` module.
 *
 * The harness holds the base path, the resource handle, the resource buffer,
 * and the out cell.  The stubs record their arguments and their call counts.
 * The `sprintf` stub rebuilds the container path and the member name, so the
 * harness checks the two formats and the letter `H`.
 *
 * The harness runs five paths:
 *
 *     0  the normal path: the path `%s\gfx\game.dat`, the member
 *        `COCK00AH.pkc`, the open with `rb`, the size 0x1234, the `COCKPIT`
 *        resource, the read, and the close
 *     1  the failed open: the file error report runs and the function still
 *        reads and closes
 *     2  the zero resource: the memory error report runs with the resource
 *        handle and the out cell becomes zero
 *     3  the second base name `COCK02B`: the member and the argument change,
 *        and the letter stays `H`
 *     4  the zero size: the resource size and the read count are zero
 *     D  the exit marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, and the module build
 * defines `COCKPIT_LOAD_HARNESS` for the same reason.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void cockpit_load(char *name, int *out);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007f44[0x20];
void *DAT_000062ac;

static char text[8];
static char resource_buffer[0x100];
static int out_cell;

static char name_text[0x10];
static char expect_path_format[0x14];
static char expect_member_format[0x10];
static char expect_mode[0x10];
static char expect_resource[0x10];
static char path_tail[0x10];
static char member_tail[0x10];
static char expect_path[0x60];
static char expect_member[0x60];

static char format_copy[2][0x20];
static char result_copy[2][0x60];
static char *format_argument[2];
static int format_letter[2];
static int format_calls;

static void *open_stream;
static char open_path[0x60];
static char open_member[0x60];
static char open_category[0x10];
static int open_calls;
static int open_result;

static int file_report_calls;
static void *file_report_stream;

static int status_calls;
static void *status_stream;
static int status_result;

static int resource_calls;
static void *resource_resource;
static char resource_name[0x10];
static int resource_size;
static void *resource_result;

static int report_calls;
static void *report_resource;

static int read_calls;
static void *read_stream;
static char *read_buffer;
static int read_count;

static int close_calls;
static void *close_stream;

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

static void append(char *destination, char *source)
{
    int index;

    for (index = 0; destination[index] != 0; ++index) {
    }
    copy(destination + index, source);
}

static void append_char(char *destination, char value)
{
    int index;

    for (index = 0; destination[index] != 0; ++index) {
    }
    destination[index] = value;
    destination[index + 1] = 0;
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

static void set_name(char a, char b, char c, char d, char e, char f, char g)
{
    name_text[0] = a;
    name_text[1] = b;
    name_text[2] = c;
    name_text[3] = d;
    name_text[4] = e;
    name_text[5] = f;
    name_text[6] = g;
    name_text[7] = 0;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *argument;
    int letter;
    int index;

    index = format_calls;
    va_start(arguments, format);
    argument = va_arg(arguments, char *);
    letter = 0;
    if (same((char *)format, expect_path_format)) {
        copy(buffer, argument);
        append(buffer, path_tail);
    } else {
        letter = va_arg(arguments, int);
        copy(buffer, argument);
        append_char(buffer, (char)letter);
        append(buffer, member_tail);
    }
    va_end(arguments);
    if (index < 2) {
        copy(format_copy[index], (char *)format);
        copy(result_copy[index], buffer);
        format_argument[index] = argument;
        format_letter[index] = letter;
    }
    format_calls = index + 1;
    return 0;
}

int fopen(char *stream, char *path, char *name, char *category)
{
    open_stream = stream;
    copy(open_path, path);
    copy(open_member, name);
    copy(open_category, category);
    open_calls = open_calls + 1;
    return open_result;
}

void file_error_report(char *stream)
{
    file_report_stream = stream;
    file_report_calls = file_report_calls + 1;
}

int stream_status(char *stream)
{
    status_stream = stream;
    status_calls = status_calls + 1;
    return status_result;
}

void *resource_get(void *resource, char *name, int size)
{
    resource_resource = resource;
    copy(resource_name, name);
    resource_size = size;
    resource_calls = resource_calls + 1;
    return resource_result;
}

void memory_error_report(void *resource)
{
    report_resource = resource;
    report_calls = report_calls + 1;
}

int stream_read(char *stream, char *buffer, int count)
{
    read_stream = stream;
    read_buffer = buffer;
    read_count = count;
    read_calls = read_calls + 1;
    return count;
}

int stream_close(char *stream)
{
    close_stream = stream;
    close_calls = close_calls + 1;
    return 0;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x100; ++index) {
        resource_buffer[index] = 0xaa;
    }
    out_cell = 0;
    format_calls = 0;
    for (index = 0; index < 2; ++index) {
        format_copy[index][0] = 0;
        result_copy[index][0] = 0;
        format_argument[index] = 0;
        format_letter[index] = 0;
    }
    open_stream = 0;
    open_path[0] = 0;
    open_member[0] = 0;
    open_category[0] = 0;
    open_calls = 0;
    open_result = 0;
    file_report_calls = 0;
    file_report_stream = 0;
    status_calls = 0;
    status_stream = 0;
    status_result = 0;
    resource_calls = 0;
    resource_resource = 0;
    resource_name[0] = 0;
    resource_size = 0;
    resource_result = 0;
    report_calls = 0;
    report_resource = 0;
    read_calls = 0;
    read_stream = 0;
    read_buffer = 0;
    read_count = 0;
    close_calls = 0;
    close_stream = 0;
    fill(DAT_00007f44, 'D', ':', '\\', 'S', 'C', 'R', 'E', 'A');
    fill(DAT_00007f44 + 8, 'M', 'E', 'R', 0, 0, 0, 0, 0);
    DAT_000062ac = (void *)0x5555;
    set_name('C', 'O', 'C', 'K', '0', '0', 'A');
    fill(expect_path_format, '%', 's', '\\', 'g', 'f', 'x', '\\', 'g');
    fill(expect_path_format + 8, 'a', 'm', 'e', '.', 'd', 'a', 't', 0);
    fill(expect_member_format, '%', 's', '%', 'c', '.', 'p', 'k', 'c');
    fill(expect_mode, 'r', 'b', 0, 0, 0, 0, 0, 0);
    fill(expect_resource, 'C', 'O', 'C', 'K', 'P', 'I', 'T', 0);
    fill(path_tail, '\\', 'g', 'f', 'x', '\\', 'g', 'a', 'm');
    fill(path_tail + 8, 'e', '.', 'd', 'a', 't', 0, 0, 0);
    fill(member_tail, '.', 'p', 'k', 'c', 0, 0, 0, 0);
    copy(expect_path, DAT_00007f44);
    append(expect_path, path_tail);
    copy(expect_member, name_text);
    append_char(expect_member, 'H');
    append(expect_member, member_tail);
}

static int normal(void)
{
    return format_calls == 2
        && same(format_copy[0], expect_path_format)
        && same(format_copy[1], expect_member_format)
        && format_argument[0] == DAT_00007f44
        && format_argument[1] == name_text
        && format_letter[1] == 'H'
        && same(result_copy[0], expect_path)
        && same(result_copy[1], expect_member)
        && open_calls == 1
        && open_stream != 0
        && same(open_path, expect_path)
        && same(open_member, expect_member)
        && same(open_category, expect_mode)
        && file_report_calls == 0
        && status_calls == 1
        && status_stream != 0
        && resource_calls == 1
        && resource_resource == DAT_000062ac
        && same(resource_name, expect_resource)
        && resource_size == 0x1234
        && out_cell == (int)resource_buffer
        && report_calls == 0
        && read_calls == 1
        && read_stream != 0
        && read_buffer == resource_buffer
        && read_count == 0x1234
        && close_calls == 1
        && close_stream != 0;
}

static int failed_open(void)
{
    return open_calls == 1
        && file_report_calls == 1
        && file_report_stream != 0
        && status_calls == 1
        && resource_calls == 1
        && resource_size == 0x1234
        && out_cell == (int)resource_buffer
        && report_calls == 0
        && read_calls == 1
        && read_count == 0x1234
        && close_calls == 1;
}

static int zero_resource(void)
{
    return open_calls == 1
        && file_report_calls == 0
        && status_calls == 1
        && resource_calls == 1
        && resource_resource == DAT_000062ac
        && same(resource_name, expect_resource)
        && resource_size == 0x1234
        && report_calls == 1
        && report_resource == DAT_000062ac
        && out_cell == 0
        && read_calls == 1
        && read_buffer == 0
        && read_count == 0x1234
        && close_calls == 1;
}

static int second_name(void)
{
    char expected[0x10];

    copy(expected, name_text);
    append_char(expected, 'H');
    append(expected, member_tail);
    return format_calls == 2
        && format_argument[1] == name_text
        && format_letter[1] == 'H'
        && same(result_copy[1], expected)
        && same(open_member, expected)
        && resource_size == 0x80
        && out_cell == (int)resource_buffer
        && close_calls == 1;
}

static int zero_size(void)
{
    return status_calls == 1
        && resource_calls == 1
        && resource_size == 0
        && out_cell == (int)resource_buffer
        && report_calls == 0
        && read_calls == 1
        && read_buffer == resource_buffer
        && read_count == 0
        && close_calls == 1;
}

int main(void)
{
    reset();
    resource_result = resource_buffer;
    status_result = 0x1234;
    cockpit_load(name_text, &out_cell);
    put(normal() ? '0' : 'x');

    reset();
    resource_result = resource_buffer;
    status_result = 0x1234;
    open_result = -1;
    cockpit_load(name_text, &out_cell);
    put(failed_open() ? '1' : 'x');

    reset();
    resource_result = 0;
    status_result = 0x1234;
    cockpit_load(name_text, &out_cell);
    put(zero_resource() ? '2' : 'x');

    reset();
    set_name('C', 'O', 'C', 'K', '0', '2', 'B');
    resource_result = resource_buffer;
    status_result = 0x80;
    cockpit_load(name_text, &out_cell);
    put(second_name() ? '3' : 'x');

    reset();
    resource_result = resource_buffer;
    status_result = 0;
    cockpit_load(name_text, &out_cell);
    put(zero_size() ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
