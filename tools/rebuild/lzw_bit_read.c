/* The rebuilt `lzw_bit_read` module.
 *
 * The original sits at linear 0x4d844 in STARTH.EXE and holds 119 instructions
 * and 374 bytes up to the `ret` at 0x4d9b9.  The next function starts at
 * 0x4d9ba, so the store record keeps the 374-byte extent and sets no separate
 * function size.  The identity check uses 119 instructions.
 *
 * The function reads one code from the compressed member stream.  The context
 * holds the bit position at `+0x406c`, the code width at `+0x4068`, the mask
 * table at `+0x8050`, the 0x400-byte bit buffer at `+0x8086`, the stream
 * position at `+0x4040`, the stream limit at `+0x4044`, and the stream handle
 * at `+0x848c`.
 *
 * When the bit buffer holds fewer than five bytes after the bit position, the
 * function moves the remaining bytes to the front of the buffer, reads more
 * bytes through `fread`, advances the stream position, and keeps only the
 * sub-byte bit offset.  The read count is the smaller of the free space and
 * the bytes left in the stream.
 *
 * The function forms a 24-bit little-endian window at the byte position,
 * shifts it right by the sub-byte bit offset, masks it with the table entry of
 * the code width, and advances the bit position by the code width.
 */
extern int fread(void *buffer, int size, int count, int stream);

int lzw_bit_read(char *context)
{
    int remaining;
    int index;
    int free_space;
    int stream_left;
    int count;
    int bit;
    int shift;
    int loop;
    char *pointer;
    unsigned int value;
    unsigned char extra;

    remaining = 0x400 - (*(int *)(context + 0x406c) >> 3);
    if (remaining < 5) {
        for (index = 0; index < remaining; index++) {
            context[0x8086 + index] =
                context[0x8086 + (0x400 - remaining) + index];
        }
        free_space = 0x400 - remaining;
        stream_left = *(int *)(context + 0x4044)
                    - *(int *)(context + 0x4040);
        count = free_space;
        if (count > stream_left) {
            count = stream_left;
        }
        *(int *)(context + 0x4040) += fread(
            context + 0x8086 + remaining, 1, count,
            *(int *)(context + 0x848c));
        *(int *)(context + 0x406c) &= 7;
    }

    bit = *(int *)(context + 0x406c) & 7;
    pointer = context + 0x8086 + (*(int *)(context + 0x406c) >> 3);
    shift = *(int *)(context + 0x4068);
    *(int *)(context + 0x406c) += shift;

    value = *(unsigned short *)pointer;
    extra = (unsigned char)pointer[2];
    for (loop = 0; loop < bit; loop++) {
        value = (value >> 1) | ((unsigned int)(extra & 1) << 15);
        extra >>= 1;
    }
    value &= *(unsigned int *)(context + 0x8050 + (shift << 2));
    return (int)value;
}
