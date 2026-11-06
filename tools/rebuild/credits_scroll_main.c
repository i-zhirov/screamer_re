/* The test harness for the rebuilt `credits_scroll` module.
 *
 * The harness holds the page index `DAT_0011ce3c`, the toggle `DAT_0011ce40`,
 * the session state `DAT_0011ce44`, the timer `DAT_0011ce48`, the page state
 * `DAT_0011ce54`, the 0x14 line positions `DAT_0011ce58`, the 0x14 cells
 * `DAT_0011cea8`, the credit line pointers `DAT_0011cef8`, and the frame delta
 * `DAT_00118e04`.
 *
 * The `filter_update_int` stub follows the original at 0x22bb4: it reads the
 * integer cell, moves it toward the target with the coefficient, truncates, and
 * writes the integer back.  The `text_measure` stub writes a chosen width.
 * The `text_draw` stub records the position, the line, the font, and the color.
 *
 * The line table holds three pages:
 *
 *     0 A, 1 BB, 2 #
 *     3 CCC, 4 DD, 5 #
 *     6 EEE, 7 #
 *     8 the -1 sentinel
 *
 * The harness runs seven paths:
 *
 *     0  the first page at the timer 0 and the delta 1.0 returns below 4.0 and
 *        draws the two lines at the position 350 and the center 135
 *     1  the second page at the toggle 1 and the delta 0 sets the timer 4.0 on
 *        the low line, filters the second line to -400, advances the page past
 *        the # line to 6, and resets the positions
 *     2  the second page at the toggle 0 and the timer 4.0 flips the toggle to
 *        1, keeps the index 3, and does not reset the positions 200
 *     3  the third page at the toggle 1 advances past the # line to the -1
 *        sentinel and sets the session state 3 and the full reset
 *     4  the first page with the width 40 uses the center 150 and draws at 1000
 *     5  the second page at the toggle 0 with the coefficient 0.5 moves 200 to
 *        100 and draws at 470, and the timer 3.0 plus the delta 1.0 advances
 *     6  the second page at the toggle 1 and the timer 2.0 plus the delta 1.0
 *        returns below 4.0 and keeps the toggle and the positions
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void credits_scroll(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011ce3c;
int DAT_0011ce40;
int DAT_0011ce44;
float DAT_0011ce48;
int DAT_0011ce54;
int DAT_0011ce58[20];
int DAT_0011cea8[20];
char *DAT_0011cef8[32];
float DAT_00118e04;

static char text[8];
static char lines[8][8];

static float coefficients[256];

static int measure_width;
static int measure_calls;
static int draw_calls;
static int draw_x[8];
static int draw_y[8];
static char *draw_text[8];
static int draw_font[8];
static int draw_color[8];
static int filter_calls;
static int filter_target;
static int filter_index;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void setup_table(void)
{
    int index;

    for (index = 0; index < 32; index++) {
        DAT_0011cef8[index] = (char *)-1;
    }
    lines[0][0] = 'A';
    lines[0][1] = 0;
    lines[1][0] = 'B';
    lines[1][1] = 'B';
    lines[1][2] = 0;
    lines[2][0] = '#';
    lines[2][1] = 0;
    lines[3][0] = 'C';
    lines[3][1] = 'C';
    lines[3][2] = 'C';
    lines[3][3] = 0;
    lines[4][0] = 'D';
    lines[4][1] = 'D';
    lines[4][2] = 0;
    lines[5][0] = '#';
    lines[5][1] = 0;
    lines[6][0] = 'E';
    lines[6][1] = 'E';
    lines[6][2] = 'E';
    lines[6][3] = 0;
    lines[7][0] = '#';
    lines[7][1] = 0;
    for (index = 0; index < 8; index++) {
        DAT_0011cef8[index] = lines[index];
    }
}

static void reset(void)
{
    int index;

    DAT_0011ce3c = 0;
    DAT_0011ce40 = 0;
    DAT_0011ce44 = 0;
    DAT_0011ce48 = 0.0f;
    DAT_0011ce54 = 0;
    DAT_00118e04 = 0.0f;
    for (index = 0; index < 20; index++) {
        DAT_0011ce58[index] = 350;
        DAT_0011cea8[index] = 100;
    }
    for (index = 0; index < 256; index++) {
        coefficients[index] = 1.0f;
    }
    measure_width = 100;
    measure_calls = 0;
    draw_calls = 0;
    filter_calls = 0;
    filter_target = 0;
    filter_index = 0;
}

void filter_update_int(int *value, float target, int index)
{
    filter_calls = filter_calls + 1;
    filter_target = (int)target;
    filter_index = index;
    *value = (int)(target + ((float)*value - target) * coefficients[index]);
}

void text_measure(int *width, int *height, char *line, int font)
{
    measure_calls = measure_calls + 1;
    *width = measure_width;
    *height = 8;
}

void text_draw(int x, int y, char *line, int font, int color)
{
    if (draw_calls < 8) {
        draw_x[draw_calls] = x;
        draw_y[draw_calls] = y;
        draw_text[draw_calls] = line;
        draw_font[draw_calls] = font;
        draw_color[draw_calls] = color;
    }
    draw_calls = draw_calls + 1;
}

static int positions_at(int value)
{
    int index;

    for (index = 0; index < 20; index++) {
        if (DAT_0011ce58[index] != value) {
            return 0;
        }
    }
    return 1;
}

static int cells_at(int value)
{
    int index;

    for (index = 0; index < 20; index++) {
        if (DAT_0011cea8[index] != value) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int ok;

    setup_table();

    reset();
    DAT_00118e04 = 1.0f;
    credits_scroll();
    ok = measure_calls == 2
        && draw_calls == 2
        && filter_calls == 2
        && filter_target == 0
        && filter_index == 0x5a
        && draw_x[0] == 970
        && draw_y[0] == 0
        && draw_x[1] == 970
        && draw_y[1] == 40
        && draw_text[1] == lines[1]
        && draw_font[0] == 0
        && draw_color[0] == 2
        && DAT_0011ce48 == 1.0f
        && DAT_0011ce40 == 0
        && DAT_0011ce3c == 0
        && DAT_0011ce54 == 0
        && DAT_0011ce44 == 0;
    put(ok ? '0' : 'x');

    reset();
    DAT_0011ce3c = 3;
    DAT_0011ce40 = 1;
    DAT_0011ce58[0] = -400;
    DAT_0011ce58[1] = -100;
    credits_scroll();
    ok = measure_calls == 2
        && draw_calls == 2
        && filter_calls == 1
        && filter_target == -400
        && filter_index == 0x5a
        && draw_x[0] == -530
        && draw_y[0] == 0
        && draw_x[1] == 70
        && draw_y[1] == 40
        && DAT_0011ce48 == 0.0f
        && DAT_0011ce40 == 0
        && DAT_0011ce3c == 6
        && DAT_0011ce54 == 1
        && DAT_0011ce44 == 0
        && positions_at(350);
    put(ok ? '1' : 'x');

    reset();
    DAT_0011ce3c = 3;
    DAT_0011ce48 = 4.0f;
    DAT_0011ce58[0] = 200;
    DAT_0011ce58[1] = 200;
    credits_scroll();
    ok = measure_calls == 2
        && draw_calls == 2
        && filter_calls == 2
        && filter_target == 0
        && draw_x[0] == 670
        && draw_x[1] == 670
        && DAT_0011ce48 == 0.0f
        && DAT_0011ce40 == 1
        && DAT_0011ce3c == 3
        && DAT_0011ce54 == 0
        && DAT_0011ce44 == 0
        && DAT_0011ce58[0] == 200
        && DAT_0011ce58[1] == 200;
    put(ok ? '2' : 'x');

    reset();
    DAT_0011ce3c = 6;
    DAT_0011ce40 = 1;
    DAT_0011ce48 = 4.0f;
    DAT_0011ce54 = 2;
    DAT_0011ce58[0] = 200;
    credits_scroll();
    ok = measure_calls == 1
        && draw_calls == 1
        && filter_calls == 1
        && filter_target == -400
        && draw_x[0] == 670
        && draw_y[0] == 0
        && DAT_0011ce48 == 0.0f
        && DAT_0011ce40 == 0
        && DAT_0011ce3c == 0
        && DAT_0011ce54 == 0
        && DAT_0011ce44 == 3
        && positions_at(350)
        && cells_at(100);
    put(ok ? '3' : 'x');

    reset();
    measure_width = 40;
    DAT_00118e04 = 1.0f;
    credits_scroll();
    ok = measure_calls == 2
        && draw_calls == 2
        && draw_x[0] == 1000
        && draw_y[0] == 0
        && draw_x[1] == 1000
        && draw_y[1] == 40
        && DAT_0011ce48 == 1.0f;
    put(ok ? '4' : 'x');

    reset();
    DAT_0011ce3c = 3;
    DAT_0011ce48 = 3.0f;
    DAT_00118e04 = 1.0f;
    DAT_0011ce58[0] = 200;
    DAT_0011ce58[1] = 200;
    coefficients[0x5a] = 0.5f;
    credits_scroll();
    ok = measure_calls == 2
        && draw_calls == 2
        && filter_calls == 2
        && filter_target == 0
        && DAT_0011ce58[0] == 100
        && DAT_0011ce58[1] == 100
        && draw_x[0] == 470
        && draw_x[1] == 470
        && DAT_0011ce48 == 0.0f
        && DAT_0011ce40 == 1
        && DAT_0011ce3c == 3;
    put(ok ? '5' : 'x');

    reset();
    DAT_0011ce3c = 3;
    DAT_0011ce40 = 1;
    DAT_0011ce48 = 2.0f;
    DAT_00118e04 = 1.0f;
    DAT_0011ce58[0] = 200;
    DAT_0011ce58[1] = 200;
    credits_scroll();
    ok = measure_calls == 2
        && draw_calls == 2
        && filter_calls == 2
        && filter_target == -400
        && DAT_0011ce48 == 3.0f
        && DAT_0011ce40 == 1
        && DAT_0011ce3c == 3
        && DAT_0011ce54 == 0
        && DAT_0011ce44 == 0
        && DAT_0011ce58[0] == 200
        && DAT_0011ce58[1] == 200;
    put(ok ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
