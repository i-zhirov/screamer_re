/* The rebuilt `setup_load` module.
 *
 * The original sits at linear 0x1bb58 in STARTH.EXE and holds 31 instructions
 * and 111 bytes: the three register saves, the frame, the 0x68-byte path
 * buffer, the `sprintf` call at `0x4d32c`, the `fopen` call at `0xc5fe5`, the
 * two-call error path, the `fread` call at `0xc607b`, the handle close at
 * `0xc6245`, and the epilogue.
 *
 * The function builds `%s\config\setup.cfg` with the base path at
 * `DAT_00007edc`, opens the file with the mode `rb`, and reads 0x90 bytes into
 * the buffer at `DAT_00008058`.  When the open fails, it reports
 * `Run SETUP before.` through `0xc656e` and exits with the value -1.
 */
extern char DAT_00007edc[];
extern char DAT_00008058[];
extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(char *path, char *mode);
extern int fread(void *buffer, int size, int count, int stream);
extern int fclose(int stream);
extern int report_message(char *message);
extern void exit(int code);

void setup_load(void)
{
    char path[0x68];
    int stream;

    sprintf(path, "%s\\config\\setup.cfg", DAT_00007edc);
    stream = fopen(path, "rb");
    if (stream == 0) {
        report_message("Run SETUP before.");
        exit(-1);
    }
    fread(DAT_00008058, 1, 0x90, stream);
    fclose(stream);
}
