/* The rebuilt `player_car_suspension_update` module.
 *
 * The original sits at linear 0x168b4 in STARTH.EXE and holds 219 instructions
 * and 740 bytes up to the `ret` at 0x16b97.  The next function starts at
 * 0x16b98, so the record keeps the 740-byte extent and sets no function size.
 *
 * The only direct caller is the effect group at 0x15bac, which runs on the
 * player object at object-3 offset 0xf91c.  The five view groups call that
 * group after the player physics group at 0x15a80, so the module takes the
 * section The vehicle state chain.
 *
 * The function walks the four 0x54-byte suspension units at `+0x194`.  A unit
 * holds the phase at `+0x30`, the travel at `+0x34`, the lateral force at
 * `+0x38`, and the longitudinal force at `+0x40`.  The two distances at
 * `+0x2e4` and `+0x2ec` separate the wheels.
 *
 * The function forms the speed change from `+0x50c` and `+0x588` and the
 * lateral load from `+0x50c`, `+0x2f4`, and the frame delta.  It stores a new
 * force when the magnitude passes the stored force, filters the longitudinal
 * force through `filter_update` at 0x22b80, and forms an oscillating travel
 * from the phase with `cos` at 0x4da66.  It clamps the travel to -18 through
 * 18, damps both forces through the filter table at 0xcf08, and clears a force
 * below the dead zone 2.0.
 *
 * The function writes the body height `+0x4c0` and the body angles `+0x4e8`
 * and `+0x4e0` from the four travel values with `atan` at 0xc2254.  The
 * sibling function at 0x166a8 reads the same units and the same distances, so
 * the module names the function `player_car_suspension_update`.
 *
 * The constants and the filter table live in globals, because the harness does
 * not load the object-3 data segment.  The conversions to an integer use the
 * local truncating __CHP stub, and `cos`, `atan`, and `fabs` come from the
 * local FPU assembly file.  The original has the EBP frame, so the module
 * builds with `-of+`.
 */
extern double cos(double value);
extern double atan(double value);
extern double fabs(double value);
extern void filter_update(float *value, float target, int index);

extern float suspension_frame_delta;
extern float *suspension_filter_table;
extern double suspension_speed_scale;
extern double suspension_lateral_scale;
extern float suspension_wheel_scale;
extern float suspension_wave_scale;
extern float suspension_wave_offset;
extern double suspension_dead_zone;

struct suspension_unit {
    unsigned char pad0[0x30];
    float phase;
    int travel;
    float lateral;
    int pad3c;
    float longitudinal;
    unsigned char pad44[0x10];
};

void player_car_suspension_update(void *object)
{
    float speed;
    float previous_speed;
    float speed_change;
    float lateral_load;
    int index;

    speed = *(float *)((char *)object + 0x50c);
    previous_speed = *(float *)((char *)object + 0x588);
    speed_change = (speed - previous_speed) * suspension_speed_scale;
    lateral_load = speed * speed * suspension_lateral_scale
        * (*(float *)((char *)object + 0x2f4) / suspension_frame_delta);
    for (index = 0; index <= 3; index++) {
        struct suspension_unit *unit;
        float wheel_force;
        float lateral_force;
        float total;
        int travel;

        unit = (struct suspension_unit *)(
            (char *)object + 0x194 + index * 0x54);
        wheel_force = (index & 1) ? speed_change * suspension_wheel_scale
            : -speed_change;
        lateral_force = (index & 2) ? lateral_load : -lateral_load;
        if (fabs((double)unit->lateral)
                < fabs((double)(lateral_force / suspension_frame_delta))) {
            unit->phase = 0.0f;
            unit->lateral = lateral_force / suspension_frame_delta;
        }
        if (fabs((double)unit->longitudinal)
                < fabs((double)(wheel_force / suspension_frame_delta))) {
            unit->phase = 0.0f;
            filter_update(
                &unit->longitudinal,
                wheel_force / suspension_frame_delta,
                0x3d);
        }
        total = unit->lateral + unit->longitudinal;
        travel = (int)(cos(
                (double)(unit->phase * suspension_wave_scale
                    + suspension_wave_offset))
            * total);
        if (travel > 0x12) {
            travel = 0x12;
        } else if (travel < -0x12) {
            travel = -0x12;
        }
        unit->travel = travel;
        unit->lateral = unit->lateral * suspension_filter_table[0x3d];
        if (fabs((double)unit->lateral) < suspension_dead_zone) {
            unit->lateral = 0.0f;
        }
        unit->longitudinal =
            unit->longitudinal * suspension_filter_table[0x47];
        if (fabs((double)unit->longitudinal) < suspension_dead_zone) {
            unit->longitudinal = 0.0f;
        }
        unit->phase = unit->phase + suspension_frame_delta;
    }
    *(int *)((char *)object + 0x4c0) =
        (*(int *)((char *)object + 0x1c8)
            + *(int *)((char *)object + 0x270)
            + *(int *)((char *)object + 0x21c)
            + *(int *)((char *)object + 0x2c4))
        / 4;
    *(float *)((char *)object + 0x4e8) = (float)-atan(
        (double)(float)(*(int *)((char *)object + 0x1c8)
            - *(int *)((char *)object + 0x270))
        / (double)*(int *)((char *)object + 0x2e4));
    *(float *)((char *)object + 0x4e0) = (float)atan(
        (double)(float)(*(int *)((char *)object + 0x270)
            - *(int *)((char *)object + 0x2c4))
        / (double)*(int *)((char *)object + 0x2ec));
}
