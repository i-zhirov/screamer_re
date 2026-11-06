/* The test harness for the rebuilt `member_list_load` module.
 *
 * The harness holds the base path, the resource handle, the resource buffer,
 * the simulated member text, and the pointer array.  The stubs record their
 * arguments and their call counts.  The stream stubs read a small script twice,
 * so the harness checks the size pass, the rewind, and the copy pass.
 *
 * The harness runs five paths:
 *
 *     0  the normal path: the two formatted names, the open, the size pass
 *        with an ignored `*` line, the allocation, the rewind, the copy pass
 *        with a copied `#` line, the four pointers, the -1 sentinel, and the
 *        close
 *     1  the failed open: the file error report runs and the empty member
 *        still gives the -1 sentinel and the close
 *     2  the zero resource: the memory error report runs with the empty member
 *        and the function returns zero
 *     3  the single copied line with the `game` name and an ignored `*` line
 *     4  an empty member: the resource size is zero and the array holds only
 *        the -1 sentinel
 *     D  the exit marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, and the module build
 * defines `MEMBER_LIST_LOAD_HARNESS` for the same reason.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void *member_list_load(char *name, int *array);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007f44[0x20];
void *DAT_000062ac;

static char text[8];
static int array[8];
static char resource_buffer[0x100];
static void *resource_result;

static char script_data[12 * 0x20];
static int script_offset[12];
static int script_count;
static int script_index;
static int read_calls;
static int read_mode;
static char read_line[0x20];

static char format_copy[4][0x20];
static char result_copy[4][0x40];
static char *format_argument[4];
static int format_calls;

static void *open_stream;
static char open_path[0x40];
static char open_member[0x20];
static char open_category[0x10];
static int open_calls;
static int open_result;

static int file_report_calls;
static void *file_report_stream;

static int seek_calls;
static int seek_offset;
static int seek_origin;

static int resource_calls;
static void *resource_resource;
static char resource_name[0x10];
static int resource_size;

static int report_calls;
static void *report_resource;

static int close_calls;
static void *close_stream;

static char name_text[0x10];
static char expect_path_format[0x14];
static char expect_member_format[0x10];
static char expect_list_name[0x10];
static char expect_mode[0x10];
static char path_mid[0x10];
static char path_tail[0x10];
static char member_tail[0x10];
static char expect_path[0x40];
static char expect_member[0x20];
static char expect_red[0x10];
static char expect_green[0x10];
static char expect_hash[0x10];
static char expect_blue[0x10];
static char expect_last[0x10];
static char expect_star[0x10];
static char expect_skip[0x10];

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

static void set_script_line(int index, char *line)
{
    copy(script_data + index * 0x20, line);
    script_offset[index] = index * 0x20;
    script_count = index + 1;
}

static void set_name(char a, char b, char c, char d)
{
    fill(name_text, a, b, c, d, 0, 0, 0, 0);
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *argument;
    int index;

    index = format_calls;
    va_start(arguments, format);
    argument = va_arg(arguments, char *);
    va_end(arguments);
    if (same((char *)format, expect_path_format)) {
        copy(buffer, argument);
        append(buffer, path_mid);
        append(buffer, path_tail);
    } else {
        copy(buffer, argument);
        append(buffer, member_tail);
    }
    if (index < 4) {
        copy(format_copy[index], (char *)format);
        copy(result_copy[index], buffer);
        format_argument[index] = argument;
    }
    format_calls = index + 1;
    return 0;
}

int fopen(void *stream, char *path, char *member, char *category)
{
    open_stream = stream;
    copy(open_path, path);
    copy(open_member, member);
    copy(open_category, category);
    open_calls = open_calls + 1;
    return open_result;
}

void file_error_report(void *stream)
{
    file_report_stream = stream;
    file_report_calls = file_report_calls + 1;
}

int stream_read_text(void *stream, char *buffer, int mode)
{
    read_calls = read_calls + 1;
    read_mode = mode;
    if (script_index >= script_count) {
        return -1;
    }
    copy(buffer, script_data + script_offset[script_index]);
    copy(read_line, buffer);
    script_index = script_index + 1;
    return 0;
}

int stream_seek(void *stream, int offset, int origin)
{
    seek_offset = offset;
    seek_origin = origin;
    seek_calls = seek_calls + 1;
    if (origin == 0) {
        script_index = 0;
    }
    return 0;
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

int stream_close(void *stream)
{
    close_stream = stream;
    close_calls = close_calls + 1;
    return 0;
}

char *strcpy(char *destination, char *source)
{
    copy(destination, source);
    return destination;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 8; ++index) {
        array[index] = 0;
    }
    for (index = 0; index < 0x100; ++index) {
        resource_buffer[index] = 0xaa;
    }
    for (index = 0; index < 12; ++index) {
        script_offset[index] = 0;
        script_data[index * 0x20] = 0;
    }
    script_count = 0;
    script_index = 0;
    read_calls = 0;
    read_mode = 0;
    read_line[0] = 0;
    format_calls = 0;
    for (index = 0; index < 4; ++index) {
        format_copy[index][0] = 0;
        result_copy[index][0] = 0;
        format_argument[index] = 0;
    }
    open_stream = 0;
    open_path[0] = 0;
    open_member[0] = 0;
    open_category[0] = 0;
    open_calls = 0;
    open_result = 0;
    file_report_calls = 0;
    file_report_stream = 0;
    seek_calls = 0;
    seek_offset = 0;
    seek_origin = 0;
    resource_calls = 0;
    resource_resource = 0;
    resource_name[0] = 0;
    resource_size = 0;
    resource_result = 0;
    report_calls = 0;
    report_resource = 0;
    close_calls = 0;
    close_stream = 0;
    fill(DAT_00007f44, 'D', ':', '\\', 'S', 'C', 'R', 'E', 'A');
    fill(DAT_00007f44 + 8, 'M', 'E', 'R', 0, 0, 0, 0, 0);
    DAT_000062ac = (void *)0x5555;
    set_name('m', 'e', 'n', 'u');
    fill(expect_path_format, '%', 's', '\\', 'd', 'a', 't', 'a', '\\');
    fill(expect_path_format + 8, 'l', 'o', 'c', 'a', 'l', '.', 'd', 'a');
    expect_path_format[16] = 't';
    expect_path_format[17] = 0;
    fill(expect_member_format, '%', 's', '.', 't', 'x', 't', 0, 0);
    fill(expect_list_name, 'T', 'E', 'X', 'T', 0, 0, 0, 0);
    fill(expect_mode, 'r', 't', 0, 0, 0, 0, 0, 0);
    fill(path_mid, '\\', 'd', 'a', 't', 'a', '\\', 0, 0);
    fill(path_tail, 'l', 'o', 'c', 'a', 'l', '.', 'd', 'a');
    path_tail[8] = 't';
    path_tail[9] = 0;
    fill(member_tail, '.', 't', 'x', 't', 0, 0, 0, 0);
    copy(expect_path, DAT_00007f44);
    append(expect_path, path_mid);
    append(expect_path, path_tail);
    copy(expect_member, name_text);
    append(expect_member, member_tail);
    fill(expect_red, 'r', 'e', 'd', 0, 0, 0, 0, 0);
    fill(expect_green, 'g', 'r', 'e', 'e', 'n', 0, 0, 0);
    fill(expect_hash, '#', ' ', 'c', 'o', 'u', 'n', 't', 0);
    fill(expect_blue, 'b', 'l', 'u', 'e', 0, 0, 0, 0);
    fill(expect_last, 'L', 'A', 'S', 'T', 0, 0, 0, 0);
    fill(expect_star, '*', ' ', 'n', 'o', 't', 'e', 0, 0);
    fill(expect_skip, '*', ' ', 's', 'k', 'i', 'p', 0, 0);
}

static int normal(void *result)
{
    return format_calls == 2
        && same(format_copy[0], expect_path_format)
        && same(format_copy[1], expect_member_format)
        && format_argument[0] == DAT_00007f44
        && format_argument[1] == name_text
        && same(result_copy[0], expect_path)
        && same(result_copy[1], expect_member)
        && open_calls == 1
        && open_stream != 0
        && same(open_path, expect_path)
        && same(open_member, expect_member)
        && same(open_category, expect_mode)
        && file_report_calls == 0
        && read_calls == 12
        && read_mode == 1
        && seek_calls == 1
        && seek_offset == 0
        && seek_origin == 0
        && resource_calls == 1
        && resource_resource == DAT_000062ac
        && same(resource_name, expect_list_name)
        && resource_size == 0x17
        && close_calls == 1
        && close_stream != 0
        && result == resource_buffer
        && array[0] == (int)resource_buffer
        && array[1] == (int)(resource_buffer + 4)
        && array[2] == (int)(resource_buffer + 10)
        && array[3] == (int)(resource_buffer + 18)
        && array[4] == -1
        && same(resource_buffer, expect_red)
        && same(resource_buffer + 4, expect_green)
        && same(resource_buffer + 10, expect_hash)
        && same(resource_buffer + 18, expect_blue);
}

static int failed_open(void *result)
{
    return open_calls == 1
        && file_report_calls == 1
        && file_report_stream != 0
        && read_calls == 2
        && seek_calls == 1
        && resource_calls == 1
        && resource_size == 0
        && report_calls == 0
        && close_calls == 1
        && result == resource_buffer
        && array[0] == -1;
}

static int zero_resource(void *result)
{
    return open_calls == 1
        && file_report_calls == 0
        && read_calls == 2
        && seek_calls == 1
        && resource_calls == 1
        && resource_resource == DAT_000062ac
        && same(resource_name, expect_list_name)
        && resource_size == 0
        && report_calls == 1
        && report_resource == DAT_000062ac
        && close_calls == 1
        && result == 0
        && array[0] == -1;
}

static int single(void *result)
{
    char expected[0x20];

    copy(expected, name_text);
    append(expected, member_tail);
    return format_calls == 2
        && same(result_copy[0], expect_path)
        && same(result_copy[1], expected)
        && same(open_member, expected)
        && read_calls == 6
        && seek_calls == 1
        && resource_calls == 1
        && resource_size == 5
        && report_calls == 0
        && close_calls == 1
        && result == resource_buffer
        && array[0] == (int)resource_buffer
        && array[1] == -1
        && same(resource_buffer, expect_last);
}

static int empty(void *result)
{
    return read_calls == 2
        && seek_calls == 1
        && resource_calls == 1
        && resource_size == 0
        && report_calls == 0
        && close_calls == 1
        && result == resource_buffer
        && array[0] == -1;
}

int main(void)
{
    void *result;

    reset();
    set_script_line(0, expect_red);
    set_script_line(1, expect_star);
    set_script_line(2, expect_green);
    set_script_line(3, expect_hash);
    set_script_line(4, expect_blue);
    resource_result = resource_buffer;
    result = member_list_load(name_text, array);
    put(normal(result) ? '0' : 'x');

    reset();
    resource_result = resource_buffer;
    open_result = -1;
    result = member_list_load(name_text, array);
    put(failed_open(result) ? '1' : 'x');

    reset();
    resource_result = 0;
    result = member_list_load(name_text, array);
    put(zero_resource(result) ? '2' : 'x');

    reset();
    set_name('g', 'a', 'm', 'e');
    resource_result = resource_buffer;
    set_script_line(0, expect_skip);
    set_script_line(1, expect_last);
    result = member_list_load(name_text, array);
    put(single(result) ? '3' : 'x');

    reset();
    resource_result = resource_buffer;
    result = member_list_load(name_text, array);
    put(empty(result) ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
