/* The rebuilt `sprite_data_release` module.
 *
 * The original sits at linear 0x258b8 in STARTH.EXE and holds 9 instructions
 * and 22 bytes up to the `ret` at 0x258cc.  The next record starts at 0x258d0
 * after the two padding bytes at 0x258ce and 0x258cf, so the store record keeps
 * the 24-byte extent and sets no separate function size.  The identity check
 * uses 9 instructions.
 *
 * The direct callers are `screen_setup_cleanup` at 0x15150 with the call site
 * at 0x15169, `race_data_release` at 0x156a8 with the call site at 0x156f4,
 * `race_end` at 0x279a4 with the call site at 0x27a12, and `race_cleanup` at
 * 0x2a1b0 with the call site at 0x2a1c3.  Every caller runs the function in a
 * teardown, so the function releases the sprite data at the end of a race or a
 * failed screen setup.
 *
 * The function frees the `SPRITES` resource at object-3 offset 0xec7 from the
 * archive `DAT_000062ac` through `resource_free_name` at 0x2f326.  The string
 * at linear 0x100ec7 holds `SPRITES`.  The sibling `sprite_load` at 0x257b0
 * gets the same `SPRITES` resource, so the two functions form the sprite load
 * and release pair.  The module sits with the graphics decoders, so it takes
 * the section The graphics decoders.
 *
 * The `SPRITES` resource, the caller group, and the sibling loader support the
 * name `sprite_data_release`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly body.
 */
extern int DAT_000062ac;

extern void resource_free_name(int context, char *name);

void sprite_data_release(void)
{
    resource_free_name(DAT_000062ac, "SPRITES");
}
