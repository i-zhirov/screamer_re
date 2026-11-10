/* The rebuilt `sound_queue_pop` module.
 *
 * The original sits at linear 0x24918 in STARTH.EXE and holds 36 instructions
 * and 98 bytes up to the `ret` at 0x24979.  Two padding bytes at 0x2497a
 * precede the next function at 0x2497c, so the store record keeps the
 * 100-byte extent and sets no separate function size.  The identity check uses
 * 36 instructions.
 *
 * The two direct call sites are in `sound_queue_update` at 0x24880, at 0x248e7
 * and 0x24906.  The update runs in the per-frame path of
 * `race_frame_update` at 0x1ff70, and the neighbour record
 * `sound_queue_promote` at 0x2497c holds the same queue, so the record takes
 * the section Program functions.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The status `DAT_00119070` holds the playing sound or -1.  The sound effect
 * queue starts at `DAT_0019080`.  Each entry holds the effect index at +0x00,
 * the variant mode at +0x04, the delay at +0x08, and one more field at +0x0c.
 * The first entry holds the terminator -1 at +0x00.
 *
 * The function clears the status to -1 and shifts every entry down by one
 * slot until it copies the terminator.  It copies only the three head fields,
 * so the field at +0x0c stays with the slot.  The shift removes the head entry
 * of the queue.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original holds the float load and store of the delay copy, so the module
 * links the shared fpu_stub.asm for the float-used marker.  The module uses no
 * assembly implementation.
 */
struct sound_queue_entry {
    int effect;
    int mode;
    float delay;
    int pad;
};

extern int DAT_00119070;
extern struct sound_queue_entry sound_queue[];

void sound_queue_pop(void)
{
    struct sound_queue_entry *entry;
    struct sound_queue_entry *next;

    DAT_00119070 = -1;
    entry = sound_queue;
    next = entry + 1;
    do {
        entry->effect = next->effect;
        entry->mode = next->mode;
        entry->delay = next->delay;
        entry++;
        next++;
    } while (entry->effect != -1);
}
