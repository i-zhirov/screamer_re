/* The test harness for the rebuilt `race_start` module.
 *
 * The harness holds the screen globals, the five state sources, the race
 * context, the car table, the `MAP%d` buffer, and the three stubs.  The
 * `choice_load` and `state_cells_set` stubs record their call.  The `sprintf`
 * stub records its three arguments and writes `MAP` and the value into the
 * destination.  The `race_session_start` stub records its argument and returns
 * the configured result.  The harness runs two paths:
 *
 *     0  the success path: the module sets the screen rectangle, copies the
 *        five state words, reads the context, clears the two words, builds
 *        `MAP1`, sets the mode, copies the old `DAT_0011d274`, and returns 0
 *     1  the non-zero result: the counter 5 builds `MAP6`, the index 3 selects
 *        the third table value, and the result 0x999 returns at once
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

int race_start(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_01015ce4;
int DAT_01015ce8;
int DAT_01015cec;
int DAT_01015cf0;
int DAT_001207f4;
int DAT_0012080c;
int DAT_00120808;
int DAT_00120810;
int DAT_00120814;
int DAT_0011d2a8;
int DAT_0011d2c0;
int DAT_0011d2bc;
int DAT_0011d2c4;
int DAT_0011d2a4;
int DAT_0011d2b4;
int DAT_0011d2b8;
int DAT_0011d2cc;
int DAT_00118e30;
int DAT_0011ce54;
int DAT_001207f8;
char DAT_0011d294[8];
int DAT_0011d260;
int DAT_0011d22c;
int DAT_0011d274;
int DAT_001021d8[8];
void *DAT_00002dec;

static char text[8];
static char context[0x50];

static int choice_calls;
static int state_calls;
static int state_arg;
static int sprintf_calls;
static char *sprintf_buffer;
static char *sprintf_format;
static int sprintf_value;
static int race_calls;
static int race_arg;
static int race_result;

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

void choice_load(void)
{
    choice_calls = choice_calls + 1;
}

void state_cells_set(int value)
{
    state_calls = state_calls + 1;
    state_arg = value;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    int value;

    va_start(arguments, format);
    value = va_arg(arguments, int);
    va_end(arguments);
    sprintf_calls = sprintf_calls + 1;
    sprintf_buffer = buffer;
    sprintf_format = (char *)format;
    sprintf_value = value;
    copy(buffer, "MAP");
    buffer[3] = (char)('0' + value);
    buffer[4] = 0;
    return 4;
}

int race_session_start(int value)
{
    race_calls = race_calls + 1;
    race_arg = value;
    return race_result;
}

static void reset(void)
{
    int index;

    DAT_01015ce4 = -1;
    DAT_01015ce8 = -1;
    DAT_01015cec = -1;
    DAT_01015cf0 = -1;
    DAT_001207f4 = 0;
    DAT_0012080c = 0;
    DAT_00120808 = 0;
    DAT_00120810 = 0;
    DAT_00120814 = 0;
    DAT_0011d2a8 = -1;
    DAT_0011d2c0 = -1;
    DAT_0011d2bc = -1;
    DAT_0011d2c4 = -1;
    DAT_0011d2a4 = -1;
    DAT_0011d2b4 = -1;
    DAT_0011d2b8 = -1;
    DAT_0011d2cc = -1;
    DAT_00118e30 = -1;
    DAT_0011ce54 = -1;
    DAT_001207f8 = 0;
    DAT_0011d260 = -1;
    DAT_0011d22c = -1;
    DAT_0011d274 = -1;
    for (index = 0; index < 8; ++index) {
        DAT_0011d294[index] = 0;
        DAT_001021d8[index] = 0;
    }
    for (index = 0; index < 0x50; ++index) {
        context[index] = 0;
    }
    DAT_00002dec = context;
    choice_calls = 0;
    state_calls = 0;
    state_arg = 0;
    sprintf_calls = 0;
    sprintf_buffer = 0;
    sprintf_format = 0;
    sprintf_value = -1;
    race_calls = 0;
    race_arg = -1;
    race_result = 0;
}

static int common(int value, int index, int table, int flag)
{
    return choice_calls == 1
        && DAT_01015ce4 == 0
        && DAT_01015ce8 == 0x280
        && DAT_01015cec == 0
        && DAT_01015cf0 == 0x190
        && DAT_0011d2a8 == 0x11
        && DAT_0011d2c0 == 0x22
        && DAT_0011d2bc == 0x33
        && DAT_0011d2c4 == 0x44
        && DAT_0011d2a4 == 0x55
        && DAT_0011d2b4 == 1
        && DAT_0011d2cc == flag
        && DAT_0011d2b8 == index
        && state_calls == 1
        && state_arg == table
        && DAT_00118e30 == 0
        && DAT_0011ce54 == 0
        && sprintf_calls == 1
        && sprintf_buffer == DAT_0011d294
        && same(sprintf_format, "MAP%d")
        && sprintf_value == value
        && DAT_0011d260 == 0x100
        && DAT_0011d2b4 == 1
        && DAT_0011d22c == 0xaaaa
        && DAT_0011d274 == 0
        && race_calls == 1
        && race_arg == 1;
}

int main(void)
{
    int result;

    reset();
    DAT_001207f4 = 0x11;
    DAT_0012080c = 0x22;
    DAT_00120808 = 0x33;
    DAT_00120810 = 0x44;
    DAT_00120814 = 0x55;
    DAT_0011d274 = 0xaaaa;
    *(int *)(context + 0x48) = 7;
    *(int *)(context + 8) = 2;
    DAT_001021d8[2] = 0x12345678;
    result = race_start();
    put(result == 0 && common(1, 2, 0x12345678, 7)
        && same(DAT_0011d294, "MAP1") ? '0' : 'x');

    reset();
    DAT_001207f4 = 0x11;
    DAT_0012080c = 0x22;
    DAT_00120808 = 0x33;
    DAT_00120810 = 0x44;
    DAT_00120814 = 0x55;
    DAT_0011d274 = 0xaaaa;
    DAT_001207f8 = 5;
    *(int *)(context + 0x48) = 0xffff;
    *(int *)(context + 8) = 3;
    DAT_001021d8[3] = 0xcafe;
    race_result = 0x999;
    result = race_start();
    put(result == 0x999 && common(6, 3, 0xcafe, 0xffff)
        && same(DAT_0011d294, "MAP6") ? '1' : 'x');

    put('D');
    quit();
    return 0;
}
