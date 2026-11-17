/* The rebuilt `sound_reset` module.
 *
 * The original sits at linear 0x2471c in STARTH.EXE and holds 45 instructions
 * and 132 bytes up to the `ret` at 0x2479f.  The next record starts at 0x247a0,
 * so the store record keeps the 132-byte extent and sets no separate function
 * size.  The identity check uses 45 instructions.
 *
 * The only direct caller is race_session_start at 0x128e0, which calls this
 * function at 0x12abb while it prepares the frame state after `video_fill`.  The
 * record takes the section Program functions, because the function resets the
 * sound state that the sound modules read.
 *
 * The status DAT_00119070 holds the playing sound or -1, the timer
 * DAT_0011907c holds the elapsed time, and the queue head DAT_00119080 holds
 * the first queued effect or -1.  The function writes -1 to the status, the
 * timer bits 0x3f333333 (the float 0.7), and -1 to the queue head.  The timer
 * is an integer here, because the original moves the bits and holds no
 * floating-point instruction.
 *
 * The two per-effect arrays hold 0x1b entries.  The function clears the array
 * at 0x1922c and fills the rotation counters at 0x191c0.  A set bit 1 of the
 * frame counter DAT_000007fd selects the low bit of the counter, and a clear
 * bit 1 selects the value 2, so every effect starts on its first or third
 * variant.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no float symbol, because it
 * only moves the timer bits, so it needs no FPU stub and no assembly.
 */
extern int DAT_00119070;
extern int DAT_0011907c;
extern int DAT_00119080;
extern int DAT_001922c[27];
extern int DAT_00191c0[27];
extern int DAT_000007fd;

void sound_reset(void)
{
    int index;

    DAT_00119070 = -1;
    DAT_0011907c = 0x3f333333;
    DAT_00119080 = -1;
    for (index = 0; index < 27; index++) {
        DAT_001922c[index] = 0;
    }
    for (index = 0; index < 27; index++) {
        if (DAT_000007fd & 2) {
            DAT_00191c0[index] = DAT_000007fd & 1;
        } else {
            DAT_00191c0[index] = 2;
        }
    }
}
