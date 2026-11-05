/* The rebuilt `map_object_update` module.
 *
 * The original sits at linear 0x2b9cc in STARTH.EXE and holds 310 instructions
 * and 1121 bytes up to the first `ret` at 0x2be2c.  The body continues to the
 * shared epilogue at 0x2be6f, so the record keeps the 1224-byte extent and sets
 * no function size.
 *
 * The only caller is the race state update at 0x29918, which runs this function
 * at 0x29b86 on each of the 0x50 records at `DAT_001db78`.  The records hold
 * two positions, two sway amplitudes, a progress value, and two timers.  The
 * map placement at 0x2b044 fills the records from a map string, so the module
 * records the name `map_object_update`.
 *
 * A record with the field `+0x08` equal to -1 holds no object.  The function
 * binds the list name at 0x6350 to the record slot and returns.  Otherwise it
 * decays the timer at `+0x60` and the progress value at `+0x58`, both clamped
 * at zero, and looks the object up in the slot table `DAT_001d2d0` and the
 * 0x1c-byte entry table `DAT_0010653c`.
 *
 * The object position at `+0x08`, `+0x0c`, and `+0x10` starts at the record
 * fields `+0x40`, `+0x44`, and `+0x48`.  The cosine of the progress scaled by
 * pi/2 interpolates each position from the target back to the fields `+0x34`,
 * `+0x38`, and `+0x3c`.  The cosine and the sine of the progress scaled by 2 pi
 * add the sway amplitudes `+0x4c` and `+0x50` to the first two positions.
 *
 * The function then adds a travelling wave to the height `+0x10`.  The wave
 * phase is the normalized position `(x + 4608) / 9216` times 2 pi plus the
 * normalized position `(y + 2080) / 4160` times pi plus the phase
 * `DAT_001d248`.  When the record flag `+0x0c` is zero, the x term uses 0.5
 * and the phase takes the factor 0.25.  The sine of the phase times 150.0 adds
 * to the height.
 *
 * The three record angles `+0x10`, `+0x14`, and `+0x18` become the object
 * angles `+0x14`, `+0x18`, and `+0x1c`, scaled by 1024.0 / pi and masked with
 * 0x7ff.  The object field `+0x20` takes the record object id `+0x08` scaled
 * by `DAT_001fab8` plus 0x226.
 *
 * The final block steps the record angle `+0x10`.  With the record flag and the
 * overlay word `DAT_001fac0` both zero, the angle falls by the frame delta
 * times 4 pi and clamps at 0.05.  Otherwise an angle above pi/2 sets the mode
 * `DAT_001fb64` to -2, the angle rises by the same rate, and an angle above pi
 * clamps at pi.  An angle already at pi returns.
 *
 * The original calls the list helpers at 0x117fc and 0x11850, the Watcom `sin`
 * and `cos` helpers at 0x4da72 and 0x4da66, and the truncate helper at 0x4d9f4.
 * This harness links no math library, so the local assembly stub supplies `sin`
 * and `cos` from the hardware and makes `__CHP` truncate toward zero.  The
 * float constants arrive through globals, because the minimal image does not
 * load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sin(double value);
extern double cos(double value);

extern void object_list_refresh(void *table, char *name, int *slot);
extern void object_list_bind(void *table, char *name, int *slot);

extern float DAT_0018e08;
extern float DAT_001d248;
extern int DAT_001d2d0[];
extern int DAT_001fab8;
extern int DAT_001fac0;
extern int DAT_001fb64;
extern char DAT_0010653c[];

extern float map_object_pi_half;
extern float map_object_two_pi;
extern float map_object_pi;
extern float map_object_quarter;
extern float map_object_angle_scale;
extern float map_object_wave_rate;
extern float map_object_bias_x;
extern float map_object_bias_y;
extern double map_object_scale_x;
extern double map_object_scale_y;
extern double map_object_wave_height;
extern double map_object_min_angle;

void map_object_update(char *record)
{
    char *object;
    int index;
    int slot;
    double a;
    double b;
    double value;
    float progress;
    float wave_x;
    float wave_y;
    float timer;
    float angle;

    if (*(int *)(record + 8) == -1) {
        object_list_bind((void *)0x6530, (char *)0x6350,
                         (int *)((char *)DAT_001d2d0
                             + *(int *)record * 8));
        return;
    }

    timer = *(float *)(record + 0x60) - DAT_0018e08;
    *(float *)(record + 0x60) = timer;
    if (timer <= 0.0f) {
        *(float *)(record + 0x60) = 0.0f;
    }

    index = *(int *)record;
    slot = DAT_001d2d0[index * 2];
    object_list_refresh((void *)0x6530, (char *)0x6350,
                        (int *)((char *)DAT_001d2d0 + index * 8));
    object = *(char **)(DAT_0010653c + slot * 28);
    *(int *)(object + 0x20) =
        DAT_001fab8 * *(int *)(record + 8) + 0x226;

    progress = *(float *)(record + 0x58);

    a = (double)(*(int *)(record + 0x34) - *(int *)(record + 0x40));
    value = (double)*(int *)(record + 0x40)
        + (1.0 - cos((double)(progress * map_object_pi_half))) * a;
    b = (double)*(int *)(record + 0x4c) * progress;
    value = cos((double)(progress * map_object_two_pi)) * b + value;
    *(int *)(object + 8) = (int)value;

    a = (double)(*(int *)(record + 0x38) - *(int *)(record + 0x44));
    value = (double)*(int *)(record + 0x44)
        + (1.0 - cos((double)(progress * map_object_pi_half))) * a;
    b = (double)*(int *)(record + 0x50) * progress;
    value = sin((double)(progress * map_object_two_pi)) * b + value;
    *(int *)(object + 0xc) = (int)value;

    a = (double)(*(int *)(record + 0x3c) - *(int *)(record + 0x48));
    value = (double)*(int *)(record + 0x48)
        + (1.0 - cos((double)(progress * map_object_pi_half))) * a;
    *(int *)(object + 0x10) = (int)value;

    wave_x = 0.5f;
    if (*(int *)(record + 0xc) != 0) {
        wave_x = (float)(((double)*(int *)(object + 8)
            + (double)map_object_bias_x) * map_object_scale_x);
    }
    wave_y = (float)(((double)*(int *)(object + 0xc)
        + (double)map_object_bias_y) * map_object_scale_y);

    if (*(int *)(record + 0xc) != 0) {
        value = (double)wave_y * (double)map_object_pi
            + (double)DAT_001d248;
    } else {
        value = (double)DAT_001d248 * (double)map_object_quarter
            + (double)wave_y * (double)map_object_pi;
    }
    value = (double)wave_x * (double)map_object_two_pi + value;
    value = sin(value) * map_object_wave_height;
    value = (double)*(int *)(object + 0x10) + value;
    *(int *)(object + 0x10) = (int)value;

    *(int *)(object + 0x14) =
        (int)(*(float *)(record + 0x10) * map_object_angle_scale
            / map_object_pi) & 0x7ff;
    *(int *)(object + 0x18) =
        (int)(*(float *)(record + 0x14) * map_object_angle_scale
            / map_object_pi) & 0x7ff;
    *(int *)(object + 0x1c) =
        (int)(*(float *)(record + 0x18) * map_object_angle_scale
            / map_object_pi) & 0x7ff;

    if ((*(unsigned int *)(record + 0x58) & 0x7fffffff) != 0) {
        progress = *(float *)(record + 0x58)
            - DAT_0018e08 * *(float *)(record + 0x5c);
        *(float *)(record + 0x58) = progress;
        if (progress < 0.0f) {
            *(float *)(record + 0x58) = 0.0f;
        }
    }

    if (*(int *)(record + 0xc) == 0 && DAT_001fac0 == 0) {
        if ((*(unsigned int *)(record + 0x10) & 0x7fffffff) == 0) {
            return;
        }
        angle = *(float *)(record + 0x10)
            - DAT_0018e08 * map_object_wave_rate;
        *(float *)(record + 0x10) = angle;
        if (angle < map_object_min_angle) {
            *(float *)(record + 0x10) = 0.0f;
        }
        return;
    }

    if (*(int *)(record + 0x10) == (int)0x40490fdb) {
        return;
    }

    if (DAT_001fac0 != 0) {
        if (*(float *)(record + 0x10) > map_object_pi_half) {
            DAT_001fb64 = -2;
        }
    }

    angle = *(float *)(record + 0x10)
        + DAT_0018e08 * map_object_wave_rate;
    *(float *)(record + 0x10) = angle;
    if (angle > map_object_pi) {
        *(float *)(record + 0x10) = map_object_pi;
    }
}
