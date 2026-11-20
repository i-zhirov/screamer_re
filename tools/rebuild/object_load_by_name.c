/* The rebuilt `object_load_by_name` module.
 *
 * The original sits at linear 0x11060 in STARTH.EXE.  The identity window holds
 * 22 instructions and 59 bytes up to the ret at 0x1109a.  The zero-result arm
 * continues at 0x1109b to the ret at 0x110ce, and the next function starts at
 * 0x110d0, so the store record keeps the 112-byte extent and sets no separate
 * function size.  The identity check uses 22 instructions.
 *
 * The function is the top-level object loader.  It takes the 0x78-record object
 * table at object-3 offset 0x6530 in eax, the wanted name in edx, and the
 * output slot pointer in ebx.
 *
 * The function copies the slot value into a local source index.  The lookup at
 * 0x110d0 searches the table by name and stores the found index in the local.
 * A found object goes to object_duplicate at 0x10ed4, which makes one more
 * instance of the source index in the output slot.  A missing object goes to
 * object_request at 0x114b4, which reserves a free record and stores the name.
 * A request result other than zero returns at once.  Otherwise the function
 * calls object_load at 0x10bfc with the new index and the count 1.
 *
 * The many callers pass names such as SHADOW and the model names from 0xd25c,
 * so the record takes the name object_load_by_name with the confidence
 * inferred, the section Resource loader, and the symbol object_load_by_name_.
 * The original has no symbol.
 *
 * The original has the EBP frame and two locals, so the module builds with
 * `-of+`.  The function holds no floating-point instruction, so the module uses
 * no FPU stub and no assembly implementation.
 */
extern int object_lookup(int *table, char *name, int *slot);
extern int object_duplicate(int *table, int *output, int *source);
extern int object_request(int *table, char *name, int *slot);
extern int object_load(int *table, int start, int count);

int object_load_by_name(int *table, char *name, int *slot)
{
    int found;
    int result;

    found = *slot;
    if (object_lookup(table, name, &found) != 0) {
        return object_duplicate(table, slot, &found);
    }
    result = object_request(table, name, slot);
    if (result != 0) {
        return result;
    }
    return object_load(table, *slot, 1);
}
