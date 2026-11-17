/* The rebuilt `lzw_expand` module.
 *
 * The original sits at linear 0x4d3fc in STARTH.EXE and holds 54 instructions
 * and 199 bytes up to the `ret 4` at 0x4d4c0.  The next function starts at
 * 0x4d4c4, so the store record keeps the 200-byte extent and sets no separate
 * function size.  The identity check uses 54 instructions.
 *
 * The two direct callers are `stream_read` at 0x4c4e5 and `member_open` at
 * 0x4c128.  The reference table in docs/reference/file-formats.md names the
 * address `lzw_expand`, so the module keeps the name with the confidence
 * inferred and the section `The graphics decoders`.
 *
 * The context holds the input position at `+0x4040`, the input size at
 * `+0x4044`, the output limit at `+0x4048`, the state at `+0x404c`, the output
 * position at `+0x4070`, the 0x400-byte bit buffer at `+0x8086`, the output
 * buffer pointer at `+0x8488`, and the stream handle at `+0x848c`.
 *
 * The function stores the handle, the output buffer, the input size, and the
 * output limit in the context, clears the output position, and reads the first
 * input block when the state is zero.  The read count is the smaller of the
 * input size and 0x400.  The function then loops on `lzw_block` until the
 * block decoder returns 0, and returns the output position.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original uses no floating-point instruction, so the module needs no FPU
 * stub.
 */
extern int fread(void *buffer, int size, int count, int stream);
extern int lzw_block(char *context);

int lzw_expand(char *context, char *output, int input_size, int count,
               int handle)
{
    int size;

    *(int *)(context + 0x848c) = handle;
    *(char **)(context + 0x8488) = output;
    *(int *)(context + 0x4044) = input_size;
    *(int *)(context + 0x4048) = count;
    *(int *)(context + 0x4070) = 0;
    if (*(int *)(context + 0x404c) == 0) {
        *(int *)(context + 0x4040) = 0;
        size = *(int *)(context + 0x4044);
        if (size > 0x400) {
            size = 0x400;
        }
        *(int *)(context + 0x4040) += fread(context + 0x8086, 1, size,
                                            *(int *)(context + 0x848c));
    }
    while (lzw_block(context) != 0) {
    }
    return *(int *)(context + 0x4070);
}
