/* The rebuilt `material_load` module.
 *
 * The original sits at linear 0x4cd9c in STARTH.EXE and holds 81 instructions
 * and 303 bytes up to the `ret` at 0x4ceca.  The next function starts at
 * 0x4cecc, so the store record keeps the 304-byte extent and sets no separate
 * function size.  The identity check uses 81 instructions.
 *
 * The two direct callers pass the material name.  `race_data_load` at 0x131a8
 * calls the function at 0x1333d with the level name at DAT_0011d294, and
 * `menu_space_init` at 0x29d08 calls it at 0x29e19 with the text `menu`.  The
 * function builds the `%s.BCK` and `%s.MLB` member names, so the record takes
 * the section Resource loader.
 *
 * The function does three steps:
 *
 *     1  it gets the decoded byte count of the `%s.BCK` PCX member through the
 *        decoder at 0x4d098 with the mode 2; a count at or below zero becomes
 *        0x10000
 *     2  it opens the `%s.MLB` member with the empty path and the mode `rb`;
 *        a failed open runs the file error report at 0x4bf88 and then the exit
 *        at 0x300be with the code 0
 *     3  it gets the `MATERIALS` resource of the stream size plus the decoded
 *        size, reads the member into the resource through the stream read at
 *        0x4c4e5, closes the stream at 0x4c38d, and decodes the `%s.BCK` member
 *        into the buffer at DAT_00214e8 through the decoder at 0x4d098 with the
 *        mode 3; the buffer starts after the member data, so the PCX image
 *        follows the material data
 *
 * DAT_0021544 holds the resource and DAT_00214e8 holds the decoded background
 * buffer that `background_scale` at 0x83ca8 reads.  The `MATERIALS` resource,
 * the two member names, and the two callers support the name `material_load`.
 * The original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses no floating-point instruction, so it needs no FPU stub.  The decoder at
 * 0x4d098 has no name in the store yet, so the module declares it as
 * `pcx_decode`.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the
 * harness build defines `MATERIAL_LOAD_HARNESS` and the function builds the
 * constant texts in local buffers.  The default build keeps the literals of
 * the original for the later period build.
 */
extern unsigned char *DAT_000214e8;
extern void *DAT_0021544;
extern void *DAT_0021574;

extern int sprintf(char *buffer, const char *format, ...);
extern int pcx_decode(char *path, char *name, int *out, char *palette, int mode);
extern int fopen(char *stream, char *path, char *name, char *category);
extern void file_error_report(char *stream);
extern void exit(int status);
extern int stream_status(char *stream);
extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern int stream_read(char *stream, char *buffer, int count);
extern int stream_close(char *stream);

#ifdef MATERIAL_LOAD_HARNESS
static void fill(char *text, char a, char b, char c, char d, char e,
                 char f, char g, char h, char i)
{
    text[0] = a;
    text[1] = b;
    text[2] = c;
    text[3] = d;
    text[4] = e;
    text[5] = f;
    text[6] = g;
    text[7] = h;
    text[8] = i;
    text[9] = 0;
}
#endif

void material_load(char *name)
{
#ifdef MATERIAL_LOAD_HARNESS
    char bck_format[0x10];
    char mlb_format[0x10];
    char mode_text[0x10];
    char path_text[0x10];
    char resource_text[0x10];
#else
    char *bck_format = "%s.BCK";
    char *mlb_format = "%s.MLB";
    char *mode_text = "rb";
    char *path_text = "";
    char *resource_text = "MATERIALS";
#endif
    char palette[0x300];
    char stream[0xb4];
    char mlb_name[0x80];
    char bck_name[0x80];
    int size;
    int status;
    int total;

#ifdef MATERIAL_LOAD_HARNESS
    fill(bck_format, '%', 's', '.', 'B', 'C', 'K', 0, 0, 0);
    fill(mlb_format, '%', 's', '.', 'M', 'L', 'B', 0, 0, 0);
    fill(mode_text, 'r', 'b', 0, 0, 0, 0, 0, 0, 0);
    fill(path_text, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    fill(resource_text, 'M', 'A', 'T', 'E', 'R', 'I', 'A', 'L', 'S');
#endif

    sprintf(bck_name, bck_format, name);
    sprintf(mlb_name, mlb_format, name);
    size = pcx_decode(path_text, bck_name, (int *)&DAT_000214e8, palette, 2);
    if (size <= 0) {
        size = 0x10000;
    }
    if (fopen(stream, path_text, mlb_name, mode_text) != 0) {
        file_error_report(stream);
        exit(0);
    }
    status = stream_status(stream);
    total = status + size;
    DAT_0021544 = resource_get(DAT_0021574, resource_text, total);
    if (DAT_0021544 == 0) {
        memory_error_report(DAT_0021574);
    }
    stream_read(stream, (char *)DAT_0021544, status);
    stream_close(stream);
    DAT_000214e8 = (unsigned char *)DAT_0021544 + status;
    pcx_decode(path_text, bck_name, (int *)&DAT_000214e8, palette, 3);
}
