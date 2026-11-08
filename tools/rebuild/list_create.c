/* The rebuilt `list_create` module.
 *
 * The original sits at linear 0x2ecf8 in STARTH.EXE and holds 46 instructions
 * and 127 bytes: the five register saves, the frame, the two argument slots,
 * the `pool_init` call at `0x2f904`, the `malloc` call at `0xd1f25`, the
 * pointer check, the two store paths, the `memset` call at `0xd1ec0`, and the
 * epilogue.
 *
 * The function allocates `count` records of 0x1c bytes through `malloc` and
 * stores the pointer at `+0`, the cursor 0 at `+4`, and the count at `+8`.  A
 * failed allocation stores the pointer 0 and the count 0.  A success zeroes the
 * block through `memset`.  The function returns the list pointer.
 */
struct list_record {
    void *pointer;
    int cursor;
    int count;
};

extern void pool_init(void *list);
extern void *malloc(unsigned int size);
extern void *memset(void *block, int value, unsigned int size);

void *list_create(struct list_record *list, int count)
{
    pool_init(list);
    list->pointer = malloc(count * 0x1c);
    if (list->pointer == 0) {
        list->cursor = 0;
        list->count = list->cursor;
    } else {
        list->count = count;
        list->cursor = 0;
        memset(list->pointer, 0, count * 0x1c);
    }
    return list;
}
