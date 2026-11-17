/* The rebuilt `opponent_records_init` module.
 *
 * The original sits at linear 0x15378 in STARTH.EXE and holds 70 instructions
 * and 229 bytes up to the `ret` at 0x1545c.  The next function starts at
 * 0x15460, so the store record keeps the 232-byte extent and sets no separate
 * function size.  The identity check uses 70 instructions.
 *
 * The only direct caller at 0x13564 is race_track_load at 0x13540, which
 * race_session_start at 0x128e0 calls at 0x1294e.  The caller passes the load
 * value and drops a non-zero result.
 *
 * The function reads the difficulty flag DAT_0011d2b8.  A value below 1 stores
 * the clear flag DAT_0010cf58 and returns 0 at once, so the opponent update
 * stays off.  A value of 1 or more stores the set flag and walks the
 * DAT_0010f178 opponent records of 0xbc bytes at DAT_0010f180.
 *
 * A non-zero load value loads the opponent object first through the object
 * table at 0x6530 and the slot at record + 0x18.  A non-zero result returns at
 * once.  For every record the function then copies the track field at +0x10 to
 * the path field at +0x44, takes the state entry at
 * DAT_0010f174 + track * 0x24 + 0x20 into +0x48, clears +0x4c and +0x50, and
 * sets the sound field at +0x38 to -1.
 *
 * After the loop the function runs opponent_update at 0x20954 with 1 and
 * returns 0.
 *
 * The opponent record array, the load path, and the closing update support the
 * name `opponent_records_init`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original uses no floating-point instruction and calls no FPU helper.
 */
#define OPPONENT_RECORD_STRIDE 0xbc
#define OPPONENT_TRACK_FIELD 0x10
#define OPPONENT_SOUND_FIELD 0x38
#define OPPONENT_PATH_FIELD 0x44
#define OPPONENT_STATE_FIELD 0x48
#define OPPONENT_CLEAR_FIELD_A 0x4c
#define OPPONENT_CLEAR_FIELD_B 0x50
#define OPPONENT_SLOT_BIAS 0x18
#define OPPONENT_STATE_STRIDE 0x24
#define OPPONENT_STATE_BIAS 0x20

extern int DAT_0011d2b8;
extern int DAT_0010cf58;
extern int DAT_0010f178;
extern char DAT_0010f180[];
extern char *DAT_0010f174;
extern char DAT_00006530[];

extern int object_lookup(char *table, char *record, char *slot);
extern void opponent_update(int value);

int opponent_records_init(int load)
{
    int index;
    char *record;
    int result;

    DAT_0010cf58 = DAT_0011d2b8 >= 1;
    if (DAT_0011d2b8 < 1) {
        return 0;
    }
    for (index = 0; index < DAT_0010f178; index++) {
        record = DAT_0010f180 + index * OPPONENT_RECORD_STRIDE;
        if (load != 0) {
            result = object_lookup(DAT_00006530, record,
                record + OPPONENT_SLOT_BIAS);
            if (result != 0) {
                return result;
            }
        }
        *(int *)(record + OPPONENT_PATH_FIELD) =
            *(int *)(record + OPPONENT_TRACK_FIELD);
        *(int *)(record + OPPONENT_STATE_FIELD) =
            *(int *)(DAT_0010f174
                + *(int *)(record + OPPONENT_PATH_FIELD)
                    * OPPONENT_STATE_STRIDE
                + OPPONENT_STATE_BIAS);
        *(int *)(record + OPPONENT_CLEAR_FIELD_A) = 0;
        *(int *)(record + OPPONENT_CLEAR_FIELD_B) = 0;
        *(int *)(record + OPPONENT_SOUND_FIELD) = -1;
    }
    opponent_update(1);
    return 0;
}
