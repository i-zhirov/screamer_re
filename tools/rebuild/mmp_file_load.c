/* The rebuilt `mmp_file_load` module.
 *
 * The original sits at linear 0x4cae4 in STARTH.EXE and holds 32 instructions
 * and 102 bytes up to the `ret` at 0x4cb49.  The next function starts at
 * 0x4cb4c after the two pad bytes, so the store record keeps the 104-byte
 * extent and sets no separate function size.  The identity check uses 32
 * instructions.
 *
 * The only direct caller at 0x4c97e sits in the wrapper at 0x4c94c.  The
 * wrapper copies its argument to DAT_002154c, calls `unr_file_load` at
 * 0x4c98c, and then calls this function for the `%s.MMP` member.
 * `race_data_load` at 0x131a8 calls the wrapper after `material_load` at
 * 0x4cd9c, so the record takes the section Resource loader.
 *
 * The function does four steps:
 *
 *     1  it builds the member name `%s.MMP` from DAT_002154c through
 *        `sprintf` at 0x4d32c
 *     2  it calls `alfile_load` at 0x4c3e6 with the local stream, the empty
 *        path at object-3 offset 0x7fb4, the name, the out cell DAT_0021468,
 *        and the resource head DAT_0021574
 *     3  a non-zero result runs the file error report at 0x4bf88
 *     4  it copies the out cell to the grid base DAT_00102fa4 and runs the
 *        spatial grid relocate at 0x4cb4c
 *
 * The `%s.MMP` member, the out cell, the grid base, and the wrapper caller
 * support the name `mmp_file_load`.  The original has no symbol, so the record
 * keeps the name with the confidence `inferred`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction, so the module needs no FPU
 * stub, and the module uses no assembly.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the
 * harness build defines `MMP_FILE_LOAD_HARNESS` and the function builds the
 * two constant texts in local buffers.  The default build keeps the literals
 * of the original for the later period build.
 */
extern char DAT_002154c[];
extern void *DAT_0021468;
extern void *DAT_0021574;
extern unsigned int DAT_00102fa4;

extern int sprintf(char *buffer, const char *format, ...);
extern int alfile_load(char *stream, char *path, char *name, void **out,
                       void *resource);
extern void file_error_report(char *stream);
extern void spatial_grid_relocate(void);

#ifdef MMP_FILE_LOAD_HARNESS
static void fill(char *text, char a, char b, char c, char d, char e,
                 char f, char g, char h)
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

void mmp_file_load(void)
{
#ifdef MMP_FILE_LOAD_HARNESS
    char mmp_format[0x10];
    char path_text[0x10];
#else
    char *mmp_format = "%s.MMP";
    char *path_text = "";
#endif
    char name[0x44];
    char stream[0xb4];

#ifdef MMP_FILE_LOAD_HARNESS
    fill(mmp_format, '%', 's', '.', 'M', 'M', 'P', 0, 0);
    fill(path_text, 0, 0, 0, 0, 0, 0, 0, 0);
#endif

    sprintf(name, mmp_format, DAT_002154c);
    if (alfile_load(stream, path_text, name, &DAT_0021468,
                    DAT_0021574) != 0) {
        file_error_report(stream);
    }
    DAT_00102fa4 = (unsigned int)DAT_0021468;
    spatial_grid_relocate();
}
