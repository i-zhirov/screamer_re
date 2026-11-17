/* The rebuilt `sound_queue_promote` module.
 *
 * The original sits at linear 0x2497c in STARTH.EXE and holds 61 instructions
 * and 170 bytes up to the `ret` at 0x24a25.  The next record starts at 0x24a28,
 * so the store record keeps the 172-byte extent and sets no separate function
 * size.  The identity check uses 61 instructions.
 *
 * The only direct caller at 0x248bc is the sound queue update at 0x24880.  The
 * update accumulates the frame delta DAT_00118e08 into the timer DAT_0011907c
 * and calls this function before it compares the timer with the head delay
 * DAT_00119088.  The only caller of the update at 0x20024 is race_frame_update
 * at 0x1ff70, so the record takes the section Program functions.
 *
 * The sound effect queue starts at DAT_0019080.  Each entry holds the effect
 * index at +0x00, the variant mode at +0x04, the delay at +0x08, and one more
 * field at +0x0c.  The first entry holds the terminator -1 at +0x00.
 *
 * The function finds the entry with the smallest delay that stays below the
 * 0.01 threshold.  It starts the search at the head entry, and when no delay is
 * below the threshold the head stays the selected entry.  It then swaps the
 * selected entry with the head entry.  The three head fields and the three
 * selected fields change, and the field at +0x0c stays.
 *
 * The delay select, the swap with the head, and the caller support the name
 * sound_queue_promote.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no conversion to an integer and calls no math routine, so the
 * shared fpu_stub.asm supplies the FPU run-time symbols and the float-used
 * marker.
 */
struct sound_queue_entry {
    int effect;
    int mode;
    float delay;
    int pad;
};

extern struct sound_queue_entry sound_queue[];

void sound_queue_promote(void)
{
    struct sound_queue_entry *best;
    struct sound_queue_entry *cursor;
    struct sound_queue_entry *first;
    int effect;
    int mode;
    float delay;
    float min;

    best = sound_queue;
    cursor = best;
    first = cursor;
    min = 0.01f;
    while (cursor->effect != -1) {
        if (cursor->delay < min) {
            min = cursor->delay;
            best = cursor;
        }
        cursor++;
    }

    effect = first->effect;
    mode = first->mode;
    delay = first->delay;

    first->effect = best->effect;
    first->mode = best->mode;
    first->delay = best->delay;

    best->effect = effect;
    best->mode = mode;
    best->delay = delay;
}
