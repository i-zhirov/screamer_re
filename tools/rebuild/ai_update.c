/* The rebuilt `ai_update` module.
 *
 * The original sits at linear 0x205c4 in STARTH.EXE and holds 260 instructions
 * and 911 bytes up to the `ret` at 0x20952.  The body holds the three register
 * saves, the frame, the 0x40-byte local block, the record loop, the motion
 * update, the collision branch, the stopped branch, the publish block, and the
 * epilogue.
 *
 * The function walks the `DAT_0010d6f8` records of 0x40 bytes at
 * `DAT_0010d700`.  A record runs only when the flag at `+0x38` is set.  A
 * running record marks the flag at `+0x20`, saves the position at `+0` and
 * `+8`, and advances the position by the speed at `+0x30` times the direction
 * at `+0x24` and `+0x2c` times the frame delta `DAT_00118e04`.
 *
 * The same product turns the three angles at `+0xc`, `+0x10`, and `+0x14` by
 * the doubles 0.008, 0.004, and 0.0015.  The speed falls by the frame delta
 * times 2000.0, and a negative speed becomes zero.  The position then goes to
 * the collision check at 0x11c00 through `DAT_00108038` and `DAT_00108040`.
 * A zero return marks a collision: the function saves the height at `+4` in
 * `DAT_0010c72c`, restores the saved position, flips the sign bit of the bytes
 * at `+0x27` and `+0x2f`, and scales the speed by 0.3.
 *
 * The vertical speed at `+0x34` falls by the frame delta times 500.0, and the
 * height at `+4` takes the truncated sum with the vertical speed.  When
 * `DAT_0010c72c` is not below the height, the height clamps to it, the
 * vertical speed reverses through the double 0.8, and a magnitude below 50.0
 * clears the vertical speed.
 *
 * When both speeds are zero, the record stops.  A collision sets the height to
 * -0x4e20.  Otherwise the function rotates the pair `DAT_0010c734` and
 * `DAT_0010c73c` through 0x2218c with the angle at `+0x10` plus -pi / 2, turns
 * the result into the angle at `+0xc`, and clears the run flag at `+0x38`.
 * A record with a non-zero speed keeps the run flag at `+0x38`.
 *
 * A record with the flag at `+0x20` publishes its position and height into the
 * target record from the pointer table `DAT_0010653c` at the 0x1c-byte entry
 * named by `+0x18`, and it scales the three angles by 1024.0 / pi and masks
 * them with 0x7ff into the words at `+0x14`, `+0x18`, and `+0x1c`.
 *
 * The calls at 0x4d9f4 truncate toward zero, so the module uses the local
 * truncating FPU stub.  The original has the EBP frame, so the module builds
 * with `-of+`.
 */
extern int DAT_0010d6f8;
extern char DAT_0010d700[];
extern float DAT_00100cc5;
extern float DAT_00100cc9;
extern float DAT_00100ccd;
extern float DAT_00100cd1;
extern float DAT_00100cd5;
extern float DAT_00100ca5;
extern float DAT_00100cb1;
extern double DAT_00100ca9;
extern double DAT_00100cb5;
extern double DAT_00100cbd;
extern double DAT_00100c8d;
extern double DAT_00100c95;
extern double DAT_00100c9d;
extern int DAT_0010c734;
extern int DAT_0010c73c;
extern int DAT_0010c72c;
extern unsigned int DAT_0010653c[];
extern int DAT_00108038;
extern int DAT_00108040;
extern float DAT_00118e04;
extern double fabs(double value);

extern void ai_rotate(int *out, int *in, float angle);
extern int ai_collision(int value);

