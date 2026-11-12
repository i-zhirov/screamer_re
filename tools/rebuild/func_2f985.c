/* The rebuilt `func_2f985` module.
 *
 * The original sits at linear 0x2f985 in STARTH.EXE and holds 21 instructions
 * and 38 bytes up to the `ret` at 0x2f9aa.  The next function starts at
 * 0x2f9b0 after the five padding bytes, so the store record keeps the 43-byte
 * extent and sets no separate function size.  The identity check uses 21
 * instructions.
 *
 * No direct call, no data reference, and no LE relocation target reaches
 * 0x2f985.  The function is a leaf getter.  It stores its argument, reloads
 * the pointer, reads the dword at `+0x3c`, stores the value in the second
 * local, reloads it, and returns it.  The sibling `pool_init` at 0x2f904
 * writes the same field as a 4 KB page count and converts it to bytes, so the
 * field holds a pool size.  The resource functions at 0x2f470 through 0x2f554
 * show the same pool layout and support the section The resource layer.  The
 * original has no caller and no data reference, so the record keeps the weak
 * name `func_2f985`.
 *
 * The original has the EBP frame and two locals, so the module builds with
 * `-of+`.  The two volatile locals reproduce the two stores and the reloads.
 * The function holds no floating-point instruction and calls no helper, so
 * the module needs no FPU stub and no assembly implementation.
 */
struct pool_record {
    unsigned char pad[0x3c];
    unsigned int size;
};

unsigned int func_2f985(struct pool_record *pool)
{
    struct pool_record *volatile saved_pool = pool;
    unsigned int volatile size = saved_pool->size;

    return size;
}
