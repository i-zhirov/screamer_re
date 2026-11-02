/* The rebuilt `pcx_load` module.
 *
 * The original sits at linear 0x1b5a4 in STARTH.EXE and holds 108 instructions
 * and 401 bytes: the frame, the four argument slots, the 0x1d4-byte local
 * area, the `sprintf` call at 0x4d32c, the `fopen` call at 0x4c08d, the two
 * seek calls at 0x4c5c7, the palette read and the shift loop, the header read
 * at 0x4c4e5, the size arithmetic, the RLE loop, the `stream_close` call at
 * 0x4c38d, and the epilogue.
 *
 * The function builds the container path `%s\%s` from the base path at
 * `DAT_00007edc` and the first argument, then opens the member named by the
 * second argument in the mode `rb`.  A failed open passes the stream to
 * `file_error_report` and continues.
 *
 * The function seeks 0x300 bytes before the end and reads the 256-colour
 * palette into the fourth argument.  It shifts every palette byte right by 2,
 * so the palette becomes the 6-bit VGA form.  It rewinds, reads the 128-byte
 * PCX header, and computes the pixel count as `(xmax - xmin + 1) *
 * (ymax - ymin + 1)`.  It decodes the RLE runs into the third argument: a byte
 * with the mask 0xc0 equal to 0xc0 repeats the next byte `byte & 0x3f` times.
 * It then closes the stream and returns 0.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The local
 * area holds the 0xb4-byte stream, the 0x80-byte header, and the 0x80-byte
 * name buffer.
 */
extern char DAT_00007edc[];
extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(char *stream, char *path, char *name, char *category);
extern int stream_seek(char *stream, int offset, int origin);
extern int stream_read(char *stream, void *buffer, int count);
extern int stream_close(char *stream);
extern void file_error_report(void *stream);

int pcx_load(char *file, char *member, unsigned char *pixels,
             unsigned char *palette)
{
    char stream[0xb4];
    char header[0x80];
    char name[0x80];
    unsigned char byte;
    unsigned char value;
    int left;
    int index;

    sprintf(name, "%s\\%s", DAT_00007edc, file);
    if (fopen(stream, name, member, "rb") != 0) {
        file_error_report(stream);
    }
    stream_seek(stream, -0x300, 2);
    stream_read(stream, palette, 0x300);
    for (left = 0; left < 0x300; ++left) {
        palette[left] = (unsigned char)((unsigned)palette[left] >> 2);
    }
    stream_seek(stream, 0, 0);
    stream_read(stream, header, 0x80);
    left = (*(short *)(header + 8) - *(short *)(header + 4) + 1)
         * (*(short *)(header + 10) - *(short *)(header + 6) + 1);
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
    stream_close(stream);
    return 0;
}
