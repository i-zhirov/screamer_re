/* The rebuilt `font_table_init` module.
 *
 * The original sits at linear 0x25620 in STARTH.EXE.  The body holds 12
 * instructions and 31 bytes up to the `ret` at 0x2563e.  The next record
 * starts at 0x25640 after one padding byte at 0x2563f, so the store record
 * keeps the 32-byte extent and sets no separate function size.  The identity
 * check uses 12 instructions.
 *
 * The function sets the two font table pointers.  `DAT_00015438` points at the
 * descriptor table 0x1ca98, and `DAT_00015434` points at the bitmap table
 * 0x19898.  `text_measure` at 0x254a8 and `text_draw` at 0x25020 read both
 * tables, and `font_load` at 0x25658 fills them.
 *
 * The direct call sites are 0x133bf in `race_data_load` at 0x131a8 and 0x29d5a
 * in `menu_space_init` at 0x29d08.  Both callers run this initializer before
 * `font_load`.  The font tables belong to the graphics decoders, so the record
 * takes the section The graphics decoders.
 *
 * The descriptor table, the bitmap table, and the two callers support the
 * name `font_table_init`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub.  The
 * module uses no assembly.
 */
extern unsigned char *DAT_00015438;
extern unsigned char *DAT_00015434;

void font_table_init(void)
{
    DAT_00015438 = (unsigned char *)0x1ca98;
    DAT_00015434 = (unsigned char *)0x19898;
}
