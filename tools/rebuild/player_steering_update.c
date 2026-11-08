/* The rebuilt `player_steering_update` module.
 *
 * The original sits at linear 0x1629c in STARTH.EXE and holds 267 instructions
 * and 997 bytes up to the `ret` at 0x16680.  The record keeps the 1036-byte
 * extent and sets no function size.
 *
 * The only caller is the player update group at 0x15a80, which runs this
 * function on the player object at object-3 offset 0xf91c after the brake step
 * at 0x16150.  The object fields hold the steering angle at `+0x108`, the
 * steering limit at `+0x10c`, the speed at `+0x50c`, the maximum speed at
 * `+0x480`, the wheelbase at `+0x2ec`, the heading at `+0x4cc`, the turn radius
 * at `+0x2f8`, the yaw step at `+0x2f4`, and the position delta at `+0x1c` and
 * `+0x24`.
 *
 * The function selects the filter index 0x50, 0x55, or 0x58 from the state at
 * `+0x148` and the flags at `+0x110`, `+0x4f4`, and `+0x170`.  It forms the
 * speed ratio `+0x50c / +0x480` and the limit
 * `(1 - sqrt(ratio)) * 17 degrees + 1 degree`, then scales the six control
 * values at 0x119880 by the factor 1/1024 and the limit.  The mode at 0x119818
 * selects the direct axis, the two-axis sum, or the filtered keyboard path.
 * The ramp at `+0x2f0`, `+0x4b0`, and `+0x4b8` adds the keyboard steering, and
 * the flags at `+0x6c`, `+0xc4`, and the state at `+0x148` reset the angle.
 *
 * A non-zero angle gives the turn radius `wheelbase / tan(angle)` and the yaw
 * step `speed * frame_delta / radius`.  The function adds the yaw step to the
 * heading and rotates the point (radius, +0x2b0) by the old and the new
 * heading.  The difference goes to `+0x1c` and `+0x24`.
 *
 * The angle conversion uses the Watcom helper at 0x4d9f4, which truncates
 * toward zero.  The module uses a local truncating FPU stub, because the
 * shared stub rounds to nearest.  The local stub also holds `sqrt`, `tan`,
 * `sin`, and `cos`, because this harness links no math library.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sqrt(double value);
extern double tan(double value);
extern double sin(double value);
extern double cos(double value);
extern void filter_update(float *p, float f, int i);

extern double steering_angle_range;
extern double steering_angle_base;
extern float steering_axis_scale;
extern double steering_epsilon;
extern double steering_key_scale;
extern int steering_mode;
extern int steering_axis;
extern int steering_axis_secondary;
extern unsigned char steering_flags;
extern float steering_frame_delta;

static float absolute_float(float value)
{
    return value < 0.0f ? -value : value;
}

static void rotate_xz(int *dest, const int *src, float angle)
{
    float cosine;
    float sine;

    cosine = (float)cos((double)angle);
    sine = (float)sin((double)angle);
    dest[0] = (int)((double)src[2] * (double)sine +
                    (double)src[0] * (double)cosine);
    dest[2] = (int)((double)src[2] * (double)cosine -
                    (double)src[0] * (double)sine);
}

void player_steering_update(void *object)
{
    int index;
    int sign;
    double ratio;
    float limit;
    float temp;
    float *steering;

    if (*(int *)((char *)object + 0x148) == 5) {
        index = 0x58;
    } else if ((*(int *)((char *)object + 0x110) & 0x7fffffff) != 0 ||
               (*(int *)((char *)object + 0x4f4) & 0x7fffffff) != 0 ||
               *(int *)((char *)object + 0x170) != 0) {
        index = 0x55;
    } else {
        index = 0x50;
    }

    ratio = (double)*(float *)((char *)object + 0x50c) /
            (double)*(float *)((char *)object + 0x480);
    limit = (float)((1.0 - sqrt(ratio)) * steering_angle_range +
                    steering_angle_base);
    *(float *)((char *)object + 0x10c) = limit;

    sign = (steering_flags & 0x10) != 0 ? -1 : 1;
    temp = (float)steering_axis * steering_axis_scale;
    temp = temp * limit;
    temp = -temp;
    temp = temp * (float)sign;

    steering = (float *)((char *)object + 0x108);
    if (steering_mode == 2) {
        filter_update(steering, temp, index);
        if (absolute_float(*steering) < steering_epsilon) {
            *steering = 0.0f;
        }
    } else if (steering_mode == 1) {
        float extra;

        extra = (float)steering_axis_secondary * steering_axis_scale;
        extra = extra * limit;
        extra = extra * (float)sign;
        temp = temp + extra;
        filter_update(steering, -temp, index);
        if (absolute_float(*steering) < steering_epsilon) {
            *steering = 0.0f;
        }
    } else if (steering_mode == 0) {
        if (index != 0x50) {
            filter_update(steering, temp, index);
        } else {
            *steering = temp;
        }
    }

    if (*(int *)((char *)object + 0x2f0) != 0) {
        int value;
        int key_direction;
        double ramp;

        value = *(int *)((char *)object + 0x4b0) +
                *(int *)((char *)object + 0x4b8);
        key_direction = (value & 1) != 0 ? -1 : 1;
        ramp = (double)key_direction * (double)(value & 0x3f) *
               steering_key_scale;
        *steering = *steering + (float)((1.0 - sqrt(ratio)) * ramp);
        if (absolute_float(*steering) > *(float *)((char *)object + 0x10c)) {
            int direction;

            direction = *steering < 0.0f ? -1 : 1;
            *steering = (float)direction *
                        *(float *)((char *)object + 0x10c);
        }
    }

    if (*steering > 0.0f && *(int *)((char *)object + 0x6c) != 0) {
        *steering = 0.0f;
    } else if (*steering < 0.0f && *(int *)((char *)object + 0xc4) != 0) {
        *steering = 0.0f;
    }
    if (*(int *)((char *)object + 0x148) == 1) {
        *steering = 0.0f;
    }

    if ((*(unsigned int *)steering & 0x7fffffff) == 0) {
        return;
    }

    {
        int source[3];
        int first[3];
        int second[3];
        double wheelbase;
        double radius;

        wheelbase = (double)*(int *)((char *)object + 0x2ec);
        radius = wheelbase / tan((double)*steering);
        *(float *)((char *)object + 0x2f8) = (float)radius;
        *(float *)((char *)object + 0x2f4) =
            (float)(((double)*(float *)((char *)object + 0x50c) *
                     (double)steering_frame_delta) /
                    (double)*(float *)((char *)object + 0x2f8));
        *(float *)((char *)object + 0x4cc) =
            *(float *)((char *)object + 0x4cc) +
            *(float *)((char *)object + 0x2f4);

        source[0] = (int)*(float *)((char *)object + 0x2f8);
        source[2] = *(int *)((char *)object + 0x2b0);
        first[0] = 0;
        first[2] = 0;
        second[0] = 0;
        second[2] = 0;
        rotate_xz(first, source,
                  -(*(float *)((char *)object + 0x4cc)));
        rotate_xz(second, first,
                  -(*(float *)((char *)object + 0x2f4)));
        *(int *)((char *)object + 0x1c) = first[0] - second[0];
        *(int *)((char *)object + 0x24) = first[2] - second[2];
    }
}
