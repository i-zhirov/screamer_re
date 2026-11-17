/* The rebuilt `player_throttle_update` module.
 *
 * The original sits at linear 0x160a4 in STARTH.EXE.  The first path holds 38
 * instructions and 136 bytes up to the `ret` at 0x1612b.  The second path runs
 * from 0x1612c to the `ret` at 0x1614f, so the body reaches 172 bytes.  The
 * next function starts at 0x16150, so the store record keeps the 172-byte
 * extent and sets no separate function size.  The identity check uses 38
 * instructions.
 *
 * The only direct caller is the player update group at 0x15a80, which runs
 * this function at 0x15aa4 before `player_brake_update` at 0x16150 and
 * `player_steering_update` at 0x1629c.  The group runs on the player object at
 * object-3 offset 0xf91c, so the module takes the section The vehicle state
 * chain.
 *
 * The function keeps the throttle +0x144 only when it is not negative and when
 * the four state fields +0x4f4, +0x148, +0x110, and +0x170 stay clear.  The
 * test of +0x4f4 and +0x110 masks the sign bit, so a sign-only value counts as
 * zero.  The control byte DAT_00118e1d bit 0x40 also clears the throttle.  The
 * sign of the axis DAT_00119880 must then equal the centre sign
 * DAT_001197ec.
 *
 * When every gate passes, the throttle takes the absolute axis over 1023.0 at
 * DAT_00100710.  Any other input clears the throttle to zero.  The sibling
 * `player_brake_update` at 0x16150 forms the brake value in the same shape.
 *
 * The original has no symbol, so the record keeps the name with the confidence
 * inferred.  The original has the EBP frame, so the module builds with `-of+`.
 */
extern float DAT_00100710;
extern int DAT_00119880;
extern int DAT_001197ec;
extern unsigned char DAT_00118e1d;

void player_throttle_update(void *object)
{
    char *self;
    int magnitude;

    self = (char *)object;
    if (*(float *)(self + 0x144) < 0.0f
        || (*(int *)(self + 0x4f4) & 0x7fffffff) != 0
        || *(int *)(self + 0x148) != 0
        || (*(int *)(self + 0x110) & 0x7fffffff) != 0
        || *(int *)(self + 0x170) != 0
        || (DAT_00118e1d & 0x40) != 0
        || (DAT_00119880 < 0 ? -1 : 1) != DAT_001197ec) {
        *(int *)(self + 0x144) = 0;
    } else {
        magnitude = DAT_00119880 < 0 ? -DAT_00119880 : DAT_00119880;
        *(float *)(self + 0x144) = (float)magnitude / DAT_00100710;
    }
}
