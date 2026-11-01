/* The test harness for the rebuilt `config_load` module.
 *
 * The harness holds the three string buffers, the flag, the file stubs, and the
 * two error stubs.  The stubs record their arguments.  The harness runs three
 * paths:
 *
 *     0  the success path opened `screamer.cfg` with the mode `rt`, read the
 *        three strings into the buffers, read two characters into the same
 *        local, set the flag to 1 for the character `n`, and closed the handle
 *     1  the second success path set the flag to 0 for another character
 *     2  the failure path reported `Files damaged. Re-install Screamer.` and
 *        called the exit with -1 (the exit stub returns, so the module then
 *        falls through to the read path, as the original does)
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void config_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007edc[0x34];
char DAT_00007f10[0x34];
char DAT_00007f44[0x34];
int DAT_00007f78;

static char text[8];
static char *got_path;
static char *got_mode;
static char *got_format[3];
static int got_stream[3];
static char *got_string[3];
static char *got_char[2];
static int got_close;
static char *got_message;
static int got_exit;
static int open_result;
static int char_value;
static int open_calls;
static int scanf_calls;
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

int fopen(char *path, char *mode)
{
    got_path = path;
    got_mode = mode;
    open_calls = open_calls + 1;
    return open_result;
}

int fscanf(int stream, const char *format, ...)
{
    va_list arguments;
    int index;

    va_start(arguments, format);
    index = scanf_calls;
    if (index < 3) {
        got_format[index] = (char *)format;
        got_stream[index] = stream;
    }
    if (index == 0) {
        got_string[0] = va_arg(arguments, char *);
        got_string[1] = va_arg(arguments, char *);
        got_string[2] = va_arg(arguments, char *);
        copy(got_string[0], "aaa");
        copy(got_string[1], "bbb");
        copy(got_string[2], "ccc");
    } else {
        got_char[index - 1] = va_arg(arguments, char *);
        *got_char[index - 1] = (char)char_value;
    }
    va_end(arguments);
    scanf_calls = scanf_calls + 1;
    return 1;
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
    int index;

    got_path = 0;
    got_mode = 0;
    got_close = 0;
    got_message = 0;
    got_exit = 0;
    open_calls = 0;
    scanf_calls = 0;
    close_calls = 0;
    message_calls = 0;
    exit_calls = 0;
    DAT_00007f78 = -1;
    for (index = 0; index < 3; ++index) {
        got_format[index] = 0;
        got_stream[index] = 0;
        got_string[index] = 0;
    }
    for (index = 0; index < 2; ++index) {
        got_char[index] = 0;
    }
}

int main(void)
{
    reset();
    open_result = 0x1234;
    char_value = 'n';
    config_load();
    put(same(got_path, "screamer.cfg") && same(got_mode, "rt")
        && open_calls == 1 && scanf_calls == 3
        && same(got_format[0], "%s%s%s")
        && same(got_format[1], "%c") && same(got_format[2], "%c")
        && got_stream[0] == 0x1234 && got_stream[1] == 0x1234
        && got_stream[2] == 0x1234
        && got_string[0] == DAT_00007edc
        && got_string[1] == DAT_00007f10
        && got_string[2] == DAT_00007f44
        && got_char[0] == got_char[1]
        && same(DAT_00007edc, "aaa") && same(DAT_00007f10, "bbb")
        && same(DAT_00007f44, "ccc") && DAT_00007f78 == 1
        && close_calls == 1 && got_close == 0x1234
        && message_calls == 0 && exit_calls == 0 ? '0' : 'x');

    reset();
    open_result = 0x1234;
    char_value = 'y';
    config_load();
    put(scanf_calls == 3 && DAT_00007f78 == 0
        && close_calls == 1 && got_close == 0x1234 ? '1' : 'x');

    reset();
    open_result = 0;
    char_value = 'n';
    config_load();
    put(message_calls == 1
        && same(got_message, "Files damaged. Re-install Screamer.")
        && exit_calls == 1 && got_exit == -1
        && open_calls == 1 && scanf_calls == 3
        && got_stream[0] == 0 && got_stream[1] == 0 && got_stream[2] == 0
        && close_calls == 1 && got_close == 0 ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
