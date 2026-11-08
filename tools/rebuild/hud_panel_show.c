/* The rebuilt `hud_panel_show` module.
 *
 * The original sits at linear 0x27a5c in STARTH.EXE and holds 135 instructions
 * and 471 bytes up to the `ret` at 0x27c32.  The next function starts at
 * 0x27c34, so the store record keeps the 472-byte extent and sets no function
 * size.  The identity check uses 135 instructions.
 *
 * The only direct caller at 0x27a40 is `hud_update` at 0x27a34.  The
 * `object_update_all` frame step calls `hud_update`, so the record takes the
 * section `hud_update callees`.
 *
 * The function sets the sprite fields `DAT_0011cb14` through `DAT_0011cb24`
 * and calls the flush at `0x25af0`.  The position cell `DAT_0011cb18` holds a
 * centered x value, and the cell `DAT_0011cb1c` holds the message row.  The
 * viewport table `DAT_0015ccc` supplies the width: record 0 and record 1 for
 * the state below 2, and record 2 for every other state.  The center is
 * `(0x140 - width / 2) / 2`.
 *
 * The filter at `0x22b80` moves the cell `DAT_0011ce4c` toward the target at
 * the coefficient `DAT_0010cf08[0x50]` for the first two states and the state
 * 0x12, and at the coefficient `DAT_0010cf08[0x5a]` for the other states.  The
 * cell `DAT_0011ce50` holds `state * 32 + 4` for the first two states and 4.0
 * for the other states.
 *
 * The original calls the truncate helper at 0x4d9f4 before each `fistp`.  This
 * harness links no math library, so the local `hud_panel_show_fpu.asm` makes
 * `__CHP` truncate toward zero.  The float constants arrive through globals,
 * because the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned char *DAT_0015ccc;
extern int DAT_001ce54;
extern int DAT_001ce40;
extern float DAT_001ce4c;
extern float DAT_001ce50;
extern int DAT_001cb14;
extern int DAT_001cb18;
extern int DAT_001cb1c;
extern int DAT_001cb20;
extern int DAT_001cb24;
extern void filter_update(float *value, float target, int index);
extern void panel_update(void);

void hud_panel_show(void)
{
    int state;
    int width;
    int offset;

    state = DAT_001ce54;
    if (state < 2) {
        width = *(int *)(DAT_0015ccc + state * 0x14 + 8);
        offset = (0x140 - width / 2) / 2;
        DAT_001ce50 = (float)(state * 0x20 + 4);
        filter_update(&DAT_001ce4c, (float)(-(DAT_001ce40 * 0x140)), 0x50);
        DAT_001cb18 = 2 * (int)((float)offset + DAT_001ce4c);
        DAT_001cb20 = state;
    } else if (state == 0x12 && DAT_001ce40 != 0) {
        width = *(int *)(DAT_0015ccc + 0x30);
        offset = (0x140 - width / 2) / 2;
        DAT_001ce50 = 4.0f;
        filter_update(&DAT_001ce4c, -340.0f, 0x50);
        DAT_001cb18 = 2 * (int)((float)offset + DAT_001ce4c);
        DAT_001cb20 = 2;
    } else {
        width = *(int *)(DAT_0015ccc + 0x30);
        offset = (0x140 - width / 2) / 2;
        DAT_001ce50 = 4.0f;
        filter_update(&DAT_001ce4c, 0.0f, 0x5a);
        DAT_001cb18 = 2 * (int)((float)offset + DAT_001ce4c);
        DAT_001cb20 = 2;
    }
    DAT_001cb1c = 2 * (int)DAT_001ce50;
    DAT_001cb14 = 1;
    DAT_001cb24 = 0;
    panel_update();
}
