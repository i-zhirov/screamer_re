/* The rebuilt `radar_world_draw` module.
 *
 * The original sits at linear 0x1057c in STARTH.EXE and holds 154 instructions
 * and 483 bytes up to the `ret` at 0x1075e.  The next function starts at
 * 0x1075f, so the store record keeps the 483-byte extent and sets no separate
 * function size.  The identity check uses 154 instructions.
 *
 * The only direct caller is `race_radar` at 0x23b5b.  It passes the player
 * position in eax and edx, the heading index in ebx, and the zoom scale in ecx.
 * The scale is 0x30, 0x60, or 0x90 for the three radar zoom steps.  The
 * function draws the world line segments of the spatial grid into the 144x144
 * radar panel, so the record takes the name `radar_world_draw` with the
 * confidence inferred.
 *
 * The entry stores the camera x, the camera y, and the scale in the globals
 * 0x7d5, 0x7d9, and 0x7ed.  It then forms the byte offset of the cell column
 * from x and the byte offset of the cell row from y, both with the 0x100000
 * world bias and the 0x2000 cell size.  The grid base is the global
 * `radar_grid`.  The row scan covers 17 rows and the cell scan covers 17
 * cells, so the function reads a 17x17 window around the camera.
 *
 * The angle index selects a signed 16-bit cosine at 0x203d0 and a signed
 * 16-bit sine at 0x203d4 with the stride 8.  The record keeps the two values
 * in the globals 0x7f1 and 0x7f5.
 *
 * A cell holds the segment pointer at +0x0c and the segment count at +0x20.
 * A segment holds the angle at +0x00, the midpoint at +0x04 and +0x08, and the
 * two endpoints at +0x0c, +0x10, +0x14, and +0x18.  The segment stride is
 * 0x1c.
 *
 * For each segment the function computes the offset from the camera, divides
 * it by four, rotates it by the cosine and the sine, scales it, and places the
 * result around the panel centre (0x50, 0x140).  Each rotated product uses a
 * signed shift by 14 and one extra decrement when the shifted result is
 * negative.  The callee at 0x102e4 clips and draws the line.
 *
 * The original has no EBP frame and uses no floating-point instruction.  The
 * module builds without `-of+` and links no FPU stub.
 */
#define WORLD_BIAS 0x100000
#define CELL_SHIFT 13
#define CELL_RADIUS 8
#define CELL_SPAN 0x44
#define CENTER_X 0x50
#define CENTER_Y 0x140
#define FIXED_SHIFT 14

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

extern struct radar_angle radar_angles[];
extern int **radar_grid;
extern int radar_camera_x;
extern int radar_camera_y;
extern int radar_scale;
extern int radar_cell_x_offset;
extern int **radar_row_end;
extern int *radar_cell_end;
extern int radar_cosine;
extern int radar_sine;
extern void radar_line_draw(int x1, int y1, int x2, int y2);

#define Q14(destination, product) \
    do { \
        int q14_value = (product); \
        q14_value = q14_value >> FIXED_SHIFT; \
        if (q14_value < 0) { \
            q14_value = q14_value - 1; \
        } \
        (destination) = q14_value; \
    } while (0)

void radar_world_draw(int x, int y, int angle, int scale)
{
    int row_offset;
    int **row_cursor;
    int *row;
    int *cell_cursor;
    struct radar_cell *cell;
    struct radar_segment *segment;
    int count;
    int dx;
    int dy;
    int rotated_y;
    int rotated_x;
    int value;
    int screen_x1;
    int screen_y1;
    int screen_x2;
    int screen_y2;

    radar_camera_x = x;
    radar_camera_y = y;
    radar_scale = scale;
    radar_cell_x_offset =
        (((x + WORLD_BIAS) >> CELL_SHIFT) - CELL_RADIUS) << 2;
    row_offset = (((y + WORLD_BIAS) >> CELL_SHIFT) - CELL_RADIUS) << 2;
    row_cursor = (int **)((char *)radar_grid + row_offset);
    radar_row_end = (int **)((char *)row_cursor + CELL_SPAN);
    radar_cosine = radar_angles[angle].cosine;
    radar_sine = radar_angles[angle].sine;

    for (; row_cursor != radar_row_end; row_cursor++) {
        row = *row_cursor;
        if ((int)row == -1) {
            continue;
        }
        cell_cursor = (int *)((char *)row + radar_cell_x_offset);
        radar_cell_end = (int *)((char *)cell_cursor + CELL_SPAN);
        for (; cell_cursor != radar_cell_end; cell_cursor++) {
            cell = (struct radar_cell *)*cell_cursor;
            if ((int)cell == -1) {
                continue;
            }
            count = cell->count;
            segment = cell->segments;
            while (count > 0) {
                dx = (segment->x1 - radar_camera_x) >> 2;
                dy = (segment->y1 - radar_camera_y) >> 2;
                Q14(value, dx * radar_sine);
                rotated_y = value;
                Q14(value, dy * radar_cosine);
                rotated_y += value;
                Q14(rotated_x, dy * radar_sine - dx * radar_cosine);
                Q14(value, radar_scale * rotated_y);
                screen_x1 = CENTER_X - value;
                Q14(value, radar_scale * rotated_x);
                screen_y1 = CENTER_Y + value;

                dx = (segment->x2 - radar_camera_x) >> 2;
                dy = (segment->y2 - radar_camera_y) >> 2;
                Q14(value, dx * radar_sine);
                rotated_y = value;
                Q14(value, dy * radar_cosine);
                rotated_y += value;
                Q14(rotated_x, dy * radar_sine - dx * radar_cosine);
                Q14(value, radar_scale * rotated_y);
                screen_x2 = CENTER_X - value;
                Q14(value, radar_scale * rotated_x);
                screen_y2 = CENTER_Y + value;

                radar_line_draw(screen_x1, screen_y1, screen_x2, screen_y2);
                segment++;
                count--;
            }
        }
    }
}
