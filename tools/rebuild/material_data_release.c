/* The rebuilt `material_data_release` module.
 *
 * The original sits at linear 0x4cecc in STARTH.EXE and holds 12 instructions
 * and 38 bytes up to the `ret` at 0x4cef1.  The next function starts at
 * 0x4cef4 after the two padding bytes at 0x4cef2 and 0x4cef3, so the store
 * record keeps the 40-byte extent and sets no separate function size.  The
 * identity check uses 12 instructions.
 *
 * The wrapper at 0x4cf2c calls this function and the sibling at 0x4cef4.
 * screen_setup_cleanup at 0x15150 and race_cleanup at 0x2a1b0 also call this
 * function during a teardown.
 *
 * The function frees the `MATERIALS` resource of the archive head
 * `DAT_0021574` through `resource_free_name`, at object-3 offset 0x1711.  It
 * then frees the material record whose allocation pointer is `DAT_00214e8`
 * through `resource_find`.  `material_load` at 0x4cd9c loads the resource and
 * sets the record, so the two functions form the material load and release
 * pair.
 *
 * The `MATERIALS` resource, the material record, the wrapper caller, and the
 * sibling map data release support the name `material_data_release`.  The
 * original has no symbol, so the record keeps the name with the confidence
 * inferred.  The module sits with the resource loaders, so it takes the
 * section Resource loader.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly body.
 */
extern int DAT_0021574;
extern int DAT_00214e8;

extern void resource_free_name(int context, char *name);
extern void resource_find(int context, int key);

void material_data_release(void)
{
    resource_free_name(DAT_0021574, "MATERIALS");
    resource_find(DAT_0021574, DAT_00214e8);
}
