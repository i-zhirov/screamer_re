/* The rebuilt `object_handle_table_release` module.
 *
 * The original sits at linear 0x11a08 in STARTH.EXE and holds 13 instructions
 * and 36 bytes up to the `ret` at 0x11a2b.  The next function starts at
 * 0x11a30, so the store record keeps the 40-byte extent and sets no separate
 * function size.  The identity check uses 13 instructions.
 *
 * The only direct caller is `race_cleanup` at 0x2a1b0, call site 0x2a1e6.  It
 * calls this function after `object_unload` at 0x11304 and before
 * `material_data_release` at 0x4cecc.  The function frees the object resource
 * that `object_handle_table_load` at 0x119cc loads, so the record takes the
 * section `Resource loader`.
 *
 * The function frees the `OBJ_LIST` name at object-3 offset 0x3f9 from the
 * resource list at `DAT_0000801c` through `resource_free_name` at 0x2f326.  A
 * non-zero result runs the memory error report at 0x2edad with the same list.
 * The string at linear 0x1003f9 holds `OBJ_LIST`.
 *
 * `object_handle_table_load` at 0x119cc requests the same `OBJ_LIST` name
 * through the suffix at object-3 offset 0x3f0 and stores the result in the
 * handle table pointer at `DAT_00021540`; the load and release pair support the
 * name `object_handle_table_release`.  The original has no symbol, so the
 * record keeps the name with the confidence `inferred`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and calls two resource helpers,
 * so the module needs no FPU stub and no assembly implementation.
 */
extern int resource_free_name(void *list, char *name);
extern void memory_error_report(void *list);
extern void *DAT_0000801c;

void object_handle_table_release(void)
{
    if (resource_free_name(DAT_0000801c, "OBJ_LIST") != 0) {
        memory_error_report(DAT_0000801c);
    }
}
