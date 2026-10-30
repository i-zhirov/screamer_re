/* The rebuilt `resource_free` module.
 *
 * The original sits at linear 0x2f470 in STARTH.EXE and holds 47 instructions
 * and 126 bytes: the five register saves, the frame, the two argument slots,
 * the record address arithmetic, the flag test at `+0x18` bit 0, the two free
 * calls at `0x2f40b` and `0x2f205`, the return paths, and the epilogue.
 *
 * The function selects a 0x1c-byte record at `*resource + index * 0x1c` and
 * frees the address at `+0x10`.  A set flag bit 0 uses the DOS memory free at
 * `0x2f40b`; a clear bit uses `memory_free` at `0x2f205`.  A non-zero free
 * result returns -1, and a success returns 0.
 */
extern int memory_free(int handle, unsigned int address);
extern int dos_memory_free(int handle, unsigned int address);

int resource_free(void **resource, int index)
{
    char *record;
    unsigned int address;

    record = (char *)*resource + index * 0x1c;
    address = *(unsigned int *)(record + 0x10);
    if ((record[0x18] & 1) != 0) {
        if (dos_memory_free((int)resource, address) != 0) {
            return -1;
        }
    } else {
        if (memory_free((int)resource, address) != 0) {
            return -1;
        }
    }
    return 0;
}
