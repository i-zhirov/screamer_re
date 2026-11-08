/* The rebuilt `replay_load_atk` module.
 *
 * The original sits at linear 0x1ba4c in STARTH.EXE and holds 82 instructions
 * and 267 bytes up to the `ret` at 0x1bb56.  The next function starts at
 * 0x1bb58, so the store record keeps the 268-byte extent and sets no separate
 * function size.  The identity check uses 82 instructions.
 *
 * The only direct caller at 0x12ea6 is game_mode_4_start at 0x12e18.  It calls
 * this function at 0x12ea6 with the map name DAT_0011d294 when the replay flag
 * DAT_0011d264 is set.  The sibling writer at 0x1b974 uses the same 0x24-byte
 * ghost record and the same `%s\replays\%s.atk` member, so the module takes the
 * section The replay records.
 *
 * The function builds the member name `%s\replays\%s.atk` from the base path
 * buffer DAT_00007edc and the name through `sprintf`, and opens it through
 * `fopen` with the mode `rb`.  A failed open returns 0x16.
 *
 * The function reads one 0x24-byte header through `fread`.  The header field at
 * +0x10 selects the 0x24-byte ghost record at DAT_0018e8c.  The function stores
 * that field in DAT_0018ed4 and copies the header fields at +0x08, +0x0c,
 * +0x18, and +0x1c to the record fields at +0x08, +0x0c, +0x18, and +0x1c.
 * It moves the record field at +0x14 to DAT_001d274 and the record field at
 * +0x1c to DAT_001d26c.
 *
 * When the header field at +0x0c is not zero, the function reads the keyframe
 * records into the record pointer at +0x20.  The read needs a record count at
 * +0x00 above the header frame count at +0x08.  Otherwise the function clears
 * the record field at +0x0c.  The function closes the stream through `fclose`,
 * toggles the low byte of DAT_0018ed4, and returns 0.
 *
 * The `.atk` member, the ghost record layout, the sibling writer at 0x1b974,
 * and the caller at 0x12e18 support the name `replay_load_atk`.  The original
 * has no symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses no floating-point instruction, so it needs no FPU stub.
 */
typedef struct ghost_record {
    int count;
    int index;
    int current;
    int active;
    int slot;
    int flags;
    int field18;
    int clock;
    void *keyframes;
} ghost_record;

extern unsigned char DAT_00007edc[];
extern unsigned char DAT_0018e8c[];
extern int DAT_0018ed4;
extern int DAT_001d274;
extern int DAT_001d26c;

extern int sprintf(char *buffer, const char *format, ...);
extern void *fopen(const char *path, const char *mode);
extern unsigned int fread(void *ptr, unsigned int size, unsigned int count,
    void *stream);
extern int fclose(void *stream);

int replay_load_atk(char *name)
{
    char path[0x74];
    int header[9];
    void *stream;
    int index;
    ghost_record *record;

    sprintf(path, "%s\\replays\\%s.atk", DAT_00007edc, name);
    stream = fopen(path, "rb");
    if (stream == 0) {
        return 0x16;
    }

    fread(header, 1, 0x24, stream);

    index = header[4];
    DAT_0018ed4 = index;
    record = (ghost_record *)(DAT_0018e8c + index * 0x24);
    record->current = header[2];
    record->active = header[3];
    record->field18 = header[6];
    DAT_001d274 = record->flags;
    record->clock = header[7];
    DAT_001d26c = record->clock;

    if (header[3] != 0) {
        if (record->count > header[2]) {
            fread(record->keyframes, 0x28, header[2], stream);
        } else {
            record->active = 0;
        }
    }

    fclose(stream);
    *(unsigned char *)&DAT_0018ed4 ^= 1;
    return 0;
}
