/* The rebuilt `stricmp` module.
 *
 * The original sits at linear 0x4d9ba in STARTH.EXE.  The identity window holds
 * 28 instructions and 57 bytes up to the ret at 0x4d9f2.  The next function
 * starts at 0x4d9f4 after the one padding byte at 0x4d9f3, so the store record
 * keeps the 58-byte extent and sets no separate function size.  The identity
 * check uses 28 instructions.
 *
 * The function takes two string pointers in eax and edx.  It loads one byte
 * from each string, turns a byte in the range A to Z into the lowercase form,
 * and compares the two bytes.  Equal bytes advance both pointers, and the loop
 * continues.  A different byte or a zero byte ends the loop.  The function
 * returns the difference of the two final bytes.
 *
 * Seven direct call sites pass two string pointers and test the result for
 * zero: object_lookup at 0x110d0 at 0x11123, object_chunk_find at 0x11410 at
 * 0x11470, list_file_load at 0x15460 at 0x1550c, 0x1553b, and 0x15553,
 * resource_free_name at 0x2f326 at 0x2f37e, and member_open at 0x4c128 at
 * 0x4c1bf.  The case folding and the difference result identify the function
 * as the Watcom string compare `stricmp`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original has no EBP frame and no local stack slot, so the module builds
 * without `-of+`.  The function holds no floating-point instruction, so the
 * module needs no FPU stub.  The module uses no assembly implementation.
 */
int stricmp(const char *first, const char *second)
{
    unsigned char left;
    unsigned char right;

    for (;;) {
        left = *first;
        right = *second;
        if (left >= 'A' && left <= 'Z') {
            left = left + 0x20;
        }
        if (right >= 'A' && right <= 'Z') {
            right = right + 0x20;
        }
        if (left != right) {
            break;
        }
        if (right == 0) {
            break;
        }
        first++;
        second++;
    }
    return left - right;
}
