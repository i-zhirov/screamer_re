/* The rebuilt function at linear 0x4c98c in STARTH.EXE.
 *
 * The plan row records a 344-byte body of 96 instructions and 13 calls up to
 * the `ret` at 0x4cae3.  The next function starts at 0x4cae4, so the store
 * record keeps the 344-byte extent and sets no function size.  The identity
 * check uses 96 instructions.
 *
 * The only direct caller at 0x4c979 sits in the wrapper at 0x4c94c.  The
 * wrapper copies its argument to the global name buffer `DAT_002154c`, calls
 * this function, and then calls the loader at 0x4cae4 for the `%s.MMP` member.
 * `race_data_load` at 0x131a8 calls the wrapper as `material_name_set`, after
 * the `material_load` call at 0x4cd9c.  The level data holds `MAP1.UNR` before
 * `MAP1.MMP`, so the function takes the section Resource loader with `fopen`,
 * `stream_read`, `stream_close`, `resource_get`, and `object_load`.
 *
 * The function builds the member name `%s.UNR` from `DAT_002154c` and opens it
 * with the empty path and the category `rb`.  A failed open runs the file
 * error report at 0x4bf88 and then the exit at 0x300be.  The first two reads
 * take the four-byte `UNR_FILE` size and the four-byte object count.
 *
 * The function gets the `OBJ_LST` resource of `count * 4 + 0x1e4` bytes and
 * the `UNR_FILE` resource of the file size through the resource layer at
 * 0x2eed0.  A zero result runs the memory error report at 0x2edad.  It stores
 * the object list at `DAT_0021540`, the value `DAT_0021464` at
 * `DAT_00214bc`, and the object list at `DAT_0021488` and `DAT_0021530`.
 *
 * The function reads `count * 4` bytes into the object list, writes the -1
 * sentinel at index `count`, and adds the `UNR_FILE` base to the low entries.
 * It then reads the file size bytes into the `UNR_FILE` resource and closes
 * the stream.  The `.UNR` member, the two resource names, and the wrapper
 * caller support the name `unr_file_load`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness build defines `UNR_FILE_LOAD_HARNESS`, so the function builds the
 * constant text in local buffers.  The default build keeps the literals of the
 * original for the later period build.
 */
extern char DAT_002154c[];
extern void *DAT_0021574;
extern void *DAT_002145c;
extern void *DAT_002152c;
extern void *DAT_0021540;
extern int DAT_0021464;
extern int DAT_00214bc;
extern void *DAT_0021488;
extern void *DAT_0021530;

extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(char *stream, char *path, char *name, char *category);
extern void file_error_report(char *stream);
extern void exit(int status);
extern int stream_read(char *stream, char *buffer, int count);
extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern int stream_close(char *stream);

#ifdef UNR_FILE_LOAD_HARNESS
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

void unr_file_load(void)
{
#ifdef UNR_FILE_LOAD_HARNESS
    char name_format[0x10];
    char object_name[0x10];
    char file_name[0x10];
    char mode_text[0x10];
    char path_text[0x10];
#else
    char *name_format = "%s.UNR";
    char *object_name = "OBJ_LST";
    char *file_name = "UNR_FILE";
    char *mode_text = "rb";
    char *path_text = "";
#endif
    char stream[0xb4];
    char name[0x28];
    int index;
    void *objects;
    int file_size;
    int object_count;

#ifdef UNR_FILE_LOAD_HARNESS
    fill(name_format, '%', 's', '.', 'U', 'N', 'R', 0, 0);
    fill(object_name, 'O', 'B', 'J', '_', 'L', 'S', 'T', 0);
    fill(file_name, 'U', 'N', 'R', '_', 'F', 'I', 'L', 'E');
    file_name[8] = 0;
    fill(mode_text, 'r', 'b', 0, 0, 0, 0, 0, 0);
    fill(path_text, 0, 0, 0, 0, 0, 0, 0, 0);
#endif

    sprintf(name, name_format, DAT_002154c);
    if (fopen(stream, path_text, name, mode_text) != 0) {
        file_error_report(stream);
        exit(0);
    }
    stream_read(stream, (char *)&file_size, 4);
    stream_read(stream, (char *)&object_count, 4);
    DAT_002145c = resource_get(DAT_0021574, object_name,
                               object_count * 4 + 0x1e4);
    if (DAT_002145c == 0) {
        memory_error_report(DAT_0021574);
    }
    objects = DAT_002145c;
    DAT_002152c = resource_get(DAT_0021574, file_name, file_size);
    if (DAT_002152c == 0) {
        memory_error_report(DAT_0021574);
    }
    DAT_0021540 = objects;
    DAT_00214bc = DAT_0021464;
    DAT_0021488 = DAT_002145c;
    DAT_0021530 = DAT_002145c;
    stream_read(stream, (char *)objects, object_count * 4);
    ((int *)objects)[object_count] = -1;
    for (index = 0; index < object_count; index++) {
        ((int *)objects)[index] += (int)DAT_002152c;
    }
    stream_read(stream, (char *)DAT_002152c, file_size);
    stream_close(stream);
}
