/* The rebuilt `cache_resource` module.
 *
 * The original sits at linear 0x2f062 in STARTH.EXE.  It holds 154 bytes and 53
 * instructions up to the `ret 4` at 0x2f0f9.  The next function starts at
 * 0x2f0fc, so the store record keeps the 154-byte extent and sets no separate
 * function size.  The identity check uses 53 instructions.
 *
 * The function takes five arguments.  The resource structure arrives in `eax`,
 * the record in `edx`, the allocation result in `ebx`, the name in `ecx`, and
 * the size on the stack.  The resource holds the record array at `+0x00`, the
 * count at `+0x04`, the limit at `+0x08`, the status at `+0x0c`, and the name
 * buffer at `+0x10`.  The record holds the name at `+0x00`, the allocation
 * result at `+0x0c`, the address cell at `+0x10`, the requested size at
 * `+0x14`, and the free flag at `+0x18`.
 *
 * A null record array or a null allocation result is the failure path.  The
 * function sets the status to 1, copies the name to the resource name buffer
 * through `sprintf`, and returns 0.  Otherwise it caches the record: it stores
 * the allocation result at `record + 0x0c`, the size at `record + 0x14`, and
 * the name at `record + 0x00`.  Then it clears the status, increments the
 * count, and returns the allocation result.
 *
 * `resource_get` at 0x2eed0 and `resource_read` at 0x2efc2 call this function
 * as the cache step.  The sibling allocators `dos_memory_alloc` at 0x2f0fc and
 * `memory_alloc` at 0x2f172 supply the allocation result.  The callers, the
 * record layout, and the data flow support the name `cache_resource`.  The
 * original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction.  It calls `sprintf` twice, and
 * the harness supplies that function, so the module needs no assembly.
 */
extern int sprintf(char *buffer, const char *format, ...);

void *cache_resource(void *resource, char *record, void *value, char *name,
                     int size)
{
    void *result;

    if (*(int *)resource == 0 || value == 0) {
        *(int *)((char *)resource + 0xc) = 1;
        sprintf((char *)resource + 0x10, "%.11s", name);
        result = 0;
    } else {
        *(int *)(record + 0xc) = (int)value;
        *(int *)(record + 0x14) = size;
        sprintf(record, "%.11s", name);
        *(int *)((char *)resource + 0xc) = 0;
        (*(int *)((char *)resource + 4))++;
        result = value;
    }
    return result;
}
