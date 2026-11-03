/* The rebuilt `object_collision` module.
 *
 * The original sits at linear 0x1e00c in STARTH.EXE and holds 193 instructions
 * and 765 bytes up to the first `ret`: the six register saves, the frame, the
 * 0x44-byte local area, the gate `DAT_00108058`, the target scan over the
 * 0x590-byte records at `DAT_00110ab8`, the two default ids, the rotation, the
 * three scaled and truncated values, and the epilogue.
 *
 * The function returns while `DAT_00108058` is 0.  When the state at `+0x414`
 * is set and the target at `+0x4ec` is -1, it scans the records while the
 * counter `DAT_00113cc8` is 3.  It skips the own record, a record with the
 * target -1, and a record whose distance at `+0x4e4` is not larger than the
 * object distance.  The first other record with a larger distance supplies
 * the target, clears its own target, and decrements the counter.  When the
 * counter stays below 3, the function takes the default id from the two-entry
 * table for the type at `+0x2e0` against 0xc, calls `table_lookup`, stores the
 * id, and increments the counter.
 *
 * A set target continues into the body.  The body copies the angle at `+0x418`
 * to `+0x4f4` while the low byte of `DAT_00118e1c` holds bit 0 or bit 2.  It
 * rotates the deltas at `+0x498` and `+0x4a0` against the reference
 * `DAT_00113ccc` and `DAT_00113cd0` with the angle `DAT_00113cd4`, forms the
 * three values from `+0x4f0`, `+0x4e4`, and `+0x4f4`, stores the truncated
 * distance at `+0x4f0`, and calls the three stores.  The stores are gated by
 * the low bit of `DAT_0011d261` and the flag at `+0x564`.
 *
 * A clear state releases the target through 0x1f880, stores the result, and
 * decrements the counter.
 *
 * The six constants arrive through globals.  The harness sets the exact bit
 * patterns.  The original reads the constants from object 3 at offsets 0xb64,
 * 0xb68, 0xb70, 0xb78, 0xb7c, and 0xb80, and the default ids from object 3 at
 * offset 0x1e30.
 *
 * The conversions to an integer use the Watcom helper at 0x4d9f4, which
 * truncates toward zero.  The module uses a local truncating FPU stub, because
 * the shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_00108058;
extern int DAT_00113cc8;
extern int DAT_0010f8ec;
extern unsigned char DAT_00110ab8[];
extern int DAT_00113ccc;
extern int DAT_00113cd0;
extern float DAT_00113cd4;
extern unsigned char DAT_00118e1c;
extern unsigned char DAT_0011d261;
extern unsigned int DAT_00118d94;
extern float collision_scale;
extern double collision_bias;
extern double collision_offset;
extern float collision_limit;
extern float collision_gain;
extern double collision_amplitude;
extern int collision_ids[2];

extern int table_lookup(int index);
extern void vector_rotate(int *out, int *in, float angle);
extern double sqrt(double value);
extern void object_store_a(int id, int value);
extern void object_store_b(unsigned int owner, int id, int value);
extern void object_store_c(unsigned int owner, int id, int value);
extern int object_release(int id);

void object_collision(char *object)
{
    char *record;
    char *end;
    int found;
    int value20;
    int value24;
    int value8;
    int sign;
    int delta[3];
    int rotated[3];
    double ratio;
    double angle;

    if (DAT_00108058 == 0) {
        return;
    }

    if (*(int *)(object + 0x414) != 0) {
        if (*(int *)(object + 0x578) != 0) {
            *(int *)(object + 0x4f0) = (int)*(float *)(object + 0x4e4);
        }
        found = -1;
        if (*(int *)(object + 0x4ec) == -1) {
            if (DAT_00113cc8 == 3) {
                record = (char *)DAT_00110ab8;
                end = record + DAT_0010f8ec * 0x590;
                while (record < end) {
                    if (*(int *)record == *(int *)object) {
                        record += 0x590;
                        continue;
                    }
                    if (*(int *)(record + 0x4ec) == -1) {
                        record += 0x590;
                        continue;
                    }
                    if (!(*(float *)(object + 0x4e4)
                          < *(float *)(record + 0x4e4))) {
                        record += 0x590;
                        continue;
                    }
                    found = *(int *)(record + 0x4ec);
                    *(int *)(record + 0x4ec) = -1;
                    DAT_00113cc8--;
                    break;
                }
            }
            if (DAT_00113cc8 < 3) {
                *(int *)(object + 0x4f0) = (int)*(float *)(object + 0x4e4);
                if (found < 0) {
                    found = table_lookup(
                        collision_ids[*(int *)(object + 0x2e0) == 0xc]);
                }
                *(int *)(object + 0x4ec) = found;
                DAT_00113cc8++;
            }
        }
    }

    if (*(int *)(object + 0x4ec) == -1) {
        return;
    }

    if (*(int *)(object + 0x414) == 0) {
        *(int *)(object + 0x4ec) = object_release(*(int *)(object + 0x4ec));
        DAT_00113cc8--;
        return;
    }

    if (DAT_00118e1c & 5) {
        *(float *)(object + 0x4f4) = *(float *)(object + 0x418);
    }

    delta[0] = DAT_00113ccc - *(int *)(object + 0x498);
    delta[2] = DAT_00113cd0 - *(int *)(object + 0x4a0);
    vector_rotate(rotated, delta, DAT_00113cd4);

    value20 = (int)(((double)*(int *)(object + 0x4f0)
        - (double)*(float *)(object + 0x4e4)) * collision_scale
        + (double)*(float *)(object + 0x4f4) * collision_bias
        + collision_offset);
    ratio = ((double)collision_limit
        - (double)*(float *)(object + 0x4e4)) / (double)collision_limit;
    value24 = (int)(ratio * ratio * collision_gain);

    angle = (double)rotated[0] / (double)*(float *)(object + 0x4e4);
    if (angle < 0.0) {
        angle = -angle;
    }
    if (rotated[0] >= 0) {
        sign = 1;
    } else {
        sign = -1;
    }
    value8 = (int)(sqrt(angle) * collision_amplitude * (double)sign) + 0x8000;
    *(int *)(object + 0x4f0) = (int)*(float *)(object + 0x4e4);

    if (!(DAT_0011d261 & 1) || *(int *)(object + 0x564) == 0) {
        object_store_a(*(int *)(object + 0x4ec), value20);
        object_store_b(DAT_00118d94, *(int *)(object + 0x4ec), value24);
        object_store_c(DAT_00118d94, *(int *)(object + 0x4ec), value8);
    }
}
