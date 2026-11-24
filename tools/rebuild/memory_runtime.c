/* The interim heap and memory run-time cluster.
 *
 * The whole-image link reports these symbols as undefined because the shipped
 * image statically linked the Watcom nheap and its stack and probing helpers,
 * and the reconstruction has not yet recovered that code.  This module defines
 * the cluster with the calling convention of the original functions:
 *
 *   heap_alloc(size, segment, block)   -> a block from the free list or 0
 *   heap_free(block, segment, next)    -> release a block
 *   heap_grow(size)                    -> extend the heap
 *   heap_expand(size)                  -> extend the heap by a new segment
 *   probe_free(pool), probe_size(pool) -> the memory probe of `memory_available`
 *   dos_memory_free(handle, address)   -> the DOS memory release
 *   nfree(block)                       -> the run-time free entry
 *
 * The functions are a compatibility layer, not yet a byte-faithful recovery of
 * the Watcom allocator.  The allocator is a bump arena with one free-list node
 * per request, so the symbols resolve and the rebuilt image can allocate.  A
 * release is a no-op and an expand reports failure.  The module has no
 * function record and no behavior check, so it does not enter the verified
 * count.
 */
#define ARENA_SIZE (1024u * 1024u)

struct heap_block {
    int field_00;
    int field_04;
    struct heap_block *next;
    int field_0c;
    int field_10;
    unsigned int size;
};

struct heap_block *heap_free_list;
struct heap_block *heap_cursor;
unsigned int heap_largest_free;
unsigned char heap_flag;

static struct heap_block free_sentinel;

void *free_list = &free_sentinel;
void *free_list_end = &free_sentinel;
unsigned int largest_free;

void *(*out_of_memory_1)(unsigned int size);
void *(*out_of_memory_2)(unsigned int size);

int DAT_00004290;
int DAT_000061c0;

static unsigned char heap_arena[ARENA_SIZE];
static unsigned int heap_used;

void *heap_alloc(unsigned int size, unsigned int segment,
                 struct heap_block *block)
{
    (void)segment;
    if (block == 0 || block->size < size) {
        return 0;
    }
    block->size = 0;
    return (char *)block + sizeof(struct heap_block);
}

int heap_grow(unsigned int size)
{
    unsigned int need;
    struct heap_block *block;

    need = size + sizeof(struct heap_block) + 16;
    if (heap_used + need > ARENA_SIZE) {
        return 0;
    }
    block = (struct heap_block *)(heap_arena + heap_used);
    heap_used += need;
    block->next = heap_free_list;
    block->size = size;
    heap_free_list = block;
    if (size > heap_largest_free) {
        heap_largest_free = size;
    }
    return 1;
}

int heap_expand(unsigned int size)
{
    (void)size;
    return 0;
}

void heap_free(void *block, unsigned int segment, void *next)
{
    (void)block;
    (void)segment;
    (void)next;
}

unsigned int probe_free(void *pool)
{
    (void)pool;
    return ARENA_SIZE - heap_used;
}

unsigned int probe_size(void *pool)
{
    (void)pool;
    return 0;
}

int dos_memory_free(int handle, unsigned int address)
{
    (void)handle;
    (void)address;
    return 0;
}

void nfree(void *block)
{
    (void)block;
}
