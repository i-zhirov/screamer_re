/* The rebuilt `replay_save` module.
 *
 * The original sits at linear 0x1b738 in STARTH.EXE and holds 69 instructions
 * and 263 bytes up to the `ret` at 0x1b83e.  The next function starts at
 * 0x1b840, so the store record keeps the 264-byte extent and sets no separate
 * function size.  The identity check uses 69 instructions.
 *
 * The only direct caller at 0x1ff2d is race_session_run at 0x1fd70.  It calls
 * this function with the member name "temp" when the replay flag
 * DAT_00018e60 is set.  The sibling reader at 0x1b840 reads the same
 * `%s\replays\%s.rpl` member and the same block layout, and the writer at
 * 0x1b974 writes the `%s\replays\%s.atk` ghost records.  The module takes the
 * section The replay records.
 *
 * The function builds the member name `%s\replays\%s.rpl` from the base path
 * buffer DAT_00007edc and the name through `sprintf`, and opens it through
 * `fopen` with the mode `wb`.  A failed open returns 1.
 *
 * The function copies six setup words into the header DAT_00018e74 and then
 * writes five blocks through `fwrite`:
 *
 *     6 records of 0x1c bytes from DAT_000197d8
 *     0x28 records of 0x1c bytes from DAT_000192a4
 *     0xa records of 4 bytes from DAT_00019704
 *     one record of 0x18 bytes from the header DAT_00018e74
 *     DAT_00018e70 records of 0x14 bytes from the buffer DAT_00018e5c
 *
 * The header holds the frame count and five setup words: DAT_00018e70,
 * DAT_0001d2a0, DAT_0001d2b0, DAT_0001d2b4, DAT_0001d260, and
 * DAT_0001d274.  The reader at 0x1b840 restores those cells from the same
 * header.
 *
 * The `.rpl` member, the block layout of the reader at 0x1b840, and the caller
 * at 0x1fd70 support the name `replay_save`.  The original has no symbol, so
 * the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses no floating-point instruction, so it needs no FPU stub.
 */
extern unsigned char DAT_00007edc[];
extern int DAT_00018e5c;
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
extern unsigned int fwrite(const void *ptr, unsigned int size,
    unsigned int count, void *stream);
extern int fclose(void *stream);

int replay_save(char *name)
{
    char path[0x64];
    void *stream;

    sprintf(path, "%s\\replays\\%s.rpl", DAT_00007edc, name);
    stream = fopen(path, "wb");
    if (stream == 0) {
        return 1;
    }

    DAT_00018e74[0] = DAT_00018e70;
    DAT_00018e74[1] = DAT_0001d2a0;
    DAT_00018e74[2] = DAT_0001d2b0;
    DAT_00018e74[3] = DAT_0001d2b4;
    DAT_00018e74[4] = DAT_0001d260;
    DAT_00018e74[5] = DAT_0001d274;

    fwrite(DAT_000197d8, 0x1c, 6, stream);
    fwrite(DAT_000192a4, 0x1c, 0x28, stream);
    fwrite(DAT_00019704, 4, 0xa, stream);
    fwrite(DAT_00018e74, 0x18, 1, stream);
    fwrite((void *)DAT_00018e5c, 0x14, DAT_00018e70, stream);

    fclose(stream);
    return 0;
}
