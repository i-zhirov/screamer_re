/* The rebuilt `palette_build` module.
 *
 * The original sits at linear 0x15720 in STARTH.EXE and holds 55 instructions
 * and 152 bytes: the four register saves, the frame, the 0x14-byte local area,
 * the float constant 0.005, the outer loop over 50 rows, the inner loop over
 * 100 columns, the row base `0x80e8 + 400 * i`, the ramp value
 * `t = j * 0.005f + 0.5`, the exponent `50.0 / (i + 1)`, the call at 0xc21e2,
 * the float store, and the epilogue.
 *
 * The table sits at linear 0x1080e8 in the original image, inside the writable
 * object 3.  The row holds 100 floats, so the row stride is 400 bytes.
 *
 * The call at 0xc21e2 loads two qword arguments and reaches the power routine
 * at 0xc20a6, which uses `fyl2x` and the `f2xm1` loop at 0xc21bf.  The rebuild
 * therefore calls `pow` with the ramp value as the base and `50.0 / (i + 1)`
 * as the exponent.  The result is stored as a float.
 *
 * The three constants arrive through the globals `palette_step`,
 * `palette_base`, and `palette_scale`.  The harness sets the exact bit
 * patterns.  This keeps the float math out of the constant segment, which the
 * shared harness image does not load for this module.
 */
extern float DAT_001080e8[];
extern float palette_step;
extern double palette_base;
extern double palette_scale;
extern double pow(double base, double exponent);

void palette_build(void)
{
    int i;
    int j;
    float t;

    for (i = 0; i < 50; i++) {
        for (j = 0; j < 100; j++) {
            t = j * palette_step + palette_base;
            DAT_001080e8[i * 100 + j] = (float)pow(t, palette_scale / (i + 1));
        }
    }
}
