/* The rebuilt `spline_load` module.
 *
 * The original starts at linear 0x1af70 in STARTH.EXE and holds 148
 * instructions and 527 bytes up to the `ret` at `0x1b17e`.  The next function
 * starts at `0x1b180`, so the store record keeps the 528-byte extent and sets
 * no function size.
 *
 * The function builds the container path `%s\data\%s.dat` from the base path
 * at `DAT_00007f44` and the first argument, and the member name `%s.spl` from
 * the second argument.  It opens the member with the mode `rb` through the
 * member open at `0x4c08d`.  A failed open returns 0.
 *
 * The first pass reads a four-byte point count at the start of every spline,
 * skips the four-byte type and the `count * 0x38` data bytes with the seek at
 * `0x4c5c7`, and increments the spline count.  The pass ends at the first
 * short read.
 *
 * The function then gets the `GAMESPLINE` resource of `count * 0x24` bytes
 * through `0x2eed0` into the third argument.  A zero result runs the memory
 * error report at `0x2edad`.  It rewinds the stream and reads each 0x24-byte
 * record: the four-byte point count at `+0x1c`, the four-byte type at `+0x20`,
 * and the `count * 0x38` data bytes into the resource at `+0x00`.  A zero data
 * resource runs the memory error report.  The function closes the stream and
 * returns the spline count.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness build defines `SPLINE_LOAD_HARNESS`, so the function builds the
 * constant text in local buffers.  The default build keeps the literals of the
 * original for the later period build.
 */
struct spline_record {
    void *data;
    int reserved[6];
    int count;
    int type;
};

extern char DAT_00007f44[];
extern void *DAT_000062ac;
extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(char *stream, char *path, char *name, char *category);
extern int stream_read(char *stream, void *buffer, int count);
extern int stream_seek(char *stream, int offset, int origin);
extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern int stream_close(char *stream);

#ifdef SPLINE_LOAD_HARNESS
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

int spline_load(char *archive, char *name, struct spline_record **table)
{
#ifdef SPLINE_LOAD_HARNESS
    char path_format[0x14];
    char member_format[0x10];
    char list_name[0x10];
    char mode_text[0x10];
#else
    char *path_format = "%s\\data\\%s.dat";
    char *member_format = "%s.spl";
    char *list_name = "GAMESPLINE";
    char *mode_text = "rb";
#endif
    char path[0x64];
    char member[0x64];
    char file[0xb4];
    int count;
    int index;
    int value;

#ifdef SPLINE_LOAD_HARNESS
    fill(path_format, '%', 's', '\\', 'd', 'a', 't', 'a', '\\');
    fill(path_format + 8, '%', 's', '.', 'd', 'a', 't', 0, 0);
    fill(member_format, '%', 's', '.', 's', 'p', 'l', 0, 0);
    fill(list_name, 'G', 'A', 'M', 'E', 'S', 'P', 'L', 'I');
    list_name[8] = 'N';
    list_name[9] = 'E';
    list_name[10] = 0;
    fill(mode_text, 'r', 'b', 0, 0, 0, 0, 0, 0);
#endif

    sprintf(path, path_format, DAT_00007f44, archive);
    sprintf(member, member_format, name);
    if (fopen(file, path, member, mode_text) != 0) {
        return 0;
    }
    count = 0;
    while (stream_read(file, &value, 4) > 0) {
        stream_seek(file, value * 0x38 + 4, 1);
        ++count;
    }
    *table = resource_get(DAT_000062ac, list_name, count * 0x24);
    if (*table == 0) {
        memory_error_report(DAT_000062ac);
    }
    stream_seek(file, 0, 0);
    for (index = 0; index < count; ++index) {
        stream_read(file, &(*table)[index].count, 4);
        stream_read(file, &(*table)[index].type, 4);
        (*table)[index].data = resource_get(
            DAT_000062ac, list_name, (*table)[index].count * 0x38);
        if ((*table)[index].data == 0) {
            memory_error_report(DAT_000062ac);
        }
        stream_read(file, (*table)[index].data,
                    (*table)[index].count * 0x38);
    }
    stream_close(file);
    return count;
}
