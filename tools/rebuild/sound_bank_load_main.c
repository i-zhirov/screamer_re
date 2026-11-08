/* The test harness for the rebuilt `sound_bank_load` module.
 *
 * The harness holds the setup word, the sample count, the resource set, the
 * base path, the 0x24-byte input records, and the 0x74-byte result records.
 * The stubs record their arguments and return the configured results.  The
 * `sprintf` stub builds the path and the member names, so the harness checks
 * the built text.
 *
 * The harness runs five paths:
 *
 *     0  the normal path with two samples: the path build, the two member
 *        builds, the two opens, the two resource reads, the two closes, and
 *        the status 0 and 1 fields
 *     1  the four status values 1, 2, 3, and 4: the type map and the three
 *        byte flags
 *     2  the zero setup word: the function returns and runs no call
 *     3  the two failed opens: the file error report and the exit 0 run twice
 *        and the path continues
 *     4  a zero resource: the memory error report runs once
 *     D  the exit marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, and the module build
 * defines `SOUND_BANK_LOAD_HARNESS` for the same reason.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void sound_bank_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct sample_input {
    char name[0x0c];
    unsigned int field_0c;
    unsigned int field_10;
    unsigned int field_14;
    unsigned int field_18;
    unsigned int field_1c;
    unsigned int field_20;
};

struct sample_info {
    void *data;
    int segment;
    int size;
    int field_0c;
    int field_10;
    int field_14;
    int index;
    int pad_1c;
    int pad_20;
    int pad_24;
    int flags;
    int pad_2c;
    int field_30;
    int field_34;
    int field_38;
    int pad_3c;
    int field_40;
    int pad_44[12];
};

int DAT_00008058;
int DAT_00018d84;
void *DAT_000062ac;
char DAT_00007f44[0x20];
struct sample_input DAT_0015224[8];
struct sample_info DAT_0016034[8];

static char text[8];

static char *sprintf_buffer[8];
static char sprintf_format_copy[8][0x20];
static char sprintf_result_copy[8][0x40];
static unsigned int sprintf_arg[8];
static int sprintf_calls;

static int fopen_calls;
static void *fopen_stream[4];
static char fopen_path_copy[4][0x40];
static char fopen_member_copy[4][0x20];
static char fopen_category_copy[4][0x10];
static int fopen_result[4];

static int file_error_calls;
static void *file_error_stream;

static int exit_calls;
static int exit_code;

static int status_index;
static int status_values[32];

static int resource_calls;
static void *resource_resource[4];
static char resource_name_copy[4][0x10];
static int resource_size[4];
static void *resource_result[4];

static int memory_error_calls;
static void *memory_error_resource;

static int stream_read_calls;
static void *stream_read_stream[4];
static void *stream_read_buffer[4];
static int stream_read_size[4];

static int stream_close_calls;

static char expect_path_format[0x14];
static char expect_member_format[0x10];
static char expect_list_name[0x10];
static char expect_mode[0x10];
static char expect_path[0x40];
static char path_suffix[0x14];
static char member_suffix[0x8];
static char expect_member[2][0x10];

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

static void setup_sample(int index, char a, char b, char c,
                         unsigned int field_0c, unsigned int field_10,
                         unsigned int field_14, unsigned int field_18,
                         unsigned int field_1c, unsigned int field_20)
{
    fill(DAT_0015224[index].name, a, b, c, 0, 0, 0, 0, 0);
    DAT_0015224[index].field_0c = field_0c;
    DAT_0015224[index].field_10 = field_10;
    DAT_0015224[index].field_14 = field_14;
    DAT_0015224[index].field_18 = field_18;
    DAT_0015224[index].field_1c = field_1c;
    DAT_0015224[index].field_20 = field_20;
}

static void set_status(int index, int value)
{
    status_values[index] = value;
}

static void reset(void)
{
    int index;
    int slot;

    DAT_00008058 = 1;
    DAT_00018d84 = 0;
    DAT_000062ac = (void *)0x5555;
    fill(DAT_00007f44, 'D', ':', '\\', 'S', 'C', 'R', 'E', 'A');
    fill(DAT_00007f44 + 8, 'M', 'E', 'R', 0, 0, 0, 0, 0);
    for (index = 0; index < 8; ++index) {
        DAT_0015224[index].name[0] = 0;
        DAT_0015224[index].field_0c = 0;
        DAT_0015224[index].field_10 = 0;
        DAT_0015224[index].field_14 = 0;
        DAT_0015224[index].field_18 = 0;
        DAT_0015224[index].field_1c = 0;
        DAT_0015224[index].field_20 = 0;
    }
    for (index = 0; index < 8; ++index) {
        DAT_0016034[index].data = 0;
        DAT_0016034[index].segment = 0;
        DAT_0016034[index].size = 0;
        DAT_0016034[index].field_0c = 0;
        DAT_0016034[index].field_10 = 0;
        DAT_0016034[index].field_14 = 0;
        DAT_0016034[index].index = 0;
        DAT_0016034[index].pad_1c = 0;
        DAT_0016034[index].pad_20 = 0;
        DAT_0016034[index].pad_24 = 0;
        DAT_0016034[index].flags = 0;
        DAT_0016034[index].pad_2c = 0;
        DAT_0016034[index].field_30 = 0;
        DAT_0016034[index].field_34 = 0;
        DAT_0016034[index].field_38 = 0;
        DAT_0016034[index].pad_3c = 0;
        DAT_0016034[index].field_40 = 0;
        for (slot = 0; slot < 12; ++slot) {
            DAT_0016034[index].pad_44[slot] = 0;
        }
    }
    for (index = 0; index < 8; ++index) {
        sprintf_buffer[index] = 0;
        sprintf_format_copy[index][0] = 0;
        sprintf_result_copy[index][0] = 0;
        sprintf_arg[index] = 0;
    }
    sprintf_calls = 0;
    for (index = 0; index < 4; ++index) {
        fopen_stream[index] = 0;
        fopen_path_copy[index][0] = 0;
        fopen_member_copy[index][0] = 0;
        fopen_category_copy[index][0] = 0;
        fopen_result[index] = 0;
        resource_resource[index] = 0;
        resource_name_copy[index][0] = 0;
        resource_size[index] = 0;
        resource_result[index] = (void *)(0x7000 + index);
        stream_read_stream[index] = 0;
        stream_read_buffer[index] = 0;
        stream_read_size[index] = 0;
    }
    fopen_calls = 0;
    file_error_calls = 0;
    file_error_stream = 0;
    exit_calls = 0;
    exit_code = 0;
    status_index = 0;
    for (index = 0; index < 32; ++index) {
        status_values[index] = 0x40;
    }
    resource_calls = 0;
    memory_error_calls = 0;
    memory_error_resource = 0;
    stream_read_calls = 0;
    stream_close_calls = 0;
    fill(expect_path_format, '%', 's', '\\', 'd', 'a', 't', 'a', '\\');
    fill(expect_path_format + 8, 's', 'o', 'u', 'n', 'd', '.', 'd', 'a');
    fill(expect_path_format + 16, 't', 0, 0, 0, 0, 0, 0, 0);
    fill(expect_member_format, '%', 's', '.', 'R', 'A', 'W', 0, 0);
    fill(expect_list_name, 'S', 'O', 'U', 'N', 'D', '_', 'F', 'X');
    fill(expect_mode, 'r', 'b', 0, 0, 0, 0, 0, 0);
    fill(path_suffix, '\\', 'd', 'a', 't', 'a', '\\', 's', 'o');
    fill(path_suffix + 8, 'u', 'n', 'd', '.', 'd', 'a', 't', 0);
    fill(member_suffix, '.', 'R', 'A', 'W', 0, 0, 0, 0);
    fill(expect_member[0], 'A', 'A', 'A', '.', 'R', 'A', 'W', 0);
    fill(expect_member[1], 'B', 'B', 'B', '.', 'R', 'A', 'W', 0);
    copy(expect_path, DAT_00007f44);
    append(expect_path, path_suffix);
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    unsigned int argument;
    int index;

    va_start(arguments, format);
    argument = va_arg(arguments, unsigned int);
    va_end(arguments);
    if (sprintf_calls < 8) {
        sprintf_buffer[sprintf_calls] = buffer;
        copy(sprintf_format_copy[sprintf_calls], (char *)format);
        sprintf_arg[sprintf_calls] = argument;
        if (sprintf_calls == 0) {
            copy(buffer, (char *)argument);
            append(buffer, path_suffix);
        } else {
            copy(buffer, (char *)argument);
            append(buffer, member_suffix);
        }
        copy(sprintf_result_copy[sprintf_calls], buffer);
    }
    index = sprintf_calls + 1;
    sprintf_calls = index;
    return 0;
}

int fopen(void *stream, char *path, char *member, char *category)
{
    int index;

    index = fopen_calls;
    if (index < 4) {
        fopen_stream[index] = stream;
        copy(fopen_path_copy[index], path);
        copy(fopen_member_copy[index], member);
        copy(fopen_category_copy[index], category);
    }
    fopen_calls = index + 1;
    if (index < 4) {
        return fopen_result[index];
    }
    return 0;
}

void file_error_report(void *stream)
{
    file_error_stream = stream;
    file_error_calls = file_error_calls + 1;
}

void exit(int code)
{
    exit_code = code;
    exit_calls = exit_calls + 1;
}

int stream_status(void *stream)
{
    int value;

    value = status_values[status_index];
    status_index = status_index + 1;
    return value;
}

void *resource_get(void *resource, char *name, int size)
{
    int index;

    index = resource_calls;
    if (index < 4) {
        resource_resource[index] = resource;
        copy(resource_name_copy[index], name);
        resource_size[index] = size;
    }
    resource_calls = index + 1;
    if (index < 4) {
        return resource_result[index];
    }
    return 0;
}

void memory_error_report(void *resource)
{
    memory_error_resource = resource;
    memory_error_calls = memory_error_calls + 1;
}

int stream_read(void *stream, void *buffer, int size)
{
    int index;

    index = stream_read_calls;
    if (index < 4) {
        stream_read_stream[index] = stream;
        stream_read_buffer[index] = buffer;
        stream_read_size[index] = size;
    }
    stream_read_calls = index + 1;
    return size;
}

int stream_close(void *stream)
{
    stream_close_calls = stream_close_calls + 1;
    return 0;
}

static int pads_zero(int index)
{
    struct sample_info *info;

    info = &DAT_0016034[index];
    return info->pad_1c == 0
        && info->pad_20 == 0
        && info->pad_24 == 0
        && info->pad_2c == 0
        && info->pad_3c == 0
        && info->pad_44[0] == 0
        && info->pad_44[11] == 0;
}

static int normal(void)
{
    struct sample_info *first;
    struct sample_info *second;

    first = &DAT_0016034[0];
    second = &DAT_0016034[1];
    return sprintf_calls == 3
        && same(sprintf_format_copy[0], expect_path_format)
        && same(sprintf_format_copy[1], expect_member_format)
        && same(sprintf_format_copy[2], expect_member_format)
        && sprintf_arg[0] == (unsigned int)DAT_00007f44
        && sprintf_arg[1] == (unsigned int)DAT_0015224[0].name
        && sprintf_arg[2] == (unsigned int)DAT_0015224[1].name
        && same(sprintf_result_copy[0], expect_path)
        && same(sprintf_result_copy[1], expect_member[0])
        && same(sprintf_result_copy[2], expect_member[1])
        && fopen_calls == 2
        && same(fopen_path_copy[0], expect_path)
        && same(fopen_path_copy[1], expect_path)
        && same(fopen_member_copy[0], expect_member[0])
        && same(fopen_member_copy[1], expect_member[1])
        && same(fopen_category_copy[0], expect_mode)
        && same(fopen_category_copy[1], expect_mode)
        && fopen_stream[0] != 0 && fopen_stream[1] != 0
        && resource_calls == 2
        && resource_resource[0] == DAT_000062ac
        && resource_resource[1] == DAT_000062ac
        && same(resource_name_copy[0], expect_list_name)
        && same(resource_name_copy[1], expect_list_name)
        && resource_size[0] == 0x100
        && resource_size[1] == 0x200
        && stream_read_calls == 2
        && stream_read_buffer[0] == resource_result[0]
        && stream_read_buffer[1] == resource_result[1]
        && stream_read_size[0] == 0x100
        && stream_read_size[1] == 0x200
        && stream_close_calls == 2
        && first->data == resource_result[0]
        && first->segment == 0
        && first->size == 0x100
        && first->field_0c == 1
        && first->field_10 == 0
        && first->field_14 == 0x1234
        && first->index == 1
        && first->flags == 0x4540
        && first->field_30 == 0x11111111
        && first->field_34 == 3
        && first->field_38 == 0x2a
        && first->field_40 == 0
        && pads_zero(0)
        && second->data == resource_result[1]
        && second->segment == 0
        && second->size == 0x200
        && second->field_0c == 0
        && second->field_10 == 2
        && second->field_14 == 0x5678
        && second->index == 2
        && second->flags == 0
        && second->field_30 == 0x22222222
        && second->field_34 == 0
        && second->field_38 == 0x123
        && second->field_40 == 5
        && pads_zero(1)
        && memory_error_calls == 0
        && file_error_calls == 0
        && exit_calls == 0;
}

static int statuses(void)
{
    return sprintf_calls == 5
        && fopen_calls == 4
        && resource_calls == 4
        && stream_close_calls == 4
        && DAT_0016034[0].field_10 == 2
        && DAT_0016034[0].flags == 0x500
        && DAT_0016034[1].field_10 == 1
        && DAT_0016034[1].flags == 0
        && DAT_0016034[2].field_10 == 3
        && DAT_0016034[2].flags == 0x200
        && DAT_0016034[3].field_10 == 2
        && DAT_0016034[3].flags == 0;
}

int main(void)
{
    reset();
    DAT_00018d84 = 2;
    set_status(0, 0x100);
    set_status(1, 0x100);
    set_status(2, 0x100);
    set_status(3, 0x200);
    set_status(4, 0x200);
    set_status(5, 0x200);
    resource_result[0] = (void *)0xaaaa;
    resource_result[1] = (void *)0xbbbb;
    setup_sample(0, 'A', 'A', 'A', 0x10001234, 1, 0x11111111, 3,
                 0x1000002a, 0x00000000);
    setup_sample(1, 'B', 'B', 'B', 0x00005678, 0, 0x22222222, 0,
                 0x00000123, 0x01000005);
    sound_bank_load();
    put(normal() ? '0' : 'x');

    reset();
    DAT_00018d84 = 4;
    setup_sample(0, 'S', '0', 0, 0x10000000, 0, 0, 0, 0x10000000,
                 0x01000000);
    setup_sample(1, 'S', '1', 0, 0, 0, 0, 0, 0, 0x02000000);
    setup_sample(2, 'S', '2', 0, 0, 0, 0, 0, 0, 0x13000000);
    setup_sample(3, 'S', '3', 0, 0, 0, 0, 0, 0, 0x04000000);
    sound_bank_load();
    put(statuses() ? '1' : 'x');

    reset();
    DAT_00008058 = 0;
    DAT_00018d84 = 2;
    DAT_0016034[0].data = (void *)0xdead;
    DAT_0016034[0].field_10 = 0x77;
    sound_bank_load();
    put(DAT_0016034[0].data == (void *)0xdead
        && DAT_0016034[0].field_10 == 0x77
        && sprintf_calls == 0
        && fopen_calls == 0
        && resource_calls == 0
        && stream_close_calls == 0 ? '2' : 'x');

    reset();
    DAT_00018d84 = 2;
    fopen_result[0] = 1;
    fopen_result[1] = 1;
    sound_bank_load();
    put(file_error_calls == 2
        && exit_calls == 2
        && exit_code == 0
        && fopen_calls == 2
        && resource_calls == 2
        && stream_close_calls == 2
        && DAT_0016034[0].data == resource_result[0]
        && DAT_0016034[1].data == resource_result[1] ? '3' : 'x');

    reset();
    DAT_00018d84 = 1;
    resource_result[0] = 0;
    setup_sample(0, 'Z', 'Z', 'Z', 0, 0, 0, 0, 0, 0);
    sound_bank_load();
    put(memory_error_calls == 1
        && memory_error_resource == DAT_000062ac
        && DAT_0016034[0].data == 0
        && stream_read_calls == 1
        && stream_read_buffer[0] == 0
        && DAT_0016034[0].size == 0x40 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
