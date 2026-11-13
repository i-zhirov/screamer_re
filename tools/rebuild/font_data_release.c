/* The rebuilt `font_data_release` module.
 *
 * The original sits at linear 0x25640 in STARTH.EXE and holds 9 instructions
 * and 22 bytes up to the `ret` at 0x25655.  The next record `font_load` starts
 * at 0x25658 after the two padding bytes at 0x25656 and 0x25657, so the store
 * record keeps the 24-byte extent and sets no separate function size.  The
 * identity check uses 9 instructions.
 *
 * The direct callers are `screen_setup_cleanup` at 0x15150 with the call site
 * at 0x15164, `race_data_release` at 0x156a8 with the call site at 0x156f9,
 * `race_end` at 0x279a4 with the call site at 0x27a0d, and `race_cleanup` at
 * 0x2a1b0 with the call site at 0x2a1be.  Every caller runs the function in a
 * teardown.
 *
 * The function frees the `FONTS` resource at object-3 offset 0xea4 from the
 * archive `DAT_000062ac` through `resource_free_name` at 0x2f326.  The string
 * at linear 0x100ea4 holds `FONTS`.  The function loads the name pointer in
 * edx and the archive context in eax before the call.
 *
 * The sibling `font_load` at 0x25658 gets the same `FONTS` resource at
 * object-3 offset 0xead, so the two functions form the font data load and
 * release pair.  The module sits with the graphics decoders, so it takes the
 * section The graphics decoders.
 *
 * The `FONTS` resource, the caller group, and the sibling loader support the
 * name `font_data_release`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly body.
 */
extern int DAT_000062ac;

extern void resource_free_name(int context, char *name);

void font_data_release(void)
{
    resource_free_name(DAT_000062ac, "FONTS");
}
