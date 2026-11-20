/* The test harness for the rebuilt `race_menu_handler` module.
 *
 * The harness holds the context record with the fields `+0x18` and `+0x28`,
 * the mode word, the counter, the track index, the member flag, and the menu
 * table.  The stub calls record their arguments and their counts.  The
 * `sprintf` stub writes the marker `R` and records the buffer, the format, the
 * base, and the counter.  The `member_check` stub records the path and returns
 * the configured result.
 *
 * The paths:
 *
 *     0  an unlisted mode runs no path and changes no global
 *     1  mode 1 runs the loading screen from the table, the menu space, the
 *        menu frame, and the record save
 *     2  mode 1 with the context value 3 moves to 4 in the third argument
 *     3  mode 4 runs the sprintf and the failing member check, clears the
 *        flag, and runs the loading screen, the menu space, and the menu frame
 *     4  mode 4 with a clearing member and a clear flag keeps the flag clear
 *     5  mode 0x10 runs only the race results screen
 *     6  mode 0x20 runs the loading screen, the menu space, and the menu frame
 *     7  mode 0x40 runs the same three calls with no record save
 *     D  the exit marker
 *
 * The counter global stays unchanged, because the function reads it and adds
 * one in a register.
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_menu_handler(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct menu_context {
    int pad_00[6];
    int field_18;
    int pad_1c[3];
    int field_28;
};

void *DAT_00002dec;
int DAT_0001d260;
int DAT_0001d264;
int DAT_000203c8;
int DAT_000203bc;
int DAT_000021e4[16];
char DAT_00000ff5[8];
char DAT_00007edc[8];

static struct menu_context context;
static char text[8];
static char expected[2];
static int loading_calls;
static int loading_screen_arg;
static int loading_counter_arg;
static int loading_value_arg;
static int space_calls;
static int frame_calls;
static int save_calls;
static int save_track;
static int results_calls;
static int format_calls;
static char *format_buffer;
static char *format_format;
static char *format_base;
static int format_counter;
static int member_calls;
static char *member_path;
static int member_result;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
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

static void reset(void)
{
    int index;

    for (index = 0; index < (int)sizeof(context); index++) {
        ((char *)&context)[index] = 0;
    }
    for (index = 0; index < 16; index++) {
        DAT_000021e4[index] = 0;
    }
    DAT_0001d260 = 0;
    DAT_0001d264 = 0;
    DAT_000203c8 = 0;
    DAT_000203bc = 0;
    loading_calls = 0;
    loading_screen_arg = 0;
    loading_counter_arg = 0;
    loading_value_arg = 0;
    space_calls = 0;
    frame_calls = 0;
    save_calls = 0;
    save_track = 0;
    results_calls = 0;
    format_calls = 0;
    format_buffer = 0;
    format_format = 0;
    format_base = 0;
    format_counter = 0;
    member_calls = 0;
    member_path = 0;
    member_result = 0;
}

void loading_screen(int screen, int counter, int value)
{
    loading_calls = loading_calls + 1;
    loading_screen_arg = screen;
    loading_counter_arg = counter;
    loading_value_arg = value;
}

void menu_space_init(void)
{
    space_calls = space_calls + 1;
}

void menu_frame(void)
{
    frame_calls = frame_calls + 1;
}

void race_record_save(int track)
{
    save_calls = save_calls + 1;
    save_track = track;
}

void race_results_screen(void)
{
    results_calls = results_calls + 1;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *base;
    int counter;

    va_start(arguments, format);
    base = va_arg(arguments, char *);
    counter = va_arg(arguments, int);
    va_end(arguments);
    buffer[0] = 'R';
    buffer[1] = 0;
    format_calls = format_calls + 1;
    format_buffer = buffer;
    format_format = (char *)format;
    format_base = base;
    format_counter = counter;
    return 1;
}

int member_check(char *path, void *record)
{
    (void)record;
    member_calls = member_calls + 1;
    member_path = path;
    return member_result;
}

int main(void)
{
    DAT_00002dec = &context;

    reset();
    DAT_0001d260 = 0x21;
    context.field_18 = 2;
    DAT_000203c8 = 0x10;
    race_menu_handler();
    check('0', loading_calls == 0 && space_calls == 0 && frame_calls == 0
        && save_calls == 0 && results_calls == 0 && format_calls == 0
        && member_calls == 0 && DAT_000203c8 == 0x10);

    reset();
    DAT_0001d260 = 1;
    context.field_18 = 2;
    context.field_28 = 3;
    DAT_000021e4[3] = 7;
    DAT_000203c8 = 0x10;
    DAT_000203bc = 0x42;
    race_menu_handler();
    check('1', loading_calls == 1 && loading_screen_arg == 7
        && loading_counter_arg == 0x11 && loading_value_arg == 2
        && space_calls == 1 && frame_calls == 1 && save_calls == 1
        && save_track == 0x42 && DAT_000203c8 == 0x10);

    reset();
    DAT_0001d260 = 1;
    context.field_18 = 3;
    context.field_28 = 1;
    DAT_000021e4[1] = 9;
    DAT_000203c8 = 5;
    race_menu_handler();
    check('2', loading_calls == 1 && loading_screen_arg == 9
        && loading_counter_arg == 6 && loading_value_arg == 4
        && space_calls == 1 && frame_calls == 1 && save_calls == 1);

    reset();
    DAT_0001d260 = 4;
    context.field_18 = 1;
    DAT_000203c8 = 8;
    DAT_0001d264 = 1;
    member_result = -1;
    expected[0] = 'R';
    expected[1] = 0;
    race_menu_handler();
    check('3', format_calls == 1 && format_buffer != 0
        && format_format == DAT_00000ff5 && format_base == DAT_00007edc
        && format_counter == 9 && member_calls == 1
        && member_path == format_buffer && same(format_buffer, expected)
        && DAT_0001d264 == 0 && loading_calls == 1
        && loading_screen_arg == 0 && loading_counter_arg == 9
        && loading_value_arg == 1 && space_calls == 1 && frame_calls == 1
        && save_calls == 0);

    reset();
    DAT_0001d260 = 4;
    context.field_18 = 2;
    DAT_000203c8 = 3;
    DAT_0001d264 = 5;
    member_result = 0;
    race_menu_handler();
    check('4', format_calls == 1 && format_counter == 4 && member_calls == 1
        && DAT_0001d264 == 5 && loading_calls == 1
        && loading_counter_arg == 4 && loading_value_arg == 2);

    reset();
    DAT_0001d260 = 0x10;
    race_menu_handler();
    check('5', results_calls == 1 && loading_calls == 0 && space_calls == 0
        && frame_calls == 0 && save_calls == 0);

    reset();
    DAT_0001d260 = 0x20;
    context.field_18 = 6;
    DAT_000203c8 = 0x20;
    race_menu_handler();
    check('6', loading_calls == 1 && loading_screen_arg == 0
        && loading_counter_arg == 0x21 && loading_value_arg == 6
        && space_calls == 1 && frame_calls == 1 && save_calls == 0);

    reset();
    DAT_0001d260 = 0x40;
    context.field_18 = 7;
    DAT_000203c8 = 0x40;
    race_menu_handler();
    check('7', loading_calls == 1 && loading_screen_arg == 0
        && loading_counter_arg == 0x41 && loading_value_arg == 7
        && space_calls == 1 && frame_calls == 1 && save_calls == 0);

    put('D');
    quit();
    return 0;
}
