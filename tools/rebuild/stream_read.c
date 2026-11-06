/* The rebuilt `stream_read` module.
 *
 * The original starts at linear 0x4c4e5 in STARTH.EXE and holds 80
 * instructions and 226 bytes: the register saves, the frame, the three
 * argument slots, the flag test at `stream + 0x10`, the two clamp branches,
 * the expander call at `0x4d3fc`, the raw `fread` call at `0xc607b`, and the
 * return.
 *
 * The function reads `count` bytes from the stream into the buffer.  A set
 * flag at `stream + 0x10` runs the LZW expander at `0x4d3fc` with the buffer
 * at `stream + 0xb0`, the field at `stream + 0x6c`, and the handle at
 * `stream + 0x00`.  A clear flag reads the bytes raw through `fread`.  The
 * function advances the position at `stream + 0x0c` and clamps it to the size
 * at `stream + 0x08`.  It returns the byte count.
 */
extern int fread(void *buffer, int size, int count, int stream);
extern int lzw_expand(void *input, void *output, int input_size, int count,
                      int handle);

int stream_read(char *stream, char *buffer, int count)
{
    int position;
    int result;

    if (*(int *)(stream + 0x10) != 0) {
        position = *(int *)(stream + 0xc);
        *(int *)(stream + 0xc) += count;
        if (*(int *)(stream + 0xc) > *(int *)(stream + 0x8)) {
            count = *(int *)(stream + 0x8) - position;
            *(int *)(stream + 0xc) = *(int *)(stream + 0x8);
        }
        result = lzw_expand(*(void **)(stream + 0xb0), buffer,
                            *(int *)(stream + 0x6c), count, *(int *)stream);
    } else {
        result = fread(buffer, 1, count, *(int *)stream);
        position = *(int *)(stream + 0xc);
        *(int *)(stream + 0xc) += result;
        if (*(int *)(stream + 0xc) > *(int *)(stream + 0x8)) {
            result = *(int *)(stream + 0x8) - position;
            *(int *)(stream + 0xc) = *(int *)(stream + 0x8);
        }
    }
    return result;
}
