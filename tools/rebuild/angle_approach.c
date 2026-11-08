/* The rebuilt `angle_approach` module.
 *
 * The original sits at linear 0x22018 in STARTH.EXE and holds 93 instructions
 * and 257 bytes up to the `ret 8` at 0x22116.  The next function starts at
 * 0x2211c, so the store record keeps the 260-byte extent and sets no separate
 * function size.  The identity check uses 93 instructions.
 *
 * The function is the angle form of the one-pole filter at 0x22b80.  The first
 * argument arrives in `eax`; the float argument makes the Watcom compiler pass
 * the remaining arguments on the stack, so the callee cleans them with `ret 8`.
 *
 * The function moves the value at `*value` toward the target with the
 * coefficient `DAT_0010cf08[step]`.  When the absolute difference is at least
 * pi, it shifts the difference by one full turn, so the value takes the short
 * path.  It then wraps the value into the range zero to 2 pi: a negative value
 * gains 2 pi, and a value of 2 pi or more loses 2 pi.
 *
 * The two direct callers are `view_transition_update` at 0x19f00 with the call
 * sites 0x19fa0, 0x19faf, and 0x19fbe, and `player_view_orbit` at 0x1a3b8 with
 * the call sites 0x1a55e, 0x1a574, and 0x1a58a.  They pass the angle deltas
 * and the player angles.  The function sits next to `filter_update` at 0x22b80
 * and `timing_update` at 0x22df8, so the module takes the section
 * The timing and filter layer.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The three
 * constants pi, 2 pi, and -2 pi arrive through globals, because the harness
 * image does not load the constant segment.  The shared `fpu_stub.asm`
 * supplies the FPU run-time symbols and the float-used marker.
 */
extern float *DAT_0010cf08;
extern float angle_pi;
extern float angle_turn;
extern float angle_turn_neg;

void angle_approach(float *value, float target, int step)
{
    float difference;
    float result;

    if (*value != target) {
        difference = *value - target;
        if (difference > 0.0f) {
            if (difference < angle_pi) {
                result = difference;
            } else {
                result = difference - angle_turn;
            }
        } else {
            if (-difference < angle_pi) {
                result = difference;
            } else {
                result = difference + angle_turn;
            }
        }
        *value = target + result * DAT_0010cf08[step];
    }
    if (*value < 0.0f) {
        *value += angle_turn;
    } else if (*value >= angle_turn) {
        *value += angle_turn_neg;
    }
}
