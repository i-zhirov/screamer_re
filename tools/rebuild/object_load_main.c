/* The test harness for the rebuilt `object_load` module.
 *
 * The harness holds four 0x1c-byte object records, the eight-slot handle table,
 * the two name buffers, and the stubs.  The stubs record their arguments and
 * return configured results.  The harness runs six paths:
 *
 *     0  the success path on slot 0 fills the name buffers, opens the member,
 *        reads the header, clears the six block words, stores the file in the
 *        handle table, binds the block, and returns 0
 *     1  the free-slot path starts at slot 1 and skips the record whose flag
 *        is not 1
 *     2  an open failure returns 2 and calls no resource, read, close, or
 *        init stub
 *     3  a missing `3D_HEADER` resource returns 3 before the read
 *     4  a missing `3D_BLKOBJ` resource returns 5 after the read and leaves
 *        the block words
 *     5  a record whose flag is not 1 returns 0 and calls no stub
 *     D  the exit marker
 *
 * The resource stub selects the first call for `3D_HEADER` and the second for
 * `3D_BLKOBJ`, because the module requests them in that order.  The stub also
 * checks that the two name pointers differ, so a swapped or repeated literal
 * fails the check.  The name, path, and output strings are built at run time
 * from character constants, so the checks do not depend on the literal data
 * that the DOS/4G loader maps at run time.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

int object_load(char *objects, int start, int count);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007f80[0x34];
char DAT_00007fe8[0x34];
int DAT_00007f7c;
int *DAT_00021540;
void *DAT_0000801c;

static char text[8];
static unsigned char records[4 * 0x1c];
static int handles[8];
static unsigned char header_data[0x100];
static unsigned char block_data[0x100];
static char name_expected[0x40];
static char dotted_expected[0x40];
static char path_expected[0x34];

static void *sprintf_buffer;
static char sprintf_text[0x40];
static char *sprintf_name;
static int sprintf_calls;

static char *copy_destination;
static char *copy_source;
static int copy_calls;

static char *status_stream;
static int status_result;
static int status_calls;

static void *resource_list;
static int resource_name[8];
static int resource_size[8];
static int resource_calls;
static void *header_result;
static void *block_result;

static char *read_stream;
static char *read_buffer;
static int read_count;
static int read_calls;

static char *close_stream;
static int close_calls;

static void *init_object;
static int init_index;
static int init_calls;

static char *open_stream;
static char open_path_text[0x34];
static char open_name_text[0x40];
static char open_category_text[8];
static int open_result;
static int open_calls;

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

static void build_texts(char a, char b, char c, char d)
{
    name_expected[0] = a;
    name_expected[1] = b;
    name_expected[2] = c;
    name_expected[3] = d;
    name_expected[4] = 0;
    dotted_expected[0] = a;
    dotted_expected[1] = b;
    dotted_expected[2] = c;
    dotted_expected[3] = d;
    dotted_expected[4] = '.';
    dotted_expected[5] = '3';
    dotted_expected[6] = 'D';
    dotted_expected[7] = 'X';
    dotted_expected[8] = 0;
}

static void build_path(void)
{
    path_expected[0] = 'G';
    path_expected[1] = 'F';
    path_expected[2] = 'X';
    path_expected[3] = 0;
}

static void set_name(int record, char a, char b, char c, char d)
{
    char *pointer;

    pointer = (char *)(records + record * 0x1c);
    pointer[0] = a;
    pointer[1] = b;
    pointer[2] = c;
    pointer[3] = d;
    pointer[4] = 0;
}

static void set_flag(int record, int flag)
{
    *(int *)(records + record * 0x1c + 0x14) = flag;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *value;

    (void)format;
    va_start(arguments, format);
    value = va_arg(arguments, char *);
    va_end(arguments);
    copy(buffer, value);
    append(buffer, '.', '3', 'D', 'X');
    sprintf_buffer = buffer;
    copy(sprintf_text, buffer);
    sprintf_name = value;
    sprintf_calls = sprintf_calls + 1;
    return 0;
}

char *strcpy(char *destination, char *source)
{
    copy(destination, source);
    copy_destination = destination;
    copy_source = source;
    copy_calls = copy_calls + 1;
    return destination;
}

int stream_status(char *stream)
{
    status_stream = stream;
    status_calls = status_calls + 1;
    return status_result;
}

void *resource_get(void *resource, char *name, int size)
{
    int call;

    call = resource_calls;
    resource_list = resource;
    resource_name[call] = (int)name;
    resource_size[call] = size;
    resource_calls = resource_calls + 1;
    if (call == 0) {
        return header_result;
    }
    return block_result;
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

void object_init_from_file(void *object, int index)
{
    init_object = object;
    init_index = index;
    init_calls = init_calls + 1;
}

int fopen(char *stream, char *path, char *name, char *category)
{
    open_stream = stream;
    copy(open_path_text, path);
    copy(open_name_text, name);
    copy(open_category_text, category);
    open_calls = open_calls + 1;
    return open_result;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 4 * 0x1c; ++index) {
        records[index] = 0;
    }
    for (index = 0; index < 8; ++index) {
        handles[index] = 0;
    }
    for (index = 0; index < 0x100; ++index) {
        header_data[index] = 0;
        block_data[index] = 0x5a;
    }
    for (index = 0; index < 0x34; ++index) {
        DAT_00007f80[index] = 0;
        DAT_00007fe8[index] = 0;
    }
    DAT_00007f80[0] = 'G';
    DAT_00007f80[1] = 'F';
    DAT_00007f80[2] = 'X';
    DAT_00007fe8[0] = 'O';
    DAT_00007fe8[1] = 'L';
    DAT_00007fe8[2] = 'D';
    build_path();
    DAT_00007f7c = 0x11111111;
    DAT_00021540 = handles;
    DAT_0000801c = (void *)0x22222222;
    header_result = header_data;
    block_result = block_data;
    *(int *)(header_data + 0x14) = 2;

    sprintf_buffer = 0;
    sprintf_text[0] = 0;
    sprintf_name = 0;
    sprintf_calls = 0;
    copy_destination = 0;
    copy_source = 0;
    copy_calls = 0;
    status_stream = 0;
    status_result = 0x1234;
    status_calls = 0;
    resource_list = 0;
    resource_name[0] = 0;
    resource_name[1] = 0;
    resource_size[0] = 0;
    resource_size[1] = 0;
    resource_calls = 0;
    read_stream = 0;
    read_buffer = 0;
    read_count = 0;
    read_calls = 0;
    close_stream = 0;
    close_calls = 0;
    init_object = 0;
    init_index = 0;
    init_calls = 0;
    open_stream = 0;
    open_path_text[0] = 0;
    open_name_text[0] = 0;
    open_category_text[0] = 0;
    open_result = 0;
    open_calls = 0;
}

static int block_cleared(void)
{
    return *(int *)(block_data + 0x24) == 0
        && *(int *)(block_data + 0x28) == 0
        && *(int *)(block_data + 0x2c) == 0
        && *(int *)(block_data + 0x30) == 0
        && *(int *)(block_data + 0x34) == 0
        && *(int *)(block_data + 0x38) == 0;
}

static int block_untouched(void)
{
    return *(int *)(block_data + 0x24) == 0x5a5a5a5a
        && *(int *)(block_data + 0x28) == 0x5a5a5a5a
        && *(int *)(block_data + 0x2c) == 0x5a5a5a5a
        && *(int *)(block_data + 0x30) == 0x5a5a5a5a
        && *(int *)(block_data + 0x34) == 0x5a5a5a5a
        && *(int *)(block_data + 0x38) == 0x5a5a5a5a;
}

int main(void)
{
    int result;

    reset();
    build_texts('C', '0', '0', '1');
    handles[0] = -1;
    set_name(0, 'C', '0', '0', '1');
    set_flag(0, 1);
    result = object_load((char *)records, 0, 1);
    put(result == 0 && DAT_00007f7c == 1
        && handles[0] == (int)header_data && handles[1] == -1
        && *(void **)(records + 0x0c) == (void *)block_data
        && *(void **)(records + 0x14) == (void *)header_data
        && *(void **)(records + 0x10) == (void *)(block_data + 0x40)
        && block_cleared()
        && same(DAT_00007fe8, name_expected)
        && sprintf_calls == 1 && same(sprintf_text, dotted_expected)
        && copy_calls == 1 && copy_destination == DAT_00007fe8
        && copy_source == (char *)records
        && open_calls == 1 && open_stream != 0
        && same(open_path_text, path_expected)
        && same(open_name_text, dotted_expected)
        && status_calls == 1 && status_stream == open_stream
        && resource_calls == 2 && resource_list == DAT_0000801c
        && resource_name[0] != 0 && resource_name[1] != 0
        && resource_name[0] != resource_name[1]
        && resource_size[0] == 0x1234
        && resource_size[1] == 2 * 0x20 + 0x60
        && read_calls == 1 && read_stream == open_stream
        && read_buffer == (char *)header_data && read_count == 0x1234
        && close_calls == 1 && close_stream == open_stream
        && init_calls == 1 && init_object == (void *)block_data
        && init_index == 0 ? '0' : 'x');

    reset();
    build_texts('C', '0', '0', '2');
    handles[0] = 0;
    handles[1] = -1;
    set_name(0, 'S', 'K', 'P', '0');
    set_flag(0, 0);
    set_name(1, 'C', '0', '0', '2');
    set_flag(1, 1);
    result = object_load((char *)records, 0, 2);
    put(result == 0 && DAT_00007f7c == 2
        && handles[1] == (int)header_data && handles[2] == -1
        && *(void **)(records + 0x0c) == 0
        && *(int *)(records + 0x14) == 0
        && *(void **)(records + 0x1c + 0x0c) == (void *)block_data
        && same(DAT_00007fe8, name_expected)
        && sprintf_calls == 1 && open_calls == 1
        && init_calls == 1 && init_index == 1 ? '1' : 'x');

    reset();
    handles[0] = -1;
    set_name(0, 'B', 'A', 'D', '0');
    set_flag(0, 1);
    open_result = -1;
    result = object_load((char *)records, 0, 1);
    put(result == 2 && open_calls == 1
        && status_calls == 0 && resource_calls == 0 && read_calls == 0
        && close_calls == 0 && init_calls == 0
        && DAT_00007f7c == 0 && handles[0] == -1
        && *(void **)(records + 0x0c) == 0 ? '2' : 'x');

    reset();
    handles[0] = -1;
    set_name(0, 'H', 'D', '0', '0');
    set_flag(0, 1);
    header_result = 0;
    result = object_load((char *)records, 0, 1);
    put(result == 3 && open_calls == 1 && status_calls == 1
        && resource_calls == 1 && read_calls == 0 && close_calls == 0
        && init_calls == 0 && *(void **)(records + 0x0c) == 0 ? '3' : 'x');

    reset();
    handles[0] = -1;
    set_name(0, 'B', 'L', 'K', '0');
    set_flag(0, 1);
    block_result = 0;
    result = object_load((char *)records, 0, 1);
    put(result == 5 && open_calls == 1 && status_calls == 1
        && resource_calls == 2 && read_calls == 1 && close_calls == 1
        && init_calls == 0 && *(void **)(records + 0x0c) == 0
        && block_untouched() && DAT_00007f7c == 0 && handles[0] == -1
        ? '4' : 'x');

    reset();
    handles[0] = -1;
    set_name(0, 'L', 'A', 'T', 'E');
    set_flag(0, 2);
    result = object_load((char *)records, 0, 1);
    put(result == 0 && sprintf_calls == 0 && open_calls == 0
        && status_calls == 0 && resource_calls == 0 && read_calls == 0
        && close_calls == 0 && init_calls == 0
        && DAT_00007f7c == 0 && handles[0] == -1
        && *(void **)(records + 0x0c) == 0 ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
