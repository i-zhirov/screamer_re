/* The rebuilt `car_collision_response` module.
 *
 * The original sits at linear 0x17828 in STARTH.EXE.  The body holds 183
 * instructions and 708 bytes up to the ret at 0x17aeb.  A branch at 0x178ee
 * jumps over that ret to the second block at 0x17aec, so the record keeps the
 * 812-byte extent to the next function at 0x17b54 and sets no function size.
 *
 * The only direct caller is at 0x17523 in the car proximity pass at 0x17408.
 * That pass is called at 0x15b30 from the player effect group at 0x15b08.  It
 * walks the 0x590-byte car records at DAT_00110ab8 and calls this function
 * when the horizontal distance falls below `self + 0x544 + record + 0x44c`,
 * the record physics gate at `+0x414` is set, and the record radius at `+0x2f8`
 * is zero.  The caller passes the player object in `eax` and the other car
 * record in `edx`.
 *
 * The caller stores the two axis deltas at `self + 0x564` and `self + 0x56c`
 * and the truncated distance at `self + 0x55c`.  This function forms the unit
 * vector from those fields, projects the overlap along it, and probes the
 * result through the globals `DAT_00108038` and `DAT_00108040`.  The map
 * collision check at 0x11c00 reads the two globals.
 *
 * When the map cell is clear, the function moves the other car record away
 * along the overlap.  It scales the record rotation `+0x418` by 0.7, sets the
 * speed factor `+0x450`, the turn rate `+0x54c`, the yaw response `+0x4c8`, and
 * the rate `+0x548`, reads the record heading `+0x4a8`, and moves the record
 * positions `+0x454` and `+0x45c`.  When the map cell is blocked, the function
 * moves the player position `+0x4b0` and `+0x4b8` instead.  The caller, the
 * callees, and the two responses support the name `car_collision_response`.
 *
 * The original calls `table_lookup` at 0x1f718, the map collision check at
 * 0x11c00, the rotation flag helper at 0x224dc, and the truncating helper at
 * 0x4d9f4.  The harness supplies the first three, and the local FPU stub makes
 * `__CHP` truncate toward zero.  The constants arrive through globals, because
 * the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int table_lookup(int index);
extern int collision_check(int value);
extern int angle_flags(void *first, void *second, float angle);

extern int DAT_00108038;
extern int DAT_00108040;
extern float *DAT_0010cf08;

extern double car_collision_slow;
extern double car_collision_half;
extern double car_collision_angle_scale;
extern float car_collision_angle_limit;
extern double car_collision_rate_scale;
extern double car_collision_heading_step;

void car_collision_response(char *self, char *target)
{
    float ax;
    float az;
    float step;
    float angle;
    double rate;
    int flags;

    table_lookup(1);
    if (*(int *)(self + 0x55c) == 0) {
        *(int *)(self + 0x55c) = 1;
    }
    ax = (float)*(int *)(self + 0x564) / (float)*(int *)(self + 0x55c);
    az = (float)*(int *)(self + 0x56c) / (float)*(int *)(self + 0x55c);
    DAT_00108038 = (int)((double)*(int *)(target + 0x498)
        - ((double)*(int *)(self + 0x560) * (double)ax
           - (double)*(int *)(self + 0x564)));
    DAT_00108040 = (int)((double)*(int *)(target + 0x4a0)
        - ((double)*(int *)(self + 0x560) * (double)az
           - (double)*(int *)(self + 0x56c)));
    if (collision_check(1) != 0) {
        flags = angle_flags(self + 0x4b0, target + 0x498,
                            *(float *)(self + 0x4cc));
        if ((flags & 2) != 0) {
            *(float *)(target + 0x418) = (float)(
                (double)*(float *)(target + 0x418) * car_collision_slow);
        } else {
            *(float *)(target + 0x450) = (float)(
                (double)*(float *)(self + 0x50c) * car_collision_half);
            step = (float)((double)*(float *)(self + 0x50c)
                * car_collision_angle_scale);
            if (step > car_collision_angle_limit) {
                step = car_collision_angle_limit;
            }
            flags = angle_flags(target + 0x498, self + 0x4b0,
                                *(float *)(target + 0x4a8));
            if ((flags & 1) != 0) {
                angle = step;
            } else {
                angle = -step;
            }
            if ((flags & 2) == 0) {
                angle = -angle;
            }
            *(float *)(target + 0x4c8) = angle;
            *(float *)(target + 0x54c) = (float)(
                (double)*(float *)(self + 0x50c) * car_collision_rate_scale);
            if ((flags & 1) != 0) {
                rate = car_collision_heading_step;
            } else {
                rate = -car_collision_heading_step;
            }
            *(float *)(target + 0x548) = (float)(
                (double)*(float *)(self + 0x50c) * rate);
            *(float *)(self + 0x50c) = (float)(
                (double)*(float *)(self + 0x50c)
                * (double)*(float *)((char *)DAT_0010cf08 + 0x168));
            *(float *)(self + 0x550) = ax;
            *(float *)(self + 0x558) = az;
        }
        *(float *)(target + 0x460) = -ax;
        *(float *)(target + 0x468) = -az;
        *(int *)(target + 0x454) = (int)(
            (double)*(int *)(target + 0x454)
            - ((double)*(int *)(self + 0x560) * (double)ax
               - (double)*(int *)(self + 0x564)));
        *(int *)(target + 0x45c) = (int)(
            (double)*(int *)(target + 0x45c)
            - ((double)*(int *)(self + 0x560) * (double)az
               - (double)*(int *)(self + 0x56c)));
    } else {
        *(int *)(self + 0x4b0) = (int)(
            (double)*(int *)(self + 0x4b0)
            + ((double)*(int *)(self + 0x560) * (double)ax
               - (double)*(int *)(self + 0x564)));
        *(int *)(self + 0x4b8) = (int)(
            (double)*(int *)(self + 0x4b8)
            + ((double)*(int *)(self + 0x560) * (double)az
               - (double)*(int *)(self + 0x56c)));
    }
}
