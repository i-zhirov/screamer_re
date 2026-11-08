/* The rebuilt `replay_load` module.
 *
 * The original sits at linear 0x1b840 in STARTH.EXE and holds 84 instructions
 * and 306 bytes up to the `ret` at 0x1b971.  The next function starts at
 * 0x1b974, so the store record keeps the 308-byte extent and sets no separate
 * function size.  The identity check uses 84 instructions.
 *
 * Two callers pass the replay name buffer DAT_0001d278 to this function:
 * race_data_load at 0x131a8 calls it at 0x13224 with the flag 0, and the mode 1
 * start at 0x12c0c calls it at 0x12cdc with the flag 1.  Both calls run only
 * when the replay flag DAT_0018e30 is set.  The sibling writer at 0x1b974 uses
 * the same record layout for the `%s\replays\%s.atk` member, so the module
 * takes the section The replay records.
 *
 * The function builds the member name `%s\replays\%s.rpl` from the base path
 * buffer DAT_00007edc and the name through `sprintf`, and opens it through
 * `fopen` with the mode `rb`.  A failed open returns 1.
 *
 * The function reads four blocks through `fread`:
 *
 *     6 records of 0x1c bytes into DAT_000197d8
 *     0x28 records of 0x1c bytes into DAT_000192a4
 *     0xa records of 4 bytes into DAT_00019704
 *     one record of 0x18 bytes into the header DAT_00018e74
 *
 * The header holds the frame count and five setup words.  The function copies
 * them to DAT_00018e70, DAT_0001d2a0, DAT_0001d2b0, DAT_0001d2b4,
 * DAT_0001d260, and DAT_0001d274.
 *
 * When the flag is not zero, the function reads the frame records into the
 * buffer pointer DAT_00018e5c.  The read needs a non-zero pointer and a frame
 * count below the limit DAT_00018e68.  Otherwise the result becomes 1.  The
 * function closes the stream through `fclose` and returns the result: 0 on
 * success and 1 on failure.
 *
 * The name `replay_load` comes from the `%s\replays\%s.rpl` member, the replay
 * record layout, and the replay flag at both callers.  The original has no
 * symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses no floating-point instruction, so it needs no FPU stub.
 */
extern unsigned char DAT_00007edc[];
extern int DAT_00018e5c;
extern int DAT_00018e68;
extern int DAT_00018e70;
extern int DAT_00018e74[];
extern int DAT_0001d2a0;
extern int DAT_0001d2b0;
extern int DAT_0001d2b4;
extern int DAT_0001d260;
extern int DAT_0001d274;
extern unsigned char DAT_000197d8[];
extern unsigned char DAT_000192a4[];
extern unsigned char DAT_00019704[];

extern int sprintf(char *buffer, const char *format, ...);
extern void *fopen(const char *path, const char *mode);
extern unsigned int fread(void *ptr, unsigned int size, unsigned int count,
    void *stream);
extern int fclose(void *stream);

int replay_load(char *name, int load_frames)
{
    char path[0x74];
    void *stream;
    int result;

    sprintf(path, "%s\\replays\\%s.rpl", DAT_00007edc, name);
    stream = fopen(path, "rb");
    if (stream == 0) {
        return 1;
    }

    fread(DAT_000197d8, 0x1c, 6, stream);
    fread(DAT_000192a4, 0x1c, 0x28, stream);
    fread(DAT_00019704, 4, 0xa, stream);
    fread(DAT_00018e74, 0x18, 1, stream);

    DAT_00018e70 = DAT_00018e74[0];
    DAT_0001d2a0 = DAT_00018e74[1];
    DAT_0001d2b0 = DAT_00018e74[2];
    DAT_0001d2b4 = DAT_00018e74[3];
    DAT_0001d260 = DAT_00018e74[4];
    DAT_0001d274 = DAT_00018e74[5];

    result = 0;
    if (load_frames != 0) {
        if (DAT_00018e5c != 0 && DAT_00018e70 < DAT_00018e68) {
            fread((void *)DAT_00018e5c, 0x14, DAT_00018e70, stream);
        } else {
            result = 1;
        }
    }

    fclose(stream);
    return result;
}
