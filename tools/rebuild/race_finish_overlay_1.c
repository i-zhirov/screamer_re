/* The rebuilt `race_finish_overlay_1` module.
 *
 * The original sits at linear 0x240c4 in STARTH.EXE and holds 39 instructions
 * and 135 bytes up to the `ret` at 0x2414a.  The next function starts at
 * 0x2414c after the one padding byte at 0x2414b, so the store record keeps the
 * 136-byte extent and sets no separate function size.  The identity check uses
 * 39 instructions.
 *
 * The only direct caller is `race_finish_update` at 0x21058.  During the
 * finish delay it switches on the mode `DAT_0011d260` and calls this overlay
 * for mode 1, 0x2414c for mode 2, 0x241d4 for mode 4, 0x240a0 for mode 0x10,
 * 0x24264 for mode 0x20, and 0x2420c for mode 0x40.  The caller sits in the
 * frame update path, so the record takes the section Program functions.
 *
 * When bit 8 of `DAT_00118e1d` is set, the function draws the text pointer
 * `DAT_00118fc0` at y 0x3e with the font 2.  It then formats the two car table
 * strings `DAT_00118f94[DAT_0010ffa4]` and `DAT_00118f94[0]` with the format
 * `%s %s` at object-3 offset 0xe50, draws the line at y 0x5a with the font 2,
 * and draws the zero clock text `00:00:00` at object-3 offset 0x1f98 at y 0x78
 * with the font 2.  The sibling overlay at 0x2414c holds the same body and
 * reads the copy of the format at offset 0xe56.
 *
 * The original has no symbol, so the record keeps the name with the confidence
 * inferred.  The original has the EBP frame, so the module builds with `-of+`.
 * The function holds no floating-point instruction, so the module uses no FPU
 * stub and no assembly.
 */
extern unsigned char DAT_00118e1d;
extern char *DAT_00118fc0;
extern int DAT_0010ffa4;
extern char *DAT_00118f94[];
extern char race_finish_format[];
extern char race_finish_clock[];

extern int sprintf(char *buffer, const char *format, ...);
extern void panel_draw_centered(char *text, int y, int font, int color);

void race_finish_overlay_1(void)
{
    char buffer[0x100];

    if ((DAT_00118e1d & 8) != 0) {
        panel_draw_centered(DAT_00118fc0, 0x3e, 2, 0);
    }
    sprintf(buffer, race_finish_format, DAT_00118f94[DAT_0010ffa4],
        DAT_00118f94[0]);
    panel_draw_centered(buffer, 0x5a, 2, 0);
    panel_draw_centered(race_finish_clock, 0x78, 2, 0);
}
