/* The rebuilt `replay_buffer_init` module.
 *
 * The original sits at linear 0x22c88 in STARTH.EXE and holds 58 instructions
 * and 188 bytes up to the `ret` at 0x22d43.  The next function starts at
 * 0x22d44, so the store record keeps the 188-byte extent and sets no separate
 * function size.  The identity check uses 58 instructions.
 *
 * The only direct caller at 0x12e74 is `game_mode_4_start` at 0x12e18.  That
 * function loads the `ATTACK_RES` resource with the size 0x1f400, calls this
 * function, and then frees the resource name through 0x2f326.  The replay
 * loader at 0x1ba4c and the replay writer at 0x1b974 read and write the two
 * 0x24-byte ghost records this function prepares, so the module takes the
 * section The replay records.
 *
 * The function asks `0x2f709` for the largest single memory block.  That
 * helper returns the larger of the DOS block at 0x2f66c and the heap block at
 * 0x2f5ae.  The function divides the block by 0x28, the size of one keyframe,
 * and stores the count at `DAT_0018e8c`.  A count below 0xc8 sets the four
 * cells `DAT_0018eac`, `DAT_0018ebc`, `DAT_0018e98`, and `DAT_0018ed0` to zero
 * and returns 0.
 *
 * A count at or above 0xc8 loads the `ATTACK` resource through 0x2eed0 with the
 * size of the largest block and stores the pointer at `DAT_0018eac`.  A zero
 * pointer takes the clear path.  Otherwise the function halves the count and
 * gives one half to each of the two ghost records: the count and the active
 * flag of both records, the index `DAT_0018ed4`, and the cell `DAT_0018ee4`
 * take their values, and the second keyframe pointer at `DAT_0018ed0` sits one
 * half times 0x28 past the resource pointer.  The function returns 1.
 *
 * The caller, the `ATTACK` resource, and the two ghost records support the name
 * `replay_buffer_init`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.
 */
extern void *DAT_000062ac;
extern char DAT_00100db3[];
extern int DAT_0018e8c;
extern int DAT_0018eac;
extern int DAT_0018eb0;
extern int DAT_0018ebc;
extern int DAT_0018e98;
extern int DAT_0018ed0;
extern int DAT_0018ed4;
extern int DAT_0018ee4;

extern unsigned int memory_block_largest(void *pool);
extern void *resource_get(void *resource, char *name, unsigned int size);

int replay_buffer_init(void)
{
    unsigned int largest;
    int count;
    int half;
    void *buffer;

    largest = memory_block_largest(DAT_000062ac);
    count = largest / 0x28;
    DAT_0018e8c = count;
    if (count < 0xc8) {
        DAT_0018eac = 0;
        DAT_0018ebc = 0;
        DAT_0018e98 = 0;
        DAT_0018ed0 = 0;
        return 0;
    }
    buffer = resource_get(DAT_000062ac, DAT_00100db3, largest);
    DAT_0018eac = (int)buffer;
    if (buffer == 0) {
        DAT_0018eac = 0;
        DAT_0018ebc = 0;
        DAT_0018e98 = 0;
        DAT_0018ed0 = 0;
        return 0;
    }
    DAT_0018ed4 = 0;
    half = DAT_0018e8c / 2;
    DAT_0018e8c = half;
    DAT_0018eb0 = half;
    DAT_0018ee4 = 0;
    DAT_0018ebc = 1;
    DAT_0018ed0 = (int)buffer + half * 0x28;
    DAT_0018e98 = 1;
    return 1;
}
