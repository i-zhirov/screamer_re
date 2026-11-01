/* The test harness for the rebuilt `controls_load` module.
 *
 * The harness holds the base path, the three buffers, the format stub, the file
 * stubs, and the handle stub.  The stubs record their arguments and their call
 * order.  The harness runs the successful open and then the failed open:
 *
 *     0  the success path built the path, opened it, read the three blocks
 *        with the sizes and counts of the original, and closed the handle
 *     1  the failed open keeps the zero handle, still reads the three blocks
 *        with the zero handle, and closes the zero handle
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void controls_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007edc[0x20];
char DAT_000197d8[0xb0];
char DAT_000192a4[0x500];
char DAT_00019704[0x40];

static char text[8];
static char *sprintf_buffer;
static char *sprintf_format;
static char *sprintf_path;
static char *got_path;
static char *got_mode;
static void *read_buffer[4];
static int read_size[4];
static int read_count[4];
static int read_stream[4];
static int got_close;
static int open_result;
static int format_calls;
static int open_calls;
static int read_calls;
static int close_calls;

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

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *path;

    va_start(arguments, format);
    path = va_arg(arguments, char *);
    va_end(arguments);
    sprintf_buffer = buffer;
    sprintf_format = (char *)format;
    sprintf_path = path;
    copy(buffer, "C:\\GAME\\config\\controls.cfg");
    format_calls = format_calls + 1;
    return 31;
}

int fopen(char *path, char *mode)
{
    got_path = path;
    got_mode = mode;
    open_calls = open_calls + 1;
    return open_result;
}

int fread(void *buffer, int size, int count, int stream)
{
    if (read_calls < 4) {
        read_buffer[read_calls] = buffer;
        read_size[read_calls] = size;
        read_count[read_calls] = count;
        read_stream[read_calls] = stream;
    }
    read_calls = read_calls + 1;
    return count;
}

int fclose(int stream)
{
    got_close = stream;
    close_calls = close_calls + 1;
    return 0;
}

static void reset(void)
{
    format_calls = 0;
    open_calls = 0;
    read_calls = 0;
    close_calls = 0;
    sprintf_buffer = 0;
    sprintf_format = 0;
    sprintf_path = 0;
    got_path = 0;
    got_mode = 0;
    got_close = 0;
    open_result = 0;
    {
        int index;

        for (index = 0; index < 4; ++index) {
            read_buffer[index] = 0;
            read_size[index] = 0;
            read_count[index] = 0;
            read_stream[index] = 0;
        }
    }
}

int main(void)
{
    copy(DAT_00007edc, "D:\\SCREAMER");

    reset();
    open_result = 0x1234;
    controls_load();
    put(same(sprintf_format, "%s\\config\\controls.cfg")
        && sprintf_path == DAT_00007edc
        && same(got_path, "C:\\GAME\\config\\controls.cfg")
        && same(got_mode, "rb")
        && format_calls == 1 && open_calls == 1
        && read_calls == 3
        && read_buffer[0] == DAT_000197d8
        && read_size[0] == 0x1c && read_count[0] == 6
        && read_stream[0] == 0x1234
        && read_buffer[1] == DAT_000192a4
        && read_size[1] == 0x1c && read_count[1] == 0x28
        && read_stream[1] == 0x1234
        && read_buffer[2] == DAT_00019704
        && read_size[2] == 4 && read_count[2] == 0xa
        && read_stream[2] == 0x1234
        && close_calls == 1 && got_close == 0x1234 ? '0' : 'x');

    reset();
    open_result = 0;
    controls_load();
    put(open_calls == 1 && read_calls == 3
        && read_stream[0] == 0 && read_stream[1] == 0 && read_stream[2] == 0
        && close_calls == 1 && got_close == 0 ? '1' : 'x');

    put('D');
    quit();
    return 0;
}
