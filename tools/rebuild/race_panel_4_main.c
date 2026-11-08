/* The test harness for the rebuilt `race_panel_4` module.
 *
 * The harness holds the title pointer, the count, and the player name.  The
 * three stubs record their arguments.  The `sprintf` stub writes the 0x01
 * prefix and the name into the buffer.  The `draw` stub copies the text that it
 * receives, because the module buffer is a stack local and a later call
 * overwrites that stack area.  The harness runs three paths:
 *
 *     0  the count 0 path draws the title, formats the name, and draws the name
 *        at (0x106, 0x98) and the separator at (0x106, 0xb0)
 *     1  the count 5 path shifts the separator to (0x1d8, 0xb0)
 *     2  the second-title path follows the new title pointer and shifts the
 *        separator to (0x15a, 0xb0)
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_panel_4(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char *DAT_0011fc38;
unsigned int DAT_0011d244;
char DAT_001203cc[0x40];

static char text[8];
static char title_a[8];
static char title_b[8];
static char name_a[8];
static char name_b[8];
static int panel_calls;
static char *panel_text;
static int panel_y;
static int panel_font;
static int panel_color;
static int sprintf_calls;
static char *sprintf_buffer;
static char *sprintf_format;
static char *sprintf_name;
static int draw_calls;
static int draw_x[4];
static int draw_y[4];
static char *draw_text[4];
static int draw_font[4];
static int draw_color[4];
static char draw_copy[4][0x20];

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

static int name_is(char *value, char a, char b, char c, char d)
{
    return value[0] == 1 && value[1] == a && value[2] == b && value[3] == c
        && value[4] == d && value[5] == 0;
}

void panel_draw_centered(char *text, int y, int font, int color)
{
    panel_text = text;
    panel_y = y;
    panel_font = font;
    panel_color = color;
    panel_calls = panel_calls + 1;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *value;

    va_start(arguments, format);
    value = va_arg(arguments, char *);
    va_end(arguments);
    sprintf_buffer = buffer;
    sprintf_format = (char *)format;
    sprintf_name = value;
    buffer[0] = 1;
    copy(buffer + 1, value);
    sprintf_calls = sprintf_calls + 1;
    return 0;
}

void draw(int x, int y, char *text, int font, int color)
{
    if (draw_calls < 4) {
        draw_x[draw_calls] = x;
        draw_y[draw_calls] = y;
        draw_text[draw_calls] = text;
        draw_font[draw_calls] = font;
        draw_color[draw_calls] = color;
        copy(draw_copy[draw_calls], text);
    }
    draw_calls = draw_calls + 1;
}

static void reset(void)
{
    title_a[0] = 'T';
    title_a[1] = 'I';
    title_a[2] = 'T';
    title_a[3] = 'L';
    title_a[4] = 'E';
    title_a[5] = 0;
    title_b[0] = 'N';
    title_b[1] = 'E';
    title_b[2] = 'W';
    title_b[3] = 0;
    name_a[0] = 'T';
    name_a[1] = 'E';
    name_a[2] = 'S';
    name_a[3] = 'T';
    name_a[4] = 0;
    name_b[0] = 'A';
    name_b[1] = 'B';
    name_b[2] = 'C';
    name_b[3] = 'D';
    name_b[4] = 0;
    DAT_0011fc38 = title_a;
    DAT_0011d244 = 0;
    copy(DAT_001203cc, name_a);
    panel_calls = 0;
    panel_text = 0;
    panel_y = 0;
    panel_font = 0;
    panel_color = 0;
    sprintf_calls = 0;
    sprintf_buffer = 0;
    sprintf_format = 0;
    sprintf_name = 0;
    draw_calls = 0;
}

int main(void)
{
    reset();
    race_panel_4();
    put(panel_calls == 1 && panel_text == title_a && panel_y == 2
        && panel_font == 3 && panel_color == 2
        && sprintf_calls == 1 && sprintf_buffer != 0
        && same(sprintf_format, "\001%s") && sprintf_name == DAT_001203cc
        && name_is(draw_copy[0], 'T', 'E', 'S', 'T')
        && draw_calls == 2
        && draw_x[0] == 0x106 && draw_y[0] == 0x98
        && draw_text[0] == sprintf_buffer
        && draw_font[0] == 2 && draw_color[0] == 2
        && draw_x[1] == 0x106 && draw_y[1] == 0xb0
        && same(draw_text[1], "\001-")
        && draw_font[1] == 2 && draw_color[1] == 2 ? '0' : 'x');

    reset();
    DAT_0011d244 = 5;
    copy(DAT_001203cc, name_b);
    race_panel_4();
    put(panel_text == title_a && sprintf_calls == 1
        && name_is(draw_copy[0], 'A', 'B', 'C', 'D')
        && draw_calls == 2 && draw_x[0] == 0x106 && draw_y[0] == 0x98
        && draw_x[1] == 0x1d8 && draw_y[1] == 0xb0
        && same(draw_text[1], "\001-") ? '1' : 'x');

    reset();
    DAT_0011fc38 = title_b;
    DAT_0011d244 = 2;
    copy(DAT_001203cc, name_b);
    race_panel_4();
    put(panel_text == title_b && draw_calls == 2
        && draw_x[0] == 0x106 && draw_y[0] == 0x98
        && draw_x[1] == 0x15a && draw_y[1] == 0xb0
        && same(draw_text[1], "\001-") ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
