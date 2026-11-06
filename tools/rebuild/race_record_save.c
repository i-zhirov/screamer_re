/* The rebuilt `race_record_save` module.
 *
 * The original sits at linear 0x2e82c in STARTH.EXE and holds 347 instructions
 * and 1185 bytes.  The record keeps the 1228-byte extent and sets no function
 * size.
 *
 * The function runs after a race.  It reads the two best-time values at
 * `DAT_0011d26c` and `DAT_0011d268` and the selected track index.  It inserts
 * the lap time into the five-slot lap table at `DAT_00120430` and, when the
 * record field `DAT_0011d2b0` is 2, the race time into the five-slot race table
 * at `DAT_00120610`.  Each table holds six 0x50-byte records: five floats at
 * +0x00, five 4-byte names at +0x14, five dwords at +0x28, and five dwords at
 * +0x3c for the race table.  The entries stay in ascending time order.
 *
 * A successful race-table insert also saves the replay.  The function builds
 * `%s\replays\temp.rpl`, checks that the file exists, selects the first free
 * replay number from the +0x28 flags, builds
 * `%s\replays\REPLAY%d%d.RPL` from the track index and the number, deletes the
 * old target, and renames the temporary file.
 *
 * The input word at `DAT_00118e1c` gates the function: the bits 0x1020 skip
 * the whole save.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned short DAT_00118e1c;
extern float DAT_0011d26c;
extern float DAT_0011d268;
extern int DAT_0011d2b0;
extern int DAT_0011d260;
extern char DAT_00007edc[];
extern char DAT_001203cc[];
extern unsigned char DAT_00120430[];
extern unsigned char DAT_00120610[];
extern int sprintf(char *buffer, const char *format, ...);
extern int find_first(char *path, void *data);
extern void delete_file(char *path);
extern void rename_file(char *old_path, char *new_path);

static void copy_text(char *destination, const char *source)
{
    int index;

    index = 0;
    for (;;) {
        destination[index] = source[index];
        if (source[index] == 0) {
            break;
        }
        index = index + 1;
    }
}

void race_record_save(int index)
{
    unsigned char *lap;
    unsigned char *race;
    char temp_path[0x80];
    char replay_path[0x80];
    char find_data[0x2c];
    int count;
    int slot;
    int counter;

    if ((DAT_00118e1c & 0x1020) != 0) {
        return;
    }

    lap = DAT_00120430 + index * 0x50;
    count = 0;
    for (slot = 0; slot < 5; slot++) {
        if (DAT_0011d26c <= *(float *)(lap + slot * 4)) {
            break;
        }
        count = count + 1;
    }
    if (count < 5) {
        for (slot = 3; slot >= count; slot--) {
            *(float *)(lap + (slot + 1) * 4) =
                *(float *)(lap + slot * 4);
            *(int *)(lap + 0x28 + (slot + 1) * 4) =
                *(int *)(lap + 0x28 + slot * 4);
            copy_text((char *)(lap + 0x14 + (slot + 1) * 4),
                      (char *)(lap + 0x14 + slot * 4));
        }
        *(float *)(lap + count * 4) = DAT_0011d26c;
        *(int *)(lap + 0x28 + count * 4) = 1;
        copy_text((char *)(lap + 0x14 + count * 4), DAT_001203cc);
    }

    if (DAT_0011d2b0 != 2) {
        return;
    }

    race = DAT_00120610 + index * 0x50;
    count = 0;
    for (slot = 0; slot < 5; slot++) {
        if (DAT_0011d268 <= *(float *)(race + slot * 4)) {
            break;
        }
        count = count + 1;
    }
    if (count >= 5) {
        return;
    }
    for (slot = 3; slot >= count; slot--) {
        *(float *)(race + (slot + 1) * 4) =
            *(float *)(race + slot * 4);
        *(int *)(race + 0x28 + (slot + 1) * 4) =
            *(int *)(race + 0x28 + slot * 4);
        *(int *)(race + 0x3c + (slot + 1) * 4) =
            *(int *)(race + 0x3c + slot * 4);
        copy_text((char *)(race + 0x14 + (slot + 1) * 4),
                  (char *)(race + 0x14 + slot * 4));
    }
    *(float *)(race + count * 4) = DAT_0011d268;
    *(int *)(race + 0x28 + count * 4) = 0;
    *(int *)(race + 0x3c + count * 4) = DAT_0011d260;
    copy_text((char *)(race + 0x14 + count * 4), DAT_001203cc);

    sprintf(temp_path, "%s\\replays\\temp.rpl", DAT_00007edc);
    if (find_first(temp_path, find_data) == -1) {
        return;
    }

    counter = 1;
    for (slot = 0; slot < 5; slot++) {
        if (*(int *)(race + 0x28 + slot * 4) == counter) {
            slot = -1;
            counter = counter + 1;
        }
    }
    *(int *)(race + 0x28 + count * 4) = counter;

    sprintf(replay_path, "%s\\replays\\REPLAY%d%d.RPL", DAT_00007edc,
            index, counter);
    delete_file(replay_path);
    rename_file(temp_path, replay_path);
}
