/* The rebuilt `sound_enqueue` module.
 *
 * The original sits at linear 0x247fc in STARTH.EXE and holds 45 instructions
 * and 132 bytes up to the `ret 4` at 0x2487d.  The next record starts at
 * 0x24880, so the store record keeps the 132-byte extent and sets no separate
 * function size.  The identity check uses 45 instructions.
 *
 * The callers push the effect index in `eax`, the variant mode in `edx`, and
 * the delay as a float on the stack, because the Watcom compiler passes a
 * floating-point argument on the stack.  The callee cleans the delay with
 * `ret 4`.
 *
 * The two flags DAT_001d2c8 and DAT_00008058 gate the enqueue.  The sound
 * effect queue starts at DAT_0019080.  Each entry holds the effect index at
 * +0x00, the variant mode at +0x04, the delay at +0x08, and one more field at
 * +0x0c.  The first entry with the effect index -1 ends the queue.
 *
 * The function scans the queue for the same effect index.  A found entry
 * returns at once, so the queue holds at most one entry per effect.  Otherwise
 * the scan stops at the terminator, writes the effect, the mode, and the
 * delay, and writes the terminator -1 to the next entry.  The field at +0x0c
 * stays.
 *
 * The caller list, the queue, and the enqueue write support the name
 * sound_enqueue.  The original has no symbol, so the record keeps the name
 * with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function only moves the delay bits and holds no floating-point instruction,
 * so it needs no FPU stub and no assembly.
 */
struct sound_queue_entry {
    int effect;
    int mode;
    float delay;
    int pad;
};

extern int DAT_001d2c8;
extern int DAT_00008058;
extern struct sound_queue_entry sound_queue[];

void sound_enqueue(int effect, int mode, float delay)
{
    struct sound_queue_entry *entry;
    int insert;

    if (DAT_001d2c8 == 0) {
        return;
    }
    if (DAT_00008058 == 0) {
        return;
    }
    entry = sound_queue;
    insert = 1;
    while (entry->effect != -1) {
        if (entry->effect == effect) {
            insert = 0;
            break;
        }
        entry++;
    }
    if (insert) {
        entry->effect = effect;
        entry->mode = mode;
        entry->delay = delay;
        entry[1].effect = -1;
    }
}
