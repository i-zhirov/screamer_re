/* The rebuilt `memory_block_largest` module.
 *
 * The original sits at linear 0x2f709 in STARTH.EXE and holds 34 instructions
 * and 79 bytes up to the `ret` at 0x2f757.  The next entry `heap_memory_available`
 * starts at 0x2f758, so the store record keeps the 79-byte extent and sets no
 * separate function size.  The identity check uses 34 instructions.
 *
 * The two direct call sites at 0x22c3d and 0x22c96 sit in
 * `replay_frame_buffer_init` at 0x22c30 and `replay_buffer_init` at 0x22c88.
 * Each caller passes the memory context `DAT_000062ac` and divides the result
 * by the size of one replay record.  Both callers already name this function
 * `memory_block_largest`, so the record keeps that name with the confidence
 * inferred.
 *
 * The function passes the context to the DOS block probe at 0x2f66c and to the
 * heap block probe at 0x2f5ae and returns the larger of the two results.  The
 * compare is signed, so a negative result loses against a zero result.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module needs no FPU stub,
 * and the module uses no assembly implementation.
 */
extern int dos_block_largest(void *pool);
extern int heap_block_largest(void *pool);

int memory_block_largest(void *pool)
{
    int dos;
    int heap;
    int largest;

    dos = dos_block_largest(pool);
    heap = heap_block_largest(pool);
    if (dos < heap) {
        largest = heap;
    } else {
        largest = dos;
    }
    return largest;
}
