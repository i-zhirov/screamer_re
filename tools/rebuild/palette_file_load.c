/* The rebuilt `palette_file_load` module.
 *
 * The original sits at linear 0x4cf3c in STARTH.EXE and holds 30 instructions
 * and 88 bytes up to the `ret` at 0x4cf93.  The next function starts at
 * 0x4cf94, so the store record keeps the 88-byte extent and sets no separate
 * function size.  The identity check uses 30 instructions.
 *
 * The only direct caller at 0x12a7b sits in race_session_start at 0x128e0.
 * The caller puts the level name buffer at object-3 offset 0x1d294 in eax and
 * the palette buffer at object-3 offset 0xcf5c in edx, then runs
 * palette_shade_build at 0x258d0 with the loaded palette.  So the record takes
 * the section Resource loader.
 *
 * The function does three steps:
 *
 *     1  it builds the member name `%s.COL` from the level name through
 *        `sprintf` at 0x4d32c
 *     2  it calls `member_load` at 0x4c485 with the local stream, the empty
 *        path at object-3 offset 0x7fb4, the member name, and the palette
 *        buffer
 *     3  a non-zero result runs the file error report at 0x4bf88 and then the
 *        exit at 0x300be with the code 0
 *
 * The `%s.COL` member, the palette buffer, the level name, and the single
 * caller support the name `palette_file_load`.  The original has no symbol, so
 * the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame and spills the two register arguments, so the
 * module builds with `-of+`.  The function uses no floating-point instruction,
 * so the module needs no FPU stub, and the module uses no assembly.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the
 * harness build defines `PALETTE_FILE_LOAD_HARNESS` and the function builds the
 * two constant texts in local buffers.  The default build keeps the literals of
 * the original for the later period build.
 */
extern int sprintf(char *buffer, const char *format, ...);
extern int member_load(char *stream, char *path, char *name, char *buffer);
extern void file_error_report(char *stream);
extern void exit(int status);

void palette_file_load(char *name, char *palette)
{
#ifdef PALETTE_FILE_LOAD_HARNESS
    char name_format[0x10];
    char path_text[0x10];
#else
    char *name_format = "%s.COL";
    char *path_text = "";
#endif
    char member_name[0x64];
    char stream[0xb4];

#ifdef PALETTE_FILE_LOAD_HARNESS
    name_format[0] = '%';
    name_format[1] = 's';
    name_format[2] = '.';
    name_format[3] = 'C';
    name_format[4] = 'O';
    name_format[5] = 'L';
    name_format[6] = 0;
    path_text[0] = 0;
#endif

    sprintf(member_name, name_format, name);
    if (member_load(stream, path_text, member_name, palette) != 0) {
        file_error_report(stream);
        exit(0);
    }
}
