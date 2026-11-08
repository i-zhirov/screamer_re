/* The test harness for the rebuilt `sound_range_reset` module.
 *
 * The harness holds the camera record, the spatial grid, the extent record, the
 * clip points, the cosine and sine, and the output globals.  It runs four paths:
 *
 *     0  small clip, no cell: the two points rotate to an 0x4000 span, the
 *        clamp expands them to [-0x2000, 0x2000], and the biased range gives
 *        the row 127, the offset 508, and the column count 3
 *     1  cell present: the original jumps over the cell block, so the extent
 *        stays at the default and the path equals path 0
 *     2  wide clip: the world limits clamp the range to [0, 0x1fffff], and the
 *        row count and the column count follow the 0xfd guard
 *     3  rotated clip: the Q14 rotation and the camera offset give the row 123,
 *        the offset 532, and the column count 3
 *     D  the exit marker
 *
 * The expected values take their form at run time.  The minimal image does not
 * load a constant array reliably, so the harness compares the scalars directly.
 */
#pragma off (check_stack)

#define WORLD_BIAS 0x100000

struct rect {
    int min_x;
    int reserved_04;
    int min_y;
    int max_x;
    int reserved_10;
    int max_y;
};

struct camera {
    int field_00;
    int field_04;
    int field_08;
    int x;
    int field_10;
    int z;
};

struct cell {
    int field_00;
    int field_04;
    int field_08;
    int field_0c;
    struct rect *extent;
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    int count;
};

int sound_range_reset(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static struct camera camera;
struct camera *camera_state;
int **spatial_grid;
struct rect view_rect;
struct rect view_clip;
int clip_ax;
int clip_ay;
int clip_bx;
int clip_by;
int view_cos;
int view_sin;
int view_camera_x;
int view_camera_z;
int view_point_a_x;
int view_point_a_y;
int view_point_b_y;
int view_point_b_x;
int view_rect_x1;
int view_rect_y1;
int view_rect_x2;
int view_rect_y2;
int grid_x_offset;
int grid_column_count;
int *grid_row;

static int *grid[256];
static int row128[256];
static struct cell view_cell;
static struct rect extent;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    int index;

    for (index = 0; index < 256; index++) {
        grid[index] = (int *)-1;
        row128[index] = -1;
    }
    grid[128] = row128;
    spatial_grid = grid;
    camera_state = &camera;
    camera.x = 0;
    camera.z = 0;
    view_rect.min_x = 0x1111;
    view_rect.min_y = 0x1111;
    view_rect.max_x = 0x2222;
    view_rect.max_y = 0x2222;
    view_clip.min_x = 0x3333;
    view_clip.min_y = 0x3333;
    view_clip.max_x = 0x4444;
    view_clip.max_y = 0x4444;
    clip_ax = 0;
    clip_ay = 0;
    clip_bx = 0;
    clip_by = 0;
    view_cos = 0x4000;
    view_sin = 0;
    view_camera_x = 0;
    view_camera_z = 0;
    view_point_a_x = 0;
    view_point_a_y = 0;
    view_point_b_y = 0;
    view_point_b_x = 0;
    view_rect_x1 = 0;
    view_rect_y1 = 0;
    view_rect_x2 = 0;
    view_rect_y2 = 0;
    grid_x_offset = 0;
    grid_column_count = 0;
    grid_row = 0;
    view_cell.count = 0;
    view_cell.extent = &extent;
    extent.min_x = 0;
    extent.min_y = 0;
    extent.max_x = 0;
    extent.max_y = 0;
}

static void set_small_clip(void)
{
    clip_ax = -100;
    clip_ay = -200;
    clip_bx = 300;
    clip_by = 400;
}

static int clip_is(int x1, int y1, int x2, int y2)
{
    return view_clip.min_x == x1
        && view_clip.min_y == y1
        && view_clip.max_x == x2
        && view_clip.max_y == y2;
}

static int rect_is(int x1, int y1, int x2, int y2)
{
    return view_rect_x1 == x1
        && view_rect_y1 == y1
        && view_rect_x2 == x2
        && view_rect_y2 == y2;
}

static int grid_is(int *row, int offset, int column)
{
    return grid_row == row
        && grid_x_offset == offset
        && grid_column_count == column;
}

int main(void)
{
    int result;

    reset();
    row128[128] = -1;
    set_small_clip();
    result = sound_range_reset();
    put(result == 3
        && rect_is(-0x2000, -0x2000, 0x2000, 0x2000)
        && clip_is(0xfe000, 0xfe000, 0x102000, 0x102000)
        && grid_is((int *)&grid[127], 508, 3) ? '0' : 'x');

    reset();
    view_cell.count = 1;
    view_cell.extent = &extent;
    extent.min_x = 0;
    extent.min_y = 0;
    extent.max_x = 1000;
    extent.max_y = 1000;
    row128[128] = (int)&view_cell;
    set_small_clip();
    result = sound_range_reset();
    put(result == 3
        && rect_is(-0x2000, -0x2000, 0x2000, 0x2000)
        && clip_is(0xfe000, 0xfe000, 0x102000, 0x102000)
        && grid_is((int *)&grid[127], 508, 3) ? '1' : 'x');

    reset();
    camera.x = 0xfe000;
    set_small_clip();
    result = sound_range_reset();
    put(result == 3
        && rect_is(0xfc000, -0x2000, 0xfffff, 0x2000)
        && clip_is(0x1fc000, 0xfe000, 0x1fffff, 0x102000)
        && grid_is((int *)&grid[127], 1016, 1) ? '2' : 'x');

    reset();
    camera.x = 50000;
    camera.z = -30000;
    view_cos = 0x3000;
    view_sin = 0x2000;
    clip_ax = 20000;
    clip_ay = 0;
    clip_bx = 0;
    clip_by = 20000;
    result = sound_range_reset();
    put(result == 4
        && rect_is(41808, -40000, 65000, -15000)
        && clip_is(1090384, 1008576, 1113576, 1033576)
        && grid_is((int *)&grid[123], 532, 3) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
