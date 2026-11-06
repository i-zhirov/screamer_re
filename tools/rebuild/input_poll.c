/* The rebuilt `input_poll` module.
 *
 * The original sits at linear 0x26448 in STARTH.EXE and holds 27 instructions
 * and 108 bytes up to the first return, plus the early-exit block at
 * `0x264b4`.  The function tests the three input flags, increments the poll
 * counter, resets it at 8, reads the input, and applies the input when the
 * device state allows it.
 *
 * The function returns 0 on every early exit and the input state at
 * `DAT_00166696` on the normal path.  The apply call at `0x263cc` receives the
 * state in `eax` and the device pointer in `edx`.  The read call is at
 * `0x260e0`.
 */
extern unsigned int DAT_0016669e;
extern unsigned int DAT_00166684;
extern unsigned int DAT_00166688;
extern unsigned int DAT_001666bc;
extern unsigned int DAT_00166696;
extern void *DAT_001666a4;
extern void *DAT_001666ac;
extern void input_read(void);
extern void input_apply(unsigned int state, void *device);

int input_poll(void)
{
    if ((DAT_0016669e & 1) == 0) {
        return 0;
    }
    if ((DAT_00166684 & 1) == 0) {
        return 0;
    }
    if ((DAT_00166688 & 1) != 0) {
        return 0;
    }
    DAT_001666bc = DAT_001666bc + 1;
    if ((int)DAT_001666bc < 8) {
        return 0;
    }
    DAT_001666bc = 0;
    input_read();
    if ((*(unsigned short *)((char *)DAT_001666a4 + 3) & 0x200) == 0) {
        if (DAT_001666ac == 0) {
            return 0;
        }
        input_apply(DAT_00166696, DAT_001666ac);
    }
    return DAT_00166696;
}
