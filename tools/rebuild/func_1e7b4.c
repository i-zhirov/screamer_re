/* The rebuilt function at linear 0x1e7b4 in STARTH.EXE.
 *
 * The original body holds 124 instructions and 350 bytes up to the `ret 4`
 * at 0x1e90f.  The next function starts at 0x1e914, so the store record keeps
 * the 352-byte extent and sets no function size.  The identity check uses 124
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
 * `+0x00` and `+0x04`.  It then samples the same basis at `t + 0.002`,
 * subtracts the first sample, normalizes the difference, and writes the two
 * tangent components to `+0x0c` and `+0x10`.  The normalized difference is the
 * forward tangent of the curve.
 *
 * The two direct callers are the unnamed function at 0x1d55c and
 * `opponent_curve_eval` at 0x22378.  Both build three two-dimensional control
 * points and call this sampler.  The sibling at 0x1e914 evaluates the same
 * basis in three dimensions and keeps the name `func_1e914`, so this record
 * keeps the weak name `func_1e7b4`.
 *
 * The original reads the double 0.5 at object-3 offset 0xbac and the float
 * 0.002 at object-3 offset 0xbb4.  The constants arrive through globals,
 * because the minimal image does not load the constant segment.  The original
 * calls the square-root helper at 0xc6cbe, so the local FPU stub supplies
 * `sqrt_`.  The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sqrt(double value);

extern double func_1e7b4_basis_scale;
extern float func_1e7b4_sample_step;

void func_1e7b4(float *out, float *points, float t)
{
    float w0;
    float w1;
    float w2;
    float s0;
    float s1;
    float dx;
    float dy;
    float inv;

    w0 = (1.0 - t) * (1.0 - t) * func_1e7b4_basis_scale;
    w2 = t * t * func_1e7b4_basis_scale;
    w1 = 1.0 - (w0 + w2);

    out[0] = points[3] * w1 + points[0] * w0 + points[6] * w2;
    out[1] = points[1] * w0 + points[4] * w1 + points[7] * w2;

    t = t + func_1e7b4_sample_step;

    w0 = (1.0 - t) * (1.0 - t) * func_1e7b4_basis_scale;
    w2 = t * t * func_1e7b4_basis_scale;
    w1 = 1.0 - (w0 + w2);

    s0 = points[0] * w0 + points[3] * w1 + points[6] * w2;
    s1 = points[1] * w0 + points[4] * w1 + points[7] * w2;

    dx = s0 - out[0];
    dy = s1 - out[1];
    inv = (float)(1.0 / sqrt((double)(dx * dx + dy * dy)));
    out[3] = dx * inv;
    out[4] = dy * inv;
}
