/* The rebuilt `name_entry_letter_update` module.
 *
 * The original sits at linear 0x2b660 in STARTH.EXE and holds 219 instructions
 * and 803 bytes up to the first `ret` at 0x2b982.  The second return block
 * continues to the shared epilogue at 0x2b9cb, so the record keeps the 876-byte
 * extent and sets no function size.
 *
 * The only caller at 0x2cec5 sits in the state 4 update at 0x2ce84.  That
 * update is the name entry screen update: the init at 0x2c430 builds the 26
 * A to Z records at DAT_001db78 and the input at 0x2893a selects the letter.
 * The update runs this function on each of the 0x50 records, so the module
 * records the name `name_entry_letter_update`.
 *
 * A record with the field +0x08 equal to -1 holds no object.  The function
 * binds the list name at 0x6350 to the record slot and returns.  Otherwise the
 * timer +0x60 falls by the frame delta DAT_0018e08.  While the timer stays at
 * or above zero, the function returns.  When the timer reaches zero, the
 * function clears it, refreshes the object list, and updates the object.
 *
 * The object slot is the table entry DAT_001d2d0 indexed by the record index,
 * and the object is the 0x1c-byte entry DAT_0010653c.  The object position
 * +0x08 and +0x10 take the record fields +0x40 and +0x48.  The height +0x0c
 * interpolates from the field +0x44 toward the field +0x38 with the square of
 * the progress +0x58.  The object angles +0x14, +0x18, and +0x1c take the
 * record angles +0x10, +0x14, and +0x18 scaled by 1024.0 / pi and masked with
 * 0x7ff.  The object frame +0x20 takes the record object id +0x08 scaled by
 * DAT_001fab8 plus 0x226.
 *
 * The progress +0x58 oscillates between zero and one.  The direction +0x5c
 * selects the step.  Above one, the progress clamps at one, the direction
 * clears, and the start field +0x44 scales by the constant; when the scaled
 * start falls below the target +0x38, the direction becomes -1 and the start
 * takes the target.  At zero, the progress clamps at zero and the direction
 * becomes +1.
 *
 * The angle +0x10 also steps.  With the record flag +0x0c set, the angle
 * rises; below the upper bound it stays, and above the bound it resets to
 * 0x400cbe4d.  With the flag clear, the angle falls; above the lower bound it
 * stays, and below the bound it resets to 0xbf20d97c.
 *
 * The original calls the list helpers at 0x117fc and 0x11850, the Watcom pow
 * helper at 0xc21e2, and the truncate helper at 0x4d9f4.  This harness links
 * no math library, so the local assembly stub supplies pow and makes __CHP
 * truncate toward zero.  The module calls pow only with the exponent 2.0, so
 * the stub squares the base and states that limit.  The float constants
 * arrive through globals, because the minimal image does not load the constant
 * segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern void object_list_refresh(void *table, char *name, int *slot);
extern void object_list_bind(void *table, char *name, int *slot);
extern double pow(double base, double exponent);

extern float DAT_0018e08;
extern int DAT_001d2d0[];
extern int DAT_001fab8;
extern char DAT_0010653c[];

extern double letter_angle_high;
extern double letter_angle_low;
extern double letter_progress_rate;
extern double letter_start_scale;
extern float letter_progress_decay;
extern float letter_angle_rate;
extern float letter_angle_scale;
extern float letter_pi;

void name_entry_letter_update(char *record)
{
    char *object;
    int slot;
    double span;
    float progress;

    if (*(int *)(record + 8) == -1) {
        object_list_bind((void *)0x6530, (char *)0x6350,
            (int *)((char *)DAT_001d2d0 + *(int *)record * 8));
        return;
    }

    if (*(float *)(record + 0x60) - DAT_0018e08 >= 0.0f)
        return;

    slot = DAT_001d2d0[*(int *)record * 2];
    *(float *)(record + 0x60) = 0.0f;
    object_list_refresh((void *)0x6530, (char *)0x6350,
        (int *)((char *)DAT_001d2d0 + *(int *)record * 8));

    object = *(char **)(DAT_0010653c + slot * 28);
    *(int *)(object + 0x20) = DAT_001fab8 * *(int *)(record + 8) + 0x226;
    *(int *)(object + 8) = *(int *)(record + 0x40);

    span = (double)(*(int *)(record + 0x38) - *(int *)(record + 0x44));
    *(int *)(object + 0xc) = (int)((double)*(int *)(record + 0x44)
        + span * pow((double)*(float *)(record + 0x58), 2.0));
    *(int *)(object + 0x10) = *(int *)(record + 0x48);

    *(int *)(object + 0x14) =
        (int)(*(float *)(record + 0x10) * letter_angle_scale / letter_pi)
        & 0x7ff;
    *(int *)(object + 0x18) =
        (int)(*(float *)(record + 0x14) * letter_angle_scale / letter_pi)
        & 0x7ff;
    *(int *)(object + 0x1c) =
        (int)(*(float *)(record + 0x18) * letter_angle_scale / letter_pi)
        & 0x7ff;

    if (*(int *)(record + 0x5c) != (int)0xbf800000) {
        if ((*(int *)(record + 0x5c) & 0x7fffffff) != 0) {
            progress = *(float *)(record + 0x58)
                + DAT_0018e08 * letter_progress_rate;
            *(float *)(record + 0x58) = progress;
            if (progress > 1.0f) {
                *(float *)(record + 0x58) = 1.0f;
                *(int *)(record + 0x5c) = 0;
                *(int *)(record + 0x44) = (int)(
                    (double)*(int *)(record + 0x44)
                    * letter_start_scale);
                if (*(int *)(record + 0x44)
                        - *(int *)(record + 0x38) < 0) {
                    *(int *)(record + 0x5c) = (int)0xbf800000;
                    *(int *)(record + 0x44) = *(int *)(record + 0x38);
                }
            }
        } else {
            progress = *(float *)(record + 0x58)
                - DAT_0018e08 * letter_progress_decay;
            *(float *)(record + 0x58) = progress;
            if (progress < 0.0f) {
                *(float *)(record + 0x58) = 0.0f;
                *(int *)(record + 0x5c) = (int)0x3f800000;
            }
        }
    }

    if (*(int *)(record + 0xc) != 0) {
        if (*(float *)(record + 0x10) == letter_angle_high)
            return;
        progress = *(float *)(record + 0x10)
            + DAT_0018e08 * letter_angle_rate;
        *(float *)(record + 0x10) = progress;
        if (progress > letter_angle_high) {
            *(int *)(record + 0x10) = (int)0x400cbe4d;
        }
    } else {
        if (*(float *)(record + 0x10) == letter_angle_low)
            return;
        progress = *(float *)(record + 0x10)
            - DAT_0018e08 * letter_angle_rate;
        *(float *)(record + 0x10) = progress;
        if (progress < letter_angle_low) {
            *(int *)(record + 0x10) = (int)0xbf20d97c;
        }
    }
}
