/* The rebuilt `engine_rpm_update` module.
 *
 * The original sits at linear 0x18db0 in STARTH.EXE and holds 448 instructions
 * and 1734 bytes up to the `ret` at 0x19475.  The record keeps the 1736-byte
 * extent and sets no function size.
 *
 * The only direct caller is the player update group at 0x15a80.  The group runs
 * this function after the player input step at 0x18a14 and before the throttle
 * step at 0x160a4, the brake step at 0x16150, and `player_steering_update`.
 *
 * The function selects the gear at `+0x7ac` from the manual input axes or from
 * the automatic thresholds 0x2134 and 0xfa0 against the engine speed at
 * `+0x7a8`.  It forms the current speed limit at `+0x510` from the base speed
 * `+0x480` and the pitch term `sin(+0x4c8) * 4 + 1` times 1000.  It forms the
 * engine speed target from the throttle `+0x144`, the maximum engine speed
 * `+0x484`, the wheel factor `+0x174`, and the gear ratios after the gear count
 * at `+0x494`.  It updates the road speed at `+0x50c`, sets the acceleration
 * flag at `+0x110`, and publishes the engine sound from `+0x7cc`.
 *
 * The FPU helpers `sin` and `sqrt` come from the local assembly stub, because
 * this harness links no math library.  The stub also truncates the sound value
 * with `__CHP`, because the original uses the Watcom helper at 0x4d9f4.
 *
 * The float constants live in globals, because the harness does not load the
 * constant segment.  The original has the EBP frame, so the module builds with
 * `-of+`.
 */
extern double sin(double value);
extern double sqrt(double value);
extern void filter_update(float *p, float f, int i);
extern void vehicle_state_set(void *object, int state);
extern void engine_sound_update(void *sound, int value);

extern unsigned char input_flags_low;
extern unsigned char input_flags_high;
extern int joystick_axis0;
extern int joystick_axis1;
extern int joystick_axis4;
extern int joystick_axis5;
extern int joystick_center4;
extern int joystick_center5;
extern int throttle_center;
extern int input_selector;
extern float engine_frame_delta;
extern double engine_sin_scale;
extern double engine_limit_base;
extern double engine_two;
extern double engine_half;
extern double engine_minus_one;
extern double engine_gravity_scale;
extern double engine_gravity_base;
extern float engine_rpm_threshold;
extern float engine_speed_low;
extern float engine_decay;
extern float engine_quarter_pi;
extern float engine_state_speed;
extern float engine_sound_scale;
extern float engine_sound_base;
extern float engine_ratio_min;
extern float engine_gear_scale[];

static int sign_of(int value)
{
    return value < 0 ? -1 : 1;
}

static float throttle_engine_speed(float *throttle, float maximum)
{
    return (float)((double)*throttle * (double)maximum);
}

static float gear_denominator(void *object, int gear)
{
    return *(float *)((char *)object + 0x174)
        * *(float *)((char *)object + 0x494 + gear * 4);
}

