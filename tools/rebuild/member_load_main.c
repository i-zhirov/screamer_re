/* The test harness for the rebuilt `member_load` module.
 *
 * The harness holds the stream record, the path and name arguments, the
 * destination buffer, and the three helper stubs.  The stubs record their
 * arguments and their call counts.
 *
 * The harness runs four paths:
 *
 *     0  the full path with the member size 0x40: the open with `rb`, the
 *        member read into the destination buffer, and the close
 *     1  the open fails: the function returns -1, calls no other helper, and
 *        keeps the destination buffer
 *     2  the member size is zero: the read count is zero, the close runs, and
 *        the function returns 0
 *     3  the member size is 0x1234: the read count comes from `stream + 0x70`,
 *        the close runs, and the function returns 0
 *     D  the exit marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, and the module build
 * defines `MEMBER_LOAD_HARNESS` for the same reason.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

int member_load(char *stream, char *path, char *name, char *buffer);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STREAM_SIZE 0xb4
#define STREAM_SIZE_FIELD 0x70

static char text[8];
static char stream_buffer[STREAM_SIZE];
static char path_text[0x20];
static char name_text[0x20];
static char destination[0x100];
static char expect_mode[8];
static int member_size;
static int result;

static char *open_stream;
static char open_path[0x20];
static char open_name[0x20];
static char open_mode[8];
static int open_calls;
static int open_result;

static char *read_stream;
static char *read_buffer;
static int read_count;
static int read_calls;

static char *close_stream;
static int close_calls;

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

static void copy(char *destination_text, char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination_text[index] = source[index];
    }
    destination_text[index] = 0;
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

static void reset(void)
{
    int index;

    for (index = 0; index < STREAM_SIZE; ++index) {
        stream_buffer[index] = 0;
    }
    *(int *)(stream_buffer + STREAM_SIZE_FIELD) = member_size;
    for (index = 0; index < 0x100; ++index) {
        destination[index] = 0;
    }
    result = 0x1234;

    open_stream = 0;
    open_path[0] = 0;
    open_name[0] = 0;
    open_mode[0] = 0;
    open_calls = 0;
    open_result = 0;

    read_stream = 0;
    read_buffer = 0;
    read_count = 0;
    read_calls = 0;

    close_stream = 0;
    close_calls = 0;
}

static void build_texts(void)
{
    fill(path_text, 'D', 'A', 'T', 'A', '\\', 'M', 'A', 'P');
    path_text[8] = 0;
    fill(name_text, 'M', 'A', 'P', '1', '.', 'C', 'O', 'L');
    fill(expect_mode, 'r', 'b', 0, 0, 0, 0, 0, 0);
}

static int full_path(void)
{
    return result == 0
        && open_calls == 1
        && open_stream == stream_buffer
        && same(open_path, path_text)
        && same(open_name, name_text)
        && same(open_mode, expect_mode)
        && read_calls == 1
        && read_stream == stream_buffer
        && read_buffer == destination
        && read_count == 0x40
        && close_calls == 1
        && close_stream == stream_buffer;
}

static int failed_open(void)
{
    return result == -1
        && open_calls == 1
        && same(open_path, path_text)
        && same(open_name, name_text)
        && same(open_mode, expect_mode)
        && read_calls == 0
        && close_calls == 0;
}

static int zero_size(void)
{
    return result == 0
        && open_calls == 1
        && read_calls == 1
        && read_buffer == destination
        && read_count == 0
        && close_calls == 1
        && close_stream == stream_buffer;
}

static int large_size(void)
{
    return result == 0
        && open_calls == 1
        && read_calls == 1
        && read_buffer == destination
        && read_count == 0x1234
        && close_calls == 1
        && close_stream == stream_buffer;
}

int fopen(char *stream, char *path, char *name, char *category)
{
    open_stream = stream;
    copy(open_path, path);
    copy(open_name, name);
    copy(open_mode, category);
    open_calls = open_calls + 1;
    return open_result;
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

int main(void)
{
    build_texts();

    member_size = 0x40;
    reset();
    result = member_load(stream_buffer, path_text, name_text, destination);
    put(full_path() ? '0' : 'x');

    member_size = 0x40;
    reset();
    open_result = 1;
    result = member_load(stream_buffer, path_text, name_text, destination);
    put(failed_open() ? '1' : 'x');

    member_size = 0;
    reset();
    result = member_load(stream_buffer, path_text, name_text, destination);
    put(zero_size() ? '2' : 'x');

    member_size = 0x1234;
    reset();
    result = member_load(stream_buffer, path_text, name_text, destination);
    put(large_size() ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
