/* The rebuilt `direction_angle` module.
 *
 * The original sits at linear 0x226bc in STARTH.EXE and holds 48 instructions
 * and 133 bytes up to the `ret 0xc` at 0x22741.  The three pad bytes at 0x22741
 * to 0x22743 and the next function `spline_interpolate` at 0x22744 give the
 * 136-byte extent.  The store keeps the extent of the record and sets no
 * separate function size.  The candidate record at 0x226bd sits on the
 * `push ebp` of the same prologue, so the store keeps its extent to that
 * record.  The identity check uses 48 instructions.
 *
 * The only direct caller is `object_revolution_step` at 0x1d30c.  It calls this
 * function at 0x1d542 and stores the result at object offset `+0x27c`.  The
 * caller passes the x, y, and z values at `+0x264`, `+0x268`, and `+0x26c`.
 * The first argument arrives on the stack at `[ebp + 0xc]`, the second at
 * `[ebp + 0x10]`, and the third at `[ebp + 0x14]`, because the first
 * instruction `push eax` reserves one stack slot.  The function reads the
 * first and the third argument and ignores the second, so the harness calls it
 * with a direction and an unused middle value.
 *
 * The function forms `abs(asin(x))` through the Watcom inverse-sine helper at
 * 0xc5e42.  When z is above zero, it adds pi / 2.  Otherwise it subtracts the
 * angle from 5 pi / 2.  It then wraps the value into the range zero to 2 pi.
 * A value of 2 pi or more loses 2 pi.
 *
 * The four constants sit at object-3 offsets 0xd94, 0xd98, 0xd9c, and 0xda0.
 * They are pi / 2, 5 pi / 2, 2 pi, and -2 pi.  They arrive through globals,
 * because the harness image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The shared
 * `fpu_stub.asm` supplies the FPU run-time symbols and the float-used marker.
 * The harness supplies the inverse-sine function.
 */
extern double asin(double value);
extern double fabs(double value);

extern float direction_half_pi;
extern float direction_five_half_pi;
extern float direction_turn;
extern float direction_turn_neg;

float direction_angle(float x, float unused, float z)
{
    float angle;

    angle = (float)fabs(asin((double)x));
    if (z > 0.0f) {
        angle = angle + direction_half_pi;
    } else {
        angle = direction_five_half_pi - angle;
    }
    while (angle < 0.0f) {
        angle = angle + direction_turn;
    }
    while (angle >= direction_turn) {
        angle = angle + direction_turn_neg;
    }
    return angle;
}
