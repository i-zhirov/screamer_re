/* The rebuilt `track_nearest_node` module.
 *
 * The original sits at linear 0x120c0 in STARTH.EXE and holds 201 instructions
 * and 695 bytes up to the `ret` at 0x12376.  The record keeps the 696-byte
 * extent, which ends at the next function at 0x12378, and sets no function
 * size.  The identity check uses 201 instructions.
 *
 * The function queries the spatial grid at `DAT_00102fa4` for the nearest
 * track entry.  It reads the world position from `DAT_00108038` and
 * `DAT_00108040`, adds the map bias 0x100000 to each axis, and divides each
 * sum by the cell size 0x2000.  It then scans the three-by-three neighborhood
 * around the position.  A row pointer or a cell pointer equal to -1 marks an
 * empty slot, so the scan skips it.
 *
 * A cell record holds two entry arrays.  The 12-byte node entries sit below
 * the pointer at `+0x08` and the count at `+0x1c`; each entry holds the angle
 * at `+0x00`, the x position at `+0x04`, and the y position at `+0x08`.  The
 * 28-byte segment entries sit below the pointer at `+0x0c` and the count at
 * `+0x20`; each entry holds the angle at `+0x00`, the x position at `+0x04`,
 * and the y position at `+0x08`.
 *
 * The mode in `eax` selects the array.  A non-zero mode scans the segment
 * entries and stores the nearest entry at `DAT_00108044`.  The zero mode scans
 * the node entries and stores the nearest entry at `DAT_00108054`.  The
 * function compares the squared distance with the best distance at
 * `DAT_00108020`, which starts at 0x7ffffff8, and sets the return value to 1
 * when a nearer entry exists.
 *
 * The zero mode then copies the node x and y to `DAT_0010804c` and
 * `DAT_00108050` and forms the heading at `DAT_00108048`.  When bit 3 of the
 * flag byte `DAT_0011d274` is set, it adds pi at `DAT_0010040c`.  A heading
 * that reaches two pi at `DAT_00100410` receives minus two pi at
 * `DAT_00100414`.
 *
 * The direct callers are `player_car_init` at 0x13ea5, `ai_update` route setup
 * at 0x1522b, `player_track_node_update` at 0x16ccb, another player-track
 * helper at 0x12048, and `player_car_reset_update` at 0x185d8.  Each caller
 * publishes the position through `DAT_00108038` and `DAT_00108040` and reads
 * the heading `DAT_00108048` back.  The callers, the grid data, and the
 * heading output support the name `track_nearest_node`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * module reuses the shared FPU stub, because it adds and compares floats but
 * never converts a float to an integer.
 */
extern int DAT_00108020;
extern int *DAT_00108030;
extern int *DAT_00108034;
extern int DAT_00108038;
extern int DAT_00108040;
extern int *DAT_00108044;
extern float DAT_00108048;
extern int DAT_0010804c;
extern int DAT_00108050;
extern int *DAT_00108054;
extern int *DAT_00102fa4;
extern float DAT_0010040c;
extern float DAT_00100410;
extern float DAT_00100414;
extern unsigned char DAT_0011d274;

int track_nearest_node(int mode)
{
    int xcell;
    int index;
    int dx;
    int dy;
    int distance;
    int row;
    int found;
    int *row_end;
    float value;

    DAT_00108020 = 0x7ffffff8;
    found = 0;
    xcell = ((DAT_00108038 + 0x100000) >> 13) - 1;
    DAT_00108030 = DAT_00102fa4 + (((DAT_00108040 + 0x100000) >> 13) - 1);
    row_end = DAT_00108030 + 3;
    for (; DAT_00108030 < row_end; DAT_00108030++) {
        int *cell_end;

        row = *DAT_00108030;
        if (row == -1) {
            continue;
        }
        DAT_00108034 = (int *)(row + xcell * 4);
        cell_end = DAT_00108034 + 3;
        for (; DAT_00108034 < cell_end; DAT_00108034++) {
            int cell;
            char *record;

            cell = *DAT_00108034;
            if (cell == -1) {
                continue;
            }
            record = (char *)cell;
            if (mode != 0) {
                int *base;
                int count;

                base = *(int **)(record + 0xc);
                count = *(int *)(record + 0x20);
                for (index = 0; index < count; index++) {
                    dx = base[index * 7 + 1] - DAT_00108038;
                    dy = base[index * 7 + 2] - DAT_00108040;
                    distance = dx * dx + dy * dy;
                    if (distance < DAT_00108020) {
                        found = 1;
                        DAT_00108020 = distance;
                        DAT_00108044 = base + index * 7;
                    }
                }
            } else {
                int *base;
                int count;

                base = *(int **)(record + 8);
                count = *(int *)(record + 0x1c);
                for (index = 0; index < count; index++) {
                    dx = base[index * 3 + 1] - DAT_00108038;
                    dy = base[index * 3 + 2] - DAT_00108040;
                    distance = dx * dx + dy * dy;
                    if (distance < DAT_00108020) {
                        found = 1;
                        DAT_00108020 = distance;
                        DAT_00108054 = base + index * 3;
                    }
                }
            }
        }
    }
    if (mode == 0) {
        int *nearest;

        nearest = DAT_00108054;
        DAT_0010804c = nearest[1];
        DAT_00108050 = nearest[2];
        value = *(float *)nearest;
        if (DAT_0011d274 & 8) {
            value = value + DAT_0010040c;
        }
        DAT_00108048 = value;
        if (!(value < DAT_00100410)) {
            DAT_00108048 = value + DAT_00100414;
        }
    }
    return found;
}
