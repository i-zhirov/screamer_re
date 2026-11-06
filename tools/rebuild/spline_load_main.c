/* The test harness for the rebuilt `spline_load` module.
 *
 * The harness holds the base path, the resource handle, the simulated container
 * member, the two name arguments, and the spline record array.  The stubs
 * record their arguments and their call counts.  The stream stubs read and seek
 * in a small simulated member, so the harness checks the skip arithmetic of the
 * first pass and the record reads of the second pass.
 *
 * The harness runs five paths:
 *
 *     0  the normal path with the two splines 2 and 5: the two formatted
 *        names, the open, the two header skips, the record allocation, the
 *        rewind, the two record reads, the two data reads, and the close
 *     1  an empty member: the first read returns zero, so the count is zero,
 *        the table size is zero, and the loop runs no read
 *     2  a failed open: the function returns zero and runs no stream call
 *     3  a zero data resource: the memory error report runs once and the data
 *        read gets the zero buffer
 *     4  the three splines 1, 0, and 7: the zero count keeps a valid type and
 *        its own seek offset
 *     D  the exit marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, and the module build
 * defines `SPLINE_LOAD_HARNESS` for the same reason.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

struct spline_record {
    void *data;
    int reserved[6];
    int count;
    int type;
};

int spline_load(char *archive, char *name, struct spline_record **table);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007f44[0x20];
void *DAT_000062ac;

static char text[8];
static struct spline_record records[4];
static unsigned char data_a[0x200];
static unsigned char data_b[0x200];
static unsigned char data_c[0x200];

static unsigned char file_data[0x800];
static int file_size;
static int file_pos;

static char *sprintf_buffer[8];
static char sprintf_format_copy[8][0x20];
static char sprintf_result_copy[8][0x40];
static unsigned int sprintf_arg_first[8];
static unsigned int sprintf_arg_second[8];
static int sprintf_two_args[8];
static int sprintf_calls;

static int fopen_calls;
static void *fopen_stream[4];
static char fopen_path_copy[4][0x40];
static char fopen_member_copy[4][0x20];
static char fopen_category_copy[4][0x10];
static int fopen_result[4];

static int stream_read_calls;
static void *stream_read_buffer[16];
static int stream_read_size[16];
static int stream_read_result[16];

static int stream_seek_calls;
static int stream_seek_offset[8];
static int stream_seek_origin[8];

static int stream_close_calls;

static int resource_calls;
static void *resource_resource[8];
static char resource_name_copy[8][0x10];
static int resource_size[8];
static void *resource_result[8];

static int memory_error_calls;
static void *memory_error_resource;

static char archive_text[0x10];
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

static void add_spline(unsigned int count, unsigned int type)
{
    int index;

    *(unsigned int *)(file_data + file_size) = count;
    file_size = file_size + 4;
    *(unsigned int *)(file_data + file_size) = type;
    file_size = file_size + 4;
    for (index = 0; index < (int)(count * 0x38); ++index) {
        file_data[file_size] = 0;
        file_size = file_size + 1;
    }
}

static void reset_records(void)
{
    int index;
    int slot;

    for (index = 0; index < 4; ++index) {
        records[index].data = 0;
        records[index].count = 0;
        records[index].type = 0;
        for (slot = 0; slot < 6; ++slot) {
            records[index].reserved[slot] = 0;
        }
    }
}

static void reset(void)
{
    int index;

    file_size = 0;
    file_pos = 0;
    reset_records();
    fill(DAT_00007f44, 'D', ':', '\\', 'S', 'C', 'R', 'E', 'A');
    fill(DAT_00007f44 + 8, 'M', 'E', 'R', 0, 0, 0, 0, 0);
    DAT_000062ac = (void *)0x5555;
    fill(archive_text, 'M', 'A', 'P', '1', 0, 0, 0, 0);
    fill(name_text, 'M', 'A', 'P', '1', 'o', 'p', 'p', 0);
    sprintf_calls = 0;
    for (index = 0; index < 8; ++index) {
        sprintf_buffer[index] = 0;
        sprintf_format_copy[index][0] = 0;
        sprintf_result_copy[index][0] = 0;
        sprintf_arg_first[index] = 0;
        sprintf_arg_second[index] = 0;
        sprintf_two_args[index] = 0;
    }
    fopen_calls = 0;
    for (index = 0; index < 4; ++index) {
        fopen_stream[index] = 0;
        fopen_path_copy[index][0] = 0;
        fopen_member_copy[index][0] = 0;
        fopen_category_copy[index][0] = 0;
        fopen_result[index] = 0;
    }
    stream_read_calls = 0;
    for (index = 0; index < 16; ++index) {
        stream_read_buffer[index] = 0;
        stream_read_size[index] = 0;
        stream_read_result[index] = 0;
    }
    stream_seek_calls = 0;
    for (index = 0; index < 8; ++index) {
        stream_seek_offset[index] = 0;
        stream_seek_origin[index] = 0;
    }
    stream_close_calls = 0;
    resource_calls = 0;
    for (index = 0; index < 8; ++index) {
        resource_resource[index] = 0;
        resource_name_copy[index][0] = 0;
        resource_size[index] = 0;
        resource_result[index] = 0;
    }
    memory_error_calls = 0;
    memory_error_resource = 0;
    fill(expect_path_format, '%', 's', '\\', 'd', 'a', 't', 'a', '\\');
    fill(expect_path_format + 8, '%', 's', '.', 'd', 'a', 't', 0, 0);
    fill(expect_member_format, '%', 's', '.', 's', 'p', 'l', 0, 0);
    fill(expect_list_name, 'G', 'A', 'M', 'E', 'S', 'P', 'L', 'I');
    expect_list_name[8] = 'N';
    expect_list_name[9] = 'E';
    expect_list_name[10] = 0;
    fill(expect_mode, 'r', 'b', 0, 0, 0, 0, 0, 0);
    fill(path_mid, '\\', 'd', 'a', 't', 'a', '\\', 0, 0);
    fill(path_tail, '.', 'd', 'a', 't', 0, 0, 0, 0);
    fill(member_tail, '.', 's', 'p', 'l', 0, 0, 0, 0);
    copy(expect_path, DAT_00007f44);
    append(expect_path, path_mid);
    append(expect_path, archive_text);
    append(expect_path, path_tail);
    copy(expect_member, name_text);
    append(expect_member, member_tail);
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *first;
    char *second;
    int index;

    index = sprintf_calls;
    va_start(arguments, format);
    if (same((char *)format, expect_path_format)) {
        first = va_arg(arguments, char *);
        second = va_arg(arguments, char *);
        copy(buffer, first);
        append(buffer, path_mid);
        append(buffer, second);
        append(buffer, path_tail);
    } else {
        first = va_arg(arguments, char *);
        second = 0;
        copy(buffer, first);
        append(buffer, member_tail);
    }
    va_end(arguments);
    if (index < 8) {
        sprintf_buffer[index] = buffer;
        copy(sprintf_format_copy[index], (char *)format);
        sprintf_arg_first[index] = (unsigned int)first;
        sprintf_arg_second[index] = (unsigned int)second;
        sprintf_two_args[index] = second != 0;
        copy(sprintf_result_copy[index], buffer);
    }
    sprintf_calls = index + 1;
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
    file_pos = 0;
    if (index < 4) {
        return fopen_result[index];
    }
    return 0;
}

int stream_read(void *stream, void *buffer, int size)
{
    int index;
    int available;
    int take;
    int slot;

    index = stream_read_calls;
    available = file_size - file_pos;
    if (available < 0) {
        available = 0;
    }
    take = size < available ? size : available;
    if (take < 0) {
        take = 0;
    }
    if (buffer != 0) {
        for (slot = 0; slot < take; ++slot) {
            ((unsigned char *)buffer)[slot] = file_data[file_pos + slot];
        }
    }
    file_pos = file_pos + take;
    if (index < 16) {
        stream_read_buffer[index] = buffer;
        stream_read_size[index] = size;
        stream_read_result[index] = take;
    }
    stream_read_calls = index + 1;
    return take;
}

int stream_seek(void *stream, int offset, int origin)
{
    int index;

    index = stream_seek_calls;
    if (index < 8) {
        stream_seek_offset[index] = offset;
        stream_seek_origin[index] = origin;
    }
    if (origin == 0) {
        file_pos = 0;
    } else if (origin == 2) {
        file_pos = file_size;
    }
    file_pos = file_pos + offset;
    stream_seek_calls = index + 1;
    return 0;
}

void *resource_get(void *resource, char *name, int size)
{
    int index;

    index = resource_calls;
    if (index < 8) {
        resource_resource[index] = resource;
        copy(resource_name_copy[index], name);
        resource_size[index] = size;
    }
    resource_calls = index + 1;
    if (index < 8) {
        return resource_result[index];
    }
    return 0;
}

void memory_error_report(void *resource)
{
    memory_error_resource = resource;
    memory_error_calls = memory_error_calls + 1;
}

int stream_close(void *stream)
{
    stream_close_calls = stream_close_calls + 1;
    return 0;
}

static int normal(void)
{
    struct spline_record *first;
    struct spline_record *second;

    first = &records[0];
    second = &records[1];
    return sprintf_calls == 2
        && same(sprintf_format_copy[0], expect_path_format)
        && same(sprintf_format_copy[1], expect_member_format)
        && sprintf_two_args[0] == 1
        && sprintf_two_args[1] == 0
        && sprintf_arg_first[0] == (unsigned int)DAT_00007f44
        && sprintf_arg_second[0] == (unsigned int)archive_text
        && sprintf_arg_first[1] == (unsigned int)name_text
        && same(sprintf_result_copy[0], expect_path)
        && same(sprintf_result_copy[1], expect_member)
        && fopen_calls == 1
        && same(fopen_path_copy[0], expect_path)
        && same(fopen_member_copy[0], expect_member)
        && same(fopen_category_copy[0], expect_mode)
        && fopen_stream[0] != 0
        && stream_seek_calls == 3
        && stream_seek_offset[0] == 0x74
        && stream_seek_offset[1] == 0x11c
        && stream_seek_offset[2] == 0
        && stream_seek_origin[0] == 1
        && stream_seek_origin[1] == 1
        && stream_seek_origin[2] == 0
        && resource_calls == 3
        && resource_resource[0] == DAT_000062ac
        && resource_resource[1] == DAT_000062ac
        && resource_resource[2] == DAT_000062ac
        && same(resource_name_copy[0], expect_list_name)
        && same(resource_name_copy[1], expect_list_name)
        && same(resource_name_copy[2], expect_list_name)
        && resource_size[0] == 0x48
        && resource_size[1] == 0x70
        && resource_size[2] == 0x118
        && stream_read_calls == 9
        && stream_read_size[0] == 4
        && stream_read_size[1] == 4
        && stream_read_size[2] == 4
        && stream_read_result[2] == 0
        && stream_read_size[3] == 4
        && stream_read_buffer[3] == &first->count
        && stream_read_size[4] == 4
        && stream_read_buffer[4] == &first->type
        && stream_read_size[5] == 0x70
        && stream_read_buffer[5] == data_a
        && stream_read_size[6] == 4
        && stream_read_buffer[6] == &second->count
        && stream_read_size[7] == 4
        && stream_read_buffer[7] == &second->type
        && stream_read_size[8] == 0x118
        && stream_read_buffer[8] == data_b
        && stream_close_calls == 1
        && first->data == data_a
        && first->count == 2
        && first->type == 0x11
        && second->data == data_b
        && second->count == 5
        && second->type == 0x22
        && memory_error_calls == 0;
}

static int empty(void)
{
    return sprintf_calls == 2
        && fopen_calls == 1
        && stream_seek_calls == 1
        && stream_seek_offset[0] == 0
        && stream_seek_origin[0] == 0
        && resource_calls == 1
        && resource_size[0] == 0
        && stream_read_calls == 1
        && stream_read_result[0] == 0
        && stream_close_calls == 1
        && records[0].count == 0
        && records[0].type == 0
        && records[0].data == 0;
}

static int failed_open(void)
{
    return sprintf_calls == 2
        && fopen_calls == 1
        && stream_seek_calls == 0
        && stream_read_calls == 0
        && resource_calls == 0
        && stream_close_calls == 0
        && memory_error_calls == 0;
}

static int zero_data(void)
{
    struct spline_record *first;

    first = &records[0];
    return resource_calls == 2
        && resource_size[0] == 0x24
        && resource_size[1] == 0x38
        && stream_read_calls == 5
        && stream_read_size[4] == 0x38
        && stream_read_buffer[4] == 0
        && stream_close_calls == 1
        && memory_error_calls == 1
        && memory_error_resource == DAT_000062ac
        && first->data == 0
        && first->count == 1
        && first->type == 0x33;
}

static int varied(void)
{
    return resource_calls == 4
        && resource_size[0] == 0x6c
        && resource_size[1] == 0x38
        && resource_size[2] == 0
        && resource_size[3] == 0x188
        && stream_seek_calls == 4
        && stream_seek_offset[0] == 0x3c
        && stream_seek_offset[1] == 4
        && stream_seek_offset[2] == 0x18c
        && stream_seek_offset[3] == 0
        && stream_seek_origin[3] == 0
        && stream_read_calls == 13
        && stream_close_calls == 1
        && memory_error_calls == 0
        && records[0].count == 1
        && records[0].type == 0xa
        && records[0].data == data_a
        && records[1].count == 0
        && records[1].type == 0xb
        && records[1].data == data_b
        && records[2].count == 7
        && records[2].type == 0xc
        && records[2].data == data_c;
}

int main(void)
{
    struct spline_record *table;
    int result;

    reset();
    add_spline(2, 0x11);
    add_spline(5, 0x22);
    resource_result[0] = (void *)records;
    resource_result[1] = (void *)data_a;
    resource_result[2] = (void *)data_b;
    table = 0;
    result = spline_load(archive_text, name_text, &table);
    put(normal() && result == 2 && table == records ? '0' : 'x');

    reset();
    resource_result[0] = (void *)records;
    table = 0;
    result = spline_load(archive_text, name_text, &table);
    put(empty() && result == 0 && table == records ? '1' : 'x');

    reset();
    fopen_result[0] = -1;
    table = 0;
    result = spline_load(archive_text, name_text, &table);
    put(failed_open() && result == 0 ? '2' : 'x');

    reset();
    add_spline(1, 0x33);
    resource_result[0] = (void *)records;
    resource_result[1] = 0;
    table = 0;
    result = spline_load(archive_text, name_text, &table);
    put(zero_data() && result == 1 && table == records ? '3' : 'x');

    reset();
    add_spline(1, 0xa);
    add_spline(0, 0xb);
    add_spline(7, 0xc);
    resource_result[0] = (void *)records;
    resource_result[1] = (void *)data_a;
    resource_result[2] = (void *)data_b;
    resource_result[3] = (void *)data_c;
    table = 0;
    result = spline_load(archive_text, name_text, &table);
    put(varied() && result == 3 && table == records ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
