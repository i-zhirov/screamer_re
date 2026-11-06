/* The rebuilt `player_car_skid_update` module.
 *
 * The original sits at linear 0x17d14 in STARTH.EXE and holds 290 instructions
 * and 1078 bytes up to the `ret` at 0x18149.  The record keeps the 1080-byte
 * extent and sets no function size.
 *
 * The only caller is the effect group at 0x15bac.  That group runs after the
 * player physics group at 0x15a80 on the player object at object-3 offset
 * 0xf91c.  The five view groups at 0x2003c, 0x201d0, 0x202c0, 0x20360, and
 * 0x203c4 call the group, so the module takes the section The vehicle state
 * chain.
 *
 * The function forms the slip value from the speed `+0x50c`, the yaw step
 * `+0x2f4`, the frame delta `DAT_00118e04`, and the two table indices at
 * `+0x1c0`, `+0x268`, `+0x214`, and `+0x2bc`.  A large slip updates the
 * heading `+0x4cc` and stores the shake amplitude `+0x4f0`, the shake rate
 * `+0x4f4`, and the shake sine and cosine `+0x500` and `+0x508`.  A small slip
 * clears the rate and the amplitude.
 *
 * The shake terms are the values that `player_car_view_update` applies to the
 * view.  The filtered slip `+0x128` gates the effect objects: an absolute
 * value above 0.23 allocates a table entry through `table_lookup` and starts
 * the four effect slots through 0x19828.  An absolute value at or below 0.23
 * releases the entry through 0x1f880 when the state `+0x148` is below 3.
 *
 * The early flags `+0x170` and `+0x148` skip the slip block but keep the
 * effect tail.  The input bit 0x40 of `DAT_00118e1d` and a zero speed `+0x110`
 * select the normal path, and a non-zero speed without the input bit selects
 * the deceleration path.
 *
 * The original calls the filter at 0x22b80, the angle filter at 0x2211c, the
 * sine at 0x4da72, the cosine at 0x4da66, and the square root at 0x22bec.
 * This harness links no math library, so the local FPU stub supplies `sin`,
 * `cos`, and `sqrt`.  The constants arrive through globals, because the
 * minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int table_lookup(int index);
extern int object_release(int id);
extern void effect_start(void *object);
extern void filter_update(float *p, float f, int i);
extern void filter_smooth(float *p, float f, int i);
extern double sin(double value);
extern double cos(double value);
extern double sqrt(double value);

extern unsigned char DAT_00118e1d;
extern float DAT_00118e04;
extern float *DAT_0010cf08;

extern float skid_speed_floor;
extern float skid_rate_scale;
extern double skid_heading_scale;
extern double skid_slip_scale;
extern double skid_gate_bias;
extern float skid_angle_offset;
extern double skid_amplitude_scale;
extern double skid_amplitude_limit;
extern double skid_root_scale;
extern float skid_release_floor;
extern double skid_effect_threshold;

static double absolute_double(double value)
{
    return value < 0.0 ? -value : value;
}

void player_car_skid_update(void *object)
{
    char *base = (char *)object;
    float flag = 0.0f;
    float magnitude;
    float delta;
    float gate;
    float angle;
    int sign;
    int term;

    if (*(int *)(base + 0x170) == 0 && *(int *)(base + 0x148) == 0) {
        if ((*(int *)(base + 0x110) & 0x7fffffff) != 0
            && (DAT_00118e1d & 0x40) == 0) {
            *(float *)(base + 0x110) =
                *(float *)(base + 0x110) * DAT_0010cf08[0x62];
            if (*(float *)(base + 0x110) * *(float *)(base + 0x50c)
                < skid_speed_floor) {
                *(int *)(base + 0x110) = 0;
            }
            magnitude = *(float *)(base + 0x110)
                * *(float *)(base + 0x50c) / *(float *)(base + 0x480);
            sign = *(float *)(base + 0x2f4) < 0.0f ? -1 : 1;
            delta = (float)sign * magnitude * DAT_00118e04 * skid_rate_scale;
            filter_update((float *)(base + 0x4f0), delta, 7);
            *(float *)(base + 0x4cc) = (float)(
                (double)*(float *)(base + 0x4cc)
                + (double)*(float *)(base + 0x4f0) * skid_heading_scale);
            flag = 1.0f;
        } else {
            term = 1000 * (*(int *)(base + 0x1c0)
                           + *(int *)(base + 0x268))
                + 600 * (*(int *)(base + 0x214)
                         + *(int *)(base + 0x2bc));
            sign = *(float *)(base + 0x2f4) < 0.0f ? -1 : 1;
            term = term * sign;
            delta = (float)(
                (double)term
                + (double)*(float *)(base + 0x50c)
                    * (double)*(float *)(base + 0x50c)
                    * (double)*(float *)(base + 0x2f4)
                    * skid_slip_scale / (double)DAT_00118e04
                    * (double)*(float *)(base + 0x670));

            if (absolute_double((double)delta)
                > (double)*(float *)(base + 0x184)) {
                gate = (float)(((double)*(float *)(base + 0x114)
                                + skid_gate_bias) * (double)delta);
                if (gate > *(float *)(base + 0x188)) {
                    *(int *)(base + 0x170) = 1;
                } else {
                    if (delta > 0.0f) {
                        delta = delta - *(float *)(base + 0x184);
                    } else {
                        delta = delta + *(float *)(base + 0x184);
                    }
                    angle = *(float *)(base + 0x4cc) + skid_angle_offset;
                    *(float *)(base + 0x4f0) =
                        (float)((double)delta * skid_amplitude_scale);
                    if (absolute_double((double)*(float *)(base + 0x4f0))
                        > skid_amplitude_limit) {
                        sign = delta >= 0.0f ? 1 : -1;
                        *(float *)(base + 0x4f0) =
                            (float)((double)sign * skid_amplitude_limit);
                    }
                    *(float *)(base + 0x4f4) = (float)(
                        (double)*(float *)(base + 0x50c)
                        * (double)*(float *)(base + 0x50c)
                        / (double)*(float *)(base + 0x2f8)
                        * ((double)*(float *)(base + 0x670)
                           * skid_heading_scale));
                    *(float *)(base + 0x500) = (float)sin((double)angle);
                    *(float *)(base + 0x508) = (float)(-cos((double)angle));
                }
            } else {
                *(int *)(base + 0x4f4) = 0;
                *(float *)(base + 0x4f0) = *(float *)(base + 0x4f4);
            }
        }
    }

    {
        float value;

        value = (float)(sqrt(absolute_double(
            cos((double)*(float *)(base + 0x4d0))) * skid_root_scale)
            * (double)*(float *)(base + 0x4f0));
        filter_smooth((float *)(base + 0x128), value, 0x5a);
    }

    filter_update((float *)(base + 0x4fc), *(float *)(base + 0x4f4), 0x50);
    if (*(float *)(base + 0x4fc) < skid_release_floor) {
        *(int *)(base + 0x4fc) = 0;
    }
    if (absolute_double((double)*(float *)(base + 0x128))
        > skid_effect_threshold) {
        flag = 1.0f;
    }

    if ((*(int *)&flag & 0x7fffffff) != 0) {
        if (*(int *)(base + 0x7d0) == -1) {
            *(int *)(base + 0x7d0) = table_lookup(0);
        }
        effect_start(object);
    } else {
        if (*(int *)(base + 0x7d0) != -1
            && *(int *)(base + 0x148) < 3) {
            *(int *)(base + 0x7d0) =
                object_release(*(int *)(base + 0x7d0));
        }
    }
}
