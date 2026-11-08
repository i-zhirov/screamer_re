/* The rebuilt `sound_range_update` module.
 *
 * The original sits at linear 0x830c8 in STARTH.EXE and holds 207 instructions
 * and 649 bytes up to the `ret` at 0x83350.  The next function starts at
 * 0x83354, so the store record keeps the 652-byte extent and sets no separate
 * function size.  The identity check uses 207 instructions.
 *
 * The only direct caller at 0x83354 builds the active sound list.  It reads the
 * returned row count from eax and the returned row pointer from esi.  The
 * source update at 0x8a958 reads the world rectangle at 0x72d94 through
 * 0x72da8 to cull the sound sources, so the record takes the name
 * `sound_range_update` with the confidence inferred.
 *
 * The function finds the spatial grid cell under the camera position.  When the
 * cell holds an extent, the function takes the extent as the view rectangle.
 * It then rotates the two clip points at 0x72d48 and 0x72d54 by the Q14 cosine
 * and sine at 0x724ec and 0x724f0, expands the result to the central
 * [-0x2000, 0x2000] box, adds the camera position, clamps the world limits to
 * [-0x100000, 0xfffff], biases the result by 0x100000, and intersects it with
 * the extent rectangle.
 *
 * The function returns the row count in eax and leaves the row pointer in esi.
 * C cannot return a value in esi, so the rebuild stores the row pointer in the
 * global `grid_row`.  The function also stores the x byte offset at 0x73377 and
 * the column count at 0x73381.
 *
 * The original has no EBP frame.  The module uses no floating-point
 * instruction.  The minimal image needs one initialized data byte, because the
 * DOS4G loader leaves the BSS object unmapped when the data object holds no
 * page.
 */

#define WORLD_BIAS 0x100000
#define CELL_SHIFT 13
#define CELL_LIMIT 0xfd
#define RANGE_MIN (-2000000000)
#define RANGE_MAX 2000000000

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

extern struct camera *camera_state;
extern int **spatial_grid;
extern struct rect view_rect;
extern struct rect view_clip;
extern int clip_ax;
extern int clip_ay;
extern int clip_bx;
extern int clip_by;
extern int view_cos;
extern int view_sin;
extern int view_camera_x;
extern int view_camera_z;
extern int view_point_a_x;
extern int view_point_a_y;
extern int view_point_b_y;
extern int view_point_b_x;
extern int view_rect_x1;
extern int view_rect_y1;
extern int view_rect_x2;
extern int view_rect_y2;
extern int grid_x_offset;
extern int grid_column_count;
extern int *grid_row;

int sound_range_marker = 0;

