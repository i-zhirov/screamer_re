/* The rebuilt `object_unload` module.
 *
 * The original sits at linear 0x11304 in STARTH.EXE and holds 56 instructions
 * and 196 bytes up to the `ret` at 0x113c7.  The next function starts at
 * 0x113c8, so the body fills the whole 196-byte extent and the record sets no
 * separate function size.  The identity check uses 56 instructions.
 *
 * The function has two direct callers.  `race_end` at 0x279a4 calls it at
 * 0x279e0, and `race_cleanup` at 0x2a1b0 calls it at 0x2a1e1.  Both callers
 * pass the object-list table `0x6530` in `eax`.  The function releases the
 * object resources that `object_load` loads, so the record takes the same
 * section `Resource loader`.
 *
 * The function frees the two object resources.  It frees the name `3D_HEADER`
 * at object-3 offset 0x3d4 and the name `3D_BLKOBJ` at object-3 offset 0x3de
 * from the resource list at `DAT_0000801c`.  A non-zero result runs the memory
 * error report at 0x2edad with the same list.
 *
 * The function then clears the 0x78 records of 0x1c bytes at the argument.  It
 * clears the field at `+0x14`, the field at `+0x0c`, and the field at `+0x10`,
 * and it sets the field at `+0x18` to -1.  It returns 0.
 *
 * `object_load` at 0x10bfc loads the same two resources into the same list, so
 * the record names this function `object_unload` with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction, so it needs no FPU stub, and
 * the module uses no assembly.
 */
struct object_entry {
    char pad_00[0x0c];
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
};

extern int resource_free_name(void *list, char *name);
extern void memory_error_report(void *list);
extern void *DAT_0000801c;

int object_unload(struct object_entry *table)
{
    int index;

    if (resource_free_name(DAT_0000801c, "3D_HEADER") != 0) {
        memory_error_report(DAT_0000801c);
    }
    if (resource_free_name(DAT_0000801c, "3D_BLKOBJ") != 0) {
        memory_error_report(DAT_0000801c);
    }
    for (index = 0; index < 0x78; ++index) {
        table[index].field_14 = 0;
        table[index].field_0c = 0;
        table[index].field_10 = 0;
        table[index].field_18 = -1;
    }
    return 0;
}
