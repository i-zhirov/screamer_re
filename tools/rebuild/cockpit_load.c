/* The rebuilt `cockpit_load` module.
 *
 * The original sits at linear 0x1b23c in STARTH.EXE and holds 57 instructions
 * and 200 bytes up to the `ret` at 0x1b303.  The next function `controls_load`
 * starts at 0x1b304, so the store record keeps the 200-byte extent and sets no
 * separate function size.  The identity check uses 57 instructions.
 *
 * The only direct caller at 0x1374b is `player_car_init` at 0x136cc.  The caller
 * builds the cockpit name `COCK%s%c` and passes the name in `eax` and the out
 * cell 0x18f0c in `edx`.  The function builds the container path
 * `%s\gfx\game.dat` from the base path DAT_00007f44 and the member name
 * `%s%c.pkc` from the name and the letter `H`.  It opens the member with the
 * mode `rb` and the stream through `fopen` at 0x4c08d.  A failed open runs the
 * file error report at 0x4bf88; the function then continues.
 *
 * The function reads the member size through `stream_status` at 0x4c7e0, gets
 * the `COCKPIT` resource of that size through `resource_get` at 0x2eed0, and
 * stores the resource pointer in the out cell.  A zero result runs the memory
 * error report at 0x2edad.  The function reads the member into the resource
 * through `stream_read` at 0x4c4e5 and closes the stream through `stream_close`
 * at 0x4c38d.
 *
 * The cockpit member, the `COCKPIT` resource, and the caller support the name
 * `cockpit_load`.  The original has no symbol, so the record keeps the name
 * with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses no floating-point instruction, so it needs no FPU stub, and the module
 * uses no assembly.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the
 * harness build defines `COCKPIT_LOAD_HARNESS` and the function builds the
 * constant texts in local buffers.  The default build keeps the literals of the
 * original for the later period build.
 */
extern char DAT_00007f44[];
extern void *DAT_000062ac;

extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(char *stream, char *path, char *name, char *category);
extern void file_error_report(char *stream);
extern int stream_status(char *stream);
extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern int stream_read(char *stream, char *buffer, int count);
extern int stream_close(char *stream);

#ifdef COCKPIT_LOAD_HARNESS
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

void cockpit_load(char *name, int *out)
{
#ifdef COCKPIT_LOAD_HARNESS
    char path_format[0x14];
    char member_format[0x10];
    char mode_text[0x10];
    char resource_text[0x10];
#else
    char *path_format = "%s\\gfx\\game.dat";
    char *member_format = "%s%c.pkc";
    char *mode_text = "rb";
    char *resource_text = "COCKPIT";
#endif
    char stream[0xb4];
    char path[0x64];
    char member[0x64];
    int size;

#ifdef COCKPIT_LOAD_HARNESS
    fill(path_format, '%', 's', '\\', 'g', 'f', 'x', '\\', 'g');
    fill(path_format + 8, 'a', 'm', 'e', '.', 'd', 'a', 't', 0);
    fill(member_format, '%', 's', '%', 'c', '.', 'p', 'k', 'c');
    fill(mode_text, 'r', 'b', 0, 0, 0, 0, 0, 0);
    fill(resource_text, 'C', 'O', 'C', 'K', 'P', 'I', 'T', 0);
#endif

    sprintf(path, path_format, DAT_00007f44);
    sprintf(member, member_format, name, 'H');
    if (fopen(stream, path, member, mode_text) != 0) {
        file_error_report(stream);
    }
    size = stream_status(stream);
    *out = (int)resource_get(DAT_000062ac, resource_text, size);
    if (*out == 0) {
        memory_error_report(DAT_000062ac);
    }
    stream_read(stream, (char *)*out, size);
    stream_close(stream);
}
