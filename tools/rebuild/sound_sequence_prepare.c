/* The rebuilt `sound_sequence_prepare` module.
 *
 * The original sits at linear 0x83354 in STARTH.EXE and holds 20 instructions
 * and 80 bytes up to the `ret` at 0x833a3.  The next function
 * `sound_source_collect` starts at 0x833a4, so the store record keeps the
 * 80-byte extent and sets no separate function size.  The identity check uses
 * 20 instructions.
 *
 * The only direct caller is `sound_effects_update` at 0x81ab0 with the call
 * site at 0x81ac1.  That caller already names this function
 * `sound_sequence_prepare`, and the finding of `0x81ab0` calls it the sequence
 * helper.  The function builds the active sound list, so the record keeps the
 * name with the confidence inferred and the section Program functions.
 *
 * The function takes the row count in `eax` and the row pointer in `esi` from
 * `sound_range_update` at 0x830c8.  C cannot return a pointer in `esi`, so the
 * callee stores the row pointer in the global `grid_row`.  The function walks
 * the row entries, skips an entry equal to -1, and calls
 * `sound_source_collect` at 0x833a4 once per other entry.  The output cursor
 * starts at 0x40330 and advances through the callee.
 *
 * The original writes the literal `0xaaaabbbb` to the row limit `0x734f4` and
 * adds the same literal to the row entry before the call.  The value is the
 * linker fill value, so the rebuild keeps both values.  The loop runs
 * `count + 1` times, because the original decrements the counter after the
 * body and continues while the result is not negative.
 *
 * The active count is the output length divided by the 8-byte record with an
 * arithmetic shift by three.  The function stores that count at 0x72488 and
 * returns it in `eax`.
 *
 * The original has no EBP frame and uses no floating-point instruction, so the
 * module builds without `-of+` and links no FPU stub.  The module uses no
 * assembly implementation.
 */
extern int *grid_row;
extern int DAT_000734f0;
extern int DAT_000734f4;
extern int DAT_000734fc;
extern int DAT_00072488;

extern int sound_range_update(void);
extern int *sound_source_collect(int *regions, int *output);

int sound_sequence_prepare(void)
{
    int *cursor;
    int *output;
    int entry;

    DAT_000734f0 = sound_range_update();
    cursor = grid_row;
    output = (int *)0x40330;
    DAT_000734fc = 0;
    do {
        entry = *cursor;
        cursor++;
        if (entry != -1) {
            DAT_000734f4 = 0xaaaabbbb;
            output = sound_source_collect(
                (int *)(entry + 0xaaaabbbb), output);
        }
        DAT_000734f0--;
    } while (DAT_000734f0 >= 0);
    DAT_00072488 = (int)(((char *)output - (char *)0x40330) >> 3);
    return DAT_00072488;
}
