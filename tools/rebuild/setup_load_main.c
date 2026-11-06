/* The test harness for the rebuilt `setup_load` module.
 *
 * The harness holds the base path, the config buffer, the format stub, the file
 * stubs, and the two error stubs.  The stubs record their arguments.  The
 * harness runs the successful open and then the failed open:
 *
 *     0  the success path built the path, opened it, read 0x90 bytes, and
 *        closed the handle
 *     1  the failure path reported `Run SETUP before.` and exited with -1
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void setup_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007edc[0x20];
char DAT_00008058[0x90];

static char text[8];
static char *sprintf_buffer;
static char *sprintf_format;
static char *sprintf_path;
static char *got_path;
static char *got_mode;
static void *got_buffer;
static int got_size;
static int got_count;
static int got_stream;
static int got_close;
static char *got_message;
static int got_exit;
static int open_result;
static int format_calls;
static int open_calls;
static int read_calls;
static int close_calls;
static int message_calls;
static int exit_calls;

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
    copy(buffer, "C:\\GAME\\config\\setup.cfg");
    format_calls = format_calls + 1;
    return 24;
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
    got_buffer = buffer;
    got_size = size;
    got_count = count;
    got_stream = stream;
    read_calls = read_calls + 1;
    return count;
}

int fclose(int stream)
{
    got_close = stream;
    close_calls = close_calls + 1;
    return 0;
}

int report_message(char *message)
{
    got_message = message;
    message_calls = message_calls + 1;
    return 0;
}

void exit(int code)
{
    got_exit = code;
    exit_calls = exit_calls + 1;
}

static void reset(void)
{
    format_calls = 0;
    open_calls = 0;
    read_calls = 0;
    close_calls = 0;
    message_calls = 0;
    exit_calls = 0;
    got_buffer = 0;
    got_size = 0;
    got_count = 0;
    got_stream = 0;
    got_close = 0;
    got_message = 0;
    got_exit = 0;
}

int main(void)
{
    copy(DAT_00007edc, "D:\\SCREAMER");

    reset();
    open_result = 0x1234;
    setup_load();
    put(same(sprintf_format, "%s\\config\\setup.cfg")
        && sprintf_path == DAT_00007edc
        && same(got_path, "C:\\GAME\\config\\setup.cfg")
        && same(got_mode, "rb")
        && format_calls == 1 && open_calls == 1
        && read_calls == 1 && got_buffer == DAT_00008058
        && got_size == 1 && got_count == 0x90 && got_stream == 0x1234
        && close_calls == 1 && got_close == 0x1234
        && message_calls == 0 && exit_calls == 0 ? '0' : 'x');

    reset();
    open_result = 0;
    setup_load();
    put(message_calls == 1 && same(got_message, "Run SETUP before.")
        && exit_calls == 1 && got_exit == -1 ? '1' : 'x');

    put('D');
    quit();
    return 0;
}
