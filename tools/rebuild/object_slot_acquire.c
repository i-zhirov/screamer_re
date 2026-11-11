/* The rebuilt function at linear 0x11660 in STARTH.EXE.
 *
 * The original body at 0x11660 holds 23 instructions and 60 bytes up to the
 * early ret at 0x1169b.  The success path continues at 0x1169c to the ret at
 * 0x11710, and the next function starts at 0x11714 after the three padding
 * bytes at 0x11711, so the store record keeps the 180-byte extent and sets no
 * separate function size.  The identity check uses 23 instructions.
 *
 * The function is one member of the object list helper group.  It takes the
 * object table at 0x6530 in eax, the list at 0x6350 in edx, and the address of
 * the entry index in ebx.  The table holds 0x78 records of 0x1c bytes.  A record
 * holds the name at +0x00, the block object at +0x0c, the chunk table at +0x10,
 * the file record at +0x14, and one slot word at +0x18.  A slot word of -1
 * marks a free entry.
 *
 * The two direct callers are object_list_link at 0x11568 and
 * replay_playback_start at 0x14d2c.  They pass the same table and list, and
 * they take a list slot for a record before they bind it.  The sibling release
 * at 0x11714 uses the same list.  The callers and the shared table support the
 * name object_slot_acquire with the confidence inferred and the section
 * Resource loader.  The original has no symbol.
 *
 * An entry with a slot other than -1 returns 0xd and changes nothing.  A free
 * entry starts at list index zero.  The scan skips each list value that is
 * neither -1 nor 0.  At the first -1 or 0 value the function clears the next
 * value when the found value is zero, stores the record block pointer in the
 * found list slot, and returns 0.  The found index stays in the entry slot.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.
 */
#define RECORD_SIZE 0x1c
#define SLOT_FREE (-1)
#define RESULT_USED 0xd

struct object_record {
    char name[0x0c];            /* +0x00 */
    void *block;                /* +0x0c */
    void *chunks;               /* +0x10 */
    void *file;                 /* +0x14 */
    int slot;                   /* +0x18 */
};

int object_slot_acquire(void *table, int *list, int *index_address)
{
    struct object_record *record;
    int slot;

    record = (struct object_record *)(
        (char *)table + *index_address * RECORD_SIZE
    );
    if (record->slot != SLOT_FREE) {
        return RESULT_USED;
    }
    record->slot = 0;
    while (list[record->slot] != SLOT_FREE && list[record->slot] != 0) {
        record->slot++;
    }
    slot = record->slot;
    if (list[slot] == 0) {
        list[slot + 1] = 0;
    }
    list[slot] = (int)record->block;
    return 0;
}
