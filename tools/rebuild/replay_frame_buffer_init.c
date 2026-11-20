/* The rebuilt `replay_frame_buffer_init` module.
 *
 * The original sits at linear 0x22c30 in STARTH.EXE and holds 30 instructions
 * and 87 bytes up to the `ret` at 0x22c86.  The next function starts at
 * 0x22c88, so the store record keeps the 88-byte extent and sets no separate
 * function size.  The identity check uses 30 instructions.
 *
 * The two direct call sites sit at 0x12cc8 and 0x12cef in `game_mode_1_start`
 * at 0x12c0c.  The caller runs this function only when a mode argument is not
 * zero, stores the result in the replay flag `DAT_0018e60`, and then loads the
 * replay record.  The sibling `replay_buffer_init` at 0x22c88 prepares the two
 * `0x24`-byte ATTACK keyframe records, and the replay reader at 0x1b840 and the
 * writer at 0x1b738 use the buffer and the count this function sets, so the
 * module takes the section The replay records.
 *
 * The function asks `0x2f709` for the largest single memory block.  That helper
 * returns the larger of the DOS block at 0x2f66c and the heap block at 0x2f5ae.
 * The function divides the block by 0x14, the size of one replay frame record,
 * and stores the count at `DAT_0018e68`.  A count below 0xc8 returns 0.
 *
 * A count at or above 0xc8 loads the `REPLAY` resource at object-3 offset
 * 0xdac through 0x2eed0 with the block size and stores the pointer at
 * `DAT_0018e5c`.  A zero pointer returns 0.  Otherwise the function returns 1,
 * so the caller sets the replay flag.
 *
 * The caller, the `REPLAY` resource, the 0x14-byte frame record, and the replay
 * reader and writer support the name `replay_frame_buffer_init`.  The original
 * has no symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.
 */
extern void *DAT_000062ac;
extern char DAT_00100dac[];
extern int DAT_0018e5c;
extern int DAT_0018e68;

extern unsigned int memory_block_largest(void *pool);
extern void *resource_get(void *resource, char *name, unsigned int size);

int replay_frame_buffer_init(void)
{
    unsigned int largest;
    int count;
    void *buffer;

    largest = memory_block_largest(DAT_000062ac);
    count = largest / 0x14;
    DAT_0018e68 = count;
    if (count < 0xc8) {
        return 0;
    }
    buffer = resource_get(DAT_000062ac, DAT_00100dac, largest);
    DAT_0018e5c = (int)buffer;
    if (buffer == 0) {
        return 0;
    }
    return 1;
}
