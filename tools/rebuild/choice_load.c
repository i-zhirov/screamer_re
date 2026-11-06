/* The rebuilt `choice_load` module.
 *
 * The original sits at linear 0x1bbc8 in STARTH.EXE and holds 189 instructions
 * and 673 bytes up to the `ret` at 0x1be68.  The next function starts at
 * 0x1be6c, so the store record keeps the 676-byte extent and sets no function
 * size.  The identity check uses 189 instructions.
 *
 * The function builds `%s\config\choice.dat` with the base path at
 * `DAT_00007edc` through `sprintf`, opens the file with the mode `rb` through
 * `fopen`, and reads 0x460 bytes into the record at `DAT_00203c4` through
 * `fread`.  It copies six fields of the record to the race context at
 * `DAT_00002dec` and closes the handle.
 *
 * When the open fails, the function writes the default record.  It clears the
 * version fields, sets the three 0x6000 limits, the three 1 flags, and the two
 * 2 flags, and marks the six index slots with -1.  It writes the tag `GRA` at
 * `+0x08`.  It then fills the six lap records at `+0x6c` with the float bit
 * pattern 0x43340000 (180.0), the tag `GRA`, and a zero flag, and the six race
 * records at `+0x24c` with the bit pattern 0x44610000 (900.0) in the same
 * layout.  Each record holds five entries of 0x50 bytes: five time slots at
 * `+0x00`, five 4-byte tags at `+0x14`, and five flags at `+0x28`.  The time
 * slots have the type `int`, because the original stores the immediate values
 * and links no FPU run-time library.
 *
 * The callers are `race_start` at 0x27770 and `menu_space_init` at 0x29d08.
 * Both call the function in the setup path, and the record name comes from the
 * `choice.dat` file that the function builds and reads.  The original holds no
 * symbol for the function, so the module records the name with the confidence
 * inferred.
 */
extern char DAT_00007edc[];
extern int DAT_001d2a8;
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
extern int fread(void *buffer, int size, int count, int stream);
extern int fclose(int stream);

static void copy_tag(char *destination, const char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

void choice_load(void)
{
    char path[0x70];
    int index;
    int slot;
    int stream;

    sprintf(path, "%s\\config\\choice.dat", DAT_00007edc);
    stream = fopen(path, "rb");
    if (stream != 0) {
        fread(&DAT_00203c4, 1, 0x460, stream);
        DAT_00002dec[2] = DAT_00203c4.field_0c;
        DAT_00002dec[6] = DAT_00203c4.field_14;
        DAT_00002dec[10] = DAT_00203c4.field_18;
        DAT_00002dec[14] = DAT_00203c4.field_438;
        DAT_00002dec[18] = DAT_00203c4.field_43c;
        DAT_00002dec[7] = DAT_00203c4.field_45c;
        fclose(stream);
        return;
    }

    DAT_00203c4.field_20 = 0;
    DAT_00203c4.field_1c = 0;
    DAT_00203c4.field_18 = 0;
    DAT_00203c4.field_04 = 0;
    DAT_00203c4.field_00 = 0;
    DAT_00203c4.field_42c = 0;
    DAT_00203c4.field_434 = 0;
    DAT_00203c4.field_444 = 0x6000;
    DAT_00203c4.field_448 = 0x6000;
    DAT_00203c4.field_44c = 0x6000;
    DAT_00203c4.field_450 = 1;
    DAT_00203c4.field_438 = 1;
    DAT_00203c4.field_43c = 1;
    DAT_001d2a8 = 2;
    DAT_00203c4.field_430 = 2;
    DAT_00203c4.field_45c = 2;
    DAT_00203c4.field_458 = 0;
    DAT_00203c4.field_454 = 0;

    for (index = 0; index < 6; ++index) {
        DAT_00203c4.index[index] = -1;
    }
    copy_tag(DAT_00203c4.name, "GRA");

    for (index = 0; index < 6; ++index) {
        for (slot = 0; slot < 5; ++slot) {
            DAT_00203c4.lap[index].time[slot] = 0x43340000;
            copy_tag(DAT_00203c4.lap[index].name[slot], "GRA");
            DAT_00203c4.lap[index].flag[slot] = 0;
            DAT_00203c4.race[index].time[slot] = 0x44610000;
            copy_tag(DAT_00203c4.race[index].name[slot], "GRA");
            DAT_00203c4.race[index].flag[slot] = 0;
        }
    }
}
