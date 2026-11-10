/* The rebuilt `spatial_grid_relocate` module.
 *
 * The original sits at linear 0x4cb4c in STARTH.EXE and holds 75 instructions
 * and 194 bytes up to the `ret` at 0x4cc0d.  The next function starts at
 * 0x4cc10 after the two `mov eax, eax` pad bytes, so the store record keeps the
 * 196-byte extent and sets no separate function size.  The identity check uses
 * 75 instructions.
 *
 * The function has one direct caller.  The loader at 0x4cae4 builds the
 * `%s.MMP` member name from DAT_002154c, loads the member into the resource
 * DAT_0021468 through the loader at 0x4c3e6, copies the resource pointer into
 * DAT_00102fa4 at 0x4cb39, and calls this function at 0x4cb3e.  The copy is the
 * last store before the call, so the loaded region base reaches the function
 * through DAT_00102fa4.
 *
 * The region at the base holds the spatial grid of the track.  The grid has
 * 0x100 rows.  A row holds 0x100 cells.  A cell holds five pointer slots.  The
 * collision checks at 0x11c00 and 0x120c0 and the radar draw at 0x1057c read
 * the same grid through DAT_00102fa4, so the record takes the section Resource
 * loader, where the member loader sits.
 *
 * The function adds the region base to three levels of the pointer tree:
 *
 *     1  every row pointer that is not -1
 *     2  every cell pointer that is not -1
 *     3  every cell slot that is not 0
 *
 * The sentinel -1 of a row or a cell and the zero slot stay unchanged.  The
 * function writes the same base back to DAT_00102fa4, so the base is unchanged.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub, and
 * the module uses no assembly.
 */
struct grid_cell {
    unsigned int slot[5];
};

extern unsigned int DAT_00102fa4;

void spatial_grid_relocate(void)
{
    unsigned int base;
    unsigned int *row_table;
    int row_index;
    int cell_index;
    int slot_index;

    base = DAT_00102fa4;
    row_table = (unsigned int *)base;
    for (row_index = 0; row_index < 0x100; ++row_index) {
        unsigned int row_offset;
        unsigned int *row;

        row_offset = row_table[row_index];
        if (row_offset == 0xffffffffu) {
            continue;
        }
        row_table[row_index] = row_offset + base;
        row = (unsigned int *)(row_offset + base);
        for (cell_index = 0; cell_index < 0x100; ++cell_index) {
            unsigned int cell_offset;
            struct grid_cell *cell;

            cell_offset = row[cell_index];
            if (cell_offset == 0xffffffffu) {
                continue;
            }
            row[cell_index] = cell_offset + base;
            cell = (struct grid_cell *)(cell_offset + base);
            for (slot_index = 0; slot_index < 5; ++slot_index) {
                if (cell->slot[slot_index] != 0) {
                    cell->slot[slot_index] += base;
                }
            }
        }
    }
    DAT_00102fa4 = base;
}
