/* The rebuilt `choice_save` module.
 *
 * The original sits at linear 0x1be6c in STARTH.EXE and holds 54 instructions
 * and 203 bytes up to the `ret` at 0x1bf36.  The next function starts at
 * 0x1bf38, so the store record does not set a function size.  The identity
 * check uses 54 instructions.
 *
 * The function builds `%s\config\choice.dat` with the base path at
 * `DAT_00007edc` through `sprintf`, opens the file with the mode `wb` through
 * `fopen`, and returns at once when the open fails.  On a successful open it
 * copies six race context fields into the record at `DAT_00203c4`, copies the
 * five option globals into the same record, writes 0x460 bytes through
 * `fwrite`, and closes the handle through `fclose`.
 *
 * The only callers are `race_cleanup` at 0x2a1b0 and `loading_screen` at
 * 0x2a364, which call it at the end of their work.  The file `choice.dat` and
 * the sibling module `choice_load` at 0x1bbc8 support the name `choice_save`.
 * The original holds no symbol for the function, so the module records the
 * name with the confidence inferred.
 *
 * The function uses no floating-point instruction and no assembly.  The
 * original has the EBP frame, so the module builds with `-of+`.
 */
extern char DAT_00007edc[];
extern int DAT_001d2a4;
extern int DAT_001d2a8;
extern int DAT_001d2bc;
extern int DAT_001d2c0;
extern int DAT_001d2c4;
extern int *DAT_00002dec;

struct choice_entry {
    int time[5];
    char name[5][4];
    int flag[5];
    char rest[0x14];
};

struct choice_record {
    int field_00;
    int field_04;
    char name[4];
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    char pad_24[0x18];
    int index[6];
    char pad_54[0x18];
    struct choice_entry lap[6];
    struct choice_entry race[6];
    int field_42c;
    int field_430;
    int field_434;
    int field_438;
    int field_43c;
    char pad_440[4];
    int field_444;
    int field_448;
    int field_44c;
    int field_450;
    int field_454;
    int field_458;
    int field_45c;
};

extern struct choice_record DAT_00203c4;
extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(char *path, char *mode);
extern int fwrite(void *buffer, int size, int count, int stream);
extern int fclose(int stream);

void choice_save(void)
{
    char path[100];
    int *context;
    int stream;

    sprintf(path, "%s\\config\\choice.dat", DAT_00007edc);
    stream = fopen(path, "wb");
    if (stream != 0) {
        context = DAT_00002dec;
        DAT_00203c4.field_0c = context[2];
        DAT_00203c4.field_10 = context[6];
        DAT_00203c4.field_14 = context[10];
        DAT_00203c4.field_438 = context[14];
        DAT_00203c4.field_43c = context[18];
        DAT_00203c4.field_45c = context[7];
        DAT_00203c4.field_450 = DAT_001d2a4;
        DAT_00203c4.field_430 = DAT_001d2a8;
        DAT_00203c4.field_448 = DAT_001d2c0;
        DAT_00203c4.field_444 = DAT_001d2bc;
        DAT_00203c4.field_44c = DAT_001d2c4;
        fwrite(&DAT_00203c4, 1, 0x460, stream);
        fclose(stream);
    }
}
