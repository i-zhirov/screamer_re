/* The rebuilt `traffic_collision_pass` module.
 *
 * The original sits at linear 0x175dc in STARTH.EXE and holds 165 instructions
 * and 587 bytes up to the `ret` at 0x17826.  The next function starts at
 * 0x17828, so the store record keeps the 588-byte extent and sets no function
 * size.  The identity check uses 165 instructions.
 *
 * The only direct caller at 0x15b6c is the traffic effect group at 0x15b44,
 * which the traffic view group at 0x201d0 calls at 0x20268.  The frame update
 * at 0x1ff70 selects that group through the mode byte `DAT_0011d260` bit 0x10.
 * The group first calls the nearby-record marker at 0x12424 and then this
 * function; it calls the clear step at 0x124c0 afterwards.  The car path has
 * the same shape at 0x15b08 with the car proximity pass at 0x17408, so the
 * record takes the section `The vehicle state chain`.
 *
 * The function clears the contact mask at `+0x2fc` and returns at once when the
 * state field `+0x148` is not zero.  Otherwise it walks the 0x2a8-byte traffic
 * records at `DAT_00113cd8` while the index stays below the count
 * `DAT_00115218`, and it skips a record whose first field is negative.
 *
 * A record whose flag field `+0x14` holds the bit of the player index
 * `DAT_0011d250` takes the velocity branch: the function calls the push helper
 * at 0x17b54 with the zero flag and copies the negated record velocity
 * `+0x30` and `+0x38` to `+0x550` and `+0x558`.
 *
 * Otherwise the function acts only on a record in the nearby set `+0x104` and
 * not flagged at `+0x16` bit 0.  It sets the contact distance at `+0x560` to
 * the player radius `+0x544` plus the traffic radius `+0x2a4`.  It then tests
 * the two player body points at `+0x52c` and `+0x534` against the two traffic
 * points at `+0x28c` and `+0x294`, in 12-byte steps.  Each pair stores its
 * deltas at `+0x564` and `+0x56c` and its truncated distance at `+0x55c`.
 *
 * When a distance stays below the contact distance, the function calls the push
 * helper with the set flag, marks the traffic index in the contact mask, and
 * sets the contact flag.  The square-root call goes through 0x22bec and the
 * truncating helper 0x4d9f4.  The last step filters `+0x7c4` toward the contact
 * flag through `filter_update` at 0x22b80 with the rate 50.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sqrt(double value);
extern void traffic_push_response(char *self, int push);
extern void filter_update(float *p, float f, int i);

extern int DAT_00115218;
extern unsigned char DAT_00113cd8[];
extern unsigned char DAT_0011d250;

void traffic_collision_pass(char *self)
{
    int touched;
    int index;
    int first;
    int second;
    char *record;
    int dist2;

    *(int *)(self + 0x2fc) = 0;
    touched = 0;
    if (*(int *)(self + 0x148) == 0) {
        record = (char *)DAT_00113cd8;
        for (index = 0; index < DAT_00115218; index++, record += 0x2a8) {
            if (*(int *)record < 0) {
                continue;
            }
            if (*(int *)(record + 0x14) & (1 << DAT_0011d250)) {
                traffic_push_response(self, 0);
                *(float *)(self + 0x550) = -*(float *)(record + 0x30);
                *(float *)(self + 0x558) = -*(float *)(record + 0x38);
                continue;
            }
            if (!(*(int *)(self + 0x104) & (1 << index))) {
                continue;
            }
            if (*(unsigned char *)(record + 0x16) & 1) {
                continue;
            }
            *(int *)(self + 0x560) = *(int *)(self + 0x544)
                + *(int *)(record + 0x2a4);
            for (first = 0; first < 2; first++) {
                for (second = 0; second < 2; second++) {
                    *(int *)(self + 0x564) =
                        *(int *)(self + 0x52c + first * 12)
                        - *(int *)(record + 0x28c + second * 12);
                    *(int *)(self + 0x56c) =
                        *(int *)(self + 0x534 + first * 12)
                        - *(int *)(record + 0x294 + second * 12);
                    dist2 = *(int *)(self + 0x564) * *(int *)(self + 0x564)
                        + *(int *)(self + 0x56c) * *(int *)(self + 0x56c);
                    *(int *)(self + 0x55c) = (int)sqrt((double)dist2);
                    if (*(int *)(self + 0x55c) < *(int *)(self + 0x560)) {
                        traffic_push_response(self, 1);
                        *(int *)(self + 0x2fc) |= 1 << *(unsigned char *)record;
                        touched = 1;
                    }
                }
            }
        }
    }
    filter_update((float *)(self + 0x7c4), (float)touched, 50);
}
