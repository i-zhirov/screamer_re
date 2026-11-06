/* The rebuilt `race_state_begin` module.
 *
 * The original sits at linear 0x294c8 in STARTH.EXE and holds 35 instructions
 * and 117 bytes: the six register saves, the frame, the parameter slot, the six
 * constant stores, the saved-state copy, the parameter store, the state store,
 * and the epilogue.
 *
 * The function sets the race constants 2.0, 0.6, 1.0, 20000, and -12.566371,
 * sets the previous-state word to -1, copies the old state into the saved-state
 * word, stores the parameter in the race parameter word, and sets the state to
 * 1.
 */
extern float DAT_00120378;
extern float DAT_00120380;
extern float DAT_0012037c;
extern int DAT_001203b0;
extern float DAT_0012038c;
extern int DAT_0011fac4;
extern int DAT_0011fac8;
extern int DAT_001203b8;
extern int DAT_0011facc;

void race_state_begin(int value)
{
    DAT_00120378 = 2.0f;
    DAT_00120380 = 0.6f;
    DAT_0012037c = 1.0f;
    DAT_001203b0 = 20000;
    DAT_0012038c = -12.566370964f;
    DAT_0011fac8 = -1;
    DAT_001203b8 = DAT_0011fac4;
    DAT_0011facc = value;
    DAT_0011fac4 = 1;
}
