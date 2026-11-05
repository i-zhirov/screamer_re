/* The rebuilt `spline_interpolate` module.
 *
 * The original sits at linear 0x22744 in STARTH.EXE and holds 360
 * instructions and 1081 bytes up to the `ret 4` at 0x22b7a.  The record keeps
 * the 1084-byte extent and sets no function size.
 *
 * The only caller is the path playback at 0x23038.  That function walks the
 * 0x28-byte keyframe records at the array of `DAT_0018e8c` and passes the
 * previous, the current, the next, and the second-next record with a fraction
 * of the two timestamps.  The function writes the interpolated position to the
 * three integers at the output pointer.  The caller copies the record angles
 * and the other fields without interpolation, so the function interpolates
 * the position along the recorded path.
 *
 * The four points are D = frame 0, A = frame 1, B = frame 2, and C = frame 3.
 * The function measures the chords AB, BD, and AC.  The chord AB divides by the
 * double constant 3.0 at object-3 offset 0xda4.  The tangents are
 *
 *     A' = A + (B - D) * (|AB| / (3 * |BD|))
 *     B' = B + (A - C) * (|AB| / (3 * |AC|))
 *
 * and the function converts them to integers by truncation.  The result is the
 * cubic Hermite form
 *
 *     P(t) = A + t * 3(A' - A)
 *              + t^2 * (3B' - 6A' + 3A)
 *              + t^3 * (3A' + B - 3B' - A)
 *
 * evaluated in double precision and truncated to an integer.  When |BD| or
 * |AC| is zero, the matching tangent stays at A or B.  The single-precision
 * squares and the single-precision tangents follow the original.
 *
 * The original calls the square-root wrapper at 0x22bec and the truncating
 * helper at 0x4d9f4.  This harness links no math library, so the local
 * assembly stub supplies `sqrt` and makes `__CHP` truncate toward zero.  The
 * chord scale arrives through a global, because the minimal image does not
 * load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sqrt(double value);

extern double spline_chord_scale;

void spline_interpolate(int **frames, int *out, float t)
{
    int *d = frames[0];
    int *a = frames[1];
    int *b = frames[2];
    int *c = frames[3];
    int a0 = a[0];
    int a1 = a[1];
    int a2 = a[2];
    int b0 = b[0];
    int b1 = b[1];
    int b2 = b[2];
    int pa0 = a0;
    int pa1 = a1;
    int pa2 = a2;
    int pb0 = b0;
    int pb1 = b1;
    int pb2 = b2;
    int distance;
    float ab;
    float bd;
    float ac;
    float t2;
    float t3;
    int term1;
    int term2;
    int term3;
    double value;

    distance = (b[1] - a[1]) * (b[1] - a[1])
        + (b[0] - a[0]) * (b[0] - a[0])
        + (b[2] - a[2]) * (b[2] - a[2]);
    ab = (float)(sqrt((double)distance) / spline_chord_scale);

    distance = (b[1] - d[1]) * (b[1] - d[1])
        + (b[0] - d[0]) * (b[0] - d[0])
        + (b[2] - d[2]) * (b[2] - d[2]);
    bd = (float)sqrt((double)distance);
    if (bd != 0.0f) {
        pa0 = (int)((double)a0 + (double)(b[0] - d[0]) * ab / bd);
        pa1 = (int)((double)a1 + (double)(b[1] - d[1]) * ab / bd);
        pa2 = (int)((double)a2 + (double)(b[2] - d[2]) * ab / bd);
    }

    distance = (a[1] - c[1]) * (a[1] - c[1])
        + (a[0] - c[0]) * (a[0] - c[0])
        + (a[2] - c[2]) * (a[2] - c[2]);
    ac = (float)sqrt((double)distance);
    if (ac != 0.0f) {
        pb0 = (int)((double)b0 + (double)(a[0] - c[0]) * ab / ac);
        pb1 = (int)((double)b1 + (double)(a[1] - c[1]) * ab / ac);
        pb2 = (int)((double)b2 + (double)(a[2] - c[2]) * ab / ac);
    }

    t2 = t * t;
    t3 = t2 * t;

    term3 = 3 * pa0 + b0 - 3 * pb0 - a0;
    term2 = 3 * pb0 - 6 * pa0 + 3 * a0;
    term1 = 3 * pa0 - 3 * a0;
    value = ((double)term3 * (double)t3 + (double)term2 * (double)t2)
        + (double)term1 * (double)t + (double)a0;
    out[0] = (int)value;

    term3 = 3 * pa1 + b1 - 3 * pb1 - a1;
    term2 = 3 * pb1 - 6 * pa1 + 3 * a1;
    term1 = 3 * pa1 - 3 * a1;
    value = ((double)term3 * (double)t3 + (double)term2 * (double)t2)
        + (double)term1 * (double)t + (double)a1;
    out[1] = (int)value;

    term3 = 3 * pa2 + b2 - 3 * pb2 - a2;
    term2 = 3 * pb2 - 6 * pa2 + 3 * a2;
    term1 = 3 * pa2 - 3 * a2;
    value = ((double)term3 * (double)t3 + (double)term2 * (double)t2)
        + (double)term1 * (double)t + (double)a2;
    out[2] = (int)value;
}
