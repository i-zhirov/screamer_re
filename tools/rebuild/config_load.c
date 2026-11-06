/* The rebuilt `config_load` module.
 *
 * The original sits at linear 0x1bf38 in STARTH.EXE and holds 46 instructions
 * and 155 bytes: the two register saves, the frame, the two frame slots, the
 * `fopen` call at `0xc5fe5`, the two-call error path, the three `fscanf` calls
 * at `0xc6618`, the flag store, the handle close at `0xc6245`, and the
 * epilogue.
 *
 * The function opens `screamer.cfg` with the mode `rt`.  When the open fails,
 * it reports `Files damaged. Re-install Screamer.` through `0xc656e` and exits
 * with the value -1.  Otherwise it reads three strings into `DAT_00007edc`,
 * `DAT_00007f10`, and `DAT_00007f44`, reads two characters, and sets the flag
 * `DAT_00007f78` to 1 when the second character is `n`.  Then it closes the
 * handle.
 */
extern char DAT_00007edc[];
extern char DAT_00007f10[];
extern char DAT_00007f44[];
extern int DAT_00007f78;
extern int fopen(char *path, char *mode);
extern int fscanf(int stream, const char *format, ...);
extern int fclose(int stream);
extern int report_message(char *message);
extern void exit(int code);

void config_load(void)
{
    char value;
    int stream;

    stream = fopen("screamer.cfg", "rt");
    if (stream == 0) {
        report_message("Files damaged. Re-install Screamer.");
        exit(-1);
    }
    fscanf(stream, "%s%s%s", DAT_00007edc, DAT_00007f10, DAT_00007f44);
    fscanf(stream, "%c", &value);
    fscanf(stream, "%c", &value);
    DAT_00007f78 = value == 'n';
    fclose(stream);
}
