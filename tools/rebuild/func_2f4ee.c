/* The rebuilt `func_2f4ee` module.
 *
 * The original sits at linear 0x2f4ee in STARTH.EXE and holds 40 instructions
 * and 102 bytes up to the `ret` at 0x2f553.  The next function starts at
 * 0x2f554, so the store record keeps the 102-byte extent and sets no separate
 * function size.  The identity check uses 40 instructions.
 *
 * No direct caller, no data reference, and no LE relocation target reaches
 * 0x2f4ee.  Ghidra reports no reference and no function at the address, so the
 * function stands alone.  The sibling functions show the record layout: the
 * resource holds the record array at `+0x00`, the count at `+0x04`, the limit
 * at `+0x08`, and the status at `+0x0c`; each 0x1c-byte record holds the
 * allocation result at `+0x0c`, the address cell at `+0x10`, the requested
 * size at `+0x14`, and the free flag at `+0x18`.
 *
 * The function does three steps:
 *
 *     1  it walks the 0x1c-byte records up to the count at `+0x04` and
 *        releases the allocation result at `+0x0c` through the run-time
 *        `free` at 0xd2008
 *     2  it zeroes the record array, `count * 0x1c` bytes, through `memset`
 *        at 0xd1ec0
 *     3  it sets the count at `+0x04` to 0
 *
 * The record array, the limit, and the status stay in place.  The record
 * layout, the two run-time calls, and the sibling `resource_free` family at
 * 0x2f470 support the section The resource layer.  The original has no
 * caller and no data reference, so the record keeps the weak name
 * `func_2f4ee`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction, so the module needs no FPU
 * stub, and the module uses no assembly.
 */
struct resource_record {
    char name[0xc];
    void *allocation;
    unsigned int address;
    int requested;
    int flag;
};

struct resource {
    struct resource_record *records;
    int count;
    int limit;
    int status;
};

extern void free(void *block);
extern void *memset(void *block, int value, unsigned int size);

void func_2f4ee(struct resource *resource)
{
    int index;

    for (index = 0; index < resource->count; index++) {
        free(resource->records[index].allocation);
    }
    memset(resource->records, 0, resource->count * 0x1c);
    resource->count = 0;
}
