/* The rebuilt `phase_delta` module.
 *
 * The original sits at linear 0x1fa04 in STARTH.EXE and holds 15 instructions
 * and 34 bytes up to the `ret` at 0x1fa25.  The return path for a current value
 * below the saved value follows at 0x1fa26, and the next function starts at
 * 0x1fa2c, so the store record keeps the 40-byte extent and sets no separate
 * function size.  The identity check uses 15 instructions.
 *
 * The function takes the saved value in eax and the current value in edx.  It
 * forms the difference `current - saved`.  When the current value is at least
 * the saved value, it returns the difference.  Otherwise it returns the
 * bitwise complement of the difference, which is `saved - current - 1`.
 *
 * The two direct families of callers use the result as a counter delta.  The
 * phase wait in `mouse_release_wait` at 0x1f8dc and the fade in
 * `view_fade_out` at 0x21da8 take the delta between the saved phase
 * DAT_000007f9 and the current phase.  The traffic step at 0x1eae0 and
 * `race_timeout` at 0x27f44 take the delta between the frame counter
 * DAT_000007fd and a saved frame value.  The callers sit in two sections, so
 * the record takes the section Program functions.
 *
 * The original has the EBP frame and two locals, so the module builds with
 * `-of+`.  The function holds no floating-point instruction and calls no
 * helper, so the module needs no FPU stub and no assembly implementation.
 */
unsigned int phase_delta(unsigned int saved, unsigned int current)
{
    unsigned int volatile start = saved;
    unsigned int volatile now = current;
    unsigned int delta = now - start;

    if (now < start) {
        return ~delta;
    }
    return delta;
}
