/* The rebuilt `stream_seek` module.
 *
 * The original sits at linear 0x4c5c7 in STARTH.EXE and holds 72 instructions
 * and 206 bytes up to the `ret` at 0x4c694.  The next function starts at
 * 0x4c695, so the store record keeps the 206-byte extent.  The identity check
 * uses 72 instructions.
 *
 * The four direct call sites at 0x1b5fe, 0x1b621, 0x1b419, and 0x1b498 sit in
 * `pcx_load` at 0x1b5a4, `member_list_load` at 0x1b394, and `spline_load` at
 * 0x1af70.  The callers pass the stream record, the offset, and the origin.
 * The sibling stream helpers at 0x4c38d, 0x4c4e5, 0x4c695, and 0x4c7e0, and
 * the callers support the name `stream_seek`.
 *
 * The stream record holds the file at +0x00, the start at +0x04, the size at
 * +0x08, the position at +0x0c, the buffer index at +0x18, the field at +0x6c,
 * and the status at +0xac.
 *
 * The origin 0 seeks from the start: the position becomes the start plus the
 * offset, and the buffer index becomes the offset.  The origin 1 seeks from the
 * current position: the position and the buffer index both add the offset.  The
 * origin 2 seeks from the end: the position becomes the size plus the offset,
 * and the buffer index becomes the field at +0x6c plus the offset.  Any other
 * origin keeps the record unchanged.
 *
 * The function then synchronizes the file position through `fseek` at 0xd484f
 * with the origin 0.  When the new position passes the size, it writes the
 * status 4 and returns -1.  Otherwise it returns 0.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses no assembly.
 */
extern int fseek(void *file, int offset, int origin);

int stream_seek(char *stream, int offset, int origin)
{
    if (origin == 0) {
        *(int *)(stream + 0xc) = *(int *)(stream + 4) + offset;
        *(int *)(stream + 0x18) = offset;
    } else if (origin == 1) {
        *(int *)(stream + 0xc) = *(int *)(stream + 0xc) + offset;
        *(int *)(stream + 0x18) = *(int *)(stream + 0x18) + offset;
    } else if (origin == 2) {
        *(int *)(stream + 0xc) = *(int *)(stream + 8) + offset;
        *(int *)(stream + 0x18) = *(int *)(stream + 0x6c) + offset;
    }
    fseek(*(void **)stream, *(int *)(stream + 0xc), 0);
    if (*(int *)(stream + 0xc) > *(int *)(stream + 8)) {
        *(int *)(stream + 0xac) = 4;
        return -1;
    }
    return 0;
}
