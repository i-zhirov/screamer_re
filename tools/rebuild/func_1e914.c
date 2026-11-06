/* The rebuilt function at linear 0x1e914 in STARTH.EXE.
 *
 * The original body holds 160 instructions and 452 bytes up to the `ret 4`
 * at 0x1ead5.  The next function starts at 0x1eae0, so the store record keeps
 * the 460-byte extent and sets no function size.  The identity check uses 160
 * instructions.
 *
 * The function takes the output pointer in eax, the three control points in
 * edx, and the fraction on the stack.  It evaluates the uniform quadratic
 * B-spline basis
 *
 *     w0 = 0.5 * (1 - t)^2
 *     w1 = 1 - (w0 + w2)
 *     w2 = 0.5 * t^2
 *
 * over the three points and writes the sampled position to the output at
 * `+0x00`, `+0x04`, and `+0x08`.  It then samples the same basis at
 * `t + 0.002`, subtracts the first sample, normalizes the difference, and
 * writes the three tangent components to `+0x0c`, `+0x10`, and `+0x14`.  The
 * normalized difference is the forward tangent of the curve.
 *
 * No direct call, no data reference, and no LE relocation target reaches the
 * function, so it has no known caller.  The sibling at 0x1e7b4 evaluates the
 * same basis in two dimensions and the opponent path update at 0x20b1c calls
 * it, so this function is the three-dimensional variant of the same curve
 * sampler.  The name stays `func_1e914`, because the relation is weak.
 *
 * The original reads the double 0.5 at object-3 offset 0xbb8 and the float
 * 0.002 at object-3 offset 0xbc0.  The constants arrive through globals,
 * because the minimal image does not load the constant segment.  The original
 * calls the square-root helper at 0xc6cbe, so the local FPU stub supplies
 * `sqrt_`.  The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sqrt(double value);

extern double func_1e914_basis_scale;
extern float func_1e914_sample_step;

void func_1e914(float *out, float *points, float t)
{
    float w0;
    float w1;
    float w2;
    float s0;
    float s1;
    float s2;
    float dx;
    float dy;
    float dz;
    float inv;

    w0 = (1.0 - t) * (1.0 - t) * func_1e914_basis_scale;
    w2 = t * t * func_1e914_basis_scale;
    w1 = 1.0 - (w0 + w2);

    out[0] = points[3] * w1 + points[0] * w0 + points[6] * w2;
    out[1] = points[1] * w0 + points[4] * w1 + points[7] * w2;
    out[2] = points[2] * w0 + points[5] * w1 + points[8] * w2;

    t = t + func_1e914_sample_step;

    w0 = (1.0 - t) * (1.0 - t) * func_1e914_basis_scale;
    w2 = t * t * func_1e914_basis_scale;
    w1 = 1.0 - (w0 + w2);

    s0 = points[0] * w0 + points[3] * w1 + points[6] * w2;
    s1 = points[1] * w0 + points[4] * w1 + points[7] * w2;
    s2 = points[2] * w0 + points[5] * w1 + points[8] * w2;

    dx = s0 - out[0];
    dy = s1 - out[1];
    dz = s2 - out[2];
    inv = (float)(1.0 / sqrt((double)(dx * dx + dy * dy + dz * dz)));
    out[3] = dx * inv;
    out[4] = dy * inv;
    out[5] = dz * inv;
}
