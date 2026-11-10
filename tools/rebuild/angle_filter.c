/* The rebuilt `angle_filter` module.
 *
 * The original sits at linear 0x2211c in STARTH.EXE and holds 41 instructions
 * and 110 bytes up to the `ret 8` at 0x22187.  The next function starts at
 * 0x2218c, so the store record keeps the 112-byte extent and sets no separate
 * function size.  The identity check uses 41 instructions.
 *
 * The first argument arrives in `eax`; the float target makes the Watcom
 * compiler pass the remaining arguments on the stack, so the callee cleans
 * them with `ret 8`.  The `push edx` in the prologue saves the second register.
 *
 * The function moves the value at `*value` toward the target with the
 * coefficient `DAT_0010cf08[step]`.  When the absolute difference is below
 * pi/2, the function takes the sine of the offset, so the last part of the
 * move is smooth.  Otherwise it takes the offset directly.
 *
 * The only direct caller is `player_car_skid_update` at 0x17d14, which calls
 * the function at 0x18085 with the shake value and the step 0x5a to filter the
 * value into `+0x128`.  The function sits between `angle_approach` at 0x22018
 * and `point_rotate_y` at 0x2218c and shares the coefficient table and the
 * constant segment with the angle filters, so the module takes the section
 * The timing and filter layer.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * coefficient table pointer and the pi/2 constant arrive through globals,
 * because the harness image does not load the constant segment.  The local
 * `angle_filter_fpu.asm` supplies the sine from the hardware and the FPU
 * run-time symbols.
 */
extern double fabs(double value);
extern double sin(double value);
extern float *DAT_0010cf08;
extern double DAT_0010d70;

void angle_filter(float *value, float target, int step)
{
    double gain;

    if (fabs(target - *value) < DAT_0010d70) {
        gain = DAT_0010cf08[step];
        *value = target + sin(*value - target) * gain;
    } else {
        *value = target + (*value - target) * DAT_0010cf08[step];
    }
}
