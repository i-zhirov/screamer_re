/* The rebuilt `map_collision_check` module.
 *
 * The original sits at linear 0x11c00 in STARTH.EXE and holds 64 instructions
 * and 193 bytes up to the `ret` at 0x11cc0.  The normal exit jumps to the
 * second epilogue at 0x11cc1, which returns 0.  The next function starts at
 * 0x11ccc, so the store record keeps the 204-byte extent and sets no function
 * size.  The identity check uses 64 instructions.
 *
 * The function walks the three-by-three cell neighborhood of the spatial grid
 * at `DAT_00102fa4`.  It reads the world position from `DAT_00108038` and
 * `DAT_00108040`, adds the map bias 0x100000 to each axis, and divides each sum
 * by the cell size 0x2000.  The table at object 3 offset 0x1cf0 holds nine
 * 0xc-byte records: the first field is the column offset, the third field is
 * the row offset, and the second field is unused.  The nine offsets cover the
 * full three-by-three neighborhood.
 *
 * A row pointer or a cell pointer equal to -1 marks an empty slot, so the scan
 * skips it.  The row slot address lands in `DAT_00108030`, and the cell slot
 * address lands in `DAT_00108034`, where the callee `collision_cell_test` reads
 * it.  The function calls `collision_cell_test` at 0x11ccc with the mode from
 * `eax` for each present cell.  A non-zero result reports the hit and returns
 * 1.  The scan returns 0 when no cell reports a hit.
 *
 * The callers are `wheel_contact_update` at 0x11a30,
 * `body_point_collision_detect` at 0x11e74, `terrain_contact_update` at
 * 0x1cbac, `car_collision_response` at 0x17828, `opponent_route_setup` at
 * 0x1517c, and `ai_update` at 0x205c4.  They publish the world position through
 * the two globals and read the contact back, so the function takes the section
 * `The vehicle state chain` and the name `map_collision_check` with the
 * confidence inferred.  `track_nearest_node` at 0x120c0 scans the same grid
 * with the same bias and cell size.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub, and
 * the module uses no assembly.
 */
struct cell_offset {
    int column;
    int spare;
    int row;
};

extern int *DAT_00102fa4;
extern struct cell_offset DAT_00101cf0[9];
extern int *DAT_00108030;
extern int *DAT_00108034;
extern int DAT_00108038;
extern int DAT_00108040;

extern int collision_cell_test(int mode);

int map_collision_check(int mode)
{
    int xcell;
    int ycell;
    int index;
    int *row_slot;
    int row;
    int *cell_slot;
    int cell;

    xcell = (DAT_00108038 + 0x100000) >> 13;
    ycell = (DAT_00108040 + 0x100000) >> 13;
    for (index = 0; index < 9; index++) {
        row_slot = DAT_00102fa4 + ycell + DAT_00101cf0[index].row;
        DAT_00108030 = row_slot;
        row = *row_slot;
        if (row == -1)
            continue;
        cell_slot = (int *)row + xcell + DAT_00101cf0[index].column;
        DAT_00108034 = cell_slot;
        cell = *cell_slot;
        if (cell == -1)
            continue;
        if (collision_cell_test(mode))
            return 1;
    }
    return 0;
}
