/* The rebuilt `sound_effect_trigger` module.
 *
 * The original sits at linear 0x24d30 in STARTH.EXE and holds 18 instructions
 * and 67 bytes up to the `ret` at 0x24d72.  The `return 0` block is out of line
 * at 0x24d73 through the `ret` at 0x24d78, and the next record starts at
 * 0x24d7c, so the store record keeps the 76-byte extent and sets no separate
 * function size.  The identity check uses 18 instructions.
 *
 * The two integer arguments arrive in the Watcom register order: `eax` holds
 * the effect index and `edx` holds the variant mode.
 *
 * The array at DAT_001922c holds one timer per effect.  The value 0xbf800000 is
 * the float -1.0.  The sibling `sound_effect_delay` at 0x24c4c latches its
 * timer at -1 after one firing, and `sound_reset` at 0x2471c clears the array
 * to 0.  A cell at -1 is latched, so this function returns 0 and enqueues
 * nothing.  Any other cell is armed, so the function enqueues the effect
 * through `sound_enqueue` at 0x247fc with the given mode and the delay 0.7,
 * latches the cell at -1, and returns 1.
 *
 * The only direct caller is `car_sound_update` at 0x242bc, with the call sites
 * at 0x24363, 0x24420, and 0x245ec.  It passes the variant mode 3 to the
 * effects 0x16, 0x1a, and 0x13.
 *
 * The array index, the latch, and the enqueue support the name
 * sound_effect_trigger.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, but the `sound_enqueue` call
 * passes a float delay, so the recipe adds the shared `fpu_stub.asm` for the
 * FPU run-time symbols.  The module uses no assembly implementation.
 */
extern unsigned int DAT_001922c[27];

extern void sound_enqueue(int effect, int mode, float delay);

int sound_effect_trigger(int effect, int mode)
{
    if (DAT_001922c[effect] == 0xbf800000u) {
        return 0;
    }
    sound_enqueue(effect, mode, 0.7f);
    DAT_001922c[effect] = 0xbf800000u;
    return 1;
}
