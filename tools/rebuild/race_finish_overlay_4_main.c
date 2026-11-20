/* The test harness for the rebuilt `race_finish_overlay_4` module.
 *
 * The harness holds the runtime text pointer `DAT_00118ff0`.  The
 * `panel_draw_centered` stub records the text pointer, the y value, the font,
 * and the colour.
 *
 * The harness builds every test string at run time, because the initialized
 * data page does not reach the DOS/4G image.
 *
 * The harness runs five paths:
 *
 *     0  the pointer to the first string: one call at y 0x5a, font 0, colour 2
 *     1  the pointer to the second string: the same four values
 *     2  the pointer set to null: the stub sees the null pointer
 *     3  the pointer back to the first string: the same four values
 *     4  two calls in a row: the stub records two calls
 *     D  the exit marker
 */
#pragma off (check_stack)

void race_finish_overlay_4(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char *DAT_00118ff0;

static char text[8];
static char first_text[8];
static char second_text[8];
static int panel_calls;
static char *panel_value[4];
static int panel_y[4];
static int panel_font[4];
static int panel_color[4];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(int number, int ok)
{
    put(ok ? (char)('0' + number) : 'x');
}

void panel_draw_centered(char *value, int y, int font, int colour)
{
    if (panel_calls < 4) {
        panel_value[panel_calls] = value;
        panel_y[panel_calls] = y;
        panel_font[panel_calls] = font;
        panel_color[panel_calls] = colour;
    }
    panel_calls = panel_calls + 1;
}

static int same_call(int index, char *value)
{
    return panel_value[index] == value
        && panel_y[index] == 0x5a
        && panel_font[index] == 0
        && panel_color[index] == 2;
}

static void reset(void)
{
    panel_calls = 0;
    panel_value[0] = 0;
    panel_value[1] = 0;
    panel_value[2] = 0;
    panel_value[3] = 0;
}

int main(void)
{
    first_text[0] = 'a';
    first_text[1] = 0;
    second_text[0] = 'b';
    second_text[1] = 0;

    reset();
    DAT_00118ff0 = first_text;
    race_finish_overlay_4();
    check(0, panel_calls == 1 && same_call(0, first_text));

    reset();
    DAT_00118ff0 = second_text;
    race_finish_overlay_4();
    check(1, panel_calls == 1 && same_call(0, second_text));

    reset();
    DAT_00118ff0 = 0;
    race_finish_overlay_4();
    check(2, panel_calls == 1 && panel_value[0] == 0
        && panel_y[0] == 0x5a && panel_font[0] == 0 && panel_color[0] == 2);

    reset();
    DAT_00118ff0 = first_text;
    race_finish_overlay_4();
    check(3, panel_calls == 1 && same_call(0, first_text));

    reset();
    DAT_00118ff0 = first_text;
    race_finish_overlay_4();
    DAT_00118ff0 = second_text;
    race_finish_overlay_4();
    check(4, panel_calls == 2 && same_call(0, first_text)
        && same_call(1, second_text));

    put('D');
    quit();
    return 0;
}
