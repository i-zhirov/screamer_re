/* The test harness for the rebuilt `panel_draw_centered` module.
 *
 * The harness holds the measure stub and the draw stub.  The measure stub
 * returns a configured width and height.  The draw stub records its five
 * arguments.  The harness calls the module with an even width, a negative
 * width, and an odd width:
 *
 *     0  the even width centers on 0x140 and the draw gets the doubled position
 *     1  the negative width uses the signed division
 *     2  the odd width rounds toward zero
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void panel_draw_centered(int text, int y, int font, int color);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static int measure_width;
static int measure_height;
static int got_text;
static int got_font;
static int got_width;
static int got_height;
static int draw_x;
static int draw_y;
static int draw_text;
static int draw_font;
static int draw_color;
static int measure_calls;
static int draw_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void text_measure(int *width, int *height, int text, int font)
{
    *width = measure_width;
    *height = measure_height;
    got_text = text;
    got_font = font;
    got_width = *width;
    got_height = *height;
    measure_calls = measure_calls + 1;
}

void text_draw(int x, int y, int text, int font, int color)
{
    draw_x = x;
    draw_y = y;
    draw_text = text;
    draw_font = font;
    draw_color = color;
    draw_calls = draw_calls + 1;
}

static void reset(void)
{
    measure_calls = 0;
    draw_calls = 0;
    draw_x = 0;
    draw_y = 0;
    draw_text = 0;
    draw_font = 0;
    draw_color = 0;
    got_text = 0;
    got_font = 0;
    got_width = 0;
    got_height = 0;
}

int main(void)
{
    reset();
    measure_width = 150;
    measure_height = 0x20;
    panel_draw_centered(11, 7, 13, 17);
    put(measure_calls == 1 && draw_calls == 1
        && got_text == 11 && got_font == 13
        && got_width == 150 && got_height == 0x20
        && draw_x == 244 && draw_y == 14 && draw_text == 11
        && draw_font == 13 && draw_color == 17 ? '0' : 'x');

    reset();
    measure_width = -101;
    panel_draw_centered(21, 1, 23, 27);
    put(draw_x == 370 && draw_y == 2 && draw_color == 27 ? '1' : 'x');

    reset();
    measure_width = 101;
    panel_draw_centered(31, 3, 33, 37);
    put(draw_x == 270 && draw_y == 6 && draw_color == 37 ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