int sound_range_update(void)
{
    struct camera *camera;
    struct cell *cell;
    struct rect *extent;
    int *row;
    int x_cell;
    int z_cell;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    int a;
    int b;
    int c;
    int d;
    int p1x;
    int p1y;
    int p2x;
    int p2y;
    int value;
    int other;
    int dy;
    int dx;
    int row_count;

    view_rect.min_x = RANGE_MIN;
    view_rect.min_y = RANGE_MIN;
    view_rect.max_x = RANGE_MAX;
    view_rect.max_y = RANGE_MAX;

    camera = camera_state;
    x_cell = (camera->x + WORLD_BIAS) >> CELL_SHIFT;
    z_cell = (camera->z + WORLD_BIAS) >> CELL_SHIFT;
    row = spatial_grid[z_cell];
    if ((int)row >= 0) {
        cell = (struct cell *)row[x_cell];
        if ((int)cell >= 0) {
            if (cell->count > 0) {
                extent = cell->extent;
                view_rect.min_x = extent->min_x + WORLD_BIAS;
                view_rect.min_y = extent->min_y + WORLD_BIAS;
                view_rect.max_x = extent->max_x + WORLD_BIAS;
                view_rect.max_y = extent->max_y + WORLD_BIAS;
            }
        }
    }

    a = clip_ax;
    b = clip_ay;
    c = clip_bx;
    d = clip_by;
    p1x = (a * view_cos + b * view_sin) >> 14;
    p1y = (b * view_cos - a * view_sin) >> 14;
    p2x = (c * view_cos + d * view_sin) >> 14;
    p2y = (d * view_cos - c * view_sin) >> 14;

    view_point_a_x = p1x;
    view_point_a_y = p1y;
    view_point_b_y = p2y;
    view_point_b_x = p2x;

    if (p2x <= p1x) {
        value = p1x;
        p1x = p2x;
        p2x = value;
    }
    if (p2y <= p1y) {
        value = p1y;
        p1y = p2y;
        p2y = value;
    }

    if (p1x >= -0x2000) {
        p1x = -0x2000;
    }
    if (p1y >= -0x2000) {
        p1y = -0x2000;
    }
    if (p2x <= 0x2000) {
        p2x = 0x2000;
    }
    if (p2y <= 0x2000) {
        p2y = 0x2000;
    }

    view_camera_x = camera->x;
    view_camera_z = camera->z;

    p1x += camera->x;
    if (p1x < -WORLD_BIAS) {
        p1x = -WORLD_BIAS;
    }
    p2x += camera->x;
    if (p2x > 0xfffff) {
        p2x = 0xfffff;
    }
    p1y += camera->z;
    if (p1y < -WORLD_BIAS) {
        p1y = -WORLD_BIAS;
    }
    p2y += camera->z;
    if (p2y > 0xfffff) {
        p2y = 0xfffff;
    }

    view_rect_x1 = p1x;
    view_rect_y1 = p1y;
    view_rect_x2 = p2x;
    view_rect_y2 = p2y;

    min_x = p1x + WORLD_BIAS;
    min_y = p1y + WORLD_BIAS;
    max_x = p2x + WORLD_BIAS;
    max_y = p2y + WORLD_BIAS;

    view_clip.min_x = min_x;
    view_clip.min_y = min_y;
    view_clip.max_x = max_x;
    view_clip.max_y = max_y;

    value = view_rect.min_x;
    if (value < min_x) {
        value = min_x;
    }
    if (value > max_x) {
        value = max_x;
    }
    other = view_rect.max_x;
    if (other < min_x) {
        other = min_x;
    }
    if (other > max_x) {
        other = max_x;
    }
    if (value != other) {
        view_clip.min_x = value;
        view_clip.max_x = other;
    }

    value = view_rect.min_y;
    if (value < min_y) {
        value = min_y;
    }
    if (value > max_y) {
        value = max_y;
    }
    other = view_rect.max_y;
    if (other < min_y) {
        other = min_y;
    }
    if (other > max_y) {
        other = max_y;
    }
    if (value != other) {
        view_clip.min_y = value;
        view_clip.max_y = other;
    }

    min_x = view_clip.min_x;
    min_y = view_clip.min_y;
    max_x = view_clip.max_x;
    max_y = view_clip.max_y;

    dy = max_y - min_y;
    if (dy < 0) {
        dy = -dy;
    }
    dy >>= CELL_SHIFT;
    z_cell = min_y;
    if (z_cell < 0) {
        z_cell = -z_cell;
    }
    z_cell >>= CELL_SHIFT;
    value = z_cell + dy;
    if (value <= CELL_LIMIT) {
        dy++;
    }
    row_count = dy;
    grid_row = (int *)&spatial_grid[z_cell];

    dx = max_x - min_x;
    if (dx < 0) {
        dx = -dx;
    }
    dx >>= CELL_SHIFT;
    x_cell = min_x;
    if (x_cell < 0) {
        x_cell = -x_cell;
    }
    x_cell >>= CELL_SHIFT;
    value = x_cell + dx;
    if (value <= CELL_LIMIT) {
        dx++;
    }
    grid_x_offset = x_cell << 2;
    grid_column_count = dx;

    return row_count;
}
