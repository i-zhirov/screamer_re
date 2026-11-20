/* The rebuilt `func_2f554` module.
 *
 * The original sits at linear 0x2f554 in STARTH.EXE and holds 37 instructions
 * and 90 bytes up to the `ret` at 0x2f5ad.  The next entry starts at 0x2f5ae,
 * so the store record keeps the 90-byte extent and sets no separate function
 * size.  The identity check uses 37 instructions.
 *
 * No direct caller, no data reference, and no LE relocation target reaches
 * 0x2f554.  Ghidra reports no reference and no function at the address, so the
 * function stands alone.  The sibling resource functions show the layout: the
 * resource holds the record array at `+0x00`, the count at `+0x04`, the limit
 * at `+0x08`, and the status at `+0x0c`; each 0x1c-byte record holds the
 * allocation result at `+0x0c`, the address cell at `+0x10`, the requested
 * size at `+0x14`, and the free flag at `+0x18`.
 *
 * The function takes the resource in eax.  It walks the 0x1c-byte records up
 * to the count at `+0x04` and sums the requested size at `+0x14` plus the
 * 0x10-byte allocation header.  It returns the total.  The record layout and
 * the sibling `resource_free` family at 0x2f470 support the section
 * The resource layer.  The original has no caller and no data reference, so
 * the record keeps the weak name `func_2f554`.
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

int func_2f554(struct resource *resource)
{
    int index;
    int total;

    total = 0;
    for (index = 0; index < resource->count; index++) {
        total += resource->records[index].requested + 0x10;
    }
    return total;
}
