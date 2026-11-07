/* The rebuilt `font_load` module.
 *
 * The original sits at linear 0x25658 in STARTH.EXE and holds 93 instructions
 * and 324 bytes up to the `ret` at 0x2579b.  The next function starts at
 * 0x257a0 after four bytes of padding at 0x2579c, so the store record keeps
 * the 328-byte extent and sets no separate function size.  The identity check
 * uses 93 instructions.
 *
 * The only direct caller is `menu_space_init` at 0x29d08, which builds the
 * member name `menu%c.fnt` and passes the `graf.dat` path.  The sibling
 * `font_table_init` at 0x25620 points `DAT_00015438` at the descriptor table
 * 0x1ca98 and `DAT_00015434` at the bitmap table 0x19898, and `text_measure`
 * at 0x254a8 reads both tables.  The loader reads a font member, so the module
 * takes the section The graphics decoders.
 *
 * The function is the font member loader:
 *
 *     open        member open with the path, the member name, and the
 *                 category `rb` at 0x100eaa; a failed open runs the file
 *                 error report and then exit(0)
 *     count       read one byte into the font count DAT_00015440
 *     descriptors read `count + 1` records of 12 bytes into the 0x1ca98
 *                 table; each record holds the height, the first character
 *                 code, and the character count
 *     bitmaps     read `count + 1` glyph rows into the 0x19898 table; row `i`
 *                 starts at 0x19898 + i * 0x500 and holds count[i] * 8 bytes
 *     tail        read the four-byte tail size at 0x1cb10, get the `FONTS`
 *                 resource of that size at 0x100ead through 0x2eed0, and read
 *                 the tail into it; a zero result runs the memory error report
 *     done        close the stream and increment the font count
 *
 * The member, the descriptor table, the bitmap table, the `FONTS` resource,
 * and the caller support the name `font_load`.  The original has no symbol,
 * so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the
 * harness build defines `FONT_LOAD_HARNESS` and the function builds the two
 * constant texts in local buffers.  The default build keeps the literals of
 * the original for the later period build.
 */
extern int DAT_00015440;
extern void *DAT_0001543c;
extern int DAT_001ca98[];
extern char DAT_00019898[];
extern int DAT_001cb10;
extern void *DAT_000062ac;

extern int fopen(char *stream, char *path, char *name, char *category);
extern void file_error_report(char *stream);
extern void exit(int status);
extern int stream_read(char *stream, char *buffer, int count);
extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern int stream_close(char *stream);

#ifdef FONT_LOAD_HARNESS
static void fill(char *text, char a, char b, char c, char d,
                 char e, char f, char g, char h)
{
    text[0] = a;
    text[1] = b;
    text[2] = c;
    text[3] = d;
    text[4] = e;
    text[5] = f;
    text[6] = g;
    text[7] = h;
    text[8] = 0;
}
#endif

int font_load(char *path, char *member)
{
#ifdef FONT_LOAD_HARNESS
    char mode_text[0x10];
    char resource_text[0x10];
#else
    char *mode_text = "rb";
    char *resource_text = "FONTS";
#endif
    char stream[0xb4];
    int index;
    int length;

#ifdef FONT_LOAD_HARNESS
    fill(mode_text, 'r', 'b', 0, 0, 0, 0, 0, 0);
    fill(resource_text, 'F', 'O', 'N', 'T', 'S', 0, 0, 0);
#endif

    if (fopen(stream, path, member, mode_text) != 0) {
        file_error_report(stream);
        exit(0);
    }

    stream_read(stream, (char *)&DAT_00015440, 1);

    for (index = 0; index < DAT_00015440 + 1; index++) {
        stream_read(stream, (char *)DAT_001ca98 + index * 12, 12);
    }

    for (index = 0; index < DAT_00015440 + 1; index++) {
        length = DAT_001ca98[index * 3 + 2] * 8;
        stream_read(stream, DAT_00019898 + index * 0x500, length);
    }

    stream_read(stream, (char *)&DAT_001cb10, 4);
    DAT_0001543c = resource_get(DAT_000062ac, resource_text, DAT_001cb10);
    if (DAT_0001543c == 0) {
        memory_error_report(DAT_000062ac);
    }
    stream_read(stream, (char *)DAT_0001543c, DAT_001cb10);
    stream_close(stream);
    DAT_00015440++;
    return 0;
}
