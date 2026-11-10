/* The rebuilt function at linear 0x11714 in STARTH.EXE.
 *
 * The original body holds 49 instructions and 139 bytes up to the `ret` at
 * 0x1179e.  The next function starts at 0x117a0 after the one padding byte at
 * 0x1179f, so the store record keeps the 140-byte extent and sets no separate
 * function size.  The identity check uses 49 instructions.
 *
 * The function is one member of the object list helper group.  It takes the
 * object table at 0x6530 in eax, the list at 0x6350 in edx, and the address of
 * the entry index in ebx.  The table holds 0x78 records of 0x1c bytes.  A
 * record holds the name at +0x00, the block object at +0x0c, the chunk table at
 * +0x10, the file record at +0x14, and one slot word at +0x18.  A slot word of
 * -1 marks a free slot.
 *
 * The first direct caller at 0x11641 is the release-all pass at 0x115f8.  It
 * walks the table and releases each active record.  Race end, race cleanup, and
 * the object update call the pass.  The second direct caller at 0x14d7d is
 * replay_playback_start at 0x14d2c, which releases the active ghost object
 * before it loads the other ghost record.  The callers pass the same table and
 * list as object_list_link at 0x11568, so the record takes the section Resource
 * loader.  The original has no symbol and no name reaches the function, so the
 * record keeps the weak fallback func_11714 with the confidence inferred.
 *
 * The function reads the entry index from the index address.  The entry with
 * slot -1 returns 0xc and changes nothing.  Otherwise the function writes -1
 * to list[slot] when list[slot + 1] is non-zero and writes 0 when it is zero.
 * The function then clears the entry slot to -1 and returns 0.  The next value
 * marks the linked slot, so the write either drops the link or clears the list
 * head.  The acquire at 0x11660 takes the slot with the same list.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.
 */
#define RECORD_SIZE 0x1c
#define SLOT_FREE (-1)
#define RESULT_FREE 0xc

struct object_record {
    char name[0x0c];            /* +0x00 */
    void *block;                /* +0x0c */
    void *chunks;               /* +0x10 */
    void *file;                 /* +0x14 */
    int slot;                   /* +0x18 */
};

int func_11714(void *table, int *list, int *index_address)
{
    struct object_record *record;
    int slot;

    record = (struct object_record *)(
        (char *)table + *index_address * RECORD_SIZE
    );
    if (record->slot == SLOT_FREE) {
        return RESULT_FREE;
    }
    slot = record->slot;
    if (list[slot + 1] != 0) {
        list[slot] = SLOT_FREE;
    } else {
        list[slot] = 0;
    }
    record->slot = SLOT_FREE;
    return 0;
}
