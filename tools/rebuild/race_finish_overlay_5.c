/* The rebuilt `race_finish_overlay_5` module.
 *
 * The original sits at linear 0x24264 in STARTH.EXE and holds 27 instructions
 * and 86 bytes up to the `ret` at 0x242b9.  The next function starts at
 * 0x242bc, so the store record keeps the 88-byte extent and sets no separate
 * function size.  The identity check uses 27 instructions.
 *
 * The only direct caller is `race_finish_update` at 0x21058.  During the
 * finish delay it switches on the mode `DAT_0011d260` and calls 0x240c4 for
 * mode 1, 0x2414c for mode 2, 0x241d4 for mode 4, 0x240a0 for mode 0x10,
 * this overlay for mode 0x20, and 0x2420c for mode 0x40.  The caller sits in
 * the frame update path, so the record takes the section Program functions.
 *
 * The function draws the car table string `DAT_00118f94[index]` at y 0x5a with
 * the font 0 and the colour 2.  The index is 0xd, or 0xe when bit 3 of
 * `DAT_00118e1d` is set.  When bit 7 (0x80) of `DAT_00118e1d` is set, the
 * function then draws the text pointer `DAT_00118fd0` at y 0x78 with the font
 * 0 and the colour 2.  The sibling overlay at 0x2420c holds the same body.
 *
 * The mode 0x20 selection, the car table, and the extra text support the name
 * `race_finish_overlay_5`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.
 */
extern unsigned char DAT_00118e1d;
extern char *DAT_00118f94[];
extern char *DAT_00118fd0;

extern void panel_draw_centered(char *text, int y, int font, int color);

void race_finish_overlay_5(void)
{
    int index;

    index = 0xd + ((DAT_00118e1d & 8) != 0);
    panel_draw_centered(DAT_00118f94[index], 0x5a, 0, 2);
    if ((DAT_00118e1d & 0x80) != 0) {
        panel_draw_centered(DAT_00118fd0, 0x78, 0, 2);
    }
}
