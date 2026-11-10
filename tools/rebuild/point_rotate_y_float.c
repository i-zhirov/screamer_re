/* The rebuilt `point_rotate_y_float` module.
 *
 * The original sits at linear 0x22278 in STARTH.EXE and holds 36 instructions
 * and 101 bytes up to the `ret 4` at 0x222da.  The next record starts at
 * 0x222e0, so the store record keeps the 104-byte extent and sets no separate
 * function size.  The identity check uses 36 instructions.
 *
 * The two arguments arrive in the Watcom register order: `eax` holds the output
 * point and `edx` holds the input point.  The float angle makes the compiler
 * pass the remaining argument on the stack, so the callee cleans it with
 * `ret 4`.
 *
 * The point holds float fields at `+0x00` and `+0x08`, and the middle value at
 * `+0x04` stays unchanged.  The function turns the (x, z) pair by the angle:
 * the output x is `in.x * cos + in.z * sin`, and the output z is
 * `in.z * cos - in.x * sin`.  A point uses x and z for the ground plane and y
 * for the height, so the function turns the ground-plane pair about the
 * vertical y axis.  The integer helper at 0x2218c turns the same pair with
 * integer fields, and `point_rotate_x` at 0x221fc turns the (y, z) pair, so the
 * three functions form the integer and float forms of the point rotation.
 *
 * The code object holds no direct call to 0x22278, so the record takes the
 * section Program functions with the point-rotation family.  The data flow
 * supports the name `point_rotate_y_float`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original calls `cos` through the Watcom helper at 0x4da66 and `sin`
 * through 0x4da72.  This harness links no math library, so the local assembly
 * stub supplies `cos` and `sin` from the hardware.  The function converts no
 * value to an integer, so it needs no `__CHP` stub.  The original has the EBP
 * frame, so the module builds with `-of+`.  The module uses no assembly
 * implementation.
 */
extern double cos(double value);
extern double sin(double value);

void point_rotate_y_float(float *out, float *in, float angle)
{
    float cosine;
    float sine;

    cosine = cos(angle);
    sine = sin(angle);

    out[0] = in[0] * cosine + in[2] * sine;
    out[2] = in[2] * cosine - in[0] * sine;
}
