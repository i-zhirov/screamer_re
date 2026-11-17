/* The rebuilt `angle_step` module.
 *
 * The original sits at linear 0x21f40 in STARTH.EXE and holds 87 instructions
 * and 215 bytes up to the `ret 8` at 0x22014.  The next function starts at
 * 0x22018, so the store record keeps the 216-byte extent and sets no separate
 * function size.  The identity check uses 87 instructions.
 *
 * The function is the fixed-step form of `angle_approach` at 0x22018.  The
 * first argument arrives in `eax`; the two float arguments make the Watcom
 * compiler pass the remaining arguments on the stack, so the callee cleans
 * them with `ret 8`.
 *
 * The function moves the value at `*value` toward the target by one step.  It
 * compares the value with the target twice: the first compare sends an equal
 * value straight to the snap, and the second compare picks the sign branch.
 * When the absolute difference is at least pi, the offset takes the other
 * sign, so the value takes the short path around the circle.  When the value
 * stays within one step of the target, it snaps to the target.  It then wraps
 * the value into the range zero to 2 pi: a negative value gains 2 pi, and a
 * value of 2 pi or more loses 2 pi.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The three
 * constants pi, 2 pi, and -2 pi arrive through globals, because the harness
 * image does not load the constant segment.  The shared `fpu_stub.asm`
 * supplies the FPU run-time symbols and the float-used marker.
 */
extern double fabs(double value);
extern float angle_pi;
extern float angle_turn;
extern float angle_turn_neg;

void angle_step(float *value, float target, float step)
{
    float offset;

    if (*value != target) {
        if (*value < target) {
            if (target - *value < angle_pi) {
                offset = step;
            } else {
                offset = -step;
            }
            *value = *value + offset;
        } else {
            if (*value - target < angle_pi) {
                offset = step;
            } else {
                offset = -step;
            }
            *value = *value - offset;
        }
    }
    if (fabs(*value - target) < step) {
        *value = target;
    }
    if (*value < 0.0f) {
        *value = *value + angle_turn;
    } else if (*value >= angle_turn) {
        *value = *value + angle_turn_neg;
    }
}
