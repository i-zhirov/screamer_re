/* The rebuilt `resource_read` module.
 *
 * The original sits at linear 0x2efc2 in STARTH.EXE and holds 56 instructions
 * and 160 bytes: the four register saves, the frame, the three argument slots,
 * the full-table test on the count at `+0x04` and the limit at `+0x08`, the
 * status write at `+0x0c`, the `sprintf` call at `0x4d32c`, the record address
 * arithmetic, the `memory_alloc_small` call at `0x2f172`, the flag clear at
 * `+0x18`, the cache call at `0x2f062`, and the epilogue.
 *
 * The function selects the next 0x1c-byte record at
 * `*resource + resource->count * 0x1c`.  A count equal to the limit sets the
 * status at `+0x0c` to 2, copies the name to `resource + 0x10`, and returns 0.
 * Otherwise it allocates the requested size with the record address cell at
 * `+0x10`, clears the free flag at `+0x18`, and caches the record with the
 * name and the size through `0x2f062`.
 */
extern int sprintf(char *buffer, const char *format, ...);
extern void *memory_alloc_small(void *resource, int size,
                                unsigned int *address);
extern void *cache_resource(void *resource, char *record, void *value,
                            char *name, int size);

void *resource_read(void *resource, char *name, int size)
{
    char *record;
    void *allocated;

    if (*(int *)((char *)resource + 4) == *(int *)((char *)resource + 8)) {
        *(int *)((char *)resource + 0xc) = 2;
        sprintf((char *)resource + 0x10, "%.11s", name);
        return 0;
    }
    record = *(char **)resource + *(int *)((char *)resource + 4) * 0x1c;
    allocated = memory_alloc_small(resource, size,
                                   (unsigned int *)(record + 0x10));
    *(int *)(record + 0x18) = 0;
    return cache_resource(resource, record, allocated, name, size);
}
