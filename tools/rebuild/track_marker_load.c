/* The rebuilt `track_marker_load` module.
 *
 * The original starts at linear 0x1b198 in STARTH.EXE and holds 48
 * instructions and 164 bytes up to the `ret` at `0x1b23b`.  The next function
 * starts at `0x1b23c`, so the store record keeps the 164-byte extent and sets
 * no function size.  The identity check uses 48 instructions.
 *
 * The function builds the container path `%s\data\%s.dat` from the base path
 * at `DAT_00007f44` and the first argument, and the member name `%s.mrk` from
 * the second argument.  It opens the member with the mode `rb` through the
 * member open at `0x4c08d`.  A failed open returns 0.
 *
 * On success the function reads a four-byte marker count, reads
 * `count * 0x20` bytes into the third argument, closes the stream, and returns
 * the count.  The caller at 0x150f4 stores the result at the track node count
 * `DAT_00117ed0` or the SLA marker count `DAT_00117ed4`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness build defines `TRACK_MARKER_LOAD_HARNESS`, so the function builds
 * the constant text in local buffers.  The default build keeps the literals of
 * the original for the later period build.
 */
extern char DAT_00007f44[];
extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(char *stream, char *path, char *name, char *category);
extern int stream_read(char *stream, void *buffer, int count);
extern int stream_close(char *stream);

#ifdef TRACK_MARKER_LOAD_HARNESS
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

int track_marker_load(char *archive, char *name, void *table)
{
#ifdef TRACK_MARKER_LOAD_HARNESS
    char path_format[0x14];
    char member_format[0x10];
    char mode_text[0x10];
#else
    char *path_format = "%s\\data\\%s.dat";
    char *member_format = "%s.mrk";
    char *mode_text = "rb";
#endif
    char path[0x64];
    char member[0x64];
    char file[0xb4];
    int count;

#ifdef TRACK_MARKER_LOAD_HARNESS
    fill(path_format, '%', 's', '\\', 'd', 'a', 't', 'a', '\\');
    fill(path_format + 8, '%', 's', '.', 'd', 'a', 't', 0, 0);
    fill(member_format, '%', 's', '.', 'm', 'r', 'k', 0, 0);
    fill(mode_text, 'r', 'b', 0, 0, 0, 0, 0, 0);
#endif

    sprintf(path, path_format, DAT_00007f44, archive);
    sprintf(member, member_format, name);
    if (fopen(file, path, member, mode_text) != 0) {
        return 0;
    }
    stream_read(file, &count, 4);
    stream_read(file, table, count * 0x20);
    stream_close(file);
    return count;
}
