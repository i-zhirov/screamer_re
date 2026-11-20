/* The rebuilt `func_113c8` module.
 *
 * The original sits at linear 0x113c8 in STARTH.EXE and holds 27 instructions
 * and 71 bytes up to the `ret` at 0x1140e.  The next entry starts at 0x11410,
 * so the store record keeps the 72-byte extent and sets no separate function
 * size.  The identity check uses 27 instructions.
 *
 * The only direct callee at 0x113fb is `object_chunk_find` at 0x11410.  The
 * function takes the wanted name in `eax`, the position block in `edx`, and the
 * object table in `ebx`, and it passes the same three arguments to the callee.
 * The position block holds the object-table slot at `+0x00` and takes the
 * chunk index at `+0x04`.
 *
 * The function walks the 0x78 object-table slots.  It writes the current slot
 * to the position block, calls `object_chunk_find`, and returns 0 on a match.
 * A missing name leaves the last slot 0x77 in the position block and returns 7.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub, and
 * the module uses no assembly.
 */
struct object_record {
    char name[0x0c];
    void *block;
    void *chunks;
    void *file;
    int reserved;
};

struct chunk_position {
    int slot;
    int index;
};

extern int object_chunk_find(char *name, struct chunk_position *position,
    struct object_record *objects);

int func_113c8(char *name, struct chunk_position *position,
    struct object_record *objects)
{
    int slot;

    for (slot = 0; slot < 0x78; slot++) {
        position->slot = slot;
        if (object_chunk_find(name, position, objects) == 0) {
            return 0;
        }
    }
    return 7;
}
