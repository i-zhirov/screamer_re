/* The rebuilt `vis_tab_load` module.
 *
 * The original sits at linear 0x4d030 in STARTH.EXE and holds 31 instructions
 * and 101 bytes: the three register saves, the frame, the stream slot, the
 * `fopen` call at `0xc5fe5`, the three-call error path, the `fread` call at
 * `0xc607b`, the handle close at `0xc6245`, and the epilogue.
 *
 * The function opens `TABS\VIS.TAB` with the mode `rb`.  When the open fails, it
 * runs the error report, the BIOS video call, and the exit with the value 0x16.
 * Otherwise it reads the file into the buffer at `DAT_0007c1c4` with a size of
 * 1 and a count of 0x27500, closes the handle, and returns.
 */
extern int fopen(char *path, char *mode);
extern int fread(void *buffer, int size, int count, int stream);
extern int fclose(int stream);
extern void error_report(int code);
extern void bios_video(int code);
extern void exit(int code);
extern void *DAT_0007c1c4;

void vis_tab_load(void)
{
    int stream;

    stream = fopen("TABS\\VIS.TAB", "rb");
    if (stream == 0) {
        error_report(3);
        bios_video(0x16);
        exit(0x16);
    }
    fread(DAT_0007c1c4, 1, 0x27500, stream);
    fclose(stream);
}
