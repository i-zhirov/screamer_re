/* The rebuilt `row_offset_table_build` module.
 *
 * The original sits at linear 0x4d2dc in STARTH.EXE and holds 22 instructions
 * and 61 bytes up to the `ret` at 0x4d318.  The next record starts at 0x4d32c,
 * so the store record keeps the 80-byte extent and sets no separate function
 * size.  The identity check uses 22 instructions.
 *
 * The function takes no argument.  It walks the index from 0 to 0x1e0 and
 * stores the row byte offset at the slot of the table at 0x20bbc.  Each entry
 * is the index times 640 bytes.  The table holds 481 entries from index 0 to
 * 0x1e0.
 *
 * The only direct caller is `stream_layer_init` at 0x4c810, with the call site
 * at 0x4c82a.  That function calls this row offset table first, then the
 * reciprocal table at 0x4d214 and the inverse-cosine table at 0x4d260, then
 * loads the perspective table and the visibility table.  The caller sits in
 * the section The TABS table loaders, so the record takes the same section.
 * The caller, the fixed 640-byte row step, and the table at 0x20bbc support
 * the name `row_offset_table_build`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no call, so the module
 * needs no FPU stub and no assembly.
 */
extern unsigned int row_offset_table[];

void row_offset_table_build(void)
{
    int index;

    for (index = 0; index <= 0x1e0; index = index + 1) {
        row_offset_table[index] = (index * 5) << 7;
    }
}
