/* The test harness for the rebuilt `race_finish_overlay_3` module.
 *
 * The harness holds the control byte and the sixteen-entry car table.  The
 * `panel_draw_centered` stub records the text, the y value, the font, and the
 * colour.
 *
 * The harness builds every test string at run time, because the initialized
 * data page does not reach the DOS/4G image.
 *
 * The harness runs five paths:
 *
 *     0  the control byte 0: the car table entry 0xd at y 0x5a
 *     1  the control byte 8: the car table entry 0xe at y 0x5a
 *     2  the control byte 0x80: the entry 0xd, because bit 7 draws no text here
 *     3  the control byte 0x88: the entry 0xe
 *     4  the control byte 0x77: the entry 0xd only, because bit 3 is clear
 *     D  the exit marker
 */
#pragma off (check_stack)

void race_finish_overlay_3(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_00118e1d;
char *DAT_00118f94[16];

static char text[8];
static char car_text[16][8];
static int panel_calls;
static char panel_value[4][0x20];
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

static void copy(char *target, char *source)
{
    int index;

    for (index = 0; source[index] != 0; index++) {
        target[index] = source[index];
    }
    target[index] = 0;
}

static int is_entry_d(char *value)
{
    return value[0] == 'c' && value[1] == 'n' && value[2] == 0;
}

static int is_entry_e(char *value)
{
    return value[0] == 'c' && value[1] == 'o' && value[2] == 0;
}

static void check(int number, int ok)
{
    put(ok ? (char)('0' + number) : 'x');
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

static void reset(void)
{
    int index;

    DAT_00118e1d = 0;
    for (index = 0; index < 16; index++) {
        DAT_00118f94[index] = car_text[index];
    }
    panel_calls = 0;
}

int main(void)
{
    int index;

    for (index = 0; index < 16; index++) {
        car_text[index][0] = 'c';
        car_text[index][1] = (char)('a' + index);
        car_text[index][2] = 0;
    }

    reset();
    race_finish_overlay_3();
    check(0, panel_calls == 1
        && is_entry_d(panel_value[0]) && panel_y[0] == 0x5a
        && panel_font[0] == 0 && panel_color[0] == 2);

    reset();
    DAT_00118e1d = 8;
    race_finish_overlay_3();
    check(1, panel_calls == 1
        && is_entry_e(panel_value[0]) && panel_y[0] == 0x5a
        && panel_font[0] == 0 && panel_color[0] == 2);

    reset();
    DAT_00118e1d = 0x80;
    race_finish_overlay_3();
    check(2, panel_calls == 1
        && is_entry_d(panel_value[0]) && panel_y[0] == 0x5a
        && panel_font[0] == 0 && panel_color[0] == 2);

    reset();
    DAT_00118e1d = 0x88;
    race_finish_overlay_3();
    check(3, panel_calls == 1
        && is_entry_e(panel_value[0]) && panel_y[0] == 0x5a
        && panel_font[0] == 0 && panel_color[0] == 2);

    reset();
    DAT_00118e1d = 0x77;
    race_finish_overlay_3();
    check(4, panel_calls == 1
        && is_entry_d(panel_value[0]) && panel_y[0] == 0x5a);

    put('D');
    quit();
    return 0;
}
