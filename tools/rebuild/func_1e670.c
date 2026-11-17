/* The rebuilt function at linear 0x1e670 in STARTH.EXE.
 *
 * The original body holds 52 instructions and 142 bytes up to the `ret 4` at
 * 0x1e6fb.  The next function starts at 0x1e700 after the padding bytes at
 * 0x1e6fe, so the store record keeps the 144-byte extent and sets no function
 * size.  The identity check uses 52 instructions.
 *
 * The function takes the output pointer in eax, the three control points in
 * edx, and the fraction on the stack.  It evaluates the uniform quadratic
 * B-spline basis
 *
 *     w0 = 0.5 * (1 - t)^2
 *     w1 = 1 - (w0 + w2)
 *     w2 = 0.5 * t^2
 *
 * over the three points at a three-float stride and writes the sampled
 * position to the output at `+0x00` and `+0x04`.  It leaves the third point
 * component unread, so it samples the two-dimensional position.
 *
 * No direct call, no data reference, and no LE relocation target reaches the
 * function, so it has no known caller.  The sibling at 0x1e700 evaluates the
 * same basis in three dimensions and writes the three components.  The sibling
 * at 0x1e7b4 evaluates the same basis in two dimensions and writes the
 * position and the forward tangent.  The relation is weak, so the name stays
 * the weak fallback `func_1e670`.
 *
 * The original reads the double 0.5 at object-3 offset 0xb9c.  The constant
 * arrives through a global, because the minimal image does not load the
 * constant segment.  The original has the EBP frame, so the module builds with
 * `-of+`.  The module uses the shared FPU stub, because it needs no truncation
 * and no math helper.
 */
extern double func_1e670_basis_scale;

void func_1e670(float *out, float *points, float t)
{
    float w0;
    float w1;
    float w2;

    w0 = (1.0 - t) * (1.0 - t) * func_1e670_basis_scale;
    w2 = t * t * func_1e670_basis_scale;
    w1 = 1.0 - (w0 + w2);

    out[0] = points[3] * w1 + points[0] * w0 + points[6] * w2;
    out[1] = points[1] * w0 + points[4] * w1 + points[7] * w2;
}
