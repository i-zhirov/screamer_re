/* The rebuilt `resource_get` module.
 *
 * The original sits at linear 0x2eed0 in STARTH.EXE and holds 78 instructions
 * and 242 bytes: the four register saves, the frame, the three argument slots,
 * the full-table test on the count at `+0x04` and the limit at `+0x08`, the
 * status write at `+0x0c`, the `sprintf` call at `0x4d32c`, the record address
 * arithmetic, the size limit test against `0x96000`, the DOS memory allocation
 * at `0x2f0fc`, the fallback allocation at `0x2f172`, the flag writes at
 * `+0x18`, the cache call at `0x2f062`, and the epilogue.
 *
 * The function selects the next 0x1c-byte record at
 * `*resource + resource->count * 0x1c`.  A count equal to the limit sets the
 * status at `+0x0c` to 2, copies the name to `resource + 0x10`, and returns 0.
 * A size below `0x96000` first tries the DOS memory allocation with the record
 * address cell at `+0x10`.  A non-zero result sets the free flag at `+0x18` to
 * 1.  A zero result falls back to the small allocation and clears the flag.
 * A size of `0x96000` or more uses the small allocation and clears the flag.
 * The function then caches the record with the name and the size.
 */
extern int sprintf(char *buffer, const char *format, ...);
extern int dos_memory_alloc(void *handle, unsigned int size,
                            unsigned int *selector);
extern int memory_alloc_small(void *handle, int size, unsigned int *address);
extern void *cache_resource(void *handle, char *record, void *value,
                            char *name, int size);

void *resource_get(void *resource, char *name, int size)
{
    char *record;
    void *allocated;

    if (*(int *)((char *)resource + 4) == *(int *)((char *)resource + 8)) {
        *(int *)((char *)resource + 0xc) = 2;
        sprintf((char *)resource + 0x10, "%.11s", name);
        return 0;
    }
    record = *(char **)resource + *(int *)((char *)resource + 4) * 0x1c;
    if (size < 0x96000) {
        allocated = (void *)dos_memory_alloc(resource, size,
                                             (unsigned int *)(record + 0x10));
        if (allocated != 0) {
            *(int *)(record + 0x18) = 1;
        } else {
            allocated = (void *)memory_alloc_small(resource, size,
                                                   (unsigned int *)(record + 0x10));
            *(int *)(record + 0x18) = 0;
        }
    } else {
        allocated = (void *)memory_alloc_small(resource, size,
                                               (unsigned int *)(record + 0x10));
        *(int *)(record + 0x18) = 0;
    }
    return cache_resource(resource, record, allocated, name, size);
}
