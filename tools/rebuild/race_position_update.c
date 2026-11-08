/* The rebuilt `race_position_update` module.
 *
 * The original sits at linear 0x20428 in STARTH.EXE.  The body runs from the
 * prologue at 0x20428 to the second `ret` at 0x205c0, but the plan row and the
 * identity check use the first 86 instructions and 308 bytes up to the first
 * `ret` at 0x2055b.  The store record keeps the 412-byte extent and sets no
 * separate function size.  The identity check uses 86 instructions.
 *
 * The three direct callers are the view groups at 0x2003c, 0x201d0, and
 * 0x202c0.  The frame update at 0x1ff70 selects a view group through the mode
 * byte DAT_0011d260.  The 0x2003c group runs on the mode bits 0 and 1, the
 * 0x201d0 group on the mode bit 0x10, and the 0x202c0 group on the mode bit 4.
 *
 * The function returns at once when bit 0 of DAT_00118e1c is clear or when the
 * gate DAT_001100a0 is zero.  It saves the old position DAT_0000ffa4 in
 * DAT_0000feb0 and then selects one of three branches from the mode byte
 * DAT_0011d260:
 *
 *     bit 0x10  the traffic mode.  The function walks the 0x2a8-byte traffic
 *               records at DAT_00113cd8 up to the count DAT_0011d254.  It sets
 *               the position to one plus the number of records that are not
 *               empty and whose progress at +0x28 is above DAT_0000ff9c.
 *     bit 3     the race mode.  The function walks the 0x590-byte car records
 *               at DAT_00110ab8 up to the count DAT_0010f8ec.  A record whose
 *               place at +0x410 and progress at +0x408 disagree with the
 *               position and the threshold DAT_0000ff9c swaps its place with
 *               the position.  The function then returns.
 *     bit 4     the replay mode.  When the replay flag DAT_0011d264 is set, the
 *               function selects the ghost state at DAT_00118e98 plus the
 *               index DAT_00118ed4 ^ 1 times 0x24.  An active ghost compares
 *               DAT_0000ff9c with DAT_00118f08, and an inactive ghost compares
 *               the clock DAT_001100a8 with the ghost time at +0x10.  The
 *               result is the position 1 or 2.
 *
 * The old position DAT_0000feb0, the position DAT_0000ffa4, the traffic and
 * car records, and the ghost compare support the name race_position_update.
 * The original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses floating-point instructions, so the recipe adds the FPU run-time stubs.
 */
extern unsigned char DAT_0018e1c;
extern int DAT_000100a0;
extern int DAT_0000ffa4;
extern int DAT_0000feb0;
extern unsigned char DAT_0001d260;
extern int DAT_0001d254;
extern float DAT_0000ff9c;
extern int DAT_0000f8ec;
extern int DAT_0001d264;
extern int DAT_00018ed4;
extern float DAT_000100a8;
extern float DAT_00018f08;
extern char DAT_0013cd8[];
extern char DAT_0010ab8[];
extern char DAT_0018e98[];

void race_position_update(void)
{
    int mode;
    char *record;
    char *end;
    int index;
    int old;

    if ((DAT_0018e1c & 1) == 0) {
        return;
    }
    if (DAT_000100a0 == 0) {
        return;
    }

    mode = DAT_0001d260;
    DAT_0000feb0 = DAT_0000ffa4;

    if ((mode & 0x10) != 0) {
        end = DAT_0013cd8 + DAT_0001d254 * 0x2a8;
        record = DAT_0013cd8;
        DAT_0000ffa4 = 1;
        while (record < end) {
            if (*(int *)record != -1) {
                if (*(float *)(record + 0x28) > DAT_0000ff9c) {
                    DAT_0000ffa4++;
                }
            }
            record += 0x2a8;
        }
        return;
    }

    if ((mode & 3) != 0) {
        end = DAT_0010ab8 + DAT_0000f8ec * 0x590;
        record = DAT_0010ab8;
        while (record < end) {
            if ((*(int *)(record + 0x410) - DAT_0000ffa4)
                    * (DAT_0000ff9c - *(float *)(record + 0x408)) < 0.0f) {
                old = *(int *)(record + 0x410);
                *(int *)(record + 0x410) = DAT_0000ffa4;
                DAT_0000ffa4 = old;
                return;
            }
            record += 0x590;
        }
        return;
    }

    if ((mode & 4) != 0) {
        if (DAT_0001d264 == 0) {
            return;
        }
        index = (DAT_00018ed4 ^ 1) * 0x24;
        if (*(int *)(DAT_0018e98 + index) != 0) {
            DAT_0000ffa4 = (DAT_0000ff9c <= DAT_00018f08) ? 2 : 1;
        } else {
            DAT_0000ffa4 = (
                DAT_000100a8 >= *(float *)(DAT_0018e98 + index + 0x10)
            ) ? 2 : 1;
        }
    }
}
