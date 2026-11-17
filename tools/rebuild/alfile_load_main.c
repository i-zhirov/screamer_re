/* The test harness for the rebuilt `alfile_load` module.
 *
 * The harness holds the stream record, the path and name arguments, the
 * resource head, the resource buffer, and the out cell.  The stubs record
 * their arguments and their call counts.
 *
 * The harness runs four paths:
 *
 *     0  the full path with the member size 0x40: the open with `rb`, the
 *        `ALFILE` resource of the size plus 4, the member read, and the close
 *     1  the open fails: the function returns -1 and keeps the out cell and
 *        the stream state, and it calls no other helper
 *     2  the resource is zero: the stream state becomes 3, the function
 *        returns -1, and it does not read or close
 *     3  the member size is zero: the resource size is 4, the read count is
 *        zero, and the function returns 0
 *     D  the exit marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, and the module build
 * defines `ALFILE_LOAD_HARNESS` for the same reason.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

int alfile_load(char *stream, char *path, char *name, void **out,
                void *resource);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STREAM_SIZE 0xb4
#define STREAM_STATUS 0x70
#define STREAM_STATE 0xac

static char text[8];
static char stream_buffer[STREAM_SIZE];
static char path_text[0x20];
static char name_text[0x20];
static char resource_buffer[0x100];
static char expect_mode[8];
static char expect_resource[8];
static void *out_cell;
static void *resource_head;
static int member_size;
static int state_before;
static int result;

static char *open_stream;
static char open_path[0x20];
static char open_name[0x20];
static char open_mode[8];
static int open_calls;
static int open_result;

static void *resource_argument;
static char resource_name[0x10];
static int resource_size;
static int resource_calls;
static void *resource_result;

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

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
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

static int state(void)
{
    return *(int *)(stream_buffer + STREAM_STATE);
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

void *resource_get(void *resource, char *name, int size)
{
    resource_argument = resource;
    copy(resource_name, name);
    resource_size = size;
    resource_calls = resource_calls + 1;
    return resource_result;
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

    for (index = 0; index < STREAM_SIZE; ++index) {
        stream_buffer[index] = 0;
    }
    *(int *)(stream_buffer + STREAM_STATUS) = member_size;
    *(int *)(stream_buffer + STREAM_STATE) = state_before;
    out_cell = (void *)0x22220000;
    resource_head = (void *)0x33330000;
    result = 0x1234;

    open_stream = 0;
    open_path[0] = 0;
    open_name[0] = 0;
    open_mode[0] = 0;
    open_calls = 0;
    open_result = 0;

    resource_argument = 0;
    resource_name[0] = 0;
    resource_size = 0;
    resource_calls = 0;
    resource_result = 0;

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
    fill(name_text, 'M', 'A', 'P', '1', '.', 'M', 'M', 'P');
    fill(expect_mode, 'r', 'b', 0, 0, 0, 0, 0, 0);
    fill(expect_resource, 'A', 'L', 'F', 'I', 'L', 'E', 0, 0);
}

static int full_path(void)
{
    return result == 0
        && open_calls == 1
        && open_stream == stream_buffer
        && same(open_path, path_text)
        && same(open_name, name_text)
        && same(open_mode, expect_mode)
        && resource_calls == 1
        && resource_argument == resource_head
        && same(resource_name, expect_resource)
        && resource_size == 0x44
        && out_cell == resource_buffer
        && read_calls == 1
        && read_stream == stream_buffer
        && read_buffer == resource_buffer
        && read_count == 0x40
        && close_calls == 1
        && close_stream == stream_buffer
        && state() == state_before;
}

static int failed_open(void)
{
    return result == -1
        && open_calls == 1
        && same(open_mode, expect_mode)
        && resource_calls == 0
        && out_cell == (void *)0x22220000
        && read_calls == 0
        && close_calls == 0
        && state() == state_before;
}

static int zero_resource(void)
{
    return result == -1
        && open_calls == 1
        && resource_calls == 1
        && same(resource_name, expect_resource)
        && resource_size == 0x24
        && out_cell == 0
        && read_calls == 0
        && close_calls == 0
        && state() == 3;
}

static int zero_size(void)
{
    return result == 0
        && open_calls == 1
        && resource_calls == 1
        && resource_size == 4
        && out_cell == resource_buffer
        && read_calls == 1
        && read_buffer == resource_buffer
        && read_count == 0
        && close_calls == 1
        && close_stream == stream_buffer;
}

int main(void)
{
    build_texts();

    member_size = 0x40;
    state_before = 0;
    reset();
    resource_result = resource_buffer;
    result = alfile_load(stream_buffer, path_text, name_text, &out_cell,
                         resource_head);
    put(full_path() ? '0' : 'x');

    member_size = 0x40;
    state_before = 5;
    reset();
    open_result = 1;
    resource_result = resource_buffer;
    result = alfile_load(stream_buffer, path_text, name_text, &out_cell,
                         resource_head);
    put(failed_open() ? '1' : 'x');

    member_size = 0x20;
    state_before = 0;
    reset();
    resource_result = 0;
    result = alfile_load(stream_buffer, path_text, name_text, &out_cell,
                         resource_head);
    put(zero_resource() ? '2' : 'x');

    member_size = 0;
    state_before = 0;
    reset();
    resource_result = resource_buffer;
    result = alfile_load(stream_buffer, path_text, name_text, &out_cell,
                         resource_head);
    put(zero_size() ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
