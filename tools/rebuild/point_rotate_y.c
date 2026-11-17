/* The rebuilt `point_rotate_y` module.
 *
 * The original sits at linear 0x2218c in STARTH.EXE and holds 38 instructions
 * and 112 bytes up to the `ret 4` at 0x221f8.  The next record starts at
 * 0x221fc, so the store record keeps the 112-byte extent and sets no separate
 * function size.  The identity check uses 38 instructions.
 *
 * The two arguments arrive in the Watcom register order: `eax` holds the output
 * point and `edx` holds the input point.  The float angle makes the compiler
 * pass the remaining argument on the stack, so the callee cleans it with
 * `ret 4`.
 *
 * The point holds three integers at `+0x00`, `+0x04`, and `+0x08`.  The
 * function turns the (x, z) pair by the angle and leaves the y value.  A point
 * uses x and z for the ground plane and y for the height, so the function turns
 * the ground-plane pair about the vertical y axis.  The integer helper at
 * 0x2218c turns the (x, z) pair, the float helper at 0x22278 turns the same
 * pair with float fields, and `point_rotate_x` at 0x221fc turns the (y, z) pair,
 * so the three functions form the integer and float forms of the point
 * rotation.
 *
 * The direct callers span the vehicle state chain, the race session, the object
 * update, the object prepare, the object physics, and the program functions, so
 * the record takes the section Program functions.  The data flow supports the
 * name `point_rotate_y`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
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

void point_rotate_y(int *out, int *in, float angle)
{
    float cosine;
    float sine;

    cosine = cos(angle);
    sine = sin(angle);

    out[0] = (int)(in[0] * cosine + in[2] * sine);
    out[2] = (int)(in[2] * cosine - in[0] * sine);
}
