/* The test harness for the rebuilt `material_load` module.
 *
 * The harness holds the three module globals, the name argument, and a stub for
 * every call target.  The `sprintf` stub builds the member name from the name
 * argument and the call order.  The other stubs record their arguments and
 * return the configured results.
 *
 * The harness runs four paths:
 *
 *     0  the full path with the decoded size 0x2000 and the stream size 0x40:
 *        the two member names, the size mode, the open, the material resource,
 *        the member read, the close, and the decode mode 3 that starts after
 *        the member data
 *     1  the decoder returns a negative size: the size becomes 0x10000
 *     2  the open fails: the file error report and the exit run with the code
 *        0, and the stub return lets the path continue
 *     3  the resource request returns zero: the memory error report runs, and
 *        the decoded buffer starts at the stream size
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void material_load(char *name);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char *DAT_000214e8;
void *DAT_0021544;
void *DAT_0021574;

static char text[8];
static char name_text[8];
static char bck_format_expected[0x10];
static char mlb_format_expected[0x10];
static char bck_name_expected[0x10];
static char mlb_name_expected[0x10];
static char mode_expected[0x10];
static char path_expected[0x10];
static char resource_expected[0x10];
static char material_buffer[0x100];
static char script_data[0x40];

static char format_text[2][0x10];
static char format_buffer[2][0x10];
static char *format_buffer_ptr[2];
static int format_calls;

static char pcx_path[2][0x10];
static char pcx_name[2][0x10];
static int *pcx_out_ptr[2];
static int pcx_out_value[2];
static char *pcx_palette[2];
static int pcx_mode[2];
static int pcx_calls;
static int script_size;

static char *open_stream;
static char open_path[0x10];
static char open_name[0x10];
static char open_category[8];
static int open_result;
static int open_calls;

static char *file_report_stream;
static int file_report_calls;

static int exit_code;
static int exit_calls;

static char *status_stream;
static int status_calls;
static int script_status;

static void *resource_resource[2];
static char resource_name[2][0x10];
static int resource_size[2];
static int resource_calls;
static void *resource_result;

static void *memory_report_resource;
static int memory_report_calls;

static char *read_stream;
static char *read_buffer[2];
static int read_count[2];
static int read_calls;

static char *close_stream;
static int close_calls;

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

static void fill(char *text_text, char a, char b, char c, char d, char e,
                 char f, char g, char h, char i)
{
    text_text[0] = a;
    text_text[1] = b;
    text_text[2] = c;
    text_text[3] = d;
    text_text[4] = e;
    text_text[5] = f;
    text_text[6] = g;
    text_text[7] = h;
    text_text[8] = i;
    text_text[9] = 0;
}

static void build_texts(void)
{
    fill(name_text, 'M', 'A', 'P', '1', 0, 0, 0, 0, 0);
    fill(bck_format_expected, '%', 's', '.', 'B', 'C', 'K', 0, 0, 0);
    fill(mlb_format_expected, '%', 's', '.', 'M', 'L', 'B', 0, 0, 0);
    fill(bck_name_expected, 'M', 'A', 'P', '1', '.', 'B', 'C', 'K', 0);
    fill(mlb_name_expected, 'M', 'A', 'P', '1', '.', 'M', 'L', 'B', 0);
    fill(mode_expected, 'r', 'b', 0, 0, 0, 0, 0, 0, 0);
    fill(path_expected, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    fill(resource_expected, 'M', 'A', 'T', 'E', 'R', 'I', 'A', 'L', 'S');
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *value;
    int call;

    call = format_calls;
    va_start(arguments, format);
    value = va_arg(arguments, char *);
    va_end(arguments);
    copy(buffer, value);
    if (call == 0) {
        append(buffer, '.', 'B', 'C', 'K');
    } else {
        append(buffer, '.', 'M', 'L', 'B');
    }
    if (call < 2) {
        copy(format_text[call], (char *)format);
        copy(format_buffer[call], buffer);
        format_buffer_ptr[call] = buffer;
    }
    format_calls = format_calls + 1;
    return 0;
}

int pcx_decode(char *path, char *name, int *out, char *palette, int mode)
{
    int call;

    call = pcx_calls;
    if (call < 2) {
        copy(pcx_path[call], path);
        copy(pcx_name[call], name);
        pcx_out_ptr[call] = out;
        pcx_out_value[call] = *out;
        pcx_palette[call] = palette;
        pcx_mode[call] = mode;
    }
    pcx_calls = pcx_calls + 1;
    if (mode == 2) {
        return script_size;
    }
    return 0;
}

int fopen(char *stream, char *path, char *name, char *category)
{
    open_stream = stream;
    copy(open_path, path);
    copy(open_name, name);
    copy(open_category, category);
    open_calls = open_calls + 1;
    return open_result;
}

void file_error_report(char *stream)
{
    file_report_stream = stream;
    file_report_calls = file_report_calls + 1;
}

void exit(int status)
{
    exit_code = status;
    exit_calls = exit_calls + 1;
}

int stream_status(char *stream)
{
    status_stream = stream;
    status_calls = status_calls + 1;
    return script_status;
}

void *resource_get(void *resource, char *name, int size)
{
    int call;

    call = resource_calls;
    if (call < 2) {
        resource_resource[call] = resource;
        copy(resource_name[call], name);
        resource_size[call] = size;
    }
    resource_calls = resource_calls + 1;
    return resource_result;
}

void memory_error_report(void *resource)
{
    memory_report_resource = resource;
    memory_report_calls = memory_report_calls + 1;
}

int stream_read(char *stream, char *buffer, int count)
{
    int index;

    read_stream = stream;
    if (read_calls < 2) {
        read_buffer[read_calls] = buffer;
        read_count[read_calls] = count;
    }
    read_calls = read_calls + 1;
    if (buffer != 0) {
        for (index = 0; index < count; index++) {
            buffer[index] = script_data[index];
        }
    }
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

    for (index = 0; index < 0x100; index++) {
        material_buffer[index] = 0;
    }
    for (index = 0; index < 0x40; index++) {
        script_data[index] = 0;
    }
    for (index = 0; index < 2; index++) {
        format_text[index][0] = 0;
        format_buffer[index][0] = 0;
        format_buffer_ptr[index] = 0;
        pcx_path[index][0] = 0;
        pcx_name[index][0] = 0;
        pcx_out_ptr[index] = 0;
        pcx_out_value[index] = 0;
        pcx_palette[index] = 0;
        pcx_mode[index] = 0;
        resource_resource[index] = 0;
        resource_name[index][0] = 0;
        resource_size[index] = 0;
        read_buffer[index] = 0;
        read_count[index] = 0;
    }
    name_text[0] = 'M';
    name_text[1] = 'A';
    name_text[2] = 'P';
    name_text[3] = '1';
    name_text[4] = 0;

    DAT_000214e8 = 0;
    DAT_0021544 = 0;
    DAT_0021574 = (void *)0x11110000;

    format_calls = 0;
    pcx_calls = 0;
    script_size = 0;

    open_stream = 0;
    open_path[0] = 0;
    open_name[0] = 0;
    open_category[0] = 0;
    open_result = 0;
    open_calls = 0;

    file_report_stream = 0;
    file_report_calls = 0;
    exit_code = -1;
    exit_calls = 0;

    status_stream = 0;
    status_calls = 0;
    script_status = 0;

    resource_calls = 0;
    resource_result = 0;

    memory_report_resource = 0;
    memory_report_calls = 0;

    read_stream = 0;
    read_calls = 0;

    close_stream = 0;
    close_calls = 0;
}

int main(void)
{
    build_texts();

    reset();
    script_size = 0x2000;
    script_status = 0x40;
    resource_result = material_buffer;
    material_load(name_text);
    put(format_calls == 2
        && same(format_text[0], bck_format_expected)
        && same(format_text[1], mlb_format_expected)
        && same(format_buffer[0], bck_name_expected)
        && same(format_buffer[1], mlb_name_expected)
        && format_buffer_ptr[0] != format_buffer_ptr[1]
        && pcx_calls == 2
        && pcx_mode[0] == 2 && pcx_mode[1] == 3
        && same(pcx_path[0], path_expected) && same(pcx_path[1], path_expected)
        && same(pcx_name[0], bck_name_expected) && same(pcx_name[1], bck_name_expected)
        && pcx_out_ptr[0] == (int *)&DAT_000214e8
        && pcx_out_ptr[1] == (int *)&DAT_000214e8
        && pcx_palette[0] == pcx_palette[1]
        && pcx_out_value[1] == (int)(material_buffer + 0x40)
        && open_calls == 1 && open_stream != 0
        && same(open_path, path_expected)
        && same(open_name, mlb_name_expected)
        && same(open_category, mode_expected)
        && file_report_calls == 0 && exit_calls == 0
        && status_calls == 1 && status_stream == open_stream
        && resource_calls == 1
        && resource_resource[0] == DAT_0021574
        && same(resource_name[0], resource_expected)
        && resource_size[0] == 0x40 + 0x2000
        && DAT_0021544 == (void *)material_buffer
        && read_calls == 1 && read_stream == open_stream
        && read_buffer[0] == material_buffer && read_count[0] == 0x40
        && DAT_000214e8 == (unsigned char *)(material_buffer + 0x40)
        && memory_report_calls == 0
        && close_calls == 1 && close_stream == open_stream
        ? '0' : 'x');

    reset();
    script_size = -2;
    script_status = 0x10;
    resource_result = material_buffer;
    material_load(name_text);
    put(pcx_calls == 2
        && resource_size[0] == 0x10 + 0x10000
        && DAT_0021544 == (void *)material_buffer
        && DAT_000214e8 == (unsigned char *)(material_buffer + 0x10)
        && pcx_out_value[1] == (int)(material_buffer + 0x10)
        && memory_report_calls == 0 && close_calls == 1
        ? '1' : 'x');

    reset();
    open_result = 1;
    resource_result = material_buffer;
    material_load(name_text);
    put(open_calls == 1 && same(open_name, mlb_name_expected)
        && file_report_calls == 1 && file_report_stream == open_stream
        && exit_calls == 1 && exit_code == 0
        && status_calls == 1
        && resource_calls == 1 && DAT_0021544 == (void *)material_buffer
        && read_calls == 1 && close_calls == 1
        && pcx_calls == 2
        && DAT_000214e8 == (unsigned char *)material_buffer
        ? '2' : 'x');

    reset();
    script_size = 0x200;
    script_status = 0x20;
    resource_result = 0;
    material_load(name_text);
    put(pcx_calls == 2
        && resource_calls == 1
        && resource_size[0] == 0x20 + 0x200
        && memory_report_calls == 1
        && memory_report_resource == DAT_0021574
        && read_calls == 1 && read_buffer[0] == 0 && read_count[0] == 0x20
        && DAT_0021544 == 0
        && close_calls == 1
        && DAT_000214e8 == (unsigned char *)0x20
        && pcx_out_value[1] == 0x20
        ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
