/* The rebuilt `object_request` module.
 *
 * The original sits at linear 0x114b4 in STARTH.EXE and holds 68 instructions
 * and 178 bytes up to the `ret` at 0x11565.  The next function starts at
 * 0x11568, so the store record keeps the 180-byte extent and sets no separate
 * function size.  The identity check uses 68 instructions.
 *
 * The only direct caller at 0x110a4 is the unnamed object loader wrapper at
 * 0x11060.  That wrapper searches the object table by name through the
 * case-insensitive compare at 0x4d9ba, makes one more instance of an existing
 * object through `object_duplicate` at 0x10ed4, and otherwise calls this
 * function and then `object_load` at 0x10bfc.  The table is the 0x78-record
 * object table at object-3 offset 0x6530.  A record holds the name at `+0x00`,
 * the block object at `+0x0c`, the chunk table at `+0x10`, the file record at
 * `+0x14`, and one reserved word at `+0x18`.  A zero file record marks a free
 * slot, and the value 1 marks a record that waits for the loader.
 *
 * The function clears the output slot index and scans the table from index 0.
 * The first record with a zero file record takes the input name at `+0x00`
 * and the value 1 at `+0x14`, so `object_load` then loads the named object.
 * The function returns 0.  A full table copies the name to the scratch buffer
 * `DAT_00007fe8` and returns 8.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no direct call, so it needs
 * no FPU stub and no assembly.
 */
struct object_record {
    char name[0x0c];            /* +0x00 */
    void *block;                /* +0x0c */
    void *chunks;               /* +0x10 */
    void *file;                 /* +0x14 */
    int reserved;               /* +0x18 */
};

extern char DAT_00007fe8[];

static void copy_name(char *destination, char *source)
{
    char value;

    for (;;) {
        value = *source;
        *destination = value;
        if (value == 0) {
            return;
        }
        source++;
        destination++;
    }
}

int object_request(struct object_record *objects, char *name, int *slot)
{
    struct object_record *record;

    *slot = 0;
    for (;;) {
        if (*slot == 0x78) {
            copy_name(DAT_00007fe8, name);
            return 8;
        }
        record = &objects[*slot];
        if (record->file == 0) {
            break;
        }
        (*slot)++;
    }

    copy_name(record->name, name);
    record->file = (void *)1;
    return 0;
}
