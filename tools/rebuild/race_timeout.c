/* The rebuilt `race_timeout` module.
 *
 * The original sits at linear 0x27f44 in STARTH.EXE and holds 16 instructions
 * and 56 bytes: the `edx` save, the frame, the two counter arguments, the frame
 * delta call at `0x1fa04`, the mask, the compare against 1000, the two state
 * stores, and the epilogue.
 *
 * The function computes the frame delta between `DAT_000107fd` and
 * `DAT_0011d228`, and when the low word of the delta is greater than 1000, it
 * resets the counter to `DAT_000107fd` and sets the timeout flag
 * `DAT_00101e04`.
 */
extern unsigned int DAT_000107fd;
extern unsigned int DAT_0011d228;
extern unsigned int DAT_00101e04;
extern unsigned int frame_delta(unsigned int a, unsigned int b);

void race_timeout(void)
{
    unsigned int delta;

    delta = frame_delta(DAT_0011d228, DAT_000107fd);
    if ((int)(delta & 0xffff) > 1000) {
        DAT_0011d228 = DAT_000107fd;
        DAT_00101e04 = 1;
    }
}
