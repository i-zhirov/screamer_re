/* The rebuilt function at linear 0x115f8 in STARTH.EXE.
 *
 * The original body holds 40 instructions and 101 bytes up to the `ret` at
 * 0x1165c.  The next function starts at 0x11660, so the store record keeps the
 * 104-byte extent and sets no separate function size.  The identity check uses
 * 40 instructions.
 *
 * The function is the release-all pass of the object list helper group.  It
 * takes the object table in `eax` and the list in `edx`.  The table holds 0x78
 * records of 0x1c bytes.  A record holds the name at `+0x00`, the block object
 * at `+0x0c`, the chunk table at `+0x10`, the file record at `+0x14`, and one
 * slot word at `+0x18`.  A zero file record marks a free slot.
 *
 * Race end at 0x279a4, race cleanup at 0x2a1b0, and race data release at
 * 0x156a8 call the pass with the object table at 0x6530 and the list at 0x6350.
 * They call it during the shutdown of a race, and race_data_release already
 * names the call `object_list_release_all`.  The shared table and list match
 * object_list_link at 0x11568, so the record takes the name
 * object_list_release_all with the confidence inferred and the section Resource
 * loader.  The original has no symbol.
 *
 * The function walks the table from index 0 to index 0x77.  For an active
 * record it calls the slot release at 0x11714 with the table, the list, and the
 * record index.  A non-zero result returns at once.  The function returns zero
 * after the last record.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.  The callee name stays local to the harness.
 */
#define RECORD_COUNT 0x78
#define RECORD_SIZE 0x1c

struct object_record {
    char name[0x0c];            /* +0x00 */
    void *block;                /* +0x0c */
    void *chunks;               /* +0x10 */
    void *file;                 /* +0x14 */
    int slot;                   /* +0x18 */
};

extern int object_slot_release(void *table, int *list, int *index);

int object_list_release_all(void *table, int *list)
{
    struct object_record *record;
    int index;
    int entry;
    int result;

    for (index = 0; index < RECORD_COUNT; index++) {
        record = (struct object_record *)((char *)table + index * RECORD_SIZE);
        if (record->file == 0) {
            continue;
        }
        entry = index;
        result = object_slot_release(table, list, &entry);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}
