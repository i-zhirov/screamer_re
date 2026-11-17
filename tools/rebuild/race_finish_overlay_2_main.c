/* The test harness for the rebuilt `race_finish_overlay_2` module.
 *
 * The harness holds the control byte, the top text pointer, the player slot,
 * the five-entry car table, the `%s %s` format, and the zero clock text.  The
 * `panel_draw_centered` stub records the text, the y value, the font, and the
 * colour.  The `sprintf` stub rebuilds the `%s %s` line and records the format
 * and the two string arguments.
 *
 * The harness runs four paths:
 *
 *     0  the control bit clear and the slot 2: the line `two zero` at y 0x5a
 *        and the zero clock at y 0x78, with no top text
 *     1  the control bit set and the slot 0: the top text at y 0x3e, the line
 *        `zero zero`, and the zero clock
 *     2  the control bit set and the slot 3: the top text at y 0x3e, the line
 *        `three zero`, and the zero clock
 *     3  the control bit clear and the slot 1: the line `one zero` and the
 *        zero clock, with no top text
 *     D  the exit marker
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_finish_overlay_2(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_00118e1d;
char *DAT_00118fc0;
int DAT_0010ffa4;
char *DAT_00118f94[8];
char race_finish_format[] = "%s %s";
char race_finish_clock[] = "00:00:00";

static char text[8];
static char car_text[5][8];
static char top_text[8];
static int panel_calls;
static char panel_value[4][0x20];
static int panel_y[4];
static int panel_font[4];
static int panel_color[4];
static int sprintf_calls;
static char sprintf_format[0x20];
static char sprintf_first[0x20];
static char sprintf_second[0x20];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void copy(char *target, char *source)
{
    int index;

    for (index = 0; source[index] != 0; index++) {
        target[index] = source[index];
    }
    target[index] = 0;
}

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; index++) {
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
    int index;

    DAT_00118e1d = 0;
    DAT_00118fc0 = top_text;
    DAT_0010ffa4 = 0;
    for (index = 0; index < 8; index++) {
        DAT_00118f94[index] = car_text[0];
    }
    DAT_00118f94[1] = car_text[1];
    DAT_00118f94[2] = car_text[2];
    DAT_00118f94[3] = car_text[3];
    DAT_00118f94[4] = car_text[4];
    panel_calls = 0;
    sprintf_calls = 0;
    sprintf_format[0] = 0;
    sprintf_first[0] = 0;
    sprintf_second[0] = 0;
}

void panel_draw_centered(char *value, int y, int font, int colour)
{
    if (panel_calls < 4) {
        copy(panel_value[panel_calls], value);
        panel_y[panel_calls] = y;
        panel_font[panel_calls] = font;
        panel_color[panel_calls] = colour;
    }
    panel_calls = panel_calls + 1;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *out;
    char *value;
    int index;
    int argument_index;

    va_start(arguments, format);
    if (sprintf_calls == 0) {
        copy(sprintf_format, (char *)format);
    }
    out = buffer;
    argument_index = 0;
    for (index = 0; format[index] != 0; index++) {
        if (format[index] == '%' && format[index + 1] == 's') {
            value = va_arg(arguments, char *);
            if (sprintf_calls == 0) {
                if (argument_index == 0) {
                    copy(sprintf_first, value);
                } else {
                    copy(sprintf_second, value);
                }
            }
            argument_index = argument_index + 1;
            while (*value != 0) {
                *out++ = *value++;
            }
            index = index + 1;
        } else {
            *out++ = format[index];
        }
    }
    *out = 0;
    sprintf_calls = sprintf_calls + 1;
    va_end(arguments);
    return 0;
}

int main(void)
{
    copy(top_text, "top");
    copy(car_text[0], "zero");
    copy(car_text[1], "one");
    copy(car_text[2], "two");
    copy(car_text[3], "three");
    copy(car_text[4], "four");

    reset();
    DAT_0010ffa4 = 2;
    race_finish_overlay_2();
    check(0, panel_calls == 2
        && same(panel_value[0], "two zero") && panel_y[0] == 0x5a
        && panel_font[0] == 2 && panel_color[0] == 0
        && same(panel_value[1], "00:00:00") && panel_y[1] == 0x78
        && panel_font[1] == 2 && panel_color[1] == 0
        && sprintf_calls == 1 && same(sprintf_format, "%s %s"));

    reset();
    DAT_00118e1d = 8;
    DAT_0010ffa4 = 0;
    race_finish_overlay_2();
    check(1, panel_calls == 3
        && same(panel_value[0], "top") && panel_y[0] == 0x3e
        && panel_font[0] == 2 && panel_color[0] == 0
        && same(panel_value[1], "zero zero") && panel_y[1] == 0x5a
        && same(panel_value[2], "00:00:00") && panel_y[2] == 0x78
        && sprintf_calls == 1);

    reset();
    DAT_00118e1d = 8;
    DAT_0010ffa4 = 3;
    race_finish_overlay_2();
    check(2, panel_calls == 3
        && same(panel_value[0], "top") && panel_y[0] == 0x3e
        && same(panel_value[1], "three zero") && panel_y[1] == 0x5a
        && same(panel_value[2], "00:00:00") && panel_y[2] == 0x78
        && sprintf_calls == 1);

    reset();
    DAT_0010ffa4 = 1;
    race_finish_overlay_2();
    check(3, panel_calls == 2
        && same(panel_value[0], "one zero") && panel_y[0] == 0x5a
        && same(panel_value[1], "00:00:00") && panel_y[1] == 0x78
        && sprintf_calls == 1);

    put('D');
    quit();
    return 0;
}
