/* The test harness for the rebuilt `stream_read_text` module.
 *
 * The harness holds one stream record, one destination buffer, one source
 * buffer, one expected buffer, and the three stubs.  The stubs record their
 * arguments and copy the scripted text.
 *
 * The harness runs ten paths:
 *
 *     0  the unbuffered line mode calls `fgets` with the count 0x100 and adds
 *        the text length plus one to the position
 *     1  the unbuffered token mode calls `fscanf`
 *     2  the buffered line mode stops at the newline and clears the terminator
 *     3  the buffered token mode stops at the space
 *     4  the buffered token mode stops at the tab
 *     5  the buffered line mode keeps the space
 *     6  the buffered token mode drops the carriage return
 *     7  the position at or past the size clamps to the size, writes the
 *        status 4, and returns -1
 *     8  the buffered token mode stops at a leading space with an empty result
 *     9  the buffered line mode stops at a leading newline with an empty result
 *     D  the exit marker
 *
 * The text arrives through explicit byte stores, because a string literal in
 * this minimal image reads the constant segment, which the harness does not
 * load.  The markers go through the data buffer `text`, because the `print`
 * helper of this harness writes garbage for a string literal (the same fault
 * shows in the output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

int stream_read_text(void *stream, char *buffer, int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char record[0xc0];
static char buffer[0x100];
static char source[0x100];
static char line_text[0x40];
static char token_text[0x40];
static char expected[0x40];

static int fgets_calls;
static char *fgets_dest;
static int fgets_count;
static void *fgets_stream;

static int fscanf_calls;
static char *fscanf_dest;
static void *fscanf_stream;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void fill(char *destination, char a, char b, char c, char d,
                 char e, char f, char g, char h)
{
    destination[0] = a;
    destination[1] = b;
    destination[2] = c;
    destination[3] = d;
    destination[4] = e;
    destination[5] = f;
    destination[6] = g;
    destination[7] = h;
    destination[8] = 0;
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

char *fgets(char *destination, int count, void *stream)
{
    int index;

    fgets_dest = destination;
    fgets_count = count;
    fgets_stream = stream;
    fgets_calls = fgets_calls + 1;
    for (index = 0; line_text[index] != 0; ++index) {
        destination[index] = line_text[index];
    }
    destination[index] = 0;
    return destination;
}

int fscanf(void *stream, const char *format, ...)
{
    va_list arguments;
    char *destination;
    int index;

    va_start(arguments, format);
    destination = va_arg(arguments, char *);
    va_end(arguments);
    fscanf_dest = destination;
    fscanf_stream = stream;
    fscanf_calls = fscanf_calls + 1;
    for (index = 0; token_text[index] != 0; ++index) {
        destination[index] = token_text[index];
    }
    destination[index] = 0;
    return 1;
}

int strlen(const char *value)
{
    int index;

    index = 0;
    while (value[index] != 0) {
        index = index + 1;
    }
    return index;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0xc0; index++) {
        record[index] = 0;
    }
    for (index = 0; index < 0x100; index++) {
        buffer[index] = 0x5a;
        source[index] = 0;
    }
    for (index = 0; index < 0x40; index++) {
        line_text[index] = 0;
        token_text[index] = 0;
        expected[index] = 0;
    }
    fgets_calls = 0;
    fgets_dest = 0;
    fgets_count = 0;
    fgets_stream = 0;
    fscanf_calls = 0;
    fscanf_dest = 0;
    fscanf_stream = 0;
}

static void set_record(int size, int position, int buffered, int index,
                       char *data)
{
    *(int *)(record + 0x00) = 0x1234;
    *(int *)(record + 0x08) = size;
    *(int *)(record + 0x0c) = position;
    *(int *)(record + 0x14) = buffered;
    *(int *)(record + 0x18) = index;
    *(void **)(record + 0x1c) = data;
    *(int *)(record + 0xac) = 0x5a5a5a5a;
}

int main(void)
{
    int ok;
    int ret;

    reset();
    set_record(0x1000, 0, 0, 0, 0);
    fill(line_text, 'h', 'e', 'l', 'l', 'o', '\n', 0, 0);
    ret = stream_read_text(record, buffer, 1);
    fill(expected, 'h', 'e', 'l', 'l', 'o', '\n', 0, 0);
    ok = ret == 0
        && same(buffer, expected)
        && fgets_calls == 1
        && fgets_dest == buffer
        && fgets_count == 0x100
        && fgets_stream == (void *)0x1234
        && fscanf_calls == 0
        && *(int *)(record + 0x0c) == 7;
    put(ok ? '0' : 'x');

    reset();
    set_record(0x1000, 0, 0, 0, 0);
    fill(token_text, 't', 'o', 'k', 'e', 'n', 0, 0, 0);
    ret = stream_read_text(record, buffer, 0);
    fill(expected, 't', 'o', 'k', 'e', 'n', 0, 0, 0);
    ok = ret == 0
        && same(buffer, expected)
        && fscanf_calls == 1
        && fscanf_dest == buffer
        && fscanf_stream == (void *)0x1234
        && fgets_calls == 0
        && *(int *)(record + 0x0c) == 6;
    put(ok ? '1' : 'x');

    reset();
    fill(source, 'a', 'b', 'c', '\n', 'd', 'e', 'f', 0);
    set_record(0x100, 0, 1, 0, source);
    ret = stream_read_text(record, buffer, 1);
    fill(expected, 'a', 'b', 'c', 0, 0, 0, 0, 0);
    ok = ret == 0
        && same(buffer, expected)
        && *(int *)(record + 0x0c) == 4
        && *(int *)(record + 0x18) == 4
        && *(int *)(record + 0xac) == 0x5a5a5a5a
        && fgets_calls == 0
        && fscanf_calls == 0;
    put(ok ? '2' : 'x');

    reset();
    fill(source, 'a', 'b', 'c', ' ', 'd', 'e', 'f', 0);
    set_record(0x100, 0, 1, 0, source);
    ret = stream_read_text(record, buffer, 0);
    fill(expected, 'a', 'b', 'c', 0, 0, 0, 0, 0);
    ok = ret == 0
        && same(buffer, expected)
        && *(int *)(record + 0x0c) == 4
        && *(int *)(record + 0x18) == 4;
    put(ok ? '3' : 'x');

    reset();
    fill(source, 'a', 'b', 'c', '\t', 'd', 'e', 'f', 0);
    set_record(0x100, 0, 1, 0, source);
    ret = stream_read_text(record, buffer, 0);
    fill(expected, 'a', 'b', 'c', 0, 0, 0, 0, 0);
    ok = ret == 0
        && same(buffer, expected)
        && *(int *)(record + 0x0c) == 4
        && *(int *)(record + 0x18) == 4;
    put(ok ? '4' : 'x');

    reset();
    fill(source, 'a', ' ', 'b', '\n', 0, 0, 0, 0);
    set_record(0x100, 0, 1, 0, source);
    ret = stream_read_text(record, buffer, 1);
    fill(expected, 'a', ' ', 'b', 0, 0, 0, 0, 0);
    ok = ret == 0
        && same(buffer, expected)
        && *(int *)(record + 0x0c) == 4
        && *(int *)(record + 0x18) == 4;
    put(ok ? '5' : 'x');

    reset();
    fill(source, 'a', 'b', '\r', 'c', 'd', '\n', 0, 0);
    set_record(0x100, 0, 1, 0, source);
    ret = stream_read_text(record, buffer, 0);
    fill(expected, 'a', 'b', 'c', 'd', 0, 0, 0, 0);
    ok = ret == 0
        && same(buffer, expected)
        && *(int *)(record + 0x0c) == 6
        && *(int *)(record + 0x18) == 6;
    put(ok ? '6' : 'x');

    reset();
    set_record(10, 12, 1, 0, source);
    ret = stream_read_text(record, buffer, 0);
    ok = ret == -1
        && *(int *)(record + 0x0c) == 10
        && *(int *)(record + 0xac) == 4
        && fgets_calls == 0
        && fscanf_calls == 0;
    put(ok ? '7' : 'x');

    reset();
    fill(source, ' ', 'x', 0, 0, 0, 0, 0, 0);
    set_record(0x100, 0, 1, 0, source);
    ret = stream_read_text(record, buffer, 0);
    fill(expected, 0, 0, 0, 0, 0, 0, 0, 0);
    ok = ret == 0
        && same(buffer, expected)
        && *(int *)(record + 0x0c) == 1
        && *(int *)(record + 0x18) == 1;
    put(ok ? '8' : 'x');

    reset();
    fill(source, '\n', 'r', 'e', 's', 't', 0, 0, 0);
    set_record(0x100, 0, 1, 0, source);
    ret = stream_read_text(record, buffer, 1);
    fill(expected, 0, 0, 0, 0, 0, 0, 0, 0);
    ok = ret == 0
        && same(buffer, expected)
        && *(int *)(record + 0x0c) == 1
        && *(int *)(record + 0x18) == 1;
    put(ok ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
