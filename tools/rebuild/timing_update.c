/* The rebuilt `timing_update` module.
 *
 * The original sits at linear 0x22df8 in STARTH.EXE and holds 20 compared
 * instructions and 71 bytes plus the float path: the guards on the input bits
 * and the replay state, the timing subtraction, the replay read, the flag
 * store, and the float timing step.
 *
 * The function advances the timing when the replay flag is set: it subtracts
 * the timing value, reads the replay, and computes the filter entry from the
 * frame rate.
 */
extern unsigned char DAT_00118e1c;
extern unsigned int DAT_00118e6c;
extern float DAT_00118e04;
extern float DAT_00118e08;
extern float DAT_00118e10;
extern unsigned int DAT_0010cf08;
extern int replay_read(void);
extern void replay_write(void);

void timing_update(void)
{
    if ((DAT_00118e1c & 0x40) == 0) {
        return;
    }
    if (DAT_00118e6c == 0) {
        replay_write();
        return;
    }
    DAT_00118e10 = DAT_00118e08 - DAT_00118e10;
    if (replay_read() != 0) {
        DAT_00118e1c |= 0x220;
        return;
    }
    DAT_00118e10 = DAT_00118e10 + DAT_00118e08;
    DAT_0010cf08 = 0x80e8 + 400 * (int)(1.0f / DAT_00118e04);
}
