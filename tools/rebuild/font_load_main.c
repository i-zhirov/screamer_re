/* The test harness for the rebuilt `font_load` module.
 *
 * The harness holds the font count, the descriptor table, the bitmap table,
 * the tail size, the tail pointer, and the engine pointer.  The stubs record
 * their arguments and return the configured results.  The harness runs three
 * paths:
 *
 *     0  the success path with the count 1 reads the two descriptors, the two
 *        glyph rows of 24 and 16 bytes, the tail size 8, and the `FONTS`
 *        resource, then closes the stream and increments the count
 *     1  the open-failure path runs the file error report and the exit with
 *        the code 0
 *     2  the zero-resource path runs the memory error report and skips the
 *        tail read
 *     D  the exit marker
 *
 * The path, the member, the expected texts, and the markers are built at run
 * time, because the DOS/4G loader maps the initialized data pages as zero.
 *
 * The markers go through the data buffer `text`, because the `print` helper
 * of this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void font_load(char *path, char *member);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00015440;
void *DAT_0001543c;
int DAT_001ca98[0x40];
char DAT_00019898[0x500 * 4];
int DAT_001cb10;
void *DAT_000062ac;

static char text[8];
static char path_text[8];
static char member_text[8];
static char expected_mode[4];
static char expected_resource[8];
static char tail_buffer[0x40];

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
static int descriptor_calls;

static void *resource_resource;
static char resource_name[0x40];
static int resource_size;
static void *resource_result;
static int resource_calls;

static void *memory_report_resource;
static int memory_report_calls;

static char *close_stream;
static int close_calls;

static int script_count;
static int script_descriptor[12];
static int script_tail_size;

static void build_texts(void)
{
    path_text[0] = 'P';
    path_text[1] = 0;
    member_text[0] = 'M';
    member_text[1] = 0;
    expected_mode[0] = 'r';
    expected_mode[1] = 'b';
    expected_mode[2] = 0;
    expected_resource[0] = 'F';
    expected_resource[1] = 'O';
    expected_resource[2] = 'N';
    expected_resource[3] = 'T';
    expected_resource[4] = 'S';
    expected_resource[5] = 0;
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

static void reset(void)
{
    int index;

    DAT_00015440 = 0;
    DAT_0001543c = (void *)0x1234;
    for (index = 0; index < 0x40; index++) {
        DAT_001ca98[index] = 0;
    }
    for (index = 0; index < 0x500 * 4; index++) {
        DAT_00019898[index] = 0;
    }
    DAT_001cb10 = 0;
    DAT_000062ac = (void *)0x1000;

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

    read_stream[0] = 0;
    read_buffer[0] = 0;
    read_count[0] = 0;
    read_calls = 0;
    descriptor_calls = 0;

    resource_resource = 0;
    resource_name[0] = 0;
    resource_size = 0;
    resource_result = 0;
    resource_calls = 0;

    memory_report_resource = 0;
    memory_report_calls = 0;

    close_stream = 0;
    close_calls = 0;

    script_count = 0;
    script_descriptor[0] = 0;
    script_descriptor[1] = 0;
    script_descriptor[2] = 0;
    script_descriptor[3] = 0;
    script_descriptor[4] = 0;
    script_descriptor[5] = 0;
    script_tail_size = 0;
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
    int base;

    read_stream[read_calls] = stream;
    read_buffer[read_calls] = buffer;
    read_count[read_calls] = count;
    read_calls = read_calls + 1;

    if (buffer == (char *)&DAT_00015440) {
        *(char *)buffer = (char)script_count;
    } else if (buffer == (char *)&DAT_001cb10) {
        DAT_001cb10 = script_tail_size;
    } else if (buffer >= (char *)DAT_001ca98
               && buffer < (char *)(DAT_001ca98 + 0x40)) {
        base = descriptor_calls * 3;
        ((int *)buffer)[0] = script_descriptor[base];
        ((int *)buffer)[1] = script_descriptor[base + 1];
        ((int *)buffer)[2] = script_descriptor[base + 2];
        descriptor_calls = descriptor_calls + 1;
    } else if (buffer != 0) {
        for (index = 0; index < count; index++) {
            buffer[index] = 'T';
        }
    }
    return count;
}

void *resource_get(void *resource, char *name, int size)
{
    resource_resource = resource;
    copy(resource_name, name);
    resource_size = size;
    resource_calls = resource_calls + 1;
    return resource_result;
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
    build_texts();

    reset();
    script_count = 1;
    script_descriptor[0] = 22;
    script_descriptor[1] = 32;
    script_descriptor[2] = 3;
    script_descriptor[3] = 18;
    script_descriptor[4] = 32;
    script_descriptor[5] = 2;
    script_tail_size = 8;
    resource_result = tail_buffer;
    font_load(path_text, member_text);
    put(open_calls == 1 && same(open_path, path_text)
        && same(open_name, member_text)
        && same(open_category, expected_mode)
        && file_report_calls == 0 && exit_calls == 0
        && read_calls == 7
        && read_stream[0] == open_stream && read_stream[6] == open_stream
        && DAT_00015440 == 2
        && DAT_001ca98[0] == 22 && DAT_001ca98[1] == 32
        && DAT_001ca98[2] == 3
        && DAT_001ca98[3] == 18 && DAT_001ca98[4] == 32
        && DAT_001ca98[5] == 2
        && read_count[3] == 24 && read_count[4] == 16
        && read_buffer[3] == DAT_00019898
        && read_buffer[4] == DAT_00019898 + 0x500
        && resource_calls == 1 && resource_resource == DAT_000062ac
        && same(resource_name, expected_resource) && resource_size == 8
        && DAT_0001543c == (void *)tail_buffer
        && tail_buffer[0] == 'T' && tail_buffer[7] == 'T'
        && memory_report_calls == 0
        && close_calls == 1 && close_stream == open_stream ? '0' : 'x');

    reset();
    open_result = 1;
    resource_result = tail_buffer;
    font_load(path_text, member_text);
    put(open_calls == 1 && file_report_stream_match
        && file_report_calls == 1
        && exit_calls == 1 && exit_code == 0
        && close_calls == 1 && DAT_00015440 == 1 ? '1' : 'x');

    reset();
    resource_result = 0;
    font_load(path_text, member_text);
    put(memory_report_calls == 1
        && memory_report_resource == DAT_000062ac
        && resource_calls == 1 && resource_size == 0
        && DAT_0001543c == 0 && close_calls == 1 ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
