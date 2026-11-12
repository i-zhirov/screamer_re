/* The rebuilt `object_handle_table_load` module.
 *
 * The original sits at linear 0x119cc in STARTH.EXE and holds 21 instructions
 * and 58 bytes up to the `ret` at 0x11a05.  The next function starts at
 * 0x11a08, so the store record keeps the 60-byte extent and sets no separate
 * function size.  The identity check uses 21 instructions.
 *
 * The only direct caller is `menu_objects_init` at 0x2aab8, call site 0x2aadc.
 * That caller runs on the menu space init path and loads the menu object list
 * before it looks the object names up, so the record takes the section
 * `Resource loader`.
 *
 * The function requests the `OBJ_LIST` name at object-3 offset 0x3f0 with the
 * size 0x17e80 through `resource_get` at 0x2eed0.  It passes the resource list
 * at object-3 offset 0x801c in `eax`, the name in `edx`, and the size in `ebx`.
 * It stores the result in the handle table pointer at object-3 offset 0x21540.
 * A zero result returns 0x12.  Otherwise the function writes the free marker
 * -1 to the first handle and returns 0.
 *
 * `object_load` at 0x10bfc walks the same handle table from the first handle
 * and stops at the -1 marker, so the loaded resource is the object file handle
 * table.  The caller, the resource name, and the free marker support the name
 * `object_handle_table_load`.  The original has no symbol, so the record keeps
 * the name with the confidence `inferred`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no other helper, so the
 * module uses no FPU stub and no assembly implementation.
 */
extern void *resource_get(void *resource, char *name, int size);
extern void *DAT_0000801c;
extern int *DAT_00021540;

int object_handle_table_load(void)
{
    int *table;

    table = resource_get(DAT_0000801c, "OBJ_LIST", 0x17e80);
    DAT_00021540 = table;
    if (table == 0) {
        return 0x12;
    }
    table[0] = -1;
    return 0;
}
