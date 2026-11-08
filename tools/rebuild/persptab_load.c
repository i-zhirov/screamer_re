/* The rebuilt `persptab_load` module.
 *
 * The original sits at linear 0x4cf94 in STARTH.EXE and holds 43 instructions
 * and 155 bytes: the three register saves, the frame, the stream slot, the
 * `fopen` call at `0xc5fe5`, the three-call error path, the `resource_read`
 * call at `0x2efc2`, the null guard with the `memory_error_report` call at
 * `0x2edad`, the `fread` call at `0xc607b`, the cell advance, the handle close
 * at `0xc6245`, and the epilogue.
 *
 * The function opens `TABS\persp01.tab` with the mode `rb`.  When the open
 * fails, it runs the BIOS video call, the error report, and the exit with the
 * value 0x16.  Otherwise it reads the `PERSPTAB` resource of 0x40740 bytes,
 * stores the pointer in `DAT_000214ec`, reads the file into that pointer, and
 * advances the cell by 0x40000, so the table holds two halves.  Then it closes
 * the handle.
 */
extern void *resource_read(void *resource, char *name, unsigned size);
extern void memory_error_report(void *resource);
extern int fopen(char *path, char *mode);
extern int fread(void *buffer, int size, int count, int stream);
extern int fclose(int stream);
extern void bios_video(int code);
extern void error_report(int code);
extern void exit(int code);
extern void *DAT_00021574;
extern char *DAT_000214ec;

void persptab_load(void)
{
    int stream;
    char *ptr;

    stream = fopen("TABS\\persp01.tab", "rb");
    if (stream == 0) {
        bios_video(3);
        error_report(0x16);
        exit(0x16);
    }
    ptr = resource_read(DAT_00021574, (char *)0x1747, 0x40740);
    DAT_000214ec = ptr;
    if (ptr == 0) {
        memory_error_report(DAT_00021574);
    }
    fread(DAT_000214ec, 1, 0x40740, stream);
    DAT_000214ec = DAT_000214ec + 0x40000;
    fclose(stream);
}
