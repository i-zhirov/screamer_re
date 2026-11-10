/* The rebuilt `angle_in_arc` module.
 *
 * The original sits at linear 0x2254c in STARTH.EXE.  The first block holds 34
 * instructions and 95 bytes up to the `ret 0x10` at 0x225a8.  The body then
 * continues through the blocks at 0x225ab and 0x225f1 to the final `ret 0x10`
 * at 0x22660, so the next function starts at 0x22664 after the three padding
 * bytes at 0x22661.  The store record keeps the 280-byte extent and sets no
 * separate function size.  The identity check uses 34 instructions.
 *
 * The function takes four float arguments on the stack, so the callee cleans
 * them with `ret 0x10`.  The arguments are two boundary angles, the test angle
 * `c`, and a direction sign `d`.
 *
 * A positive `d` reduces `c` from above by one full turn while `c` is larger
 * than 2 pi.  The function then returns 1 when `c` lies strictly inside the
 * forward arc from the first boundary to the second boundary.  A zero or
 * negative `d` raises `c` by one full turn while `c` is below zero.  The
 * function then returns 1 when `c` lies strictly inside the forward arc from
 * the second boundary to the first boundary.  An arc that starts and ends at
 * the same angle contains only that angle.
 *
 * The function has no direct caller and no symbol, so the name comes from the
 * data flow.  The two boundary angles, the test angle, and the direction sign
 * support the name angle_in_arc; the record keeps the name with the confidence
 * inferred.
 *
 * The float 2 pi and -2 pi arrive through globals, because the minimal image
 * does not load the constant segment.  The original has the EBP frame, so the
 * module builds with `-of+`.  The function calls no helper, so the shared
 * `fpu_stub.asm` supplies the FPU run-time symbols and the float-used marker.
 * The module uses no assembly implementation.
 */
extern float arc_turn;
extern float arc_turn_neg;

int angle_in_arc(float start, float end, float angle, float direction)
{
    if (direction > 0.0f) {
        while (angle > arc_turn) {
            angle = angle + arc_turn_neg;
        }
        if (angle > end) {
            return end < start && start < angle;
        }
        if (end > angle) {
            return start < angle || start > end;
        }
        return start == end;
    }
    while (angle < 0.0f) {
        angle = angle + arc_turn;
    }
    if (angle > end) {
        return end > start || start > angle;
    }
    if (end > angle) {
        return start > angle && start < end;
    }
    return start == end;
}
