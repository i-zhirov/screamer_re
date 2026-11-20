/* The rebuilt `object_lookup` module.
 *
 * The original sits at linear 0x110d0 in STARTH.EXE and holds 40 instructions
 * and 110 bytes up to the early `ret` at 0x1113d.  The full body continues
 * through the failure return at 0x1113e to the `ret` at 0x11144, and the next
 * function starts at 0x11148, so the store record keeps the 120-byte extent and
 * sets no separate function size.  The identity check uses 40 instructions.
 *
 * The arguments arrive in the Watcom register order: `eax` holds the object
 * table at object-3 offset 0x6530, `edx` holds the wanted name, and `ebx` holds
 * the output slot pointer.  The prologue saves the three arguments in local
 * cells, so the function uses no stack argument.
 *
 * A record holds the name at `+0x00`, the block object at `+0x0c`, the chunk
 * table at `+0x10`, the file record at `+0x14`, and one reserved word at
 * `+0x18`.  A zero file record marks a free slot.  The function walks the 0x78
 * records of 0x1c bytes and skips a free slot.  For a busy slot it compares the
 * record name with the wanted name through the case-insensitive compare at
 * 0x4d9ba.  A match stores the index in the output slot and returns 1.  A
 * missing name returns 0 and leaves the output slot unchanged.
 *
 * The only direct caller at 0x11080 is the unnamed object loader wrapper at
 * 0x11060.  That wrapper looks the name up, makes one more instance of an
 * existing object through object_duplicate at 0x10ed4, and otherwise calls
 * object_request at 0x114b4 and object_load at 0x10bfc.  The rebuilt
 * opponent_records_init module already calls this function as `object_lookup`,
 * and the object table, the name search, and the index output support the name
 * with the confidence inferred.  The function sits in the section Resource
 * loader.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction, so it needs no FPU stub.  The
 * rebuilt C uses the standard Watcom calling convention, so the identity check
 * differs in the prologue and in the register use.  The harness supplies the
 * case-insensitive compare as `stricmp`.
 */
struct object_record {
    char name[0x0c];            /* +0x00 */
    int block;                  /* +0x0c */
    int chunks;                 /* +0x10 */
    int file;                   /* +0x14 */
    int reserved;               /* +0x18 */
};

extern int stricmp(const char *left, const char *right);

int object_lookup(char *table, char *name, int *slot)
{
    struct object_record *record;
    int index;

    for (index = 0; index < 0x78; index++) {
        record = &((struct object_record *)table)[index];
        if (record->file != 0) {
            if (stricmp(name, record->name) == 0) {
                *slot = index;
                return 1;
            }
        }
    }
    return 0;
}
