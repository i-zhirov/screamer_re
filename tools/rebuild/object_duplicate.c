/* The rebuilt `object_duplicate` module.
 *
 * The original sits at linear 0x10ed4 in STARTH.EXE.  The first `ret` sits at
 * 0x10f87, so the body up to that early return holds 62 instructions and 180
 * bytes.  The full body continues through the shared epilogue at 0x11056 to
 * the next function at 0x11060, so the store record keeps the 396-byte extent
 * and sets no separate function size.  The identity check uses 62
 * instructions.
 *
 * The two direct callers support the name.  `menu_objects_init` at 0x2aab8
 * calls the function at 0x2ab43 for 79 slots and uses the object in slot 0 as
 * the source.  The object loader at 0x11060 calls it at 0x11092 when the
 * requested object already exists.  The function therefore makes one more
 * instance of an existing object in a free slot.
 *
 * The first argument is the 0x78-record object table at 0x6530.  The second
 * argument points to the output slot index.  The third argument points to the
 * source slot index.  A record holds the name at `+0x00`, the block object at
 * `+0x0c`, the chunk table at `+0x10`, the shared file record at `+0x14`, and
 * one reserved word at `+0x18`.  A zero file record marks a free slot.
 *
 * The function clears the output index and scans the table from index 0.  The
 * first record with a zero file record is the new slot.  A full table returns
 * 8.
 *
 * The function takes the source record, reads the object count at `+0x14` of
 * its file record, and requests the `3D_BLKOBJ` resource at object-3 offset
 * 0x3ca with the size `count * 0x20 + 0x60` through `resource_get` at 0x2eed0.
 * It stores the result in the new record at `+0x0c`.  A zero result returns 6.
 *
 * On success the function initializes the new block from the source file index
 * through `object_init_from_file` at 0x4cc10, copies the source name to the new
 * record, stores the chunk table `block + 0x40` at `+0x10`, shares the file
 * record at `+0x14`, and returns 0.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.
 */
struct object_record {
    char name[0x0c];            /* +0x00 */
    void *block;                /* +0x0c */
    void *chunks;               /* +0x10 */
    void *file;                 /* +0x14 */
    int reserved;               /* +0x18 */
};

extern void *DAT_0000801c;

extern void *resource_get(void *resource, char *name, int size);
extern void object_init_from_file(void *object, int index);

static void copy_name(char *destination, char *source)
{
    char value;

    for (;;) {
        value = *source;
        *destination = value;
        if (value == 0) {
            return;
        }
        source++;
        destination++;
    }
}

int object_duplicate(struct object_record *objects, int *out_index,
                     int *source_index)
{
    struct object_record *source;
    struct object_record *target;
    void *block;
    int count;
    int size;

    *out_index = 0;
    for (;;) {
        if (objects[*out_index].file == 0) {
            break;
        }
        (*out_index)++;
        if (*out_index == 0x78) {
            return 8;
        }
    }

    source = &objects[*source_index];
    count = *(int *)((char *)source->file + 0x14);
    size = count * 0x20 + 0x60;
    block = resource_get(DAT_0000801c, (char *)0x3ca, size);

    target = &objects[*out_index];
    target->block = block;
    if (block == 0) {
        return 6;
    }

    object_init_from_file(block, *(int *)source->block);
    copy_name((char *)target, (char *)source);
    target->chunks = (char *)block + 0x40;
    target->file = source->file;
    return 0;
}
