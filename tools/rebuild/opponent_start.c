/* The rebuilt `opponent_start` module.
 *
 * The original sits at linear 0x124c0 in STARTH.EXE and holds 164 instructions
 * and 545 bytes up to the `ret` at 0x126e0.  The next function starts at
 * 0x126e4, so the store record keeps the 548-byte extent and sets no function
 * size.  The identity check uses 164 instructions.
 *
 * The callers at 0x15b38, 0x15b74, and 0x15ba0 are the vehicle groups at
 * 0x15b0c, 0x15b44, and 0x15b80.  The car path runs the car proximity pass at
 * 0x17408 first, and the traffic path runs the nearby marker at 0x12424 and
 * the traffic collision pass at 0x175dc first.  All three groups run this
 * function on the player object at object-3 offset 0xf91c, so the record takes
 * the section `The vehicle state chain`.
 *
 * The function walks the `DAT_0010d6f8` opponent records of 0x40 bytes at
 * `DAT_0010d700`.  A record with a set run flag `+0x38` is skipped.  Otherwise
 * the function tests the record position at `+0`, `+4`, and `+8` against the
 * player position at `+0x4b0`, `+0x4b4`, and `+0x4b8`.  A delta of 1500 units
 * or more on any axis skips the record.
 *
 * For a record in range, the function sets the contact distance at `+0x560` to
 * the player radius `+0x544` plus 100.  It then tests the two 12-byte player
 * body points at `+0x52c` and `+0x538` against the record position and stores
 * the truncated distance at `+0x55c`.  When the distance stays below the
 * contact distance by less than 100 units, the function starts the record:
 * the direction `+0x24` and `+0x2c` takes the sine and the negated cosine of
 * the player heading `+0x4cc`, the vertical speed `+0x34` takes the player
 * speed `+0x50c` times 0.02, and the speed `+0x30` takes the player speed
 * times 1.4.
 *
 * The first start, which the flag `+0x3c` marks, increments the counter
 * `+0x7b8` of the player.  The function then sets the flag `+0x3c` and the run
 * flag `+0x38` and calls `table_lookup` at 0x1f718 with 5, so a nearby
 * opponent starts with the player heading and a scaled speed.
 *
 * The original calls `sin` at 0x4da72, `cos` at 0x4da66, `sqrt` at 0x22bec,
 * and the truncating helper at 0x4d9f4.  This harness links no math library, so
 * the local assembly stub supplies `sin`, `cos`, and `sqrt` from the hardware
 * and makes `__CHP` truncate toward zero.  The two scales arrive through
 * globals, because the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sin(double value);
extern double cos(double value);
extern double sqrt(double value);

extern int table_lookup(int index);

extern int DAT_0010d6f8;
extern char DAT_0010d700[];
extern double opponent_vertical_speed_scale;
extern double opponent_speed_scale;

void opponent_start(char *self)
{
    char *record;
    char *end;
    int point;
    int dx;
    int dz;

    record = DAT_0010d700;
    end = record + DAT_0010d6f8 * 0x40;
    while (record < end) {
        if (*(int *)(record + 0x38) == 0) {
            dx = *(int *)(self + 0x4b0) - *(int *)record;
            if (dx < 0) {
                dx = -dx;
            }
            if (dx < 0x5dc) {
                dz = *(int *)(self + 0x4b8) - *(int *)(record + 8);
                if (dz < 0) {
                    dz = -dz;
                }
                if (dz < 0x5dc) {
                    dx = *(int *)(self + 0x4b4) - *(int *)(record + 4);
                    if (dx < 0) {
                        dx = -dx;
                    }
                    if (dx < 0x5dc) {
                        *(int *)(self + 0x560) =
                            *(int *)(self + 0x544) + 0x64;
                        for (point = 0; point < 2; point++) {
                            dx = *(int *)(self + 0x52c + point * 12)
                                - *(int *)record;
                            dz = *(int *)(self + 0x534 + point * 12)
                                - *(int *)(record + 8);
                            *(int *)(self + 0x55c) = (int)sqrt(
                                (double)(dx * dx + dz * dz));
                            if (*(int *)(self + 0x55c)
                                - *(int *)(self + 0x560) >= 0x64) {
                                continue;
                            }
                            *(float *)(record + 0x24) = (float)sin(
                                (double)*(float *)(self + 0x4cc));
                            *(float *)(record + 0x2c) = -(float)cos(
                                (double)*(float *)(self + 0x4cc));
                            *(float *)(record + 0x34) =
                                *(float *)(self + 0x50c)
                                * opponent_vertical_speed_scale;
                            *(float *)(record + 0x30) =
                                *(float *)(self + 0x50c)
                                * opponent_speed_scale;
                            if (*(int *)(record + 0x3c) == 0) {
                                (*(int *)(self + 0x7b8))++;
                            }
                            *(int *)(record + 0x3c) = 1;
                            *(int *)(record + 0x38) = 1;
                            table_lookup(5);
                        }
                    }
                }
            }
        }
        record = record + 0x40;
    }
}
