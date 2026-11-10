/* The rebuilt `sound_effect_delay` module.
 *
 * The original sits at linear 0x24c4c in STARTH.EXE and holds 36 instructions
 * and 122 bytes up to the `ret 4` at 0x24cc3.  Two pad bytes follow the `ret`,
 * and the next record starts at 0x24cc8, so the store record keeps the
 * 124-byte extent and sets no separate function size.  The identity check uses
 * 36 instructions.
 *
 * The two integer arguments arrive in the Watcom register order: `eax` holds
 * the enable signal and `edx` holds the effect index.  The float limit makes
 * the compiler pass the remaining argument on the stack, so the callee cleans
 * it with `ret 4`.
 *
 * The array at `DAT_001922c` holds one float timer per effect.  The function
 * adds the frame delta `DAT_00118e08` to the timer of the effect.  When the
 * timer reaches the limit, the function enqueues the effect through
 * `sound_enqueue` with the variant mode 3 and the delay 0.7, then latches the
 * timer at -1.  A latched timer returns at once, so the effect fires one time
 * per enabled period.  A clear enable signal resets the timer to 0, so the
 * next enabled period starts a new delay.
 *
 * The array index, the frame delta, the enqueue, and the latch support the
 * name sound_effect_delay.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function needs no math helper and no truncation, so the shared
 * `fpu_stub.asm` supplies the FPU run-time symbols.  The module uses no
 * assembly.
 */
extern float DAT_00118e08;
extern float DAT_001922c[27];

extern void sound_enqueue(int effect, int mode, float delay);

int sound_effect_delay(int enabled, int effect, float limit)
{
    if (enabled == 0) {
        DAT_001922c[effect] = 0.0f;
        return 0;
    }
    if (DAT_001922c[effect] < 0.0f) {
        return 0;
    }
    DAT_001922c[effect] = DAT_001922c[effect] + DAT_00118e08;
    if (DAT_001922c[effect] < limit) {
        return 0;
    }
    sound_enqueue(effect, 3, 0.7f);
    DAT_001922c[effect] = -1.0f;
    return 1;
}
