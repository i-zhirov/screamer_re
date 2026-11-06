/* The rebuilt `frame_timer` module.
 *
 * The original sits at linear 0x1fc38 in STARTH.EXE and holds 76 instructions
 * and 299 bytes: the two register saves, the frame, the 0xc-byte local area,
 * the wait for the frame counter `DAT_000007f9`, the raw delta from the counter
 * and the double 0.01, the smooth step with the float constants 9.0 and 10.0,
 * the two clamps at the doubles 0.2 and 0.02, the frame rate from the double
 * 0.3 and the float constant 65535.0, the filter table pointer
 * `DAT_0010cf08`, and the phase word `DAT_0002153c`.
 *
 * The function waits until the frame counter reaches 2 and then clears it.  It
 * forms the raw delta `counter * 0.01` in `DAT_00118e08` and the smooth delta
 * `(old * 9.0 + raw) / 10.0` in `DAT_00118e04`.  It accumulates the raw delta
 * in `DAT_00118e10` and clamps the smooth delta between 0.02 and 0.2.  The
 * frame rate is the truncated `1 / delta`, so the filter table pointer is
 * `0x80e8 + 400 * rate`.  The phase `DAT_00118e0c` advances by
 * `delta * 0.3` and wraps at 1.0.  The word `DAT_0002153c` holds the phase
 * scaled by 65535.
 *
 * The constants arrive through the globals `frame_delta_scale`,
 * `frame_delta_gain`, `frame_delta_norm`, `frame_delta_high`,
 * `frame_delta_low`, `frame_phase_step`, `frame_phase_wrap`, and
 * `frame_phase_scale`.  The harness sets the exact bit patterns.  The original
 * reads the constants from object 3 near offset 0xc58.
 *
 * The two conversions to an integer use the Watcom helper at `0x4d9f4`, which
 * truncates toward zero.  The module uses the truncating FPU stub, because the
 * shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned int DAT_000007f9;
extern float DAT_00118e04;
extern float DAT_00118e08;
extern float DAT_00118e0c;
extern float DAT_00118e10;
extern float DAT_00118e14;
extern unsigned int DAT_0010cf08;
extern unsigned int DAT_0002153c;
extern double frame_delta_scale;
extern float frame_delta_gain;
extern float frame_delta_norm;
extern double frame_delta_high;
extern double frame_delta_low;
extern double frame_phase_step;
extern float frame_phase_wrap;
extern float frame_phase_scale;

void frame_timer(void)
{
    unsigned int frames;
    int rate;

    do {
        frames = DAT_000007f9;
    } while (frames < 2);
    DAT_000007f9 = 0;
    DAT_00118e14 = DAT_00118e04;
    DAT_00118e08 = (float)(frames * frame_delta_scale);
    DAT_00118e04 =
        (DAT_00118e14 * frame_delta_gain + DAT_00118e08) / frame_delta_norm;
    DAT_00118e10 = DAT_00118e10 + DAT_00118e08;
    if (DAT_00118e04 >= frame_delta_high) {
        DAT_00118e04 = 0.2f;
    }
    if (DAT_00118e04 <= frame_delta_low) {
        DAT_00118e04 = 0.02f;
    }
    rate = (int)(1.0f / DAT_00118e04);
    DAT_0010cf08 = 0x80e8 + 400 * rate;
    DAT_00118e0c = DAT_00118e0c + DAT_00118e04 * frame_phase_step;
    if (DAT_00118e0c >= 1.0f) {
        DAT_00118e0c = DAT_00118e0c + frame_phase_wrap;
    }
    DAT_0002153c = (unsigned int)(int)(DAT_00118e0c * frame_phase_scale);
}
