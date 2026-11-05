/* The rebuilt `race_heading_update` module.
 *
 * The original sits at linear 0x29610 in STARTH.EXE and holds 136 instructions
 * and 519 bytes up to the `ret` at 0x29816.  The merged store record keeps the
 * 520-byte extent.  The body holds the five register saves, the frame, the
 * 0x10-byte local block, the eight truncating conversions, the power call, the
 * four interpolations, the reset block, and the epilogue.
 *
 * The function converts the angles `DAT_00120388`, `DAT_00120394`,
 * `DAT_00120384`, and `DAT_0012038c` to the 11-bit heading values with the
 * scale 1024.0 and pi.  The first two headings wrap by 0x800 when they differ
 * by at least 0x400, and the interpolated heading becomes `DAT_00109a4`.
 *
 * The flag `DAT_0011d240` selects the interpolation factor.  A set flag makes
 * the factor the power `DAT_0012037c` raised to 2.0, and a clear flag uses
 * `DAT_0012037c` itself.  The function interpolates the three integer pairs
 * `DAT_0012039c`/`DAT_001203a8`, `DAT_001203a0`/`DAT_001203ac`, and
 * `DAT_001203a4`/`DAT_001203b0` into `DAT_00109ac`, `DAT_00109b0`, and
 * `DAT_00109b4`.
 *
 * A zero `DAT_0012037c` returns after the outputs.  Otherwise the function
 * advances `DAT_0012037c` by `DAT_00118e08 * DAT_00120380`.  A negative result
 * copies the new base angles and integer pairs, and it clears `DAT_0012037c`.
 *
 * The constants arrive through the globals `race_heading_scale`,
 * `race_heading_pi`, `race_heading_power`, and `race_heading_zero`, so the
 * harness sets the exact bit patterns.  The original reads the scale and pi at
 * object-3 offsets 0xf33 and 0xf37.
 *
 * The eight conversions to an integer use the Watcom helper at 0x4d9f4, which
 * truncates toward zero.  The module uses the local truncating FPU stub,
 * because the shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0011d240;
extern float DAT_00120388;
extern float DAT_00120394;
extern float DAT_00120384;
extern float DAT_0012038c;
extern float DAT_0012037c;
extern float DAT_00120380;
extern float DAT_00118e08;
extern int DAT_0012039c;
extern int DAT_001203a0;
extern int DAT_001203a4;
extern int DAT_001203a8;
extern int DAT_001203ac;
extern int DAT_001203b0;
extern int DAT_00109a0;
extern int DAT_00109a4;
extern int DAT_00109a8;
extern int DAT_00109ac;
extern int DAT_00109b0;
extern int DAT_00109b4;
extern float race_heading_scale;
extern float race_heading_pi;
extern double race_heading_power;
extern float race_heading_zero;
extern double pow(double base, double exponent);

void race_heading_update(void)
{
    int first;
    int second;
    float factor;

    first = (int)(DAT_00120388 * race_heading_scale / race_heading_pi)
        & 0x7ff;
    second = (int)(DAT_00120394 * race_heading_scale / race_heading_pi)
        & 0x7ff;
    if (first < second && second - first >= 0x400) {
        first = first + 0x800;
    }
    if (second < first && first - second >= 0x400) {
        second = second + 0x800;
    }
    if (DAT_0011d240 != 0) {
        factor = (float)pow((double)DAT_0012037c, race_heading_power);
    } else {
        factor = DAT_0012037c;
    }
    DAT_00109a0 = (int)(DAT_00120384 * race_heading_scale / race_heading_pi)
        & 0x7ff;
    DAT_00109a8 = (int)(DAT_0012038c * race_heading_scale / race_heading_pi)
        & 0x7ff;
    DAT_00109a4 = (int)(second + (first - second) * factor);
    DAT_00109ac = (int)(DAT_001203a8
        + (DAT_0012039c - DAT_001203a8) * factor);
    DAT_00109b0 = (int)(DAT_001203ac
        + (DAT_001203a0 - DAT_001203ac) * factor);
    DAT_00109b4 = (int)(DAT_001203b0
        + (DAT_001203a4 - DAT_001203b0) * factor);
    if ((*(unsigned int *)&DAT_0012037c & 0x7fffffff) == 0) {
        return;
    }
    DAT_0012037c = DAT_0012037c - DAT_00118e08 * DAT_00120380;
    if (DAT_0012037c < race_heading_zero) {
        DAT_00120388 = DAT_00120394;
        DAT_0012039c = DAT_001203a8;
        DAT_001203a0 = DAT_001203ac;
        DAT_001203a4 = DAT_001203b0;
        DAT_0012037c = race_heading_zero;
    }
}
