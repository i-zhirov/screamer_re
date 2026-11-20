/* The rebuilt `map_object_record_init` module.
 *
 * The original sits at linear 0x2b450 in STARTH.EXE and holds 22 instructions
 * and 65 bytes up to the `ret` at 0x2b490.  The next function starts at
 * 0x2b494, so the store record keeps the 68-byte extent and sets no separate
 * function size.  The identity check uses 22 instructions.
 *
 * The function receives the record in `eax` and the index in `edx`.  It stores
 * the index in the first record field, sets the object field +0x08 to -1, and
 * binds the object slot at 0x1d2d0 + index * 8 through object_list_bind at
 * 0x11850.  The helper reads the index from the slot and binds the record into
 * the display list.
 *
 * The only direct caller is screen_object_lists_init at 0x2dff4, which resets
 * the 0x50 map object records at DAT_0011db78, stride 0x64.  The module
 * map_object_place at 0x2b044 and the module map_object_update at 0x2b9cc read
 * and fill the same records, so the record takes the name
 * map_object_record_init with the confidence inferred and the section
 * race_frame callees.
 *
 * The original has the EBP frame, so the module builds with -of+; the function
 * holds no floating-point instruction and makes no floating-point call, so the
 * module needs no FPU stub and no assembly implementation.
 */
extern void object_list_bind(void *table, char *name, int *slot);
extern char DAT_001d2d0[];

void map_object_record_init(int *record, int index)
{
    record[0] = index;
    record[2] = -1;
    object_list_bind((void *)0x6530, (char *)0x6350,
                     (int *)(DAT_001d2d0 + index * 8));
}
