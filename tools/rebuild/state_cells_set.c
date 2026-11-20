/* The rebuilt `state_cells_set` module.
 *
 * The original sits at linear 0x8bd54 in STARTH.EXE and holds 16 instructions
 * and 64 bytes up to the `ret` at 0x8bd93.  A later function starts at
 * 0x8bd94, and the next recorded function starts at 0x8bdec, so the store
 * record keeps the 152-byte extent and sets no separate function size.  The
 * identity check uses 16 instructions.
 *
 * The function stores its argument in the eight data cells at 0x73bf7,
 * 0x73c05, 0x73c13, 0x73c21, 0x73c68, 0x73c76, 0x73c84, and 0xc7cf0.  It
 * negates the value and stores it at 0xc7cf4.  It subtracts 0x80 from the
 * value and stores the result at 0x72bf0.  The object data holds the default
 * state for the argument 0xffff54e0, and the four direct callers pass a table
 * value or that default.
 *
 * The four callers are race_start at 0x27770, loading_screen at 0x2a364,
 * menu_space_init at 0x29d08, and race_results_screen at 0x2a558.  The table
 * values and the default select the current state, and the cells hold the
 * far depth limit at 0xc7cf0 that sound_mix_prepare at 0x8aa60 and the renderer
 * read.  The caller data flow supports the name state_cells_set with the
 * confidence inferred.
 *
 * The original holds no EBP frame, so the module builds without `-of+`.  The
 * function uses no floating-point instruction and calls no helper, so the
 * module needs no FPU stub and no assembly implementation.
 */
extern int DAT_00073bf7;
extern int DAT_00073c05;
extern int DAT_00073c13;
extern int DAT_00073c21;
extern int DAT_00073c68;
extern int DAT_00073c76;
extern int DAT_00073c84;
extern int DAT_000c7cf0;
extern int DAT_000c7cf4;
extern int DAT_00072bf0;

void state_cells_set(int value)
{
    DAT_00073bf7 = value;
    DAT_00073c05 = value;
    DAT_00073c13 = value;
    DAT_00073c21 = value;
    DAT_00073c68 = value;
    DAT_00073c76 = value;
    DAT_00073c84 = value;
    DAT_000c7cf0 = value;
    DAT_000c7cf4 = -value;
    DAT_00072bf0 = value - 0x80;
}
