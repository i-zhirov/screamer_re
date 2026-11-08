/* The rebuilt `car_proximity_pass` module.
 *
 * The original sits at linear 0x17408 in STARTH.EXE and holds 133 instructions
 * and 466 bytes up to the `ret` at 0x175d9.  The next function starts at
 * 0x175dc, so the store record keeps the 468-byte extent and sets no function
 * size.  The identity check uses 133 instructions.
 *
 * The only direct caller at 0x15b30 is the player effect group at 0x15b08.
 * That group first calls the nearby marker at 0x12378, then this function, and
 * then the opponent start at 0x124c0.  The traffic path has the same shape at
 * 0x15b44 with the traffic collision pass at 0x175dc.  The record therefore
 * takes the section `The vehicle state chain`.
 *
 * The function returns at once when the state field `+0x148` is not zero.
 * Otherwise it walks the 0x590-byte car records at `DAT_00110ab8` while the
 * index stays below the count `DAT_0010f8ec`.
 *
 * For each record the function sets the contact distance at `+0x560` to the
 * player radius `+0x544` plus the record radius `+0x44c`.  The bit of the
 * index in the nearby mask at `+0x104` selects the record, the physics gate at
 * `+0x414` must be set, and the record state at `+0x2f8` must be zero.
 *
 * The function then tests the two 12-byte player body points at `+0x52c` and
 * `+0x534` against the two record body points at `+0x434` and `+0x43c`.  Each
 * pair stores its deltas at `+0x564` and `+0x56c`, squares and adds them, takes
 * the square root through 0x22bec, truncates it toward zero through 0x4d9f4,
 * and stores the distance at `+0x55c`.  A distance below the contact distance
 * calls the collision response at 0x17828 with the player in `eax` and the
 * record in `edx`, sets the contact flag, and ends the second-point pass for
 * that first point.
 *
 * The last step filters `+0x7c4` toward the contact flag through
 * `filter_update` at 0x22b80 with the rate 50.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * module takes the square root and truncates the distance, so the local
 * `car_proximity_pass_fpu.asm` supplies `sqrt` and a truncating `__CHP`.
 */
extern double sqrt(double value);
extern void car_collision_response(char *self, char *other);
extern void filter_update(float *value, float target, int index);

extern int DAT_0010f8ec;
extern unsigned char DAT_00110ab8[];

void car_proximity_pass(char *self)
{
    int found;
    int index;
    int first;
    int second;
    char *record;
    int distance;

    found = 0;
    if (*(int *)(self + 0x148) == 0) {
        record = (char *)DAT_00110ab8;
        for (index = 0; index < DAT_0010f8ec; index++, record += 0x590) {
            *(int *)(self + 0x560) = *(int *)(self + 0x544)
                + *(int *)(record + 0x44c);
            if (!(*(int *)(self + 0x104) & (1 << index))) {
                continue;
            }
            if (*(int *)(record + 0x414) == 0) {
                continue;
            }
            if (*(int *)(record + 0x2f8) != 0) {
                continue;
            }
            for (first = 0; first < 2; first++) {
                for (second = 0; second < 2; second++) {
                    *(int *)(self + 0x564) =
                        *(int *)(self + 0x52c + first * 12)
                        - *(int *)(record + 0x434 + second * 12);
                    *(int *)(self + 0x56c) =
                        *(int *)(self + 0x534 + first * 12)
                        - *(int *)(record + 0x43c + second * 12);
                    distance = *(int *)(self + 0x564)
                        * *(int *)(self + 0x564)
                        + *(int *)(self + 0x56c)
                        * *(int *)(self + 0x56c);
                    *(int *)(self + 0x55c) = (int)sqrt((double)distance);
                    if (*(int *)(self + 0x55c) < *(int *)(self + 0x560)) {
                        found = 1;
                        car_collision_response(self, record);
                        break;
                    }
                }
            }
        }
    }
    filter_update((float *)(self + 0x7c4), (float)found, 0x32);
}
