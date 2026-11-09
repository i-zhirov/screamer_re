/* The rebuilt `replay_playback_start` module.
 *
 * The original sits at linear 0x14d2c in STARTH.EXE.  The first `ret` sits at
 * 0x14ddf, so the body up to that early return holds 58 instructions and 180
 * bytes.  The full body continues through the shared epilogue at 0x14e1d to
 * the next function at 0x14e20, so the store record keeps the 244-byte extent
 * and sets no separate function size.  The identity check uses 58
 * instructions.
 *
 * The only direct caller at 0x12ed4 is `game_mode_4_start` at 0x12e18.  That
 * function runs after the ghost load at 0x1ba4c.  It loads the ghost index
 * `DAT_0018ed4`, flips its low bit with `xor dl, 1`, and passes the playback
 * block `0x18ed8` in `eax` and the flipped index in `edx`.  So a call selects
 * the other of the two ghost records.  The ghost records sit at 0x18e8c with
 * a stride of 0x24.
 *
 * The first argument points to the playback block.  The block holds the entry
 * index at `+0x00`, the ghost record pointer at `+0x08`, and the active flag
 * at `+0x0c`.  The ghost record holds the index at `+0x04`, the active flag at
 * `+0x0c`, and the model index at `+0x18`.
 *
 * The function selects the ghost record `0x18e8c + index * 0x24` and stores it
 * in the playback block at `+0x08`.  When the record active flag is zero, the
 * function goes at once to the playback start.  Otherwise, when the playback
 * block flag is not zero, the function releases the current object slot
 * through 0x11714 with the table `0x6530` and the array `0x6350`.  A non-zero
 * result returns at once.  The function then releases the shared entry through
 * 0x11230; a result above zero returns at once.
 *
 * The function then loads the object named by the model table entry
 * `0xd25c + record model index * 0x40` through 0x11060.  A non-zero load
 * result returns at once.  On success the function takes the free slot
 * through 0x11660, sets the playback block flag to one, and starts the
 * playback.
 *
 * Every path ends with `path_playback_update` at 0x23038 with the playback
 * block and the flag 1.  The function then reloads the ghost record from the
 * playback block, clears the record index at `+0x04`, and returns zero.
 *
 * The caller, the ghost records, and the object table support the name
 * `replay_playback_start`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.
 */
struct ghost_record {
    int count;              /* +0x00 */
    int index;              /* +0x04 */
    int reserved_08;        /* +0x08 */
    int active;             /* +0x0c */
    int reserved_10;        /* +0x10 */
    int reserved_14;        /* +0x14 */
    int model;              /* +0x18 */
};

struct playback_block {
    int entry;              /* +0x00 */
    int reserved_04;        /* +0x04 */
    struct ghost_record *state; /* +0x08 */
    int flag;               /* +0x0c */
};

extern char DAT_0018e8c[];

extern int object_release_entry(void *table, void *array, int *entry);
extern int func_11230(void *table, int *index);
extern int object_load_named(void *table, char *name, int *slot);
extern int object_slot_acquire(void *table, void *array, int *entry);
extern void path_playback_update(char *out, int flag);

int replay_playback_start(struct playback_block *self, int index)
{
    struct ghost_record *record;
    int result;

    record = (struct ghost_record *)(DAT_0018e8c + index * 0x24);
    self->state = record;

    if (record->active == 0) {
        goto start;
    }
    if (self->flag != 0) {
        result = object_release_entry((void *)0x6530, (void *)0x6350,
                                      (int *)self);
        if (result != 0) {
            return result;
        }
        result = func_11230((void *)0x6530, (int *)self);
        if (result > 0) {
            return result;
        }
    }

    result = object_load_named((void *)0x6530,
                               (char *)(0xd25c + (record->model << 6)),
                               (int *)self);
    if (result != 0) {
        return result;
    }
    object_slot_acquire((void *)0x6530, (void *)0x6350, (int *)self);
    self->flag = 1;

start:
    path_playback_update((char *)self, 1);
    record = self->state;
    record->index = 0;
    return 0;
}