void ai_update(void)
{
    char *record;
    char *end;
    int active;
    int saved0;
    int saved8;
    int *target;
    int pair[2];
    int rotated[2];

    record = DAT_0010d700;
    end = record + DAT_0010d6f8 * 0x40;
    while (record < end) {
        if (*(int *)(record + 0x38) != 0) {
            *(int *)(record + 0x20) = 1;
            saved0 = *(int *)record;
            saved8 = *(int *)(record + 8);
            *(int *)record = (int)(*(float *)(record + 0x30)
                * *(float *)(record + 0x24) * DAT_00118e04
                + *(int *)record);
            *(int *)(record + 8) = (int)(*(float *)(record + 0x30)
                * *(float *)(record + 0x2c) * DAT_00118e04
                + *(int *)(record + 8));
            *(float *)(record + 0xc) = *(float *)(record + 0xc)
                - *(float *)(record + 0x30) * *(float *)(record + 0x24)
                * DAT_00118e04 * DAT_00100c8d;
            *(float *)(record + 0x10) = *(float *)(record + 0x10)
                - *(float *)(record + 0x30) * *(float *)(record + 0x28)
                * DAT_00118e04 * DAT_00100c95;
            *(float *)(record + 0x14) = *(float *)(record + 0x14)
                - *(float *)(record + 0x30) * *(float *)(record + 0x2c)
                * DAT_00118e04 * DAT_00100c9d;
            *(float *)(record + 0x30) = *(float *)(record + 0x30)
                - DAT_00118e04 * DAT_00100ca5;
            if (*(float *)(record + 0x30) < 0.0f) {
                *(float *)(record + 0x30) = 0.0f;
            }
            DAT_00108038 = *(int *)record;
            DAT_00108040 = *(int *)(record + 8);
            active = ai_collision(0) == 0;
            if (active) {
                DAT_0010c72c = *(int *)(record + 4);
                *(int *)record = saved0;
                *(int *)(record + 8) = saved8;
                *(unsigned char *)(record + 0x27) ^= 0x80;
                *(unsigned char *)(record + 0x2f) ^= 0x80;
                *(float *)(record + 0x30) = *(float *)(record + 0x30)
                    * DAT_00100ca9;
            }
            *(float *)(record + 0x34) = *(float *)(record + 0x34)
                - DAT_00118e04 * DAT_00100cb1;
            *(int *)(record + 4) = (int)(*(int *)(record + 4)
                + *(float *)(record + 0x34));
            if (DAT_0010c72c >= *(int *)(record + 4)) {
                *(int *)(record + 4) = DAT_0010c72c;
                *(float *)(record + 0x34) = -*(float *)(record + 0x34)
                    * DAT_00100cb5;
                if (fabs((double)*(float *)(record + 0x34))
                    < DAT_00100cbd) {
                    *(float *)(record + 0x34) = 0.0f;
                }
            }
            if ((*(unsigned int *)(record + 0x30) & 0x7fffffff) == 0
                && (*(unsigned int *)(record + 0x34) & 0x7fffffff) == 0) {
                if (active) {
                    *(int *)(record + 4) = -0x4e20;
                } else {
                    pair[0] = DAT_0010c734;
                    pair[1] = DAT_0010c73c;
                    ai_rotate(rotated, pair, *(float *)(record + 0x10)
                        + DAT_00100cc5);
                    *(float *)(record + 0xc) = (float)rotated[0]
                        * DAT_00100cc9 / DAT_00100ccd + DAT_00100cd1;
                    *(int *)(record + 0x38) = active;
                }
            } else {
                *(int *)(record + 0x38) = 1;
            }
        }
        if (*(int *)(record + 0x20) != 0) {
            *(int *)(record + 0x20) = 0;
            target = (int *)DAT_0010653c[*(int *)(record + 0x18) * 7];
            target[2] = *(int *)record;
            target[3] = *(int *)(record + 4);
            target[4] = *(int *)(record + 8);
            target[5] = (int)(*(float *)(record + 0xc)
                * DAT_00100cd5 / DAT_00100cc9) & 0x7ff;
            target[6] = (int)(*(float *)(record + 0x10)
                * DAT_00100cd5 / DAT_00100cc9) & 0x7ff;
            target[7] = (int)(*(float *)(record + 0x14)
                * DAT_00100cd5 / DAT_00100cc9) & 0x7ff;
        }
        record = record + 0x40;
    }
}
