/* The rebuilt `object_init_from_file` module.
 *
 * The original sits at linear 0x4cc10 in STARTH.EXE and holds 120 instructions
 * and 396 bytes: the three register saves, the frame, the two argument slots,
 * the index bound at 0x3e8, the message call at 0xc656e, the handle table read
 * at 0x21540, the object field stores, the chunk loop, and the end marker.
 *
 * The function reads the file record from the handle table at the index.  It
 * reports the message `error` when the index is above 0x3e8 but continues,
 * because the original has no return on that path.  It sets the object field
 * at +0x04 to 1, the index at +0x00, and the fields at +0x08 through +0x1c to
 * zero.  It sets the field at +0x20 to -1 and clears the six words at +0x24
 * through +0x38.
 *
 * The function then fills the 0x20-byte chunk records at object + 0x40.  The
 * file record holds the chunk count at +0x14 and the offset table at +0x18.
 * A chunk offset points at a chunk record, and the word at +0x0c of that
 * record points at the chunk header.  Each table record holds the file, the
 * chunk count, and the six words at +0x04 through +0x18 of the header.  The
 * table ends with the marker -1 at +0x04 of the record after the last chunk.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int report_message(char *message);
extern int *DAT_00021540;

void object_init_from_file(void *object, int index)
{
    char *file;
    char *entry;
    char *chunk;
    char *header;
    int count;
    int slot;

    if (index > 0x3e8) {
        report_message("error");
    }
    file = (char *)DAT_00021540[index];
    *(int *)((char *)object + 4) = 1;
    *(int *)object = index;
    *(int *)((char *)object + 0x10) = 0;
    *(int *)((char *)object + 0xc) = *(int *)((char *)object + 0x10);
    *(int *)((char *)object + 8) = *(int *)((char *)object + 0xc);
    *(int *)((char *)object + 0x1c) = 0;
    *(int *)((char *)object + 0x18) = *(int *)((char *)object + 0x1c);
    *(int *)((char *)object + 0x14) = *(int *)((char *)object + 0x18);
    *(int *)((char *)object + 0x20) = -1;
    *(int *)((char *)object + 0x24) = 0;
    *(int *)((char *)object + 0x28) = 0;
    *(int *)((char *)object + 0x2c) = 0;
    *(int *)((char *)object + 0x30) = 0;
    *(int *)((char *)object + 0x34) = 0;
    *(int *)((char *)object + 0x38) = 0;
    count = *(int *)(file + 0x14);
    entry = (char *)object + 0x40;
    for (slot = 0; slot < count; ++slot) {
        chunk = file + *(int *)(file + slot * 4 + 0x18);
        header = file + *(int *)(chunk + 0xc);
        *(char **)entry = file;
        *(int *)(entry + 4) = count;
        *(int *)(entry + 8) = *(int *)(header + 4);
        *(int *)(entry + 0xc) = *(int *)(header + 8);
        *(int *)(entry + 0x10) = *(int *)(header + 0xc);
        *(int *)(entry + 0x14) = *(int *)(header + 0x10);
        *(int *)(entry + 0x18) = *(int *)(header + 0x14);
        *(int *)(entry + 0x1c) = *(int *)(header + 0x18);
        entry += 0x20;
    }
    *(int *)(entry + 4) = -1;
}
