/* The rebuilt `object_chunk_find` module.
 *
 * The original sits at linear 0x11410 in STARTH.EXE and holds 63 instructions
 * and 163 bytes up to the `ret` at 0x114b2.  The next function starts at
 * 0x114b4, so the store record keeps the 164-byte extent and sets no separate
 * function size.  The identity check uses 63 instructions.
 *
 * The direct callers pass the object table at 0x6530 in `ebx`, the name in
 * `eax`, and a position block in `edx`.  The callers are the wrapper at
 * 0x113c8, `player_car_init` at 0x136cc, `car_setup` at 0x142ec, and the
 * unnamed function at 0x14970.  The position block holds the object table slot
 * at `+0x00` and takes the chunk index at `+0x04`.  The names at object-3
 * offsets 0x58c, 0x592, and 0x5a1 are `fauto`, `MV_mauto`, and `rauto`; the
 * names at 0x1d5c through 0x1d74 are the wheel names `flx`, `fli`, `blx`,
 * `rli`, `frx`, `fri`, `brx`, and `rri`.
 *
 * A record holds the block object at `+0x0c`, the chunk table at `+0x10`, the
 * file record at `+0x14`, and one reserved word at `+0x18`.  The file record
 * holds the chunk count at `+0x14` and the chunk offset table at `+0x18`, one
 * 32-bit file-relative offset per chunk.  A chunk holds its name at `+0x68`.
 *
 * The function selects the record of the position slot, walks the chunk table,
 * and compares each chunk name with the wanted name through the
 * case-insensitive compare at 0x4d9ba.  A match stores the chunk index at
 * position `+0x04` and returns 0.  A missing name copies the name to the
 * scratch buffer `DAT_00007fe8` and returns 7.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction, so it needs no FPU stub.  The
 * rebuilt C uses the standard Watcom calling convention, so the identity check
 * differs in the prologue and in the register use.  The behavior check covers
 * the search.
 */
struct object_record {
    char name[0x0c];            /* +0x00 */
    void *block;                /* +0x0c */
    void *chunks;               /* +0x10 */
    void *file;                 /* +0x14 */
    int reserved;               /* +0x18 */
};

struct chunk_position {
    int slot;                   /* +0x00 */
    int index;                  /* +0x04 */
};

extern char DAT_00007fe8[];
extern int stricmp(const char *left, const char *right);

static void copy_name(char *destination, char *source)
{
    char value;

    for (;;) {
        value = source[0];
        destination[0] = value;
        if (value == 0) {
            return;
        }
        value = source[1];
        destination[1] = value;
        source += 2;
        destination += 2;
        if (value == 0) {
            return;
        }
    }
}

int object_chunk_find(char *name, struct chunk_position *position,
    struct object_record *objects)
{
    struct object_record *record;
    char *file;
    char *chunk;
    int count;
    int index;
    int offset;

    record = &objects[position->slot];
    file = record->file;
    count = *(int *)(file + 0x14);
    for (index = 0; index < count; index++) {
        offset = *(int *)(file + 0x18 + index * 4);
        chunk = file + offset;
        if (stricmp(chunk + 0x68, name) == 0) {
            position->index = index;
            return 0;
        }
    }
    copy_name(DAT_00007fe8, name);
    return 7;
}
