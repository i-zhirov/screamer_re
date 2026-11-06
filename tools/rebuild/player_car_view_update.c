/* The rebuilt `player_car_view_update` module.
 *
 * The original sits at linear 0x16f34 in STARTH.EXE and holds 318 instructions
 * and 1236 bytes up to the `ret` at 0x17407.  The record keeps the 1236-byte
 * extent and sets no function size.
 *
 * The five callers at 0x2003c, 0x201d0, 0x202c0, 0x20360, and 0x203c4 run
 * before the player physics group at 0x15a80 on the same player object at
 * object-3 offset 0xf91c.  The callers are the five view groups that the frame
 * update at 0x1ff70 selects, so the function prepares the player car for the
 * render path.
 *
 * The function normalizes the three body angles at `+0x4c8`, `+0x4cc`, and
 * `+0x4d0` to the range 0 to 2 pi.  It copies the speed `+0x50c` to
 * `+0x588`, filters the view scale `+0x300` from the ratio
 * `+0x688 / DAT_0011d254` when the input bit 0x10 is set, and projects the
 * forward motion into the screen offsets `+0x4b0` and `+0x4b8`.  The state at
 * `+0x148` selects the full projection with the pitch angle or the flat
 * projection with the yaw angle.  The flag at `+0x108` selects the pitch path
 * or the roll angle path.
 *
 * The function then adds the lateral rate `+0x54c` with the offsets `+0x550`
 * and `+0x558`, applies the two rate factors at `DAT_0010cf08 + 0x140` and
 * `DAT_0010cf08 + 0x16c`, adds the heading rate `+0x548`, and adds the shake
 * terms from `+0x4fc`, `+0x500`, and `+0x508`.  It sets the render flags at
 * `+0x7b4` from `+0x47c`, `+0x114`, and the speed threshold, and it clears the
 * base offsets `+0x10` and `+0x18`.
 *
 * The float constants arrive through globals, because the harness does not
 * load the constant segment.  The original truncates through the Watcom helper
 * at 0x4d9f4, so the local FPU stub truncates toward zero.  The stub also
 * holds `sin`, `cos`, and `sqrt`, because this harness links no math library.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sin(double value);
extern double cos(double value);
extern double sqrt(double value);
extern void filter_update(float *p, float f, int i);

extern unsigned char DAT_0011d260;
extern int DAT_0011d254;
extern float DAT_00118e04;
extern float *DAT_0010cf08;
extern float view_angle_wrap;
extern float view_angle_unwrap;
extern double view_rate_epsilon;
extern double view_shake_scale;
extern float view_speed_threshold;
extern float view_filter_scale;
extern float view_filter_base;

static double absolute_double(double value)
{
    return value < 0.0 ? -value : value;
}

void player_car_view_update(void *object)
{
    float *pitch;
    float *yaw;
    float *roll;
    float *speed;
    float *scale;
    double cosine;
    double sine;
    float xterm;
    float yterm;
    float value;
    double ydelta;
    double term;
    int flags;

    pitch = (float *)((char *)object + 0x4c8);
    yaw = (float *)((char *)object + 0x4cc);
    roll = (float *)((char *)object + 0x4d0);
    speed = (float *)((char *)object + 0x50c);
    scale = (float *)((char *)object + 0x300);

    if (0.0f > *pitch) {
        *pitch = *pitch + view_angle_wrap;
    }
    if (0.0f > *yaw) {
        *yaw = *yaw + view_angle_wrap;
    }
    if (0.0f > *roll) {
        *roll = *roll + view_angle_wrap;
    }
    if (*yaw > view_angle_wrap) {
        *yaw = *yaw + view_angle_unwrap;
    }

    *(float *)((char *)object + 0x588) = *speed;

    if (*(int *)((char *)object + 0x148) == 0) {
        if ((DAT_0011d260 & 0x10) != 0) {
            value = (float)*(int *)((char *)object + 0x688)
                / (float)DAT_0011d254;
            value = value * view_filter_scale + view_filter_base;
            filter_update(scale, value, 0x58);
        }
        if ((*(int *)((char *)object + 0x108) & 0x7fffffff) == 0) {
            cosine = cos((double)*pitch);
            sine = sin((double)*yaw);
            xterm = (float)(sine * cosine);
            yterm = (float)(-(cos((double)*yaw) * cosine));
            *(int *)((char *)object + 0x4b0) = (int)(
                (double)xterm * (double)*speed * DAT_00118e04
                * (double)*scale + (double)*(int *)((char *)object + 0x10)
                + (double)*(int *)((char *)object + 0x4b0));
            ydelta = (double)yterm * (double)*speed * DAT_00118e04
                * (double)*scale + (double)*(int *)((char *)object + 0x18);
        } else {
            term = absolute_double(cos((double)*roll))
                * (double)*(int *)((char *)object + 0x1c);
            *(int *)((char *)object + 0x4b0) = (int)(
                term * (double)*scale
                + (double)*(int *)((char *)object + 0x10)
                + (double)*(int *)((char *)object + 0x4b0));
            term = absolute_double(cos((double)*roll))
                * (double)*(int *)((char *)object + 0x24);
            ydelta = term * (double)*scale
                + (double)*(int *)((char *)object + 0x18);
        }
    } else {
        term = sin((double)*yaw) * (double)*speed * DAT_00118e04;
        *(int *)((char *)object + 0x4b0) = (int)(
            term + (double)*(int *)((char *)object + 0x10)
            + (double)*(int *)((char *)object + 0x4b0));
        term = cos((double)*yaw) * (double)*speed * DAT_00118e04;
        ydelta = (double)*(int *)((char *)object + 0x18) - term;
    }
    *(int *)((char *)object + 0x4b8) = (int)(
        ydelta + (double)*(int *)((char *)object + 0x4b8));

    *(int *)((char *)object + 0x4b0) = (int)(
        (double)*(float *)((char *)object + 0x550)
        * (double)*(float *)((char *)object + 0x54c) * DAT_00118e04
        + (double)*(int *)((char *)object + 0x4b0));
    *(int *)((char *)object + 0x4b8) = (int)(
        (double)*(float *)((char *)object + 0x558)
        * (double)*(float *)((char *)object + 0x54c) * DAT_00118e04
        + (double)*(int *)((char *)object + 0x4b8));

    *(float *)((char *)object + 0x548) =
        *(float *)((char *)object + 0x548) * DAT_0010cf08[0x50];
    *yaw = *yaw + *(float *)((char *)object + 0x548) * DAT_00118e04;
    *(float *)((char *)object + 0x54c) =
        *(float *)((char *)object + 0x54c) * DAT_0010cf08[0x5b];
    if (absolute_double((double)*(float *)((char *)object + 0x54c))
        < view_rate_epsilon) {
        *(int *)((char *)object + 0x54c) = 0;
    }

    term = sqrt(absolute_double(cos((double)*roll)) * view_shake_scale);
    *(int *)((char *)object + 0x4b0) = (int)(
        term * DAT_00118e04
        * (double)(*(float *)((char *)object + 0x4fc)
                   * *(float *)((char *)object + 0x500))
        + (double)*(int *)((char *)object + 0x4b0));
    *(int *)((char *)object + 0x4b8) = (int)(
        term * DAT_00118e04
        * (double)(*(float *)((char *)object + 0x4fc)
                   * *(float *)((char *)object + 0x508))
        + (double)*(int *)((char *)object + 0x4b8));

    flags = *(int *)((char *)object + 0x47c);
    if ((*(int *)((char *)object + 0x114) & 0x7fffffff) == 0) {
        flags = flags | 0x4000;
    }
    if (*speed <= view_speed_threshold) {
        flags = flags | 0x2000;
    }
    *(int *)((char *)object + 0x7b4) = flags;
    *(int *)((char *)object + 0x18) = 0;
    *(int *)((char *)object + 0x10) =
        *(int *)((char *)object + 0x18);
}
