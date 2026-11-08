/* The rebuilt `fade_update` module.
 *
 * The original sits at linear 0x29818 in STARTH.EXE and holds 48 instructions
 * and 154 bytes: the five register saves, the frame, the state read from
 * `DAT_0011fac8`, the fade-in path, the fade-out path, the call at 0x27ec8,
 * and the `ret 4` epilogue.
 *
 * The state 0 does nothing.  The state 1 ramps the fade value up by
 * `DAT_0018e08 * delta` and clamps it to 1.0.  Any other state ramps the fade
 * value down by the same product and clamps it to 0.0.  A clamp clears the
 * state.  Every active state then calls `video_present` with the fade value and
 * the offset 0.
 *
 * The original argument sits at `[ebp + 0x18]`, after the five saved registers
 * and the return address, so it is the single float argument.  The callee
 * removes it with `ret 4`.
 */
extern unsigned int DAT_0011fac8;
extern float DAT_0018e08;
extern float DAT_0011d224;
extern void video_present(float scale, int offset);

void fade_update(float delta)
{
    unsigned int state;

    state = DAT_0011fac8;
    if (state == 0) {
        return;
    }
    if (state == 1) {
        DAT_0011d224 = DAT_0018e08 * delta + DAT_0011d224;
        if (DAT_0011d224 >= 1.0f) {
            DAT_0011d224 = 1.0f;
            DAT_0011fac8 = 0;
        }
    } else {
        DAT_0011d224 = DAT_0011d224 - DAT_0018e08 * delta;
        if (DAT_0011d224 < 0.0f) {
            DAT_0011d224 = 0.0f;
            DAT_0011fac8 = 0;
        }
    }
    video_present(DAT_0011d224, 0);
}
