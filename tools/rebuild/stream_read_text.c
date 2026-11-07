/* The rebuilt `stream_read_text` module.
 *
 * The original sits at linear 0x4c695 in STARTH.EXE and holds 106 instructions
 * and 331 bytes up to the `ret` at 0x4c7df.  The next function starts at
 * 0x4c7e0, so the store record keeps the 331-byte extent and sets no function
 * size.  The identity check uses 106 instructions.
 *
 * The two direct callers are `list_file_load` at 0x15460 and
 * `member_list_load` at 0x1b394.  Both pass the stream record, the destination
 * buffer, and the mode.  The line mode 1 stops at a newline only.  The token
 * mode 0 stops at a newline, a tab, or a space.  The callers and the sibling
 * `stream_read` at 0x4c4e5 support the name `stream_read_text`.
 *
 * The stream record holds the handle at +0x00, the size at +0x08, the
 * position at +0x0c, the buffer flag at +0x14, the buffer index at +0x18, the
 * buffer pointer at +0x1c, and the status at +0xac.
 *
 * At or past the size, the function clamps the position to the size, writes
 * the status 4, and returns -1.
 *
 * With the buffer flag set, the function copies one character at a time from
 * the buffer at +0x1c plus the index at +0x18, advances the index and the
 * position, and stops at the mode terminator.  It replaces the terminator with
 * the zero byte and keeps a carriage return out of the result.
 *
 * Without the buffer flag, the function reads a line through `fgets` with the
 * count 0x100 in the line mode, or a token through `fscanf` with the format
 * `%s` in the token mode.  It adds the text length plus one to the position.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern char *fgets(char *buffer, int count, void *stream);
extern int fscanf(void *stream, const char *format, ...);
extern int strlen(const char *text);

int stream_read_text(void *stream, char *buffer, int mode)
{
    char *self;
    int index;
    int separator;

    self = (char *)stream;
    if (*(int *)(self + 0xc) >= *(int *)(self + 0x8)) {
        *(int *)(self + 0xc) = *(int *)(self + 0x8);
        *(int *)(self + 0xac) = 4;
        return -1;
    }
    if (*(int *)(self + 0x14) != 0) {
        index = 0;
        for (;;) {
            char character;

            character = *(char *)(*(int *)(self + 0x1c)
                + *(int *)(self + 0x18));
            buffer[index] = character;
            *(int *)(self + 0x18) = *(int *)(self + 0x18) + 1;
            *(int *)(self + 0xc) = *(int *)(self + 0xc) + 1;
            if (character == 0x0a) {
                buffer[index] = 0;
                index = index + 1;
                break;
            }
            if (mode != 0) {
                separator = 0;
            } else {
                separator = character == 0x09 || character == 0x20;
            }
            if (separator != 0) {
                buffer[index] = 0;
                index = index + 1;
                break;
            }
            if (character != 0x0d) {
                index = index + 1;
            }
        }
    } else {
        if (mode != 0) {
            fgets(buffer, 0x100, *(void **)self);
        } else {
            fscanf(*(void **)self, "%s", buffer);
        }
        *(int *)(self + 0xc) = *(int *)(self + 0xc) + strlen(buffer) + 1;
    }
    return 0;
}
