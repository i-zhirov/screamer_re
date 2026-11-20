/* The rebuilt `sound_effect_timer_clear` module.
 *
 * The original sits at linear 0x24d7c in STARTH.EXE and holds 12 instructions
 * and 27 bytes up to the `ret` at 0x24d96.  Nine pad bytes follow the `ret`,
 * and the next record starts at 0x24da0, so the store record keeps the 36-byte
 * extent and sets no separate function size.  The identity check uses 12
 * instructions.
 *
 * The integer argument arrives in the Watcom register order: `eax` holds the
 * effect index.  The function stores the value 0 into the cell of the index in
 * the array at DAT_001922c.
 *
 * The array at DAT_001922c holds one timer per effect.  `sound_reset` at
 * 0x2471c clears the whole array, and `sound_effect_delay` at 0x24c4c,
 * `sound_effect_repeat` at 0x24cc8, and `sound_effect_trigger` at 0x24d30 read
 * and write one cell each.  A cell at 0 is re-armed, so the effect can fire
 * again after the next delay.
 *
 * The only direct caller is `track_timer_update` at 0x1960c, with the call site
 * at 0x1966b.  It loads the index 0x16 into `eax` and calls this function, so
 * the effect timer at index 0x16 restarts when the track timer updates.
 *
 * The array index, the single-cell store, and the caller support the name
 * sound_effect_timer_clear.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no call, so the module
 * needs no FPU stub and no assembly implementation.
 */
extern int DAT_001922c[27];

void sound_effect_timer_clear(int effect)
{
    DAT_001922c[effect] = 0;
}
