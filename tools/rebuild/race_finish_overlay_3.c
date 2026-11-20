/* The rebuilt `race_finish_overlay_3` module.
 *
 * The original sits at linear 0x241d4 in STARTH.EXE and holds 20 instructions
 * and 55 bytes up to the `ret` at 0x2420a.  The one padding byte at 0x2420b
 * is part of the 56-byte extent and the next function starts at 0x2420c, so
 * the store record keeps the extent and sets no separate function size.  The
 * identity check uses 20 instructions.
 *
 * The only direct caller is `race_finish_update` at 0x21058.  During the
 * finish delay it switches on the mode `DAT_0011d260` and calls 0x240c4 for
 * mode 1, 0x2414c for mode 2, this overlay for mode 4, 0x240a0 for mode
 * 0x10, 0x24264 for mode 0x20, and 0x2420c for mode 0x40.  The caller sits in
 * the frame update path, so the record takes the section Program functions.
 *
 * The function draws the car table string `DAT_00118f94[index]` at y 0x5a with
 * the font 0 and the colour 2.  The index is 0xd, or 0xe when bit 3 of
 * `DAT_00118e1d` is set.  The sibling overlays at 0x24264 and 0x2420c hold the
 * same line and add the extra text at y 0x78 when bit 7 is set; this overlay
 * holds the line only.
 *
 * The mode 4 selection, the car table, and the shortest body support the name
 * `race_finish_overlay_3`.  The suffix 3 is the third case of the mode switch.
 * The original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.
 */
extern unsigned char DAT_00118e1d;
extern char *DAT_00118f94[];

extern void panel_draw_centered(char *text, int y, int font, int color);

void race_finish_overlay_3(void)
{
    int index;

    index = 0xd + ((DAT_00118e1d & 8) != 0);
    panel_draw_centered(DAT_00118f94[index], 0x5a, 0, 2);
}
