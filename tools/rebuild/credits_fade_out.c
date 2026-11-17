/* The rebuilt `credits_fade_out` module.
 *
 * The original sits at linear 0x27e08 in STARTH.EXE and holds 55 instructions
 * and 190 bytes up to the `ret` at 0x27ec5.  The next function starts at
 * 0x27ec8 after two padding bytes, so the store record keeps the 192-byte
 * extent and sets no separate function size.  The identity check uses 55
 * instructions.
 *
 * The only direct caller at 0x27a53 is `hud_update` at 0x27a34.  It runs the
 * function when bit 1 of the session state `DAT_0011ce44` is set, so the
 * function is the second credits overlay.  `race_loop` at 0x27700 runs
 * `object_update_all` until the session state reaches 4, and this function
 * sets the state 4 when the fade reaches zero.  The caller sits in the
 * `object_update_all callees` section, so the record takes that section.
 *
 * The function decreases the fade level `DAT_0011d224` by the frame delta
 * `DAT_00118e04`.  A new level below zero presents the black screen with the
 * scale 0 and the offset 0, moves the view to zero, and sets the session state
 * `DAT_0011ce44` to 4, so the race loop ends.
 *
 * A new level at or above zero scales the three view offsets
 * `DAT_0011d2c0`, `DAT_0011d2bc`, and `DAT_0011d2c4` by the new fade level,
 * truncates each value toward zero through the Watcom helper at 0x4d9f4, and
 * passes the values to `view_move` at 0x1f94c in the order
 * `view_move(1d2c0, 1d2bc, 1d2c4)`.  The function then passes the scaled
 * `DAT_0011d2bc` value to the mouse helper at 0x1f8b4 and presents the screen
 * with the new fade level as the scale and 0 as the offset.
 *
 * The credits context, the two present paths, and the caller support the name
 * `credits_fade_out`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function truncates through 0x4d9f4, so the local `credits_fade_out_fpu.asm`
 * makes the `__CHP` stub truncate toward zero.  The module uses no assembly
 * body.
 */
extern float DAT_00118e04;
extern float DAT_0011d224;
extern int DAT_0011d2bc;
extern int DAT_0011d2c0;
extern int DAT_0011d2c4;
extern int DAT_0011ce44;

extern void view_move(int x, int y, int z);
extern void mouse_y_set(int value);
extern void video_present(float scale, int offset);

void credits_fade_out(void)
{
    float fade;
    int x;
    int y;
    int z;

    fade = DAT_0011d224 - DAT_00118e04;
    DAT_0011d224 = fade;
    if (fade < 0.0f) {
        video_present(0.0f, 0);
        view_move(0, 0, 0);
        DAT_0011ce44 = 4;
        return;
    }
    x = (int)((float)DAT_0011d2c0 * fade);
    y = (int)((float)DAT_0011d2bc * fade);
    z = (int)((float)DAT_0011d2c4 * fade);
    view_move(x, y, z);
    mouse_y_set(y);
    video_present(fade, 0);
}
