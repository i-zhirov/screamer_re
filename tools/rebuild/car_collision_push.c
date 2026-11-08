/* The rebuilt `car_collision_push` module.
 *
 * The original sits at linear 0x1d200 in STARTH.EXE and holds 70 instructions
 * and 268 bytes up to the `ret` at 0x1d30b.  The next function starts at
 * 0x1d30c, so the store record keeps the 268-byte extent and sets no separate
 * function size.  The identity check uses 70 instructions.
 *
 * The only direct caller is `car_collision_pass` at 0x1d00c.  It calls this
 * function at 0x1d130 with the scanning car record in `eax` and the candidate
 * record in `edx`, after it stores the contact distance at `+0x488`, the x
 * delta at `+0x48c`, the z delta at `+0x494`, and the distance at `+0x484`.
 * `car_collision_pass` runs inside the per-object step at 0x1c7d0, which runs
 * in the object update at 0x2003c, so the record takes the section
 * `object update callees`.
 *
 * The function sets the distance at `+0x484` to 1 when it is zero, so the two
 * divisions cannot fail.  It compares the speed at `+0x418` of the two records
 * and replaces the larger speed with the average of the two.  It then forms
 * the two unit values `dx / distance` and `dz / distance` and moves the
 * candidate x position at `+0x454` and z position at `+0x45c` back to the
 * contact distance `+0x488` along the two directions.  The original truncates
 * both positions toward zero through the Watcom helper at 0x4d9f4.
 *
 * The average uses the double constant 0.5 at object-3 offset 0xb1c.  The
 * harness supplies it as a global, because the minimal image does not load the
 * constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The two
 * conversions to an integer use the truncating `__CHP` stub in
 * `car_collision_push_fpu.asm`, because the shared stub rounds to nearest.
 */
extern double car_collision_push_half;

void car_collision_push(char *self, char *other)
{
    float unit_x;
    float unit_z;

    if (*(int *)(self + 0x484) == 0) {
        *(int *)(self + 0x484) = 1;
    }
    if (*(float *)(self + 0x418) < *(float *)(other + 0x418)) {
        *(float *)(other + 0x418) = (float)(
            (*(float *)(other + 0x418) + *(float *)(self + 0x418))
            * car_collision_push_half);
    } else {
        *(float *)(self + 0x418) = (float)(
            (*(float *)(self + 0x418) + *(float *)(other + 0x418))
            * car_collision_push_half);
    }
    unit_x = (float)*(int *)(self + 0x48c)
        / (float)*(int *)(self + 0x484);
    unit_z = (float)*(int *)(self + 0x494)
        / (float)*(int *)(self + 0x484);
    *(int *)(other + 0x454) = (int)(
        (double)*(int *)(other + 0x454)
        - ((double)*(int *)(self + 0x488) * (double)unit_x
           - (double)*(int *)(self + 0x48c)));
    *(int *)(other + 0x45c) = (int)(
        (double)*(int *)(other + 0x45c)
        - ((double)*(int *)(self + 0x488) * (double)unit_z
           - (double)*(int *)(self + 0x494)));
}
