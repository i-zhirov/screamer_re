/* The rebuilt `object_list_bind` module.
 *
 * The original sits at linear 0x11850 in STARTH.EXE and holds 18 instructions
 * and 51 bytes up to the first `ret` at 0x11882.  The bind path continues at
 * 0x11883 to the second `ret` at 0x118a3, and the next function starts at
 * 0x118a4, so the store record keeps the 84-byte extent and sets no separate
 * function size.  The identity check uses 18 instructions.
 *
 * The helper is one member of the object list group.  It takes the object table
 * at 0x6530 in `eax`, the list at 0x6350 in `edx`, and the address of the entry
 * index in `ebx`.  The table holds 0x78 records of 0x1c bytes.  A record holds
 * the name at `+0x00`, the block object at `+0x0c`, the chunk table at
 * `+0x10`, the file record at `+0x14`, and one slot word at `+0x18`.
 *
 * The index selects one record.  A record with the slot word `-1` returns 0xe
 * and changes nothing.  Otherwise the function writes the block pointer with
 * the two top bits inverted into `list[slot]` and returns 0.  The helper at
 * 0x117fc writes the same pointer without the inversion, and the helper at
 * 0x117a0 inverts only the top bit.
 *
 * The callers pass the shared table and list.  The name `object_list_bind`
 * reaches the function through the object list group in program-map.md.  The
 * original has no symbol, so the record keeps the name with the confidence
 * inferred and the section Resource loader.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no direct call, so it needs
 * no FPU stub and no assembly.
 */
#define RECORD_SIZE 0x1c
#define SLOT_FREE (-1)
#define RESULT_UNBOUND 0xe
#define LIST_TAG 0xc0000000

struct object_record {
    char name[0x0c];            /* +0x00 */
    void *block;                /* +0x0c */
    void *chunks;               /* +0x10 */
    void *file;                 /* +0x14 */
    int slot;                   /* +0x18 */
};

int object_list_bind(void *table, int *list, int *index_address)
{
    struct object_record *record;
    int index;
    int slot;

    index = *index_address;
    record = (struct object_record *)((char *)table + index * RECORD_SIZE);
    if (record->slot == SLOT_FREE) {
        return RESULT_UNBOUND;
    }
    slot = record->slot;
    list[slot] = (int)record->block ^ (int)LIST_TAG;
    return 0;
}
