/* The rebuilt `map_data_release` module.
 *
 * The original sits at linear 0x4cef4 in STARTH.EXE and holds 15 instructions
 * and 53 bytes up to the `ret` at 0x4cf28.  The next function starts at
 * 0x4cf2c after the three padding bytes at 0x4cf29, so the store record keeps
 * the 56-byte extent and sets no separate function size.  The identity check
 * uses 15 instructions.
 *
 * The only direct caller is the wrapper at 0x4cf2c.  It calls the sibling at
 * 0x4cecc and then this function, and `race_data_release` at 0x156a8 calls the
 * wrapper during a shutdown.  The sibling frees the `MATERIALS` resource, so
 * this function frees the map data that `unr_file_load` at 0x4c98c and
 * `mmp_file_load` at 0x4cae4 load.
 *
 * The function frees the `OBJ_LST` and `UNR_FILE` resources of the archive
 * head `DAT_0021574` through `resource_free_name`, at object-3 offsets 0x171b
 * and 0x1723.  It then frees the `ALFILE` map grid record whose allocation
 * pointer is `DAT_0021468` through `resource_find`.
 *
 * The two resource names, the map grid record, the wrapper caller, and the
 * sibling material release support the name `map_data_release`.  The original
 * has no symbol, so the record keeps the name with the confidence inferred.
 * The module sits with the resource loaders, so it takes the section Resource
 * loader.
 *
 * The original has the EBP frame.  The C form with `-of+` adds `mov esp,ebp`
 * and reorders the argument setup, so the recipe uses the exact
 * `map_data_release.asm` body.  This C source stays as the readable behavioral
 * reference.  The function holds no floating-point instruction, so the module
 * needs no FPU stub.
 */
extern int DAT_0021574;
extern int DAT_0021468;

extern void resource_free_name(int context, char *name);
extern void resource_find(int context, int key);

void map_data_release(void)
{
    resource_free_name(DAT_0021574, "OBJ_LST");
    resource_free_name(DAT_0021574, "UNR_FILE");
    resource_find(DAT_0021574, DAT_0021468);
}
