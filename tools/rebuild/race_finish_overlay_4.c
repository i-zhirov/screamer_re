/* The rebuilt `race_finish_overlay_4` module.
 *
 * The original sits at linear 0x240a0 in STARTH.EXE and holds 15 instructions
 * and 33 bytes up to the `ret` at 0x240c0.  The next function starts at
 * 0x240c4 after the three padding bytes at 0x240c1 through 0x240c3, so the
 * store record keeps the 36-byte extent and sets no separate function size.
 * The identity check uses 15 instructions.
 *
 * The only direct caller is `race_finish_update` at 0x21058.  During the
 * finish delay it switches on the mode `DAT_0011d260` and calls 0x240c4 for
 * mode 1, 0x2414c for mode 2, 0x241d4 for mode 4, this overlay for mode
 * 0x10, 0x24264 for mode 0x20, and 0x2420c for mode 0x40.  The caller sits in
 * the frame update path, so the record takes the section Program functions.
 *
 * The function draws the runtime text pointer `DAT_00118ff0` at y 0x5a with
 * the font 0 and the colour 2 through `panel_draw_centered` at 0x2e7cc.  The
 * body loads the pointer first, sets the colour, the y value, and the font,
 * and calls the helper.  The sibling overlay at 0x241d4 draws the car table
 * line at the same y with the same font and colour.
 *
 * The mode 0x10 selection, the runtime text pointer, and the shared y, font,
 * and colour support the name `race_finish_overlay_4`.  The suffix 4 is the
 * fourth case of the mode switch.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and calls one helper, so the
 * module needs no FPU stub and no assembly implementation.
 */
extern char *DAT_00118ff0;

extern void panel_draw_centered(char *text, int y, int font, int color);

void race_finish_overlay_4(void)
{
    panel_draw_centered(DAT_00118ff0, 0x5a, 0, 2);
}
