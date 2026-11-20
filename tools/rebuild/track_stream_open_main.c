/* The test harness for the rebuilt `track_stream_open` module.
 *
 * The harness holds the engine buffer and its pointer cell PTR_DAT_000bdd20,
 * and the member_load and file_error_report stubs.  The two stubs record their
 * arguments and the configured result.
 *
 * The paths:
 *
 *     0  the path `BASE\gfx\MAP1H.dat`, the member `MAP1.smg`, and a
 *        successful load: one member_load call with the engine buffer, and no
 *        file error report
 *     1  the member_load result 1: the file error report runs with the same
 *        stream record
 *     2  the empty path and member, the third value 0xabcd, and the second
 *        buffer in the pointer cell: the empty texts reach member_load, the
 *        third value does not change the path or the name, the load uses the
 *        second buffer, and the report stays clear
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void track_stream_open(char *path, char *member, int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char *PTR_DAT_000bdd20;

static char text[8];
static char engine_buffer[0x200];
static char other_buffer[0x200];
static char *path_text = "BASE\\gfx\\MAP1H.dat";
static char *member_text = "MAP1.smg";
static char *empty_text = "";

static int load_result;
static int load_calls;
static char *load_stream;
static char *load_path;
static char *load_name;
static char *load_buffer;

static int report_calls;
static char *report_stream;

int member_load(char *stream, char *path, char *name, char *buffer)
{
    load_calls = load_calls + 1;
    load_stream = stream;
    load_path = path;
    load_name = name;
    load_buffer = buffer;
    return load_result;
}

void file_error_report(char *stream)
{
    report_calls = report_calls + 1;
    report_stream = stream;
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

static void check(int number, int ok)
{
    put(ok ? (char)('0' + number) : 'x');
}

static void reset(void)
{
    load_calls = 0;
    load_stream = 0;
    load_path = 0;
    load_name = 0;
    load_buffer = 0;
    report_calls = 0;
    report_stream = 0;
}

int main(void)
{
    int value;

    value = 0;

    reset();
    load_result = 0;
    PTR_DAT_000bdd20 = engine_buffer;
    track_stream_open(path_text, member_text, 0x104);
    check(0, load_calls == 1
        && same(load_path, path_text)
        && same(load_name, member_text)
        && load_buffer == engine_buffer
        && load_stream != 0
        && report_calls == 0);

    reset();
    load_result = 1;
    PTR_DAT_000bdd20 = engine_buffer;
    track_stream_open(path_text, member_text, 0);
    check(1, load_calls == 1
        && same(load_path, path_text)
        && same(load_name, member_text)
        && load_buffer == engine_buffer
        && report_calls == 1
        && report_stream == load_stream);

    reset();
    load_result = 0;
    PTR_DAT_000bdd20 = other_buffer;
    track_stream_open(empty_text, empty_text, 0xabcd);
    check(2, load_calls == 1
        && same(load_path, empty_text)
        && same(load_name, empty_text)
        && load_buffer == other_buffer
        && report_calls == 0);

    put('D');
    quit();
    return value;
}
