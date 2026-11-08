/* The test harness for the rebuilt `gauge_marker_draw` module.
 *
 * The harness holds the point record, the two angle constants, and the line
 * sink.  It runs seven paths:
 *
 *     0  centre (100, 50), radius 10, bias 0, angle 1: the three lines start
 *        at the doubled centre (200, 100) and end at (216, 89), (210, 116),
 *        and (189, 83)
 *     1  centre (-30, -20), radius 3, angle -2: the three lines start at the
 *        doubled centre (-60, -40) and end at (-65, -37), (-62, -45), and
 *        (-57, -34), so the negative sums truncate toward zero
 *     2  radius 0: the three lines start and end at the doubled centre
 *        (200, 100)
 *     3  bias 5 and angle 6: the delta is 1, so the three lines equal the
 *        three lines of path 0
 *     4  centre (12, -7), radius 25, angle 0.3: the three lines start at
 *        (24, -14) and end at (38, -61), (71, 0), and (-23, -28)
 *     5  centre (0, 0), radius 100, angle -1.2: the three lines start at the
 *        origin and end at (-186, -72), (72, -186), and (-72, 186)
 *     6  centre (-50, 30), radius 7, bias 4, angle -0.4: the three lines
 *        start at (-100, 60) and end at (-86, 64), (-104, 73), and (-95, 46)
 *     D  the exit marker
 *
 * The expected integers come from the same arithmetic as the original.  The
 * harness uses only BSS state, because the minimal image does not load the
 * initialized data page.
 */
#pragma off (check_stack)

struct gauge_point {
    int x;
    int y;
};

struct line_record {
    int x1;
    int y1;
    int x2;
    int y2;
};

void gauge_marker_draw(struct gauge_point *point, int radius, int angle_bias,
    float angle);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float gauge_half_pi;
float gauge_minus_pi;

static char text[8];
static struct gauge_point point;
static struct line_record lines[8];
static int line_count;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void gauge_line_draw(int x1, int y1, int x2, int y2)
{
    if (line_count < 8) {
        lines[line_count].x1 = x1;
        lines[line_count].y1 = y1;
        lines[line_count].x2 = x2;
        lines[line_count].y2 = y2;
    }
    line_count++;
}

static void set_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void reset(void)
{
    int index;

    point.x = 0;
    point.y = 0;
    line_count = 0;
    for (index = 0; index < 8; index++) {
        lines[index].x1 = 0;
        lines[index].y1 = 0;
        lines[index].x2 = 0;
        lines[index].y2 = 0;
    }
    set_float(&gauge_half_pi, 0x3fc90fdb);
    set_float(&gauge_minus_pi, 0xc0490fdb);
}

static int line_is(int index, int x1, int y1, int x2, int y2)
{
    return lines[index].x1 == x1 && lines[index].y1 == y1
        && lines[index].x2 == x2 && lines[index].y2 == y2;
}

int main(void)
{
    reset();
    point.x = 100;
    point.y = 50;
    gauge_marker_draw(&point, 10, 0, 1.0f);
    put(line_count == 3
        && line_is(0, 200, 100, 216, 89)
        && line_is(1, 200, 100, 210, 116)
        && line_is(2, 200, 100, 189, 83)
        ? '0' : 'x');

    reset();
    point.x = -30;
    point.y = -20;
    gauge_marker_draw(&point, 3, 0, -2.0f);
    put(line_count == 3
        && line_is(0, -60, -40, -65, -37)
        && line_is(1, -60, -40, -62, -45)
        && line_is(2, -60, -40, -57, -34)
        ? '1' : 'x');

    reset();
    point.x = 100;
    point.y = 50;
    gauge_marker_draw(&point, 0, 0, 1.0f);
    put(line_count == 3
        && line_is(0, 200, 100, 200, 100)
        && line_is(1, 200, 100, 200, 100)
        && line_is(2, 200, 100, 200, 100)
        ? '2' : 'x');

    reset();
    point.x = 100;
    point.y = 50;
    gauge_marker_draw(&point, 10, 5, 6.0f);
    put(line_count == 3
        && line_is(0, 200, 100, 216, 89)
        && line_is(1, 200, 100, 210, 116)
        && line_is(2, 200, 100, 189, 83)
        ? '3' : 'x');

    reset();
    point.x = 12;
    point.y = -7;
    gauge_marker_draw(&point, 25, 0, 0.3f);
    put(line_count == 3
        && line_is(0, 24, -14, 38, -61)
        && line_is(1, 24, -14, 71, 0)
        && line_is(2, 24, -14, -23, -28)
        ? '4' : 'x');

    reset();
    point.x = 0;
    point.y = 0;
    gauge_marker_draw(&point, 100, 0, -1.2f);
    put(line_count == 3
        && line_is(0, 0, 0, -186, -72)
        && line_is(1, 0, 0, 72, -186)
        && line_is(2, 0, 0, -72, 186)
        ? '5' : 'x');

    reset();
    point.x = -50;
    point.y = 30;
    gauge_marker_draw(&point, 7, 4, -0.4f);
    put(line_count == 3
        && line_is(0, -100, 60, -86, 64)
        && line_is(1, -100, 60, -104, 73)
        && line_is(2, -100, 60, -95, 46)
        ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
