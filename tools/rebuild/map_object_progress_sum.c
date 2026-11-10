/* The rebuilt `map_object_progress_sum` module.
 *
 * The original sits at linear 0x2b3e5 in STARTH.EXE and holds 39 instructions
 * and 107 bytes up to the `ret` at 0x2b44f.  The next function starts at
 * 0x2b450, so the store record keeps the 107-byte extent and sets no separate
 * function size.  The identity check uses 39 instructions.
 *
 * The function walks the 0x50 map object records at `DAT_001db78`, each 0x64
 * bytes long.  A record with the field `+0x08` equal to -1 holds no object, so
 * the function skips it.  For a busy record the function adds the progress
 * value `+0x58` to a float total.  The function returns the total.
 *
 * The same records carry the menu text.  `map_object_place` at 0x2b044 sets the
 * progress to 1.0, `map_object_update` at 0x2b9cc decays the progress, and
 * `map_object_finish` at 0x2b494 resets the progress to zero.  The sum over the
 * active records therefore measures how much of the menu animation remains, so
 * the module records the name `map_object_progress_sum`.  The original has no
 * symbol, so the record keeps the name with the confidence `inferred`.  The
 * function sits in the section race_frame callees.
 *
 * The function makes no call, so it needs no case-insensitive compare and no
 * math helper.  It uses the x87 stack for the float add, so the module links
 * the shared `fpu_stub.asm` for the run-time symbols `__8087`,
 * `__init_387_emulator`, and `_fltused_`.  The original has the EBP frame, so
 * the module builds with `-of+`.
 */
extern char DAT_001db78[];

#define RECORD_SIZE 0x64
#define RECORD_COUNT 0x50
#define OBJECT_OFFSET 0x08
#define PROGRESS_OFFSET 0x58

float map_object_progress_sum(void)
{
    char *record;
    float total;
    int index;

    total = 0.0f;
    for (index = 0; index < RECORD_COUNT; index++) {
        record = DAT_001db78 + index * RECORD_SIZE;
        if (*(int *)(record + OBJECT_OFFSET) != -1) {
            total = total + *(float *)(record + PROGRESS_OFFSET);
        }
    }
    return total;
}
