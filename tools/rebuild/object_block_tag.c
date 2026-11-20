/* The rebuilt `object_block_tag` module.
 *
 * The original sits at linear 0x118a4 in STARTH.EXE and holds 21 instructions
 * and 60 bytes up to the `ret` at 0x118df.  The success path jumps to the
 * shared epilogue at 0x118e0, and the next function starts at 0x118e8 after
 * two padding bytes.  The store keeps the 68-byte extent and sets no separate
 * function size.  The identity check uses 21 instructions.
 *
 * The only direct caller at 0x13805 is `player_car_init` at 0x136cc.  The
 * caller loads each of the eight 0x28-byte wheel effect slots at player object
 * offset 0x308 with the object loader at 0x11060.  It then calls this function
 * with the object table 0x6530 in `eax` and the slot in `edx`.
 *
 * The slot holds the object index in its first int.  The index selects one
 * 0x1c-byte record of the object table.  The file record at `+0x14` marks a
 * loaded record.  When it is zero, the function returns 0x10 and changes
 * nothing.  Otherwise the block object at `+0x0c` takes bit 0x80 of the byte
 * at `+0x05`, and the function returns 0.
 *
 * The byte at `+0x05` is the flag that `object_list_link` tests.  A set bit
 * 0x80 binds the record with the tagged list slot through 0x11850, and a clear
 * bit keeps the plain slot through 0x117fc.  The caller and the two sibling
 * binders support the name `object_block_tag` with the confidence inferred.
 * The original has no symbol.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no direct call, so it needs
 * no FPU stub and no assembly.
 */
struct object_record {
    char name[0x0c];            /* +0x00 */
    void *block;                /* +0x0c */
    void *chunks;               /* +0x10 */
    void *file;                 /* +0x14 */
    int reserved;               /* +0x18 */
};

int object_block_tag(void *table, int *slot)
{
    struct object_record *record;
    unsigned char *block;
    int index;

    index = *slot;
    record = (struct object_record *)((char *)table + index * 0x1c);
    if (record->file == 0) {
        return 0x10;
    }
    block = (unsigned char *)record->block;
    block[5] = block[5] | 0x80;
    return 0;
}