void engine_rpm_update(void *object)
{
    float *rpm;
    float *speed;
    float *pitch;
    float *throttle;
    float *limit;
    int *gear;
    int *shift;
    float maximum_rpm;
    int gear_count;
    float ratio;
    float value;
    float gear_factor;
    float old_speed;

    rpm = (float *)((char *)object + 0x7a8);
    speed = (float *)((char *)object + 0x50c);
    pitch = (float *)((char *)object + 0x4c8);
    throttle = (float *)((char *)object + 0x144);
    limit = (float *)((char *)object + 0x510);
    gear = (int *)((char *)object + 0x7ac);
    shift = (int *)((char *)object + 0x140);
    maximum_rpm = *(float *)((char *)object + 0x484);
    gear_count = *(int *)((char *)object + 0x494);

    if ((input_flags_high & 0x40) != 0) {
        joystick_axis1 = (input_selector < 0 ? -1 : 1) * 512;
        filter_update(rpm, throttle_engine_speed(throttle, maximum_rpm), 0x3d);
    } else if ((input_flags_low & 1) == 0) {
        filter_update(rpm, throttle_engine_speed(throttle, maximum_rpm), 0x4b);
    } else {
        if (*(int *)((char *)object + 0x490) == 0) {
            if (sign_of(joystick_axis4) == joystick_center4
                && joystick_axis4 != 0) {
                if (*gear < gear_count && *shift == 0) {
                    *gear = *gear + 1;
                    *shift = 1;
                }
            } else if (sign_of(joystick_axis5) == joystick_center5
                       && joystick_axis5 != 0) {
                if (*gear > 0 && *shift == 0) {
                    *gear = *gear - 1;
                    *shift = 1;
                }
            } else {
                *shift = 0;
            }
        } else {
            if ((float)(*gear + 0x2134) < *rpm) {
                if (*gear < gear_count) {
                    *gear = *gear + 1;
                }
            } else if ((float)(0xfa0 - *gear) > *rpm) {
                if (*gear > 1) {
                    *gear = *gear - 1;
                }
            }
            if (*(int *)((char *)object + 0x148) == 5) {
                *gear = 1;
            }
        }

        if (*gear == 0) {
            *speed = *speed - engine_frame_delta * engine_decay;
            if (*speed < 0.0f) {
                *speed = 0.0f;
            }
        } else {
            *limit = *(float *)((char *)object + 0x480)
                - (float)((sin((double)*pitch) * engine_sin_scale
                           + 1.0) * engine_limit_base);

            if (*(float *)((char *)object + 0x488) * engine_rpm_threshold < *rpm
                && *speed < engine_speed_low
                && sign_of(joystick_axis0) == throttle_center
                && (joystick_axis0 < 0 ? -joystick_axis0 : joystick_axis0)
                    > 200) {
                vehicle_state_set(object, 5);
                *rpm = *rpm - engine_frame_delta * engine_decay;
            } else {
                ratio = (float)((double)maximum_rpm
                    / (double)gear_denominator(object, *gear));

                if (gear_count != 1) {
                    value = (float)(engine_two * (double)*speed
                        / (double)ratio + engine_minus_one);
                    if (*gear == 1 && value < 0.0f) {
                        value = engine_ratio_min;
                    }
                    value = (float)((sin((double)(value * engine_quarter_pi))
                                     + 1.0) * engine_half);
                    gear_factor = *(float *)((char *)object + 0x48c)
                        * engine_gear_scale[*gear];
                } else {
                    gear_factor = *(float *)((char *)object + 0x48c);
                    value = (float)sqrt((double)((ratio - *speed) / ratio));
                }

                if (*rpm < throttle_engine_speed(throttle, maximum_rpm)) {
                    *rpm = *rpm + (float)((*throttle * gear_factor * value
                        - sin((double)*pitch) * engine_gravity_scale)
                        * (double)engine_frame_delta);
                }

                if (*rpm > maximum_rpm) {
                    float over;

                    over = (float)((double)(*rpm - maximum_rpm) * engine_two);
                    *rpm = *rpm - (float)((sin((double)*pitch)
                        * engine_gravity_scale + engine_gravity_base
                        + (double)over) * (double)engine_frame_delta);
                    if (*rpm > throttle_engine_speed(throttle, maximum_rpm)) {
                        *rpm = throttle_engine_speed(throttle, maximum_rpm);
                    }
                } else if (*rpm
                           > throttle_engine_speed(throttle, maximum_rpm)) {
                    float base;

                    base = *(float *)((char *)object + 0x178);
                    *rpm = *rpm - (float)(sqrt((double)(*rpm / maximum_rpm))
                        * (double)base * (double)engine_frame_delta);
                    if (*rpm > throttle_engine_speed(throttle, maximum_rpm)) {
                        *rpm = throttle_engine_speed(throttle, maximum_rpm);
                    }
                }

                old_speed = *speed;
                if (*(int *)((char *)object + 0x148) != 5) {
                    *speed = (float)((double)*rpm
                        / (double)gear_denominator(object, *gear));
                } else {
                    *speed = *speed + (float)((double)engine_frame_delta
                        * (double)engine_state_speed);
                }
                if (*speed > *limit) {
                    *speed = *limit;
                }
                if ((double)old_speed - (double)*speed
                    > (double)*(float *)((char *)object + 0x18c)
                        * (double)engine_frame_delta) {
                    *(float *)((char *)object + 0x110) = 1.0f;
                }
            }
        }
        filter_update(rpm, throttle_engine_speed(throttle, maximum_rpm), 0x4b);
    }

    if (engine_frame_delta * engine_decay > *speed) {
        *gear = 0;
    }
    engine_sound_update(*(void **)((char *)object + 0x7cc),
        (int)((double)*rpm * (double)engine_sound_scale
              + (double)engine_sound_base));
}
