/* The rebuilt `sprite_list_flush` module.
 *
 * The original sits at linear 0x25af0 in STARTH.EXE and holds 11 instructions
 * and 32 bytes up to the `ret` at 0x25b0f.  The next function starts at
 * 0x25b10, so the store record keeps the 32-byte extent and sets no separate
 * function size.  The identity check uses 11 instructions.
 *
 * The function loads the entry count from DAT_0015cd8 and the list base from
 * DAT_0015cd0.  It walks the 0x28-byte entries and calls `sprite_entry_draw` at
 * 0x25b10 for every entry with bit 0 of the first dword set.  The list base
 * holds the sprite fields of the viewport record, and the init function at
 * 0x257a0 sets it to 0x1cb14.  The entry holds the active flag at `+0x00`, the
 * x at `+0x04`, the y at `+0x08`, the sprite record index at `+0x0c`, and the
 * reserved cell at `+0x10`.
 *
 * The direct callers are `race_intro_update` at 0x20d20, `options_screen` at
 * 0x21338, `race_gauges` at 0x2369c, `race_radar` at 0x23a94, `hud_panel_show`
 * at 0x27a5c, and `panel_show` at 0x2dfac.  They write the sprite position
 * fields first and then flush the list.  The sibling modules `sprite_entry_draw`
 * at 0x25b10 and `shade_rect` at 0x25d28 sit in the section Program functions,
 * so the record takes that section.
 *
 * The original loops with `dec` and `jne`, so the loop runs at least one time.
 * The module keeps the `do`/`while` form.  The original uses `pushal` and
 * `popal` and has no EBP frame, so the module builds without `-of+`.  The
 * function holds no floating-point instruction, so the module needs no FPU stub
 * and no assembly implementation.
 */
struct sprite_entry {
    unsigned long flags;
    int x;
    int y;
    int sprite;
    int reserved;
    int pad[5];
};

extern struct sprite_entry *DAT_0015cd0;
extern int DAT_0015cd8;

extern void sprite_entry_draw(struct sprite_entry *entry);

void sprite_list_flush(void)
{
    struct sprite_entry *entry = DAT_0015cd0;
    int count = DAT_0015cd8;

    do {
        if (entry->flags & 1) {
            sprite_entry_draw(entry);
        }
        entry++;
        count--;
    } while (count != 0);
}
