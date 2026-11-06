/* The test harness for the rebuilt `list_file_load` module.
 *
 * The harness holds the base path, the table, the text script, and the stubs.
 * The stubs record their arguments and their call counts.  The harness runs
 * three paths:
 *
 *     0  the normal path: the two formatted names, the open, the line search,
 *        the `*` skip, the `d`, `f`, `x`, and `s` fields, the ignored `z`
 *        token, two full records, and the close
 *     1  the failed open: the file error report, the exit 1, the empty read,
 *        and the close
 *     D  the exit marker
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

int list_file_load(char *type, char *name, char *list, void *table,
                   int stride, int count);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007edc[0x20];

static char text[8];
static char table[0x100];
static char script_data[14 * 0x10];
static int script_offset[20];
static int script_count;
static int script_index;
static int read_calls;
static int stricmp_calls;

static char type_text[0x10];
static char name_text[0x10];
static char list_text[0x10];
static char expect_path_format[0x14];
static char expect_member_format[0x10];
static char expect_mode[0x10];
static char expect_hex[0x10];
static char expect_path[0x20];
static char expect_member[0x18];
static char expect_name[0x10];
static char expect_atoi[0x10];
static char expect_atof[0x10];
static char expect_sscanf[0x10];
static char piece_data[0x10];
static char piece_dat[0x10];
static char piece_dtr[0x10];

static char format_copy[2][0x20];
static char *format_buffer[2];
static char *format_text[2];
static char *format_base;
static char *format_type;
static char *format_name;
static int format_calls;

static void *open_stream;
static char open_path_text[0x40];
static char open_member_text[0x20];
static char open_category_text[0x10];
static char *open_path;
static char *open_member;
static char *open_category;
static int open_calls;
static int open_result;
static int report_calls;
static void *report_stream;
static int exit_calls;
static int exit_code;
static int close_calls;
static void *close_stream;

static char atoi_value[0x20];
static char atof_value[0x20];
static char sscanf_value[0x20];
static char *atoi_text;
static int atoi_calls;
static char *atof_text;
static int atof_calls;
static char *sscanf_text;
static char *sscanf_format;
static int sscanf_calls;

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

static unsigned int bits(float value)
{
    return *(unsigned int *)&value;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *first;
    char *second;

    va_start(arguments, format);
    if (format_calls == 0) {
        first = va_arg(arguments, char *);
        second = va_arg(arguments, char *);
        va_end(arguments);
        format_base = first;
        format_type = second;
        copy(buffer, first);
        append(buffer, piece_data);
        append(buffer, second);
        append(buffer, piece_dat);
    } else {
        first = va_arg(arguments, char *);
        va_end(arguments);
        format_name = first;
        copy(buffer, first);
        append(buffer, piece_dtr);
    }
    if (format_calls < 2) {
        copy(format_copy[format_calls], (char *)format);
        format_buffer[format_calls] = buffer;
        format_text[format_calls] = format_copy[format_calls];
    }
    format_calls = format_calls + 1;
    return 0;
}

int fopen(void *stream, char *path, char *member, char *category)
{
    open_stream = stream;
    copy(open_path_text, path);
    copy(open_member_text, member);
    copy(open_category_text, category);
    open_path = open_path_text;
    open_member = open_member_text;
    open_category = open_category_text;
    open_calls = open_calls + 1;
    return open_result;
}

void file_error_report(void *stream)
{
    report_stream = stream;
    report_calls = report_calls + 1;
}

void exit(int code)
{
    exit_code = code;
    exit_calls = exit_calls + 1;
}

int stream_read_text(void *stream, char *buffer, int mode)
{
    if (script_index >= script_count) {
        read_calls = read_calls + 1;
        return -1;
    }
    read_calls = read_calls + 1;
    copy(buffer, script_data + script_offset[script_index]);
    script_index = script_index + 1;
    return 0;
}

int stricmp(const char *left, const char *right)
{
    int index;

    stricmp_calls = stricmp_calls + 1;
    for (index = 0; left[index] != 0 || right[index] != 0; ++index) {
        char a = left[index];
        char b = right[index];

        if (a >= 'A' && a <= 'Z') {
            a = (char)(a + 0x20);
        }
        if (b >= 'A' && b <= 'Z') {
            b = (char)(b + 0x20);
        }
        if (a != b) {
            return a - b;
        }
    }
    return 0;
}

int sscanf(char *source, char *format, ...)
{
    va_list arguments;
    int *target;
    int value;
    int index;

    va_start(arguments, format);
    target = va_arg(arguments, int *);
    va_end(arguments);
    copy(sscanf_value, source);
    sscanf_text = sscanf_value;
    sscanf_format = format;
    sscanf_calls = sscanf_calls + 1;
    value = 0;
    for (index = 0; source[index] != 0; ++index) {
        int digit;

        if (source[index] >= '0' && source[index] <= '9') {
            digit = source[index] - '0';
        } else if (source[index] >= 'a' && source[index] <= 'f') {
            digit = source[index] - 'a' + 10;
        } else if (source[index] >= 'A' && source[index] <= 'F') {
            digit = source[index] - 'A' + 10;
        } else {
            break;
        }
        value = value * 16 + digit;
    }
    *target = value;
    return 1;
}

int atoi(char *source)
{
    int index;
    int sign;
    int value;

    copy(atoi_value, source);
    atoi_text = atoi_value;
    atoi_calls = atoi_calls + 1;
    index = 0;
    sign = 1;
    if (source[index] == '-') {
        sign = -1;
        index = index + 1;
    }
    value = 0;
    while (source[index] >= '0' && source[index] <= '9') {
        value = value * 10 + source[index] - '0';
        index = index + 1;
    }
    return sign * value;
}

double atof(char *source)
{
    int index;
    int sign;
    int whole;
    int fraction;
    int divisor;

    copy(atof_value, source);
    atof_text = atof_value;
    atof_calls = atof_calls + 1;
    index = 0;
    sign = 1;
    if (source[index] == '-') {
        sign = -1;
        index = index + 1;
    }
    whole = 0;
    while (source[index] >= '0' && source[index] <= '9') {
        whole = whole * 10 + source[index] - '0';
        index = index + 1;
    }
    fraction = 0;
    divisor = 1;
    if (source[index] == '.') {
        index = index + 1;
        while (source[index] >= '0' && source[index] <= '9') {
            fraction = fraction * 10 + source[index] - '0';
            divisor = divisor * 10;
            index = index + 1;
        }
    }
    if (sign < 0) {
        whole = -whole;
        fraction = -fraction;
    }
    return (double)whole + (double)fraction / (double)divisor;
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

static void build_script(void)
{
    script_count = 0;
    script_index = 0;
    fill(script_data + 0x00, 'O', 'T', 'H', 'E', 'R', 0, 0, 0);
    script_offset[script_count] = 0x00;
    script_count = script_count + 1;
    fill(script_data + 0x10, 'C', 'A', 'R', 'S', 0, 0, 0, 0);
    script_offset[script_count] = 0x10;
    script_count = script_count + 1;
    fill(script_data + 0x20, '*', 0, 0, 0, 0, 0, 0, 0);
    script_offset[script_count] = 0x20;
    script_count = script_count + 1;
    fill(script_data + 0x30, 'd', ' ', 'f', ' ', 'x', ' ', 's', 0);
    script_offset[script_count] = 0x30;
    script_count = script_count + 1;
    fill(script_data + 0x40, 'd', '1', '2', '3', 0, 0, 0, 0);
    script_offset[script_count] = 0x40;
    script_count = script_count + 1;
    fill(script_data + 0x50, 'z', '9', 0, 0, 0, 0, 0, 0);
    script_offset[script_count] = 0x50;
    script_count = script_count + 1;
    fill(script_data + 0x60, 'f', '1', '.', '5', 0, 0, 0, 0);
    script_offset[script_count] = 0x60;
    script_count = script_count + 1;
    fill(script_data + 0x70, 'x', '2', 'a', 0, 0, 0, 0, 0);
    script_offset[script_count] = 0x70;
    script_count = script_count + 1;
    fill(script_data + 0x80, 's', 'N', 'A', 'M', 'E', 0, 0, 0);
    script_offset[script_count] = 0x80;
    script_count = script_count + 1;
    fill(script_data + 0x90, 'd', '-', '7', 0, 0, 0, 0, 0);
    script_offset[script_count] = 0x90;
    script_count = script_count + 1;
    fill(script_data + 0xa0, 'f', '2', '.', '2', '5', 0, 0, 0);
    script_offset[script_count] = 0xa0;
    script_count = script_count + 1;
    fill(script_data + 0xb0, 'x', '1', '0', 0, 0, 0, 0, 0);
    script_offset[script_count] = 0xb0;
    script_count = script_count + 1;
    fill(script_data + 0xc0, 's', 0, 0, 0, 0, 0, 0, 0);
    script_offset[script_count] = 0xc0;
    script_count = script_count + 1;
    fill(script_data + 0xd0, 'E', 'N', 'D', '_', 'L', 'I', 'S', 'T');
    script_offset[script_count] = 0xd0;
    script_count = script_count + 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x100; ++index) {
        table[index] = 0xaa;
    }
    script_index = 0;
    script_count = 0;
    read_calls = 0;
    stricmp_calls = 0;
    format_calls = 0;
    format_base = 0;
    format_type = 0;
    format_name = 0;
    open_calls = 0;
    open_stream = 0;
    open_path = 0;
    open_member = 0;
    open_category = 0;
    open_result = 0;
    report_calls = 0;
    report_stream = 0;
    exit_calls = 0;
    exit_code = 0;
    close_calls = 0;
    close_stream = 0;
    atoi_text = 0;
    atoi_calls = 0;
    atof_text = 0;
    atof_calls = 0;
    sscanf_text = 0;
    sscanf_format = 0;
    sscanf_calls = 0;
    for (index = 0; index < 2; ++index) {
        format_buffer[index] = 0;
        format_text[index] = 0;
    }
    fill(DAT_00007edc, 'D', ':', '\\', 'S', 'C', 'R', 'E', 'A');
    fill(DAT_00007edc + 8, 'M', 'E', 'R', 0, 0, 0, 0, 0);
    fill(type_text, 'G', 'E', 'N', 0, 0, 0, 0, 0);
    fill(name_text, 'M', 'E', 'N', 'U', 0, 0, 0, 0);
    fill(list_text, 'C', 'A', 'R', 'S', 0, 0, 0, 0);
    fill(expect_path_format, '%', 's', '\\', 'd', 'a', 't', 'a', '\\');
    fill(expect_path_format + 8, '%', 's', '.', 'd', 'a', 't', 0, 0);
    fill(expect_member_format, '%', 's', '.', 'd', 't', 'r', 0, 0);
    fill(expect_mode, 'r', 't', 0, 0, 0, 0, 0, 0);
    fill(expect_hex, '%', 'x', 0, 0, 0, 0, 0, 0);
    fill(expect_path, 'D', ':', '\\', 'S', 'C', 'R', 'E', 'A');
    fill(expect_path + 8, 'M', 'E', 'R', '\\', 'd', 'a', 't', 'a');
    fill(expect_path + 16, '\\', 'G', 'E', 'N', '.', 'd', 'a', 't');
    fill(expect_member, 'M', 'E', 'N', 'U', '.', 'd', 't', 'r');
    fill(expect_name, 'N', 'A', 'M', 'E', 0, 0, 0, 0);
    fill(expect_atoi, '-', '7', 0, 0, 0, 0, 0, 0);
    fill(expect_atof, '2', '.', '2', '5', 0, 0, 0, 0);
    fill(expect_sscanf, '1', '0', 0, 0, 0, 0, 0, 0);
    fill(piece_data, '\\', 'd', 'a', 't', 'a', '\\', 0, 0);
    fill(piece_dat, '.', 'd', 'a', 't', 0, 0, 0, 0);
    fill(piece_dtr, '.', 'd', 't', 'r', 0, 0, 0, 0);
}

int main(void)
{
    int result;

    reset();
    build_script();
    open_result = 0;
    result = list_file_load(type_text, name_text, list_text, table, 0x40, 24);
    put(result == 2
        && format_calls == 2
        && same(format_text[0], expect_path_format)
        && format_base == DAT_00007edc
        && format_type == type_text
        && same(format_text[1], expect_member_format)
        && format_name == name_text
        && open_calls == 1
        && open_stream != 0
        && same(open_path, expect_path)
        && same(open_member, expect_member)
        && same(open_category, expect_mode)
        && report_calls == 0 && exit_calls == 0
        && atoi_calls == 2 && same(atoi_text, expect_atoi)
        && atof_calls == 2 && same(atof_text, expect_atof)
        && sscanf_calls == 2 && same(sscanf_text, expect_sscanf)
        && same(sscanf_format, expect_hex)
        && *(int *)(table + 0x00) == 123
        && bits(*(float *)(table + 0x04)) == 0x3fc00000
        && *(int *)(table + 0x08) == 0x2a
        && same(table + 0x0c, expect_name)
        && *(int *)(table + 0x40) == -7
        && bits(*(float *)(table + 0x44)) == 0x40100000
        && *(int *)(table + 0x48) == 0x10
        && *(char *)(table + 0x4c) == 0
        && close_calls == 1 && close_stream != 0 ? '0' : 'x');

    reset();
    open_result = -1;
    result = list_file_load(type_text, name_text, list_text, table, 0x40, 24);
    put(result == 0
        && open_calls == 1
        && report_calls == 1 && report_stream != 0
        && exit_calls == 1 && exit_code == 1
        && close_calls == 1 && close_stream != 0
        && atoi_calls == 0 && atof_calls == 0 && sscanf_calls == 0 ? '1' : 'x');

    put('D');
    quit();
    return 0;
}
