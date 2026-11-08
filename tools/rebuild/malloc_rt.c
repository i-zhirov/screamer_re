/* The rebuilt `malloc` module.
 *
 * The original starts at linear 0xd1f25 in STARTH.EXE and holds 84
 * instructions and 227 bytes: the nine register saves, the frame, the four-byte
 * local area, the two size checks, the round-up, the free-list walk, the three
 * helper calls at `0xe09dc`, `0xe0f73`, and `0xe112e`, the largest-free update,
 * and the epilogue.
 *
 * The function rejects the size 0 and every size above 0xffffffd4.  It rounds
 * the size up to a multiple of 4 and raises a small size to 12.  It walks the
 * free list at `0x31d0` and asks `heap_alloc` for a block.  When no block fits,
 * it calls `heap_grow` once and walks again with the flag set.  When
 * `heap_expand` succeeds, it clears the flag and walks again.  It returns the
 * block or 0.  A success clears the heap flag at `0x27de5`.
 *
 * A requested size at or below the cached largest free size restarts the walk
 * at the head and clears the cache.  A larger size resumes at the saved cursor
 * `0x31d4`, because no block in the scanned prefix can fit.
 *
 * The original passes the data segment in the second argument of the first
 * helper, because the helper uses the segment to reach the free list.  The flat
 * target has no segment, so the rebuild passes 0 and the helper uses the near
 * pointer.  The module is named `malloc_rt`, so it does not collide with the C
 * run-time `malloc`.
 */
struct free_block {
    int field_00;
    int field_04;
    struct free_block *next;
    int field_0c;
    int field_10;
    unsigned int size;
};

extern struct free_block *heap_free_list;
extern struct free_block *heap_cursor;
extern unsigned int heap_largest_free;
extern unsigned char heap_flag;

extern void *heap_alloc(unsigned int size, unsigned int segment,
                        struct free_block *block);
extern int heap_grow(unsigned int size);
extern int heap_expand(unsigned int size);

void *malloc_rt(unsigned int size)
{
    struct free_block *block;
    unsigned int rounded;
    int flag;
    void *result;

    if (size == 0 || size > 0xffffffd4) {
        return 0;
    }
    rounded = (size + 3) & 0xfffffffc;
    if (rounded < 0xc) {
        rounded = 0xc;
    }
    flag = 0;
    for (;;) {
        if (rounded <= heap_largest_free) {
            heap_largest_free = 0;
            block = heap_free_list;
        } else {
            block = heap_cursor;
            if (block == 0) {
                heap_largest_free = 0;
                block = heap_free_list;
            }
        }
        while (block != 0) {
            heap_cursor = block;
            result = heap_alloc(size, 0, block);
            if (result != 0) {
                heap_flag = 0;
                return result;
            }
            if (block->size > heap_largest_free) {
                heap_largest_free = block->size;
            }
            block = block->next;
        }
        if (flag == 0) {
            if (heap_grow(size)) {
                flag = 1;
                continue;
            }
        }
        if (heap_expand(size)) {
            flag = 0;
            continue;
        }
        return 0;
    }
}
