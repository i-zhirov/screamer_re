/* The test harness for the rebuilt `sprite_load` module.
 *
 * The harness holds the record count, the data size, the two resource
 * pointers, the engine pointer, and the two resource buffers.  The stubs
 * record their arguments and return the configured results.  The harness runs
 * four paths:
 *
 *     0  the success path with two records reads the two counts, the records
 *        of 40 bytes, the data of 10 bytes, and the two `SPRITES` resources,
 *        then closes the stream and returns 0
 *     1  the open-failure path runs the file error report and the exit with
 *        the code 0
 *     2  the zero-record-resource path runs the memory error report and reads
 *        no record buffer
 *     3  the zero-data-resource path runs the memory error report and reads no
 *        data buffer
 *     D  the exit marker
 *
 * The path, the member, the expected texts, and the markers are built at run
 * time, because the DOS/4G loader maps the initialized data pages as zero.
 *
 * The markers go through the data buffer `text`, because the `print` helper
 * of this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void sprite_load(char *path, char *member);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00015cd4;
int DAT_00015cdc;
void *DAT_00015ccc;
void *DAT_00015ce0;
void *DAT_000062ac;

static char text[8];
static char path_text[8];
static char member_text[8];
static char expected_mode[4];
static char expected_resource[8];
static char record_buffer[0x40];
static char data_buffer[0x40];

static char *open_stream;
static char open_path[0x40];
static char open_name[0x40];
static char open_category[0x40];
static int open_result;
static int open_calls;

static int file_report_stream_match;
static int file_report_calls;

static int exit_code;
static int exit_calls;

static char *read_stream[8];
static char *read_buffer[8];
static int read_count[8];
static int read_calls;

static void *resource_resource;
static char resource_name[8][0x40];
static int resource_size[8];
static void *resource_result[8];
static int resource_calls;

static void *memory_report_resource;
static int memory_report_calls;

static char *close_stream;
static int close_calls;

static int script_count;
static int script_size;

static void build_texts(void)
{
    path_text[0] = 'P';
    path_text[1] = 0;
    member_text[0] = 'M';
    member_text[1] = 0;
    expected_mode[0] = 'r';
    expected_mode[1] = 'b';
    expected_mode[2] = 0;
    expected_resource[0] = 'S';
    expected_resource[1] = 'P';
    expected_resource[2] = 'R';
    expected_resource[3] = 'I';
    expected_resource[4] = 'T';
    expected_resource[5] = 'E';
    expected_resource[6] = 'S';
    expected_resource[7] = 0;
}

static void copy(char *target, char *source)
{
    int index;

    index = 0;
    while (source[index] != 0 && index < 0x3f) {
        target[index] = source[index];
        index++;
    }
    target[index] = 0;
}

static int same(char *left, char *right)
{
    int index;

    index = 0;
    while (left[index] != 0 && left[index] == right[index]) {
        index++;
    }
    return left[index] == right[index];
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_bytes(char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static void reset(void)
{
    DAT_00015cd4 = 0;
    DAT_00015cdc = 0;
    DAT_00015ccc = 0;
    DAT_00015ce0 = 0;
    DAT_000062ac = (void *)0x1000;

    clear_bytes(record_buffer, 0x40);
    clear_bytes(data_buffer, 0x40);

    open_stream = 0;
    open_path[0] = 0;
    open_name[0] = 0;
    open_category[0] = 0;
    open_result = 0;
    open_calls = 0;

    file_report_stream_match = 0;
    file_report_calls = 0;

    exit_code = -1;
    exit_calls = 0;

    read_calls = 0;

    resource_resource = 0;
    resource_calls = 0;

    memory_report_resource = 0;
    memory_report_calls = 0;

    close_stream = 0;
    close_calls = 0;

    script_count = 0;
    script_size = 0;
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
    file_report_stream_match = stream == open_stream;
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

    read_stream[read_calls] = stream;
    read_buffer[read_calls] = buffer;
    read_count[read_calls] = count;
    read_calls = read_calls + 1;

    if (buffer == (char *)&DAT_00015cd4) {
        DAT_00015cd4 = script_count;
    } else if (buffer == (char *)&DAT_00015cdc) {
        DAT_00015cdc = script_size;
    } else if (buffer == record_buffer) {
        for (index = 0; index < count; index++) {
            buffer[index] = 'R';
        }
    } else if (buffer == data_buffer) {
        for (index = 0; index < count; index++) {
            buffer[index] = 'D';
        }
    }
    return count;
}

void *resource_get(void *resource, char *name, int size)
{
    int index;

    index = resource_calls;
    resource_resource = resource;
    copy(resource_name[index], name);
    resource_size[index] = size;
    resource_calls = resource_calls + 1;
    return resource_result[index];
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

int main(void)
{
    int ok;
    int index;

    build_texts();

    reset();
    script_count = 2;
    script_size = 10;
    resource_result[0] = record_buffer;
    resource_result[1] = data_buffer;
    sprite_load(path_text, member_text);
    ok = open_calls == 1 && same(open_path, path_text)
        && same(open_name, member_text)
        && same(open_category, expected_mode)
        && file_report_calls == 0 && exit_calls == 0
        && read_calls == 4
        && read_stream[0] == open_stream && read_stream[3] == open_stream
        && read_buffer[0] == (char *)&DAT_00015cd4
        && read_count[0] == 4 && DAT_00015cd4 == 2
        && read_buffer[1] == (char *)&DAT_00015cdc
        && read_count[1] == 4 && DAT_00015cdc == 10
        && resource_calls == 2 && resource_resource == DAT_000062ac
        && same(resource_name[0], expected_resource)
        && same(resource_name[1], expected_resource)
        && resource_size[0] == 40 && resource_size[1] == 10
        && DAT_00015ccc == (void *)record_buffer
        && DAT_00015ce0 == (void *)data_buffer
        && read_buffer[2] == record_buffer && read_count[2] == 40
        && read_buffer[3] == data_buffer && read_count[3] == 10
        && record_buffer[0] == 'R' && record_buffer[39] == 'R'
        && data_buffer[0] == 'D' && data_buffer[9] == 'D'
        && memory_report_calls == 0
        && close_calls == 1 && close_stream == open_stream;
    for (index = 0; index < 40; index++) {
        if (record_buffer[index] != 'R') {
            ok = 0;
        }
    }
    for (index = 0; index < 10; index++) {
        if (data_buffer[index] != 'D') {
            ok = 0;
        }
    }
    put(ok ? '0' : 'x');

    reset();
    open_result = 1;
    sprite_load(path_text, member_text);
    put(open_calls == 1 && file_report_stream_match
        && file_report_calls == 1
        && exit_calls == 1 && exit_code == 0 ? '1' : 'x');

    reset();
    script_count = 2;
    script_size = 10;
    resource_result[0] = 0;
    resource_result[1] = data_buffer;
    sprite_load(path_text, member_text);
    put(memory_report_calls == 1
        && memory_report_resource == DAT_000062ac
        && resource_calls == 2 && resource_size[0] == 40
        && DAT_00015ccc == 0
        && read_buffer[2] == 0 && read_count[2] == 40
        && DAT_00015ce0 == (void *)data_buffer
        && close_calls == 1 ? '2' : 'x');

    reset();
    script_count = 2;
    script_size = 10;
    resource_result[0] = record_buffer;
    resource_result[1] = 0;
    sprite_load(path_text, member_text);
    put(memory_report_calls == 1
        && memory_report_resource == DAT_000062ac
        && resource_calls == 2 && resource_size[1] == 10
        && DAT_00015ccc == (void *)record_buffer
        && DAT_00015ce0 == 0
        && read_buffer[3] == 0 && read_count[3] == 10
        && close_calls == 1 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
