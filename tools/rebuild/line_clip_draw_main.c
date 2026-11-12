/* The test harness for the rebuilt `line_clip_draw` module.
 *
 * The harness holds the draw color at `DAT_0000055b`, the four sorted endpoint
 * cells at `DAT_0000055c`, `DAT_00000560`, `DAT_00000564`, and `DAT_00000568`,
 * the four clip bounds at `DAT_0000056c`, `DAT_00000570`, `DAT_00000574`, and
 * `DAT_00000578`, and the 0x3e800-byte video buffer at `DAT_00021528`.
 *
 * The harness runs these paths:
 *
 *     0  both endpoints above the top bound: the function returns before any
 *        draw, so the buffer stays clear
 *     1  a horizontal line with the endpoints out of order: the function sorts
 *        them and fills the row from the left x to the right x
 *     2  a vertical line: the function fills one column between the two y
 *        values
 *     3  a shallow diagonal line: the function draws both endpoints
 *     4  a line past the right bound: the function clips the right endpoint to
 *        xmax - 1 and draws nothing past it
 *     5  a line that starts above the top bound: the function clips the left
 *        endpoint onto the top bound
 *     6  a line that starts left of the left bound: the function clips the left
 *        endpoint onto the left bound
 *     7  a line that ends below the bottom bound: the function clips the right
 *        endpoint onto the bottom bound
 *     D  the exit marker
 *
 * Every path checks the touched pixels and the untouched pixels against the
 * color.
 */
#pragma off (check_stack)

void line_clip_draw(int x1, int y1, int x2, int y2);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_0000055b;
int DAT_0000055c;
int DAT_00000560;
int DAT_00000564;
int DAT_00000568;
int DAT_0000056c;
int DAT_00000570;
int DAT_00000574;
int DAT_00000578;
unsigned char *DAT_00021528;

static unsigned char video[0x3e800];
static char text[8];

static int pixel(int x, int y)
{
    return video[y * 0x280 + x];
}

static int marked(void)
{
    int index;
    int count;

    count = 0;
    for (index = 0; index < 0x3e800; index++) {
        if (video[index] != 0) {
            count = count + 1;
        }
    }
    return count;
}

static void clear(void)
{
    int index;

    for (index = 0; index < 0x3e800; index++) {
        video[index] = 0;
    }
}

static void set_clip(int xmin, int xmax, int ymin, int ymax)
{
    DAT_0000056c = xmin;
    DAT_00000570 = xmax;
    DAT_00000574 = ymin;
    DAT_00000578 = ymax;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

int main(void)
{
    DAT_00021528 = video;

    set_clip(0, 639, 10, 399);
    DAT_0000055b = 0x55;
    clear();
    line_clip_draw(100, 5, 200, 8);
    report(marked() == 0, '0');

    set_clip(0, 639, 0, 399);
    DAT_0000055b = 0x22;
    clear();
    line_clip_draw(30, 20, 10, 20);
    report(pixel(10, 20) == 0x22 && pixel(20, 20) == 0x22 &&
           pixel(30, 20) == 0x22 && pixel(9, 20) == 0 &&
           pixel(31, 20) == 0, '1');

    set_clip(0, 639, 0, 399);
    DAT_0000055b = 0x33;
    clear();
    line_clip_draw(50, 10, 50, 30);
    report(pixel(50, 10) == 0x33 && pixel(50, 30) == 0x33 &&
           pixel(50, 9) == 0 && pixel(50, 31) == 0, '2');

    set_clip(0, 639, 0, 399);
    DAT_0000055b = 0x44;
    clear();
    line_clip_draw(10, 10, 20, 15);
    report(pixel(10, 10) == 0x44 && pixel(20, 15) == 0x44 &&
           pixel(0, 0) == 0, '3');

    set_clip(0, 100, 0, 399);
    DAT_0000055b = 0x66;
    clear();
    line_clip_draw(10, 50, 150, 50);
    report(pixel(10, 50) == 0x66 && pixel(99, 50) == 0x66 &&
           pixel(100, 50) == 0 && pixel(150, 50) == 0, '4');

    set_clip(0, 639, 15, 399);
    DAT_0000055b = 0x77;
    clear();
    line_clip_draw(10, 10, 20, 20);
    report(pixel(15, 15) == 0x77 && pixel(20, 20) == 0x77 &&
           pixel(10, 10) == 0 && pixel(14, 15) == 0, '5');

    set_clip(20, 639, 0, 399);
    DAT_0000055b = 0x88;
    clear();
    line_clip_draw(5, 50, 60, 50);
    report(pixel(20, 50) == 0x88 && pixel(60, 50) == 0x88 &&
           pixel(19, 50) == 0 && pixel(5, 50) == 0, '6');

    set_clip(0, 639, 0, 30);
    DAT_0000055b = 0x99;
    clear();
    line_clip_draw(50, 20, 50, 40);
    report(pixel(50, 20) == 0x99 && pixel(50, 30) == 0x99 &&
           pixel(50, 31) == 0 && pixel(50, 40) == 0, '7');

    put('D');
    quit();
    return 0;
}
