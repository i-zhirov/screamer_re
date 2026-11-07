/* The test harness for the rebuilt `unr_file_load` module.
 *
 * The harness holds the name buffer `DAT_002154c`, the engine pointer, the
 * object list, the unresolved file data, and the stubs.  The stubs record
 * their arguments and return configured results.  The harness runs three
 * paths:
 *
 *     0  the success path with the file size 8 and the object count 2 reads
 *        the two counts, gets the `OBJ_LST` and `UNR_FILE` resources, relocates
 *        the two object entries, writes the -1 sentinel, reads the file data,
 *        and closes the stream
 *     1  the open-failure path runs the file error report and the exit with
 *        the code 0
 *     2  the zero-count path writes only the -1 sentinel and requests the
 *        `OBJ_LST` resource of the base size 0x1e4
 *     D  the exit marker
 *
 * The name and the expected text are built at run time, so the checks do not
 * depend on the literal data that the DOS/4G loader maps at run time.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void unr_file_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_002154c[0x28];
void *DAT_0021574;
void *DAT_002145c;
void *DAT_002152c;
void *DAT_0021540;
int DAT_0021464;
int DAT_00214bc;
void *DAT_0021488;
void *DAT_0021530;

static char text[8];
static char object_buffer[0x100];
static char unr_buffer[0x100];
static char format_expected[8];
static char object_expected[0x10];
static char file_expected[0x10];
static char mode_expected[8];
static char path_expected[4];
static char name_expected[0x28];

static char *sprintf_buffer;
static char sprintf_text[0x28];
static int sprintf_format_ok;
static int sprintf_calls;

static char *open_stream;
static char open_path[0x40];
static char open_name[0x40];
static char open_category[8];
static int open_result;
static int open_calls;

static void *file_report_stream;
static int file_report_calls;

static int exit_code;
static int exit_calls;

static char *read_stream;
static char *read_buffer[8];
static int read_count[8];
static int read_calls;

static void *resource_result[2];
static void *resource_resource[2];
static char resource_name[2][0x10];
static int resource_size[2];
static int resource_calls;

static void *memory_report_resource;
static int memory_report_calls;

static char *close_stream;
static int close_calls;

static int script_file_size;
static int script_object_count;
static int script_offsets[8];
static char script_data[0x40];

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

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *value;

    va_start(arguments, format);
    value = va_arg(arguments, char *);
    va_end(arguments);
    copy(buffer, value);
    append(buffer, '.', 'U', 'N', 'R');
    sprintf_format_ok = same((char *)format, format_expected);
    sprintf_buffer = buffer;
    copy(sprintf_text, buffer);
    sprintf_calls = sprintf_calls + 1;
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

int stream_read(char *stream, char *buffer, int count)
{
    int index;
    int call;

    call = read_calls;
    read_stream = stream;
    if (call < 8) {
        read_buffer[call] = buffer;
        read_count[call] = count;
    }
    read_calls = read_calls + 1;
    if (call == 0) {
        *(int *)buffer = script_file_size;
    } else if (call == 1) {
        *(int *)buffer = script_object_count;
    } else if (call == 2) {
        for (index = 0; index < script_object_count; index++) {
            ((int *)buffer)[index] = script_offsets[index];
        }
    } else if (call == 3) {
        for (index = 0; index < count; index++) {
            buffer[index] = script_data[index];
        }
    }
    return count;
}

void *resource_get(void *resource, char *name, int size)
{
    int call;

    call = resource_calls;
    resource_resource[call] = resource;
    copy(resource_name[call], name);
    resource_size[call] = size;
    resource_calls = resource_calls + 1;
    return resource_result[call];
}

void memory_error_report(void *resource)
{
    memory_report_resource = resource;
    memory_report_calls = memory_report_calls + 1;
}

int stream_close(char *stream)
{
    close_stream = stream;
    close_calls = close_calls + 1;
    return 0;
}

static void build_texts(void)
{
    format_expected[0] = '%';
    format_expected[1] = 's';
    format_expected[2] = '.';
    format_expected[3] = 'U';
    format_expected[4] = 'N';
    format_expected[5] = 'R';
    format_expected[6] = 0;

    object_expected[0] = 'O';
    object_expected[1] = 'B';
    object_expected[2] = 'J';
    object_expected[3] = '_';
    object_expected[4] = 'L';
    object_expected[5] = 'S';
    object_expected[6] = 'T';
    object_expected[7] = 0;

    file_expected[0] = 'U';
    file_expected[1] = 'N';
    file_expected[2] = 'R';
    file_expected[3] = '_';
    file_expected[4] = 'F';
    file_expected[5] = 'I';
    file_expected[6] = 'L';
    file_expected[7] = 'E';
    file_expected[8] = 0;

    mode_expected[0] = 'r';
    mode_expected[1] = 'b';
    mode_expected[2] = 0;

    path_expected[0] = 0;

    DAT_002154c[0] = 'M';
    DAT_002154c[1] = 'A';
    DAT_002154c[2] = 'P';
    DAT_002154c[3] = '1';
    DAT_002154c[4] = 0;

    name_expected[0] = 'M';
    name_expected[1] = 'A';
    name_expected[2] = 'P';
    name_expected[3] = '1';
    name_expected[4] = '.';
    name_expected[5] = 'U';
    name_expected[6] = 'N';
    name_expected[7] = 'R';
    name_expected[8] = 0;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x100; index++) {
        object_buffer[index] = 0;
        unr_buffer[index] = 0;
    }
    for (index = 0; index < 0x40; index++) {
        script_data[index] = 0;
    }
    for (index = 0; index < 8; index++) {
        script_offsets[index] = 0;
        read_buffer[index] = 0;
        read_count[index] = 0;
    }
    script_file_size = 0;
    script_object_count = 0;

    DAT_0021574 = (void *)0x11110000;
    DAT_002145c = (void *)0x22220000;
    DAT_002152c = (void *)0x33330000;
    DAT_0021540 = (void *)0x44440000;
    DAT_0021464 = 0x5a5a5a5a;
    DAT_00214bc = 0;
    DAT_0021488 = (void *)0x55550000;
    DAT_0021530 = (void *)0x66660000;

    sprintf_buffer = 0;
    sprintf_text[0] = 0;
    sprintf_format_ok = 0;
    sprintf_calls = 0;

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

    read_stream = 0;
    read_calls = 0;

    resource_result[0] = object_buffer;
    resource_result[1] = unr_buffer;
    resource_resource[0] = 0;
    resource_resource[1] = 0;
    resource_name[0][0] = 0;
    resource_name[1][0] = 0;
    resource_size[0] = 0;
    resource_size[1] = 0;
    resource_calls = 0;

    memory_report_resource = 0;
    memory_report_calls = 0;

    close_stream = 0;
    close_calls = 0;
}

int main(void)
{
    build_texts();

    reset();
    script_file_size = 8;
    script_object_count = 2;
    script_offsets[0] = 0x10;
    script_offsets[1] = 0x20;
    script_data[0] = 'A';
    script_data[1] = 'B';
    unr_file_load();
    put(sprintf_calls == 1 && sprintf_format_ok
        && same(sprintf_text, name_expected)
        && open_calls == 1 && open_stream != 0
        && same(open_path, path_expected)
        && same(open_name, name_expected)
        && same(open_category, mode_expected)
        && file_report_calls == 0 && exit_calls == 0
        && resource_calls == 2
        && resource_resource[0] == DAT_0021574
        && resource_resource[1] == DAT_0021574
        && same(resource_name[0], object_expected)
        && same(resource_name[1], file_expected)
        && resource_size[0] == 2 * 4 + 0x1e4
        && resource_size[1] == 8
        && DAT_002145c == (void *)object_buffer
        && DAT_002152c == (void *)unr_buffer
        && DAT_0021540 == (void *)object_buffer
        && DAT_0021488 == (void *)object_buffer
        && DAT_0021530 == (void *)object_buffer
        && DAT_00214bc == 0x5a5a5a5a
        && read_calls == 4 && read_stream == open_stream
        && read_buffer[2] == object_buffer && read_count[2] == 2 * 4
        && read_buffer[3] == unr_buffer && read_count[3] == 8
        && *(int *)(object_buffer + 0) == 0x10 + (int)unr_buffer
        && *(int *)(object_buffer + 4) == 0x20 + (int)unr_buffer
        && *(int *)(object_buffer + 8) == -1
        && unr_buffer[0] == 'A' && unr_buffer[1] == 'B'
        && memory_report_calls == 0
        && close_calls == 1 && close_stream == open_stream ? '0' : 'x');

    reset();
    script_file_size = 4;
    script_object_count = 1;
    script_offsets[0] = 0x30;
    open_result = 1;
    unr_file_load();
    put(open_calls == 1 && same(open_name, name_expected)
        && file_report_calls == 1 && file_report_stream == open_stream
        && exit_calls == 1 && exit_code == 0
        && resource_calls == 2 && close_calls == 1 ? '1' : 'x');

    reset();
    script_file_size = 4;
    script_object_count = 0;
    script_data[0] = 'Z';
    unr_file_load();
    put(resource_calls == 2
        && same(resource_name[0], object_expected)
        && resource_size[0] == 0x1e4
        && resource_size[1] == 4
        && *(int *)object_buffer == -1
        && read_calls == 4 && read_count[2] == 0 && read_count[3] == 4
        && unr_buffer[0] == 'Z'
        && memory_report_calls == 0 && close_calls == 1 ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
