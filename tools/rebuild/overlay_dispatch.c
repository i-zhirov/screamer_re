/* The rebuilt `overlay_dispatch` module.
 *
 * The original sits at linear 0x28160 in STARTH.EXE.  The first-return body
 * holds 42 instructions and 152 bytes: the six register saves, the frame, the
 * option-flag test, the mode and state reads, the state switch, the case 0
 * stack pop, and the epilogue.  The full function extends to 0x289bc (2140
 * bytes), and the remaining case bodies extend after the first return.
 *
 * The rebuild keeps the dispatcher, implements case 0 because it sits in the
 * first-return body, and represents the other case bodies as external step
 * functions for a later pass.  This is the same shape as the
 * `net_race_screen` and `race_state_update` modules.
 *
 * The case targets:
 *
 *     0        0x281ab   inline stack pop
 *     1        0x282e6   shared state entry
 *     2        0x281f8
 *     3        0x282a2
 *     4        0x283da
 *     5        0x2837d
 *     6        0x282e6
 *     7        0x282e6
 *     8        0x28358
 *     other    0x282e6
 *
 * The option flag `DAT_000f0002` selects the second overlay path at 0x283ff.
 * The mode `0x20` at `DAT_0011fb68` returns through the shared epilogue at
 * 0x289b1.
 *
 * The original state dispatch uses the compiler jump table at
 * `cs:[eax*4 + 0x1813c]`.  The DOS/4G loader does not relocate the jump table
 * of the rebuilt image, so the rebuild uses an if chain, as `race_state_update`
 * does, and the identity check stays open.
 */
extern unsigned char DAT_000f0002;
extern unsigned int DAT_0011fb68;
extern unsigned int DAT_0011d24c;
extern unsigned int DAT_0011fac4;
extern unsigned int DAT_0011fb58;
extern unsigned int DAT_0011fb60;
extern unsigned int DAT_00120350[];
extern unsigned int DAT_00200988;
extern unsigned int DAT_0011d234;
extern unsigned int DAT_00007ed8;

extern void overlay_option_zero(void);
extern void overlay_case_281f8(void);
extern void overlay_case_282a2(void);
extern void overlay_case_282e6(void);
extern void overlay_case_28358(void);
extern void overlay_case_2837d(void);
extern void overlay_case_283da(void);
extern void overlay_notify(unsigned int value);

void overlay_dispatch(void)
{
    unsigned int mode;
    unsigned int state;

    if (DAT_000f0002 != 0) {
        mode = DAT_0011fb68;
        DAT_0011d24c = 0;
        if (mode != 0x20) {
            DAT_00200988 = 0;
            state = DAT_0011fac4;
            if (state == 0) {
                if (DAT_0011fb58 != 0) {
                    mode = DAT_0011fb58 - 1;
                    DAT_0011fb60 = DAT_00120350[mode];
                    DAT_0011fb58 = mode;
                    overlay_notify(0);
                } else {
                    DAT_0011d234 = 1;
                    DAT_00007ed8 = 1;
                    DAT_0011fb68 = 0x20;
                    return;
                }
            } else if (state == 1) {
                overlay_case_282e6();
            } else if (state == 2) {
                overlay_case_281f8();
            } else if (state == 3) {
                overlay_case_282a2();
            } else if (state == 4) {
                overlay_case_283da();
            } else if (state == 5) {
                overlay_case_2837d();
            } else if (state == 6) {
                overlay_case_282e6();
            } else if (state == 7) {
                overlay_case_282e6();
            } else if (state == 8) {
                overlay_case_28358();
            } else {
                overlay_case_282e6();
            }
            DAT_0011fb68 = 0x20;
        }
    } else {
        overlay_option_zero();
    }
}
