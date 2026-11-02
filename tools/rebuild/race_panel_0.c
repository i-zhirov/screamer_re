/* The rebuilt `race_panel_0` module.
 *
 * The original sits at linear 0x2e0b4 in STARTH.EXE and holds 103 instructions
 * and 357 bytes: the three register saves, the frame, the 8-byte local area,
 * the two smoothed axis positions with the frame delta and the two step
 * constants, the x offset, the two truncating conversions at `0x4d9f4`, the
 * two `panel_show` calls at `0x2dfac`, the state and blink test, and the
 * `panel_draw_centered` call at `0x2e7cc`.  The merged store record keeps the
 * 360-byte extent.
 *
 * Each axis target centers half of the object field on the 0x140 screen width:
 * the field at `+0x134` for x and the field at `+0x120` for y.  The position
 * moves up by `delta * step` while the target stays above it, and it snaps
 * down to the target when the target stays below it.  The x value then adds
 * the offset at `DAT_00012d8` before the integer conversion.
 *
 * The function draws the two panels through `panel_show`.  When the race state
 * is 1 or 2 and the low seven bits of the frame counter stay below 0x48, it
 * draws the matching title at y 0xbe with the font 0 and the color 2.
 *
 * The constants arrive through the globals `race_panel_0_x_step`,
 * `race_panel_0_y_step`, and `race_panel_0_x_offset`.  The harness sets the
 * exact bit patterns.  The original reads the constants at 0x12d4, 0x12dc,
 * and 0x12d8.
 *
 * The two conversions to an integer use the Watcom helper at 0x4d9f4, which
 * truncates toward zero.  The module uses the truncating FPU stub, because the
 * shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned char *DAT_0015ccc;
extern float DAT_0018e08;
extern float DAT_00020824;
extern float DAT_00020828;
extern float race_panel_0_x_step;
extern float race_panel_0_y_step;
extern float race_panel_0_x_offset;
extern unsigned int DAT_00020988;
extern unsigned int DAT_000007fd;
extern char *DAT_0011fc58;
extern char *DAT_0011fc5c;
extern void panel_show(int x, int y, int size);
extern void panel_draw_centered(char *text, int y, int font, int color);

void race_panel_0(void)
{
    int target;
    int value;

    target = (0x140 - *(int *)(DAT_0015ccc + 0x134) / 2) / 2;
    if ((float)target > DAT_00020824) {
        DAT_00020824 =
            DAT_0018e08 * race_panel_0_x_step + DAT_00020824;
    }
    if ((float)target < DAT_00020824) {
        DAT_00020824 = (float)target;
    }
    value = (int)(DAT_00020824 + race_panel_0_x_offset);
    panel_show(value, 0, 0xf);

    target = (0x140 - *(int *)(DAT_0015ccc + 0x120) / 2) / 2;
    if ((float)target > DAT_00020828) {
        DAT_00020828 =
            DAT_0018e08 * race_panel_0_y_step + DAT_00020828;
    }
    if ((float)target < DAT_00020828) {
        DAT_00020828 = (float)target;
    }
    value = (int)DAT_00020828;
    panel_show(value, 0x10, 0xe);

    if (DAT_00020988 == 1 && (DAT_000007fd & 0x7f) < 0x48) {
        panel_draw_centered(DAT_0011fc58, 0xbe, 0, 2);
    } else if (DAT_00020988 == 2 && (DAT_000007fd & 0x7f) < 0x48) {
        panel_draw_centered(DAT_0011fc5c, 0xbe, 0, 2);
    }
}
