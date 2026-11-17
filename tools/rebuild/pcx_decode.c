/* The rebuilt `pcx_decode` module.
 *
 * The original body sits at linear 0x4d098 and holds 109 instructions and 377
 * bytes up to the last `ret 4` at 0x4d20e.  The next function starts at
 * 0x4d214 after the padding bytes at 0x4d211, so the store record keeps the
 * 380-byte extent and sets no separate function size.  The identity check uses
 * 109 instructions.
 *
 * The function takes the path in eax, the member name in edx, the output
 * pointer in ebx, and the palette buffer in ecx.  The mode arrives on the
 * stack.  It opens the member through `fopen` with the mode `rb` at object-3
 * offset 0x1760, reads the 0x80-byte PCX header through `stream_read` at
 * 0x4c4e5, and computes the pixel count as
 * `(xmax - xmin + 1) * (ymax - ymin + 1)`.
 *
 * In mode 2 the function closes the stream through `stream_close` at 0x4c38d
 * and returns the pixel count, so the caller learns the decoded size.
 *
 * In mode 1 the function gets the `PCX` resource at object-3 offset 0x1763 of
 * the pixel-count size through `resource_get` at 0x2eed0, stores the pointer
 * in the output slot, and returns -1 when the resource is zero.
 *
 * In every other mode the output slot already holds the destination pointer.
 * The function decodes the RLE runs into it: a byte with the mask 0xc0 equal to
 * 0xc0 repeats the next byte `byte & 0x3f` times.  It then reads the 0x300-byte
 * palette into the palette buffer, closes the stream, and returns 0.  A failed
 * open returns -2.
 *
 * The two callers are material_load at 0x4cdf3 with mode 2 and at 0x4cebb with
 * mode 3, so the record takes the name pcx_decode and the section Resource
 * loader.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the
 * harness build defines PCX_DECODE_HARNESS and the function builds the two
 * constant texts in local buffers.  The original has the EBP frame, so the
 * module builds with -of+.  The function uses no floating-point instruction,
 * so it needs no FPU stub.
 */
extern void *DAT_0021574;
extern int fopen(char *stream, char *path, char *name, char *category);
extern int stream_read(char *stream, void *buffer, int count);
extern int stream_close(char *stream);
extern void *resource_get(void *resource, char *name, int size);

int pcx_decode(char *path, char *name, int *out, unsigned char *palette,
               int mode)
{
    char stream[0xb4];
    char header[0x80];
#ifdef PCX_DECODE_HARNESS
    char mode_text[4];
    char resource_text[4];
#else
    char *mode_text = "rb";
    char *resource_text = "PCX";
#endif
    unsigned char byte;
    unsigned char value;
    unsigned char *pixels;
    int left;
    int index;

#ifdef PCX_DECODE_HARNESS
    mode_text[0] = 'r';
    mode_text[1] = 'b';
    mode_text[2] = 0;
    mode_text[3] = 0;
    resource_text[0] = 'P';
    resource_text[1] = 'C';
    resource_text[2] = 'X';
    resource_text[3] = 0;
#endif

    if (fopen(stream, path, name, mode_text) != 0) {
        return -2;
    }
    stream_read(stream, header, 0x80);
    left = (*(short *)(header + 8) - *(short *)(header + 4) + 1)
         * (*(short *)(header + 10) - *(short *)(header + 6) + 1);
    if (mode == 2) {
        stream_close(stream);
        return left;
    }
    if (mode == 1) {
        *out = (int)resource_get(DAT_0021574, resource_text, left);
        if (*out == 0) {
            return -1;
        }
    }
    pixels = (unsigned char *)*out;
    while (left > 0) {
        stream_read(stream, &byte, 1);
        if ((byte & 0xc0) == 0xc0) {
            stream_read(stream, &value, 1);
            left -= byte & 0x3f;
            for (index = 0; index < (byte & 0x3f); ++index) {
                *pixels++ = value;
            }
        } else {
            *pixels++ = byte;
            left--;
        }
    }
    stream_read(stream, palette, 0x300);
    stream_close(stream);
    return 0;
}
