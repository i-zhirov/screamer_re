/* The rebuilt `memory_alloc` module.
 *
 * The original sits at linear 0x30050 in STARTH.EXE and holds 31 instructions
 * and 75 bytes: the register saves, the size round-up, the allocator call at
 * `0xd1f25`, the two out-of-memory handlers at `0x274ac` and `0x274a8`, and
 * the epilogue.
 *
 * The function allocates the size, and when the allocator fails it retries
 * through the two out-of-memory handlers.  The size 0 becomes 1.
 */
extern void *malloc(unsigned int size);
extern void *(*out_of_memory_1)(unsigned int size);
extern void *(*out_of_memory_2)(unsigned int size);

void *memory_alloc(unsigned int size)
{
    void *ptr;

    if (size == 0) {
        size = 1;
    }
    for (;;) {
        ptr = malloc(size);
        if (ptr) {
            return ptr;
        }
        if (out_of_memory_1) {
            if (out_of_memory_1(size)) {
                continue;
            }
        }
        if (out_of_memory_2) {
            out_of_memory_2(size);
            continue;
        }
        return 0;
    }
}
