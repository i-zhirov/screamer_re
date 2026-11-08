/* The rebuilt `resource_find` module.
 *
 * The original sits at linear 0x2f25f in STARTH.EXE and holds 69 instructions
 * and 199 bytes: the five register saves, the frame, the two argument slots,
 * the two local slots, the record walk, the key test at `+0xc`, the
 * `resource_free` call at `0x2f470`, the memmove call at `0xd2066`, the count
 * decrement, the status write at `+0xc`, and the epilogue.
 *
 * The function walks the 0x1c-byte records of `*resource` up to the count at
 * `+0x04`.  A key equal to the record field at `+0xc` frees the record with
 * `resource_free`.  A non-zero free result returns -1.  Otherwise the function
 * shifts the records after the match one slot to the left with `memmove`, one
 * record less at the count, and returns 0.  A missing match returns -1.  The
 * status at `+0xc` is 3 on a failure and 0 on a success.
 */
extern int resource_free(void **resource, int index);
extern void *memmove(void *destination, const void *source, unsigned int size);

int resource_find(void *resource, int key)
{
    int index;
    int result;
    char *records;

    result = -1;
    index = 0;
    while (index < *(int *)((char *)resource + 4)) {
        records = *(char **)resource;
        if (key == *(int *)(records + index * 0x1c + 0xc)) {
            if (resource_free((void **)resource, index) != 0) {
                result = -1;
                break;
            }
            records = *(char **)resource;
            memmove(records + index * 0x1c, records + (index + 1) * 0x1c,
                    (*(int *)((char *)resource + 4) - index) * 0x1c);
            result = 0;
            (*(int *)((char *)resource + 4))--;
            break;
        }
        index++;
    }
    if (result == -1) {
        *(int *)((char *)resource + 0xc) = 3;
    } else {
        *(int *)((char *)resource + 0xc) = 0;
    }
    return result;
}
