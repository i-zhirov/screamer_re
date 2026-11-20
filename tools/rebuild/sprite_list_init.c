/* The rebuilt `sprite_list_init` module.
 *
 * The original sits at linear 0x257a0 in STARTH.EXE.  The body holds 5
 * instructions and 15 bytes up to the `ret` at 0x257ae.  The next function
 * starts at 0x257b0 after one padding byte at 0x257af, so the store record
 * keeps the 16-byte extent and sets no separate function size.  The identity
 * check uses 5 instructions.
 *
 * The function stores the list base 0x1cb14 in the pointer DAT_00015cd0.  The
 * fixup at the pointer slot points at object 1 offset 0x15cd0, and the fixup at
 * the immediate points at object 3 offset 0x1cb14, so the loaded pointer is
 * DAT_0011cb14.  The sprite-list flush at 0x25af0 walks the 0x28-byte entries
 * from this pointer, and the HUD panel at 0x27a5c and the panel at 0x2dfac
 * write the active flag at 0x1cb14 and the fields up to 0x1cb24 before they
 * flush the list.
 *
 * The two direct call sites are 0x133c4 in `race_data_load` at 0x131a8 and
 * 0x29d5f in `menu_space_init` at 0x29d08.  Both callers run this initializer
 * after `font_table_init` at 0x25620 and before the sprite load, so the
 * function sets the sprite list base for the menu screen.  The sprite-list
 * modules sit in the section Program functions, so the record takes that
 * section.
 *
 * The pointer, the two callers, and the sprite-list flush support the name
 * `sprite_list_init`.  The original has no symbol, so the record keeps the name
 * with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub.  The
 * module uses no assembly.
 */
extern void *DAT_00015cd0;

void sprite_list_init(void)
{
    DAT_00015cd0 = (void *)0x1cb14;
}
