/* The test harness for the rebuilt `track_marker_load` module.
 *
 * The harness holds the base path, the simulated container member, the two
 * name arguments, the marker table, and the stubs.  The stubs record their
 * arguments and their call counts.  The stream stub reads a small simulated
 * member, so the harness checks the count read and the `count * 0x20` table
 * read.
 *
 * The harness runs four paths:
 *
 *     0  the normal path with three markers: the two formatted names, the
 *        open, the four-byte count read, the 0x60-byte table read, the close,
 *        the count 3, and the table bytes
 *     1  an empty table: the count is zero, the first read returns the four
 *        bytes, the table read asks for zero bytes, and the count is 0
 *     2  a failed open: the function returns zero and runs no stream call
 *     3  one marker: the table read asks for 0x20 bytes and fills the table
 *     D  the exit marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, and the module build
 * defines `TRACK_MARKER_LOAD_HARNESS` for the same reason.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

int track_marker_load(char *archive, char *name, void *table);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007f44[0x20];

static char text[8];
static unsigned char table[0x100];

static unsigned char file_data[0x200];
static int file_size;
static int file_pos;

static char archive_text[0x10];
static char name_text[0x10];
static char expect_path_format[0x14];
static char expect_member_format[0x10];
static char expect_mode[0x10];
static char path_mid[0x10];
static char path_tail[0x10];
static char member_tail[0x10];
static char expect_path[0x40];
static char expect_member[0x20];

static char sprintf_path_copy[4][0x40];
static char sprintf_member_copy[4][0x20];
static int sprintf_calls;

static int fopen_calls;
static char fopen_path_copy[4][0x40];
static char fopen_member_copy[4][0x20];
static char fopen_category_copy[4][0x10];
static int fopen_result[4];

static int stream_read_calls;
static void *stream_read_buffer[8];
static int stream_read_size[8];

static int stream_close_calls;

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

static void add_markers(unsigned int count, unsigned int seed)
{
    int index;

    *(unsigned int *)(file_data + file_size) = count;
    file_size = file_size + 4;
    for (index = 0; index < (int)(count * 0x20); ++index) {
        file_data[file_size] = (unsigned char)(seed + index);
        file_size = file_size + 1;
    }
}

static void reset(void)
{
    int index;

    file_size = 0;
    file_pos = 0;
    for (index = 0; index < 0x100; ++index) {
        table[index] = 0;
    }
    fill(DAT_00007f44, 'D', ':', '\\', 'S', 'C', 'R', 'E', 'A');
    fill(DAT_00007f44 + 8, 'M', 'E', 'R', 0, 0, 0, 0, 0);
    fill(archive_text, 'M', 'A', 'P', '1', 'I', 0, 0, 0);
    fill(name_text, 'M', 'A', 'P', '1', 0, 0, 0, 0);
    fill(expect_path_format, '%', 's', '\\', 'd', 'a', 't', 'a', '\\');
    fill(expect_path_format + 8, '%', 's', '.', 'd', 'a', 't', 0, 0);
    fill(expect_member_format, '%', 's', '.', 'm', 'r', 'k', 0, 0);
    fill(expect_mode, 'r', 'b', 0, 0, 0, 0, 0, 0);
    fill(path_mid, '\\', 'd', 'a', 't', 'a', '\\', 0, 0);
    fill(path_tail, '.', 'd', 'a', 't', 0, 0, 0, 0);
    fill(member_tail, '.', 'm', 'r', 'k', 0, 0, 0, 0);
    copy(expect_path, DAT_00007f44);
    append(expect_path, path_mid);
    append(expect_path, archive_text);
    append(expect_path, path_tail);
    copy(expect_member, name_text);
    append(expect_member, member_tail);
    sprintf_calls = 0;
    for (index = 0; index < 4; ++index) {
        sprintf_path_copy[index][0] = 0;
        sprintf_member_copy[index][0] = 0;
        fopen_path_copy[index][0] = 0;
        fopen_member_copy[index][0] = 0;
        fopen_category_copy[index][0] = 0;
        fopen_result[index] = 0;
    }
    fopen_calls = 0;
    stream_read_calls = 0;
    for (index = 0; index < 8; ++index) {
        stream_read_buffer[index] = 0;
        stream_read_size[index] = 0;
    }
    stream_close_calls = 0;
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
        if (index < 4) {
            copy(sprintf_path_copy[index], buffer);
        }
    } else {
        first = va_arg(arguments, char *);
        copy(buffer, first);
        append(buffer, member_tail);
        if (index < 4) {
            copy(sprintf_member_copy[index], buffer);
        }
    }
    va_end(arguments);
    sprintf_calls = index + 1;
    return 0;
}

int fopen(void *stream, char *path, char *member, char *category)
{
    int index;

    index = fopen_calls;
    if (index < 4) {
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
    if (index < 8) {
        stream_read_buffer[index] = buffer;
        stream_read_size[index] = size;
    }
    stream_read_calls = index + 1;
    return take;
}

int stream_close(void *stream)
{
    stream_close_calls = stream_close_calls + 1;
    return 0;
}

static int normal(void)
{
    int result;

    reset();
    add_markers(3, 0x40);
    result = track_marker_load(archive_text, name_text, table);
    return result == 3
        && sprintf_calls == 2
        && same(sprintf_path_copy[0], expect_path)
        && same(sprintf_member_copy[1], expect_member)
        && fopen_calls == 1
        && same(fopen_path_copy[0], expect_path)
        && same(fopen_member_copy[0], expect_member)
        && same(fopen_category_copy[0], expect_mode)
        && stream_read_calls == 2
        && stream_read_size[0] == 4
        && stream_read_size[1] == 0x60
        && stream_read_buffer[1] == table
        && stream_close_calls == 1
        && table[0] == 0x40
        && table[0x1f] == 0x5f
        && table[0x20] == 0x60
        && table[0x5f] == 0x9f;
}

static int empty(void)
{
    int result;

    reset();
    add_markers(0, 0);
    result = track_marker_load(archive_text, name_text, table);
    return result == 0
        && sprintf_calls == 2
        && fopen_calls == 1
        && stream_read_calls == 2
        && stream_read_size[0] == 4
        && stream_read_size[1] == 0
        && stream_close_calls == 1;
}

static int failed_open(void)
{
    int result;

    reset();
    add_markers(2, 0);
    fopen_result[0] = -1;
    result = track_marker_load(archive_text, name_text, table);
    return result == 0
        && sprintf_calls == 2
        && fopen_calls == 1
        && stream_read_calls == 0
        && stream_close_calls == 0;
}

static int single(void)
{
    int result;

    reset();
    add_markers(1, 0x10);
    result = track_marker_load(archive_text, name_text, table);
    return result == 1
        && stream_read_calls == 2
        && stream_read_size[1] == 0x20
        && stream_close_calls == 1
        && table[0] == 0x10
        && table[0x1f] == 0x2f;
}

int main(void)
{
    put(normal() ? '0' : 'a');
    put(empty() ? '1' : 'b');
    put(failed_open() ? '2' : 'c');
    put(single() ? '3' : 'd');
    put('D');
    quit();
    return 0;
}
