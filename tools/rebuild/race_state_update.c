/* The rebuilt `race_state_update` module.
 *
 * The original sits at linear 0x29564 in STARTH.EXE and holds 13 dispatch
 * instructions plus the case bodies: the frame, the state word read from
 * `DAT_0011fac4`, the bound check against 8, and the compiler jump table at
 * `cs:[eax*4 + 0x19540]`.  The case bodies call the state step functions, and
 * case 1 also advances the frame timing value and copies the next state.
 *
 * The case bodies and their call targets:
 *
 *     case 0  `state_step_0` at 0x29918
 *     case 1  `state_step_1` at 0x2be94, with the frame timing and the state copy
 *     case 2  `state_step_2` at 0x2cb20
 *     case 3  `state_step_3` at 0x2cc98
 *     case 4  `state_step_4` at 0x2ce84
 *     case 5  `state_step_5` at 0x2cf68
 *     case 6  `state_step_6` at 0x2d4b8
 *     case 7  `state_step_7` at 0x2d7c4
 *
 * The jump table in the code object is the same compiler-table case as
 * `error_report`: the loader does not relocate it, so the C switch in this
 * rebuild runs through the compiler's own dispatch, and the identity check
 * stays open.
 */
extern unsigned int DAT_0011fac4;
extern unsigned int DAT_0011fac8;
extern unsigned int DAT_0011facc;
extern unsigned int DAT_000203a4;
extern unsigned int DAT_000203b0;
extern unsigned int DAT_000203b8;
extern float DAT_0002038c;
extern float DAT_00018e08;
extern float DAT_0000f2f;

extern void state_step_0(void);
extern void state_step_1(void);
extern void state_step_2(void);
extern void state_step_3(void);
extern void state_step_4(void);
extern void state_step_5(void);
extern void state_step_6(void);
extern void state_step_7(void);

void race_state_update(void)
{
    unsigned int state;

    state = DAT_0011fac4;
    if (state == 0) {
        state_step_0();
    } else if (state == 1) {
        if (DAT_000203a4 != DAT_000203b0) {
            DAT_0002038c = DAT_0002038c + DAT_00018e08 * DAT_0000f2f;
        }
        if (DAT_0011fac8 == 0) {
            DAT_0011fac4 = DAT_0011facc;
            DAT_000203b8 = DAT_0011facc;
            state_step_1();
        }
    } else if (state == 2) {
        state_step_2();
    } else if (state == 3) {
        state_step_3();
    } else if (state == 4) {
        state_step_4();
    } else if (state == 5) {
        state_step_5();
    } else if (state == 6) {
        state_step_6();
    } else if (state == 7) {
        state_step_7();
    }
}
