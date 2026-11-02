/* The rebuilt `object_proximity_check` module.
 *
 * The original sits at linear 0x1e30c in STARTH.EXE and holds 74 instructions
 * and 241 bytes up to the first `ret`: the five register saves, the frame, the
 * 0x14-byte local area, the two absolute-value guards, the two scaled and
 * truncated position deltas, the squared sum, the `sqrt` call at 0x22bec, the
 * scale by 10.0, and the float store with the gate store.
 *
 * The function clears the gate `+0x414` and returns while the distance between
 * the object position at `+0x498` and `+0x4a0` and the reference
 * `DAT_00113ccc` and `DAT_00113cd0` reaches 35000 units on either axis.  It
 * otherwise scales each delta by 0.1, truncates it to an integer, forms the
 * squared sum, calls `sqrt`, and scales the result by 10.0 into `+0x4e4`.  It
 * then sets the gate from the value against 35000.0.
 *
 * The constants arrive through the globals `proximity_scale`,
 * `proximity_radius`, and `proximity_limit`.  The harness sets the exact bit
 * patterns.  The original reads the constants from object 3 near offset 0xb88.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_00113ccc;
extern int DAT_00113cd0;
extern double proximity_scale;
extern double proximity_radius;
extern float proximity_limit;
extern double sqrt(double value);

void object_proximity_check(void *object)
{
    int dx;
    int dy;
    int sx;
    int sy;
    float distance;

    dx = *(int *)((char *)object + 0x498) - DAT_00113ccc;
    if (dx < 0) {
        dx = -dx;
    }
    if (dx >= 35000) {
        *(int *)((char *)object + 0x414) = 0;
        return;
    }
    dy = *(int *)((char *)object + 0x4a0) - DAT_00113cd0;
    if (dy < 0) {
        dy = -dy;
    }
    if (dy >= 35000) {
        *(int *)((char *)object + 0x414) = 0;
        return;
    }
    dx = DAT_00113ccc - *(int *)((char *)object + 0x498);
    dy = DAT_00113cd0 - *(int *)((char *)object + 0x4a0);
    sx = (int)((double)dx * proximity_scale);
    sy = (int)((double)dy * proximity_scale);
    distance = (float)(sqrt((double)(sx * sx + sy * sy)) * proximity_radius);
    *(float *)((char *)object + 0x4e4) = distance;
    *(int *)((char *)object + 0x414) =
        *(float *)((char *)object + 0x4e4) < proximity_limit;
}
