/* The rebuilt `point_rotate_x` module.
 *
 * The original sits at linear 0x221fc in STARTH.EXE and holds 42 instructions
 * and 124 bytes up to the `ret 4` at 0x22275.  The next record starts at
 * 0x22278, so the store record keeps the 124-byte extent and sets no separate
 * function size.  The identity check uses 42 instructions.
 *
 * The two arguments arrive in the Watcom register order: `eax` holds the output
 * point and `edx` holds the input point.  The float angle makes the compiler
 * pass the remaining argument on the stack, so the callee cleans it with
 * `ret 4`.
 *
 * The point holds three integers at `+0x00`, `+0x04`, and `+0x08`.  The
 * function copies the x value and turns the (y, z) pair by the angle.  The
 * helper at 0x2218c turns the (x, z) pair, and the float helper at 0x22278
 * turns the same pair with float fields, so the three functions form the integer
 * and float forms of the point rotation.
 *
 * The only direct caller is `player_car_wheel_effect_update` at 0x19980, with
 * the call site at 0x19a01.  That function first turns the slot pair by the
 * negated heading plus the yaw through the helper at 0x2218c, and then it turns
 * the result by the pitch through this function into the slot position at
 * `+0x0c`, `+0x10`, and `+0x14`.  The caller runs in the vehicle state chain,
 * so the record takes the section The vehicle state chain.  The data flow
 * supports the name `point_rotate_x`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original calls `cos` through the Watcom helper at 0x4da66 and `sin`
 * through 0x4da72.  This harness links no math library, so the local assembly
 * stub supplies `cos` and `sin` from the hardware.  The original truncates
 * through the helper at 0x4d9f4, so the local stub also makes `__CHP` truncate
 * toward zero.  The original has the EBP frame, so the module builds with
 * `-of+`.  The module uses no assembly implementation.
 */
extern double cos(double value);
extern double sin(double value);

void point_rotate_x(int *out, int *in, float angle)
{
    float cosine;
    float sine;

    cosine = cos(angle);
    sine = sin(angle);

    out[0] = in[0];
    out[1] = (int)(in[1] * cosine + in[2] * sine);
    out[2] = (int)(in[2] * cosine - in[1] * sine);
}
