/* The rebuilt `player_car_cornering_update` module.
 *
 * The original sits at linear 0x16ca0 in STARTH.EXE and holds 182 instructions
 * and 657 bytes up to the `ret` at 0x16f30.  The next function starts at
 * 0x16f34, so the record keeps the 660-byte extent and sets no function size.
 *
 * The only direct caller at 0x15af4 is the player physics group at 0x15ab8.
 * The group runs on the player object at object-3 offset 0xf91c, and the five
 * view groups at 0x2003c, 0x201d0, 0x202c0, 0x20360, and 0x203c4 call the
 * group, so the module takes the section The vehicle state chain.
 *
 * The function publishes the map position `+0x4b0` and `+0x4b8`, calls the
 * nearest node search, and copies the track heading `DAT_00108048` to the car
 * field `+0x13c`.  It then forms the heading error between the car heading
 * `+0x4cc` and the track heading, folds the error into the range 0 to pi/2,
 * and scales it to 0 through 1.  A speed times error above the gate `+0x180`
 * sets the effect state `+0x148` to 2 when the flag `+0x7b0` is clear.
 *
 * The function normalizes the velocity `+0x44` and `+0x4c` into the lateral
 * unit vector `+0x550` and `+0x558`, stores the lateral rate `+0x54c` from the
 * sine of the world heading and the speed, removes the speed loss from `+0x50c`,
 * and stores the heading rate `+0x548` from `+0xc4` minus `+0x6c`.  It clears
 * the transient fields `+0x110`, `+0x4f0`, and `+0x4f4`, clears the filtered
 * slip `+0x128` when `+0x170` is clear, and keeps the effect table entry
 * `+0x7d4` through the type-4 acquire when the state did not change.
 *
 * The original calls the nearest node search at 0x120c0, the effect state set
 * at 0x19478, the sine at 0x4da72, the arc tangent at 0xc21e2, the square root
 * at 0x22bec, and the table acquire at 0x1f7ac.  This harness links no math
 * library, so the local FPU stub supplies `sin`, `atan2`, and `sqrt`.  The
 * constants arrive through globals, because the minimal image does not load
 * the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern void track_nearest_node(int mode);
extern void effect_state_set(void *object, int state);
extern int table_entry_acquire(int type, int handle);
extern double sin(double value);
extern double atan2(double y, double x);
extern double sqrt(double value);

extern int map_position_x;
extern int map_position_y;
extern float map_heading;

extern float corner_pi;
extern float corner_neg_pi;
extern float corner_half_pi;
extern float corner_speed_scale;
extern double corner_four;
extern double corner_lateral_bias;
extern float corner_lateral_limit;
extern double corner_speed_loss;
extern float corner_heading_scale;

static double absolute_value(double value)
{
    return value < 0.0 ? -value : value;
}

void player_car_cornering_update(void *object)
{
    char *base = (char *)object;
    float sine_heading;
    float error;
    int state_changed = 0;
    int squared;
    float magnitude;
    double lateral;

    map_position_x = *(int *)(base + 0x4b0);
    map_position_y = *(int *)(base + 0x4b8);
    track_nearest_node(0);
    *(float *)(base + 0x13c) = map_heading;

    if (*(int *)(base + 0x40) == 0) {
        return;
    }
    if (*(int *)(base + 0x148) != 0) {
        return;
    }

    sine_heading = (float)absolute_value(sin((double)(
        *(float *)(base + 0x4cc) + *(float *)(base + 0x50))));
    error = (float)absolute_value(
        (double)*(float *)(base + 0x4cc)
        - (double)*(float *)(base + 0x13c));
    while (error > corner_pi) {
        error = error + corner_neg_pi;
    }
    if (error > corner_half_pi) {
        error = corner_pi - error;
    }
    error = error / corner_half_pi;

    if (*(float *)(base + 0x50c) * error > *(float *)(base + 0x180)
        && *(int *)(base + 0x7b0) == 0) {
        effect_state_set(object, 2);
        state_changed = 1;
    }

    squared = *(int *)(base + 0x44) * *(int *)(base + 0x44)
        + *(int *)(base + 0x4c) * *(int *)(base + 0x4c);
    magnitude = (float)sqrt((double)squared);
    *(float *)(base + 0x550) = (float)*(int *)(base + 0x44) / magnitude;
    *(float *)(base + 0x558) = (float)*(int *)(base + 0x4c) / magnitude;

    lateral = atan2((double)sine_heading, corner_four)
        * ((double)*(float *)(base + 0x50c)
           * (double)corner_speed_scale)
        + corner_lateral_bias;
    *(float *)(base + 0x54c) = (float)lateral;
    if (*(float *)(base + 0x54c) > corner_lateral_limit) {
        *(float *)(base + 0x54c) = corner_lateral_limit;
    }

    *(float *)(base + 0x50c) = *(float *)(base + 0x50c)
        - (float)((double)*(float *)(base + 0x50c)
                  * corner_speed_loss * (double)sine_heading);
    if (*(float *)(base + 0x50c) <= 0.0f) {
        *(int *)(base + 0x50c) = 0;
    }

    *(float *)(base + 0x548) = (float)(
        *(int *)(base + 0xc4) - *(int *)(base + 0x6c))
        * sine_heading * corner_heading_scale;

    *(int *)(base + 0x110) = 0;
    *(float *)(base + 0x4f0) = *(float *)(base + 0x110);
    *(float *)(base + 0x4f4) = *(float *)(base + 0x4f0);
    if (*(int *)(base + 0x170) == 0) {
        *(int *)(base + 0x128) = 0;
    }
    if (state_changed == 0) {
        *(int *)(base + 0x7d4) = table_entry_acquire(
            4, *(int *)(base + 0x7d4));
    }
}
