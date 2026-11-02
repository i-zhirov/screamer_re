/* The rebuilt `object_integrate` module.
 *
 * The original sits at linear 0x1d6bc in STARTH.EXE.  The body holds 82
 * instructions and 339 bytes up to the first `ret` at 0x1d80e: the `edx` save,
 * the frame, the 8-byte local slot, the two float integrations, the factor
 * damping, the factor threshold, the two target compares, the dwell timer, the
 * two snap stores, and the epilogue.  The merged store record keeps the
 * 376-byte extent.
 *
 * The function adds the scaled velocity to the position at `+0x454` and
 * `+0x45c`.  The scale is the factor at `+0x450` times the frame delta
 * `DAT_00118e04`, and each sum truncates toward zero.  The function then
 * multiplies the factor by the damping at `DAT_0010cf08 + 0x13c` and clears
 * the factor when it stays below the 10.0 threshold.
 *
 * When both positions equal the target at `+0x160` and `+0x168`, the function
 * adds the frame delta to the dwell timer `+0x46c`.  When the timer reaches the
 * 3.0 threshold, it multiplies both positions by the snap scale at
 * `DAT_0010cf08 + 0x180` and truncates.  Otherwise it clears the timer.
 *
 * The two constants arrive through the globals `integrate_factor_threshold`
 * and `integrate_dwell_threshold`.  The harness sets the exact bit patterns.
 * The original reads the constants from object 3 at offsets 0xb24 and 0xb28.
 *
 * The four conversions to an integer use the Watcom helper at 0x4d9f4, which
 * truncates toward zero.  The module uses the truncating FPU stub, because the
 * shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern float DAT_00118e04;
extern unsigned int DAT_0010cf08;
extern float integrate_factor_threshold;
extern float integrate_dwell_threshold;

void object_integrate(char *object)
{
    float factor;

    factor = *(float *)(object + 0x450);
    *(int *)(object + 0x454) = (int)(*(float *)(object + 0x460) * factor
        * DAT_00118e04 + *(int *)(object + 0x454));
    *(int *)(object + 0x45c) = (int)(*(float *)(object + 0x468) * factor
        * DAT_00118e04 + *(int *)(object + 0x45c));
    factor = factor * *(float *)(DAT_0010cf08 + 0x13c);
    *(float *)(object + 0x450) = factor;
    if (!(factor >= integrate_factor_threshold)) {
        *(float *)(object + 0x450) = 0.0f;
    }
    if (*(int *)(object + 0x454) == *(int *)(object + 0x160)
        && *(int *)(object + 0x45c) == *(int *)(object + 0x168)) {
        *(float *)(object + 0x46c) =
            *(float *)(object + 0x46c) + DAT_00118e04;
        if (*(float *)(object + 0x46c) >= integrate_dwell_threshold) {
            *(int *)(object + 0x454) = (int)(*(int *)(object + 0x454)
                * *(float *)(DAT_0010cf08 + 0x180));
            *(int *)(object + 0x45c) = (int)(*(int *)(object + 0x45c)
                * *(float *)(DAT_0010cf08 + 0x180));
        }
    } else {
        *(int *)(object + 0x46c) = 0;
    }
}
