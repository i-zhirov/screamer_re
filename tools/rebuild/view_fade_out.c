/* The rebuilt `view_fade_out` module.
 *
 * The original sits at linear 0x21da8 in STARTH.EXE and holds 63 instructions
 * and 195 bytes up to the `ret` at 0x21e6a.  One padding byte at 0x21e6b
 * follows the `ret`, and the next function starts at 0x21e6c, so the record
 * keeps the 196-byte extent and sets no separate function size.  The identity
 * check uses 63 instructions.
 *
 * The function has two direct callers.  The default path of `race_session_start`
 * at 0x128e0 calls it at 0x12a66 with `value ^ 1`, and `race_session_run` at
 * 0x1fd70 calls it at 0x1fefd with 1 before the cleanup at 0x156a8.  A non-zero
 * argument moves the view on every step.  The callers sit in two sections, so
 * the record takes the section Program functions.
 *
 * The function fades the screen out and slides the view home.  It saves the
 * phase counter `DAT_000007f9` and starts the fraction at zero.  Each step
 * either moves the view or skips the move, presents the screen with the scale
 * `1 - fraction`, and then forms the next fraction.
 *
 * A non-zero argument takes the three view offsets `DAT_001d2c0`,
 * `DAT_001d2bc`, and `DAT_001d2c4`, scales each one by `1 - fraction`, truncates
 * toward zero through the Watcom helper at 0x4d9f4, and passes the three values
 * to `view_move` at 0x1f94c in the order `view_move(1d2c0, 1d2bc, 1d2c4)`.
 *
 * The screen present at 0x27ec8 receives the scale `1 - fraction` and the
 * offset 0.  The next fraction comes from the phase delta at 0x1fa04 between
 * the saved phase and the current phase; the low 16 bits scale by the double
 * 0.01 at object-3 offset 0xd41 and the double 2.0 at object-3 offset 0xd49.
 * The loop repeats while 1.0 is at least the fraction.  After the loop the
 * function presents the scale 0 and the offset 0.
 *
 * The two doubles arrive through the globals `view_fade_out_rate` and
 * `view_fade_out_step`, because the minimal image does not load the constant
 * segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function truncates through 0x4d9f4, so the local `view_fade_out_fpu.asm` makes
 * the `__CHP` stub truncate toward zero.  The module uses no assembly body.
 */
extern int DAT_000007f9;
extern int DAT_001d2bc;
extern int DAT_001d2c0;
extern int DAT_001d2c4;
extern double view_fade_out_rate;
extern double view_fade_out_step;
extern int phase_delta(int start, int now);
extern void view_move(int x, int y, int z);
extern void video_present(float scale, int offset);

void view_fade_out(int move_view)
{
    int start;
    int delta;
    float fraction;
    int x;
    int y;
    int z;

    start = DAT_000007f9;
    fraction = 0.0f;
    do {
        if (move_view) {
            x = (int)((float)DAT_001d2c0 * (1.0f - fraction));
            y = (int)((float)DAT_001d2bc * (1.0f - fraction));
            z = (int)((float)DAT_001d2c4 * (1.0f - fraction));
            view_move(x, y, z);
        }
        video_present(1.0f - fraction, 0);
        delta = phase_delta(start, DAT_000007f9) & 0xffff;
        fraction = (float)(
            (double)delta * view_fade_out_rate * view_fade_out_step);
    } while (1.0f >= fraction);
    video_present(0.0f, 0);
}
