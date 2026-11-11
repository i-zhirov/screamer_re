/* The rebuilt `reciprocal_table_build` module.
 *
 * The original sits at linear 0x4d214 in STARTH.EXE and holds 28 instructions
 * and 76 bytes up to the `ret` at 0x4d25f.  The next record starts at 0x4d260,
 * so the store record keeps the 76-byte extent and sets no separate function
 * size.  The identity check uses 28 instructions.
 *
 * The function takes no argument.  It walks the index from 0 to 699.  For each
 * index it forms the reciprocal 1 / index, scales it by the double constant
 * 32767.0 at object-3 offset 0x1767, and truncates it to an integer through the
 * Watcom helper at 0x4d9f4.  It stores the result at the slot of the table
 * through the pointer at 0xc5da0, so the table holds 700 entries.
 *
 * The only direct caller is `stream_layer_init` at 0x4c810, with the call site
 * at 0x4c82f.  That function calls the row offset table at 0x4d2dc, then this
 * table, then the inverse-cosine table at 0x4d260, then loads the perspective
 * table and the visibility table.  The caller sits in the section The TABS
 * table loaders, so the record takes the same section.  The caller, the
 * reciprocal arithmetic, and the 32767.0 scale support the name
 * `reciprocal_table_build`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original truncates through the Watcom helper at 0x4d9f4, so the local FPU
 * stub makes `__CHP` truncate toward zero.  The harness does not load the
 * constant segment, so the scale and the table pointer arrive through globals
 * that the harness sets at run time.  The module uses no assembly
 * implementation.
 */
extern double reciprocal_scale;
extern int *DAT_000c5da0;

void reciprocal_table_build(void)
{
    volatile int index;
    volatile int scaled;

    for (index = 0; index < 700; index++) {
        scaled = (int)((1.0 / (double)index) * reciprocal_scale);
        DAT_000c5da0[index] = scaled;
    }
}
