/* The rebuilt `controls_load` module.
 *
 * The original sits at linear 0x1b304 in STARTH.EXE and holds 40 instructions
 * and 144 bytes: the three register saves, the frame, the 0x80-byte path
 * buffer, the handle slot, the `sprintf` call at `0x4d32c`, the `fopen` call
 * at `0xc5fe5`, the three `fread` calls at `0xc607b`, the handle close at
 * `0xc6245`, and the epilogue.
 *
 * The function builds `%s\config\controls.cfg` with the base path at
 * `DAT_00007edc` and opens the file with the mode `rb`.  It reads three blocks:
 * six 28-byte records into `DAT_000197d8`, forty 28-byte records into
 * `DAT_000192a4`, and ten 4-byte records into `DAT_00019704`.  Then it closes
 * the handle.
 *
 * The function has no error path.  When the open fails, the handle stays zero
 * and the three reads receive the invalid handle.
 */
extern char DAT_00007edc[];
extern char DAT_000197d8[];
extern char DAT_000192a4[];
extern char DAT_00019704[];
extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(char *path, char *mode);
extern int fread(void *buffer, int size, int count, int stream);
extern int fclose(int stream);

void controls_load(void)
{
    char path[0x80];
    int handle;

    sprintf(path, "%s\\config\\controls.cfg", DAT_00007edc);
    handle = fopen(path, "rb");
    fread(DAT_000197d8, 0x1c, 6, handle);
    fread(DAT_000192a4, 0x1c, 0x28, handle);
    fread(DAT_00019704, 4, 0xa, handle);
    fclose(handle);
}
