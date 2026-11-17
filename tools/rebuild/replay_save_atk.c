/* The rebuilt `replay_save_atk` module.
 *
 * The original sits at linear 0x1b974 in STARTH.EXE and holds 67 instructions
 * and 213 bytes up to the `ret` at 0x1ba48.  The next function starts at
 * 0x1ba4c, so the store record keeps the 216-byte extent and sets no separate
 * function size.  The identity check uses 67 instructions.
 *
 * The only direct caller at 0x1fde4 is race_session_run at 0x1fd70.  It calls
 * this function with the map name DAT_0011d294 when the replay flag at
 * DAT_0018e1d holds bit 3.  The sibling loader at 0x1ba4c reads the same
 * `%s\replays\%s.atk` member and the same 0x24-byte ghost record, so the module
 * takes the section The replay records.
 *
 * The function selects the 0x24-byte ghost record at DAT_0018e8c through the
 * index DAT_0018ed4.  It builds the member name `%s\replays\%s.atk` from the
 * base path buffer DAT_00007edc and the name through `sprintf`, and it opens
 * the member through `fopen` with the mode `wb`.  A failed open returns 0x16.
 *
 * The function then updates the record: it copies the record field at +0x04 to
 * +0x08, clears the float field at +0x1c, stores the index at +0x10, and stores
 * DAT_001d274 at +0x14.  It writes one 0x24-byte header through `fwrite`.  When
 * the active field at +0x0c is not zero, it writes the keyframe records from
 * the pointer at +0x20: the count comes from the record field at +0x08 and the
 * size is 0x28 bytes.  The function closes the stream through `fclose` and
 * returns 0.
 *
 * The `.atk` member, the ghost record layout, the sibling loader at 0x1ba4c,
 * and the caller at 0x1fd70 support the name `replay_save_atk`.  The original
 * has no symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original copies the zero float at object-3 offset 0x100a8, so the harness
 * passes that cell through the global DAT_00100a8.
 */
typedef struct ghost_record {
    int count;
    int index;
    int current;
    int active;
    int slot;
    int flags;
    int field18;
    float clock;
    void *keyframes;
} ghost_record;

extern unsigned char DAT_00007edc[];
extern unsigned char DAT_0018e8c[];
extern int DAT_0018ed4;
extern int DAT_001d274;
extern float DAT_00100a8;

extern int sprintf(char *buffer, const char *format, ...);
extern void *fopen(const char *path, const char *mode);
extern unsigned int fwrite(const void *ptr, unsigned int size,
    unsigned int count, void *stream);
extern int fclose(void *stream);

int replay_save_atk(char *name)
{
    char path[0x70];
    void *stream;
    int index;
    ghost_record *record;

    index = DAT_0018ed4;
    record = (ghost_record *)(DAT_0018e8c + index * 0x24);

    sprintf(path, "%s\\replays\\%s.atk", DAT_00007edc, name);
    stream = fopen(path, "wb");
    if (stream == 0) {
        return 0x16;
    }

    record->current = record->index;
    record->clock = DAT_00100a8;
    record->slot = index;
    record->flags = DAT_001d274;

    fwrite(record, 0x24, 1, stream);
    if (record->active != 0) {
        fwrite(record->keyframes, 0x28, record->current, stream);
    }

    fclose(stream);
    return 0;
}
