/* The rebuilt `sprite_load` module.
 *
 * The original sits at linear 0x257b0 in STARTH.EXE and holds 67 instructions
 * and 261 bytes up to the `ret` at 0x258b4.  The next function starts at
 * 0x258b8, so the store record keeps the 264-byte extent and sets no separate
 * function size.  The identity check uses 67 instructions.
 *
 * The two direct callers name the function.  `race_data_load` at 0x131a8
 * calls it at 0x13426 with `logo%c.spr` and at 0x134b6 with `game%c.spr`.
 * `menu_space_init` at 0x29d08 calls it at 0x29db4 with `menu%c.spr`.  Each
 * caller builds the member name and the container path first, so the module
 * takes the section The graphics decoders.
 *
 * The function is the sprite member loader:
 *
 *     open        member open with the path, the member name, and the
 *                 category `rb` at object-3 offset 0xeb4; a failed open runs
 *                 the file error report and then exit(0)
 *     counts      read the four-byte record count at object-3 offset 0x15cd4
 *                 and the four-byte data size at 0x15cdc
 *     records     get the `SPRITES` resource of `count * 0x14` bytes at
 *                 object-3 offset 0xeb7 and read it into 0x15ccc; a zero
 *                 result runs the memory error report
 *     data        get the `SPRITES` resource of the data size at 0xebf and
 *                 read it into 0x15ce0; a zero result runs the memory error
 *                 report
 *     done        close the stream and return 0
 *
 * The two member names, the caller group, and the `SPRITES` resources support
 * the name `sprite_load`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the
 * harness build defines `SPRITE_LOAD_HARNESS` and the function builds the two
 * constant texts in local buffers.  The default build keeps the literals of
 * the original for the later period build.
 */
extern int DAT_00015cd4;
extern int DAT_00015cdc;
extern void *DAT_00015ccc;
extern void *DAT_00015ce0;
extern void *DAT_000062ac;

extern int fopen(char *stream, char *path, char *name, char *category);
extern void file_error_report(char *stream);
extern void exit(int status);
extern int stream_read(char *stream, char *buffer, int count);
extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern int stream_close(char *stream);

#ifdef SPRITE_LOAD_HARNESS
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

int sprite_load(char *path, char *member)
{
#ifdef SPRITE_LOAD_HARNESS
    char mode_text[0x10];
    char resource_text[0x10];
#else
    char *mode_text = "rb";
    char *resource_text = "SPRITES";
#endif
    char stream[0xb4];

#ifdef SPRITE_LOAD_HARNESS
    fill(mode_text, 'r', 'b', 0, 0, 0, 0, 0, 0);
    fill(resource_text, 'S', 'P', 'R', 'I', 'T', 'E', 'S', 0);
#endif

    if (fopen(stream, path, member, mode_text) != 0) {
        file_error_report(stream);
        exit(0);
    }

    stream_read(stream, (char *)&DAT_00015cd4, 4);
    stream_read(stream, (char *)&DAT_00015cdc, 4);

    DAT_00015ccc = resource_get(
        DAT_000062ac, resource_text, DAT_00015cd4 * 0x14);
    if (DAT_00015ccc == 0) {
        memory_error_report(DAT_000062ac);
    }
    stream_read(stream, (char *)DAT_00015ccc, DAT_00015cd4 * 0x14);

    DAT_00015ce0 = resource_get(
        DAT_000062ac, resource_text, DAT_00015cdc);
    if (DAT_00015ce0 == 0) {
        memory_error_report(DAT_000062ac);
    }
    stream_read(stream, (char *)DAT_00015ce0, DAT_00015cdc);

    stream_close(stream);
    return 0;
}
