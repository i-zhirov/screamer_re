/* The rebuilt `object_list_link` module.
 *
 * The original sits at linear 0x11568 in STARTH.EXE and holds 53 instructions
 * and 144 bytes up to the `ret` at 0x115f7.  The next function starts at
 * 0x115f8, so the store record keeps the 144-byte extent and sets no separate
 * function size.  The identity check uses 53 instructions.
 *
 * Two direct callers reach this function:
 *
 *     0x12b92  race_session_start at 0x128e0
 *     0x2ae3e  menu_objects_init at 0x2aab8
 *
 * Both callers pass the object table at 0x6530 in `eax` and the list at 0x6350
 * in `edx`.  The two callers build the object list at session start and at menu
 * start.  The record takes the name `object_list_link` with the confidence
 * inferred and the section Resource loader.  The original has no symbol.
 *
 * The table holds 0x78 records of 0x1c bytes.  A record holds the name at
 * `+0x00`, the block object at `+0x0c`, the chunk table at `+0x10`, the file
 * record at `+0x14`, and one slot word at `+0x18`.  A zero file record marks a
 * free slot.
 *
 * The function walks the table from index 0 to index 0x77.  For an active
 * record it takes a list slot through 0x11660 with the table, the list, and the
 * record index.  It then reads the block object and tests bit 0x80 of the byte
 * at block offset 5.  When the bit is clear, the function leaves the plain slot
 * and continues.  When the bit is set, the
 * function tags the slot through 0x11850 and returns at once on a non-zero
 * result.  The function ignores the result of 0x11660.  At the end it returns
 * zero.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.  The two callee names stay local to the harness.
 */
#define RECORD_COUNT 0x78
#define RECORD_SIZE 0x1c
#define BLOCK_FLAG 0x80

struct object_record {
    char name[0x0c];            /* +0x00 */
    void *block;                /* +0x0c */
    void *chunks;               /* +0x10 */
    void *file;                 /* +0x14 */
    int reserved;               /* +0x18 */
};

extern int object_slot_acquire(void *table, void *list, int *index);
extern int object_list_bind(void *table, void *list, int *index);

int object_list_link(void *table, void *list)
{
    struct object_record *record;
    unsigned char *block;
    int index;
    int current;
    int result;

    for (index = 0; index < RECORD_COUNT; index++) {
        record = (struct object_record *)((char *)table + index * RECORD_SIZE);
        if (record->file == 0) {
            continue;
        }
        current = index;
        object_slot_acquire(table, list, &current);
        record = (struct object_record *)((char *)table + index * RECORD_SIZE);
        block = (unsigned char *)record->block;
        if ((block[5] & BLOCK_FLAG) == 0) {
            continue;
        }
        result = object_list_bind(table, list, &current);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}
