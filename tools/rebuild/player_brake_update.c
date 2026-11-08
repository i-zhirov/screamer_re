/* The rebuilt `player_brake_update` module.
 *
 * The original sits at linear 0x16150 in STARTH.EXE and holds 95 instructions
 * and 332 bytes up to the `ret` at 0x1629b.  The next function starts at
 * 0x1629c, so the store record keeps the 332-byte extent and sets no function
 * size.  The identity check uses 95 instructions.
 *
 * The only direct caller is the player update group at 0x15a80, which runs
 * this function at 0x15aa4 after the brake step at 0x160a4 and before
 * `player_steering_update` at 0x1629c.  The group runs on the player object at
 * object-3 offset 0xf91c, so the module takes the section The vehicle state
 * chain.
 *
 * The function reads the pedal axis DAT_00119884 and its centre sign
 * DAT_00119808.  When the sign matches and the axis is non-zero, it forms the
 * brake value +0x114 as the absolute axis over 1023.0, or filters the value
 * toward 1.0 through filter_update at 0x22b80 with the index 0x52.  Any other
 * input clears the brake value.  The sibling step at 0x160a4 forms the throttle
 * +0x144 from the axis DAT_00119880 in the same shape.
 *
 * The function then runs only when the state +0x128 stays below 0.1.  It
 * subtracts +0x17c * DAT_00118e04 * +0x114 from the road speed +0x50c, clamps
 * the road speed to zero, and, when the brake value times the road speed
 * passes the limit +0x190, either sets the ramp flag +0x170 or copies the
 * brake value to the deceleration flag +0x110.  The flag +0x170 stays set.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern void filter_update(float *value, float target, int index);

extern int DAT_00119884;
extern int DAT_00119808;
extern int DAT_001197fc;
extern int DAT_001197f8;
extern float DAT_00100714;
extern double DAT_00100718;
extern float DAT_00118e04;

void player_brake_update(void *object)
{
    char *self;
    int sign;
    int magnitude;
    float change;

    self = (char *)object;
    sign = DAT_00119884 < 0 ? -1 : 1;
    if (sign == DAT_00119808 && DAT_00119884 != 0) {
        if (DAT_001197fc == 0 && DAT_001197f8 == 1) {
            magnitude = DAT_00119884 < 0 ? -DAT_00119884 : DAT_00119884;
            *(float *)(self + 0x114) = (float)magnitude / DAT_00100714;
        } else {
            filter_update((float *)(self + 0x114), 1.0f, 0x52);
        }
    } else {
        *(int *)(self + 0x114) = 0;
    }

    if (*(float *)(self + 0x128) < DAT_00100718) {
        change = *(float *)(self + 0x17c) * DAT_00118e04
            * *(float *)(self + 0x114);
        *(float *)(self + 0x50c) = *(float *)(self + 0x50c) - change;
        if (*(float *)(self + 0x50c) < 0.0f) {
            *(int *)(self + 0x50c) = 0;
        }
        if (*(float *)(self + 0x114) * *(float *)(self + 0x50c)
                > *(float *)(self + 0x190)) {
            if (*(int *)(self + 0x170) == 0) {
                if (*(int *)(self + 0x2f0) != 0) {
                    *(int *)(self + 0x170) = 1;
                } else {
                    *(float *)(self + 0x110) = *(float *)(self + 0x114);
                }
            }
        }
    }
}
