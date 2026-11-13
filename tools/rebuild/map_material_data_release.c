/* The rebuilt `map_material_data_release` module.
 *
 * The original sits at linear 0x4cf2c in STARTH.EXE and holds 6 instructions
 * and 15 bytes up to the `ret` at 0x4cf3a.  The next function starts at
 * 0x4cf3c after the padding byte at 0x4cf3b, so the store record keeps the
 * 16-byte extent and sets no separate function size.  The identity check uses
 * 6 instructions.
 *
 * The two direct callers are `race_data_release` at 0x156a8 with the call site
 * at 0x156e5 and `race_end` at 0x279a4 with the call site at 0x279fe.  Both
 * callers run during a shutdown, so the function releases the map and material
 * data at the end of a race.
 *
 * The function calls `material_data_release` at 0x4cecc and then
 * `map_data_release` at 0x4cef4 with no arguments.  The first helper frees the
 * `MATERIALS` resource and the material record.  The second helper frees the
 * `OBJ_LST` and `UNR_FILE` resources and the map grid record.
 *
 * The two callers, the two helpers, and the sibling names support the name
 * `map_material_data_release`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.  The callers span several sections,
 * so the module takes the section Resource loader.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly body.
 */
extern void material_data_release(void);
extern void map_data_release(void);

void map_material_data_release(void)
{
    material_data_release();
    map_data_release();
}
