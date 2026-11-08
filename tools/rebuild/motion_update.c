/* The rebuilt `motion_update` module.
 *
 * The original sits at linear 0x21e6c in STARTH.EXE and holds 53 instructions
 * and 202 bytes: the three register saves, the frame, the 0xc-byte local area,
 * the guard on the time base, the three scaled position values with the
 * floating-point helper at `0x4d9f4`, the `view_move` call at `0x1f94c`, the
 * `video_present` call at `0x27ec8`, the advance of the time base, and the wrap
 * branch at 1.0.
 *
 * The function returns while the time base is below zero.  It scales the three
 * object position values by the time base, passes the scaled values to
 * `view_move`, and passes the time base to `video_present`.  It then advances
 * the time base by twice the frame value.  When the new value reaches 1.0, it
 * sets the time base to -1.0, presents the full palette, and passes the raw
 * position values to `view_move`.
 */
extern float DAT_00118e38;
extern float DAT_00118e08;
extern float DAT_00100d51;
extern int DAT_0011d2c0;
extern int DAT_0011d2bc;
extern int DAT_0011d2c4;
extern void view_move(int a, int b, int c);
extern void video_present(float scale, int offset);

void motion_update(void)
{
    int a;
    int b;
    int c;

    if (DAT_00118e38 < 0.0f) {
        return;
    }
    a = (int)((float)DAT_0011d2c0 * DAT_00118e38);
    b = (int)((float)DAT_0011d2bc * DAT_00118e38);
    c = (int)((float)DAT_0011d2c4 * DAT_00118e38);
    view_move(a, b, c);
    video_present(DAT_00118e38, 0);
    DAT_00118e38 = DAT_00118e08 * DAT_00100d51 + DAT_00118e38;
    if (DAT_00118e38 < 1.0f) {
        return;
    }
    DAT_00118e38 = -1.0f;
    video_present(1.0f, 0);
    view_move(DAT_0011d2c0, DAT_0011d2bc, DAT_0011d2c4);
}
