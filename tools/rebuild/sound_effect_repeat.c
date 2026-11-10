/* The rebuilt `sound_effect_repeat` module.
 *
 * The original sits at linear 0x24cc8 in STARTH.EXE and holds 33 instructions
 * and 104 bytes up to the `ret 4` at 0x24d2d.  The next record starts at
 * 0x24d30, so the store record keeps the 104-byte extent and sets no separate
 * function size.  The identity check uses 33 instructions.
 *
 * The two integer arguments arrive in the Watcom register order: `eax` holds
 * the enable signal and `edx` holds the effect index.  The float limit makes
 * the compiler pass the remaining argument on the stack, so the callee cleans
 * it with `ret 4`.
 *
 * The array at `DAT_001922c` holds one float timer per effect.  The function
 * adds the frame delta `DAT_00118e08` to the timer of the effect.  When the
 * timer reaches the limit, the function enqueues the effect through
 * `sound_enqueue` with the variant mode 3 and the delay 0.7, then restarts the
 * timer at 0.  A restarted timer can reach the limit again, so the effect
 * repeats at each limit crossing.  This is the difference from the sibling
 * `sound_effect_delay` module, which latches the timer at -1 after one firing.
 * A clear enable signal resets the timer to 0 and enqueues nothing.
 *
 * The array index, the frame delta, the enqueue, and the restart support the
 * name sound_effect_repeat.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses the x87 stack for the add and the compare, so the module links
 * the shared `fpu_stub.asm` for the run-time symbols `__8087`,
 * `__init_387_emulator`, and `_fltused_`.  The module uses no assembly
 * implementation.
 */
extern float DAT_00118e08;
extern float DAT_001922c[27];

extern void sound_enqueue(int effect, int mode, float delay);

int sound_effect_repeat(int enabled, int effect, float limit)
{
    if (enabled == 0) {
        DAT_001922c[effect] = 0.0f;
        return 0;
    }
    DAT_001922c[effect] = DAT_00118e08 + DAT_001922c[effect];
    if (DAT_001922c[effect] < limit) {
        return 0;
    }
    sound_enqueue(effect, 3, 0.7f);
    DAT_001922c[effect] = 0.0f;
    return 1;
}
