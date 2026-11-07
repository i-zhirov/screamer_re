/* The test harness for the rebuilt `radar_world_draw` module.
 *
 * The harness holds the spatial grid, one row of cells, the cell records, the
 * segment records, the angle table, and the line sink.  It runs eight paths:
 *
 *     0  empty grid: no cell holds a segment, so the function draws no line
 *     1  angle 0: the cosine 0x4000 and the sine 0 rotate the first segment to
 *        the panel points (76, 314) and (72, 310)
 *     2  angle 90: the cosine 0 and the sine 0x4000 rotate the same segment to
 *        (76, 324) and (72, 328)
 *     3  negative endpoints: the extra decrement after the negative shift
 *        gives the points (84, 324) and (86, 328)
 *     4  two segments: the cell count 2 draws two lines and the second line
 *        holds the points (82, 318) and (80, 320)
 *     5  row pointer -1: every row pointer is -1, so no line is drawn
 *     6  cell count 0: the cell holds no segment, so no line is drawn
 *     7  column offset: the camera x 0x2000 moves the window to column 121,
 *        and the cell at that column draws the line (76, 314) to (72, 314)
 *     D  the exit marker
 *
 * The expected values take their form at run time.  The minimal image does not
 * load a constant array reliably, so the harness compares the scalars directly.
 */
#pragma off (check_stack)

#define WORLD_BIAS 0x100000
#define CELL_SHIFT 13
#define CELL_RADIUS 8
#define CELL_SPAN 0x44
#define ROW_COUNT 160
#define COLUMN_COUNT 160
#define FIRST_CELL 120
#define LAST_CELL 136

struct radar_angle {
    short cosine;
    short reserved_00;
    short sine;
    short reserved_04;
};

struct radar_segment {
    int angle;
    int x;
    int y;
    int x1;
    int y1;
    int x2;
    int y2;
};

struct radar_cell {
    int field_00;
    int field_04;
    int field_08;
    struct radar_segment *segments;
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    int count;
};

struct line_record {
    int x1;
    int y1;
    int x2;
    int y2;
};

void radar_world_draw(int x, int y, int angle, int scale);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct radar_angle radar_angles[8];
int **radar_grid;
int radar_camera_x;
int radar_camera_y;
int radar_scale;
int radar_cell_x_offset;
int **radar_row_end;
int *radar_cell_end;
int radar_cosine;
int radar_sine;

static char text[8];
static struct radar_cell cells[8];
static struct radar_segment segments[16];
static int *grid_rows[ROW_COUNT];
static int cell_rows[ROW_COUNT][COLUMN_COUNT];
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

void radar_line_draw(int x1, int y1, int x2, int y2)
{
    if (line_count < 8) {
        lines[line_count].x1 = x1;
        lines[line_count].y1 = y1;
        lines[line_count].x2 = x2;
        lines[line_count].y2 = y2;
    }
    line_count++;
}

static void reset(void)
{
    int row;
    int column;
    int index;

    for (row = 0; row < ROW_COUNT; row++) {
        grid_rows[row] = (int *)-1;
        for (column = 0; column < COLUMN_COUNT; column++) {
            cell_rows[row][column] = -1;
        }
    }
    for (row = FIRST_CELL; row <= LAST_CELL; row++) {
        grid_rows[row] = cell_rows[row];
    }
    radar_grid = grid_rows;
    for (index = 0; index < 8; index++) {
        cells[index].count = 0;
        cells[index].segments = segments;
    }
    line_count = 0;
    radar_angles[0].cosine = 0x4000;
    radar_angles[0].sine = 0;
    radar_angles[1].cosine = 0;
    radar_angles[1].sine = 0x4000;
}

static void place_cell(int row, int column, int cell, int count, int segment)
{
    cell_rows[row][column] = (int)&cells[cell];
    cells[cell].count = count;
    cells[cell].segments = &segments[segment];
}

static void set_segment(int index, int x1, int y1, int x2, int y2)
{
    segments[index].angle = 0;
    segments[index].x = 0;
    segments[index].y = 0;
    segments[index].x1 = x1;
    segments[index].y1 = y1;
    segments[index].x2 = x2;
    segments[index].y2 = y2;
}

static int line_is(int index, int x1, int y1, int x2, int y2)
{
    return lines[index].x1 == x1
        && lines[index].y1 == y1
        && lines[index].x2 == x2
        && lines[index].y2 == y2;
}

int main(void)
{
    int row;

    reset();
    radar_world_draw(0, 0, 0, 0x40);
    put(line_count == 0 ? '0' : 'x');

    reset();
    place_cell(FIRST_CELL, FIRST_CELL, 0, 1, 0);
    set_segment(0, 0x1000, 0x1000, 0x2000, 0x2000);
    radar_world_draw(0, 0, 0, 0x40);
    put(line_count == 1 && line_is(0, 76, 314, 72, 310) ? '1' : 'x');

    reset();
    place_cell(FIRST_CELL, FIRST_CELL, 0, 1, 0);
    set_segment(0, 0x1000, 0x1000, 0x2000, 0x2000);
    radar_world_draw(0, 0, 1, 0x40);
    put(line_count == 1 && line_is(0, 76, 324, 72, 328) ? '2' : 'x');

    reset();
    place_cell(FIRST_CELL, FIRST_CELL, 0, 1, 0);
    set_segment(0, -0x1000, -0x800, -0x2000, -0x1000);
    radar_world_draw(0, 0, 0, 0x40);
    put(line_count == 1 && line_is(0, 84, 324, 86, 328) ? '3' : 'x');

    reset();
    place_cell(FIRST_CELL, FIRST_CELL, 0, 2, 0);
    set_segment(0, 0x1000, 0x1000, 0x2000, 0x2000);
    set_segment(1, 0x100, -0x100, -0x100, 0x100);
    radar_world_draw(0, 0, 0, 0x40);
    put(line_count == 2
        && line_is(0, 76, 314, 72, 310)
        && line_is(1, 82, 318, 80, 320) ? '4' : 'x');

    reset();
    for (row = FIRST_CELL; row <= LAST_CELL; row++) {
        grid_rows[row] = (int *)-1;
    }
    radar_world_draw(0, 0, 0, 0x40);
    put(line_count == 0 ? '5' : 'x');

    reset();
    place_cell(FIRST_CELL, FIRST_CELL, 0, 0, 0);
    radar_world_draw(0, 0, 0, 0x40);
    put(line_count == 0 ? '6' : 'x');

    reset();
    place_cell(FIRST_CELL, FIRST_CELL + 1, 0, 1, 0);
    set_segment(0, 0x3000, 0x1000, 0x3000, 0x2000);
    radar_world_draw(0x2000, 0, 0, 0x40);
    put(line_count == 1 && line_is(0, 76, 314, 72, 314) ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
