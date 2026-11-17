/* The rebuilt `curve_speed` module.
 *
 * The original sits at linear 0x222e0 in STARTH.EXE and holds 49 instructions
 * and 150 bytes up to the `ret 4` at 0x22373.  The two pad bytes at 0x22376 and
 * the next record at 0x22378 give the 152-byte extent, so the store record
 * keeps the 152-byte extent and sets no separate function size.  The identity
 * check uses 49 instructions.
 *
 * The function takes a pointer in eax and a float fraction on the stack.  The
 * pointer addresses three two-dimensional control points at a 12-byte stride:
 * the first point at `+0x00` and `+0x04`, the second point at `+0x0c` and
 * `+0x10`, and the third point at `+0x18` and `+0x1c`.  The function forms the
 * interpolated chord
 *
 *     dx = (P0x - 2 * P1x + P2x) * t + P1x - P0x
 *     dy = (P0y - 2 * P1y + P2y) * t + P1y - P0y
 *
 * and returns the length of that chord.  The length is the magnitude of the
 * quadratic curve derivative at the fraction t, so the module names the
 * function curve_speed.
 *
 * A zero length returns 1.0, because the caller divides the frame step by the
 * result.  The two callers at 0x1d33d in object_revolution_step at 0x1d30c and
 * at 0x20b35 in opponent_path_update at 0x20b1c both divide a frame step by the
 * result.  The callers sit in the section The vehicle state chain and the
 * section object_update_all callees, and the function serves the spline family
 * of the vehicle state chain, so the record takes the section The vehicle state
 * chain.  The original has no symbol, so the record keeps the name with the
 * confidence inferred.
 *
 * The float 2.0 sits at object-3 offset 0xd78 and arrives through a global,
 * because the minimal image does not load the constant segment.  The original
 * calls the square-root wrapper at 0x22bec, so the local curve_speed_fpu.asm
 * supplies `sqrt_` from the hardware.  The original has the EBP frame, so the
 * module builds with `-of+`.  The module uses no assembly implementation.
 */
extern float curve_speed_scale;

extern double sqrt(double value);

float curve_speed(float *curve, float t)
{
    float x;
    float y;
    float distance;

    x = (curve[0] - curve[3] * curve_speed_scale + curve[6]) * t
        + curve[3] - curve[0];
    y = (curve[1] - curve[4] * curve_speed_scale + curve[7]) * t
        + curve[4] - curve[1];
    distance = (float)sqrt((double)(x * x + y * y));
    if (distance == 0.0f) {
        return 1.0f;
    }
    return distance;
}
