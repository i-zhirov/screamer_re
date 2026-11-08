/* The rebuilt `car_collision_pass` module.
 *
 * The original sits at linear 0x1d00c in STARTH.EXE and holds 144 instructions
 * and 497 bytes up to the `ret` at 0x1d1fc.  The next function starts at
 * 0x1d200, so the store record keeps the 500-byte extent and sets no function
 * size.  The identity check uses 144 instructions.
 *
 * The only direct caller at 0x1c7fc is the per-object step at 0x1c7d0.  That
 * step first marks the nearby objects at 0x126e4, then runs this function, and
 * then runs `opponent_avoid_player` at 0x1e414.  The object update at 0x2003c
 * runs the step on each 0x590-byte car record at `DAT_00110ab8` after
 * `object_prepare`.  The record therefore takes the section
 * `object update callees`.
 *
 * The function reads the record number at `+0x00` and scans every later car
 * record up to the count `DAT_0010f8ec`.  For each candidate it stores the
 * contact distance at `+0x488` as the two radii at `+0x44c`.  The bit of the
 * candidate index in the mask at `+0x0c` selects the record, and the physics
 * gate at `+0x414` must be set.
 *
 * The function tests the two body points of the record against the two body
 * points of the candidate.  A point holds x at `+0x00`, y at `+0x04`, and z at
 * `+0x08`, and the point array starts at `+0x434`.  The function stores the
 * two deltas at `+0x48c` and `+0x494`, squares and adds them, takes the square
 * root through 0x22bec, truncates it toward zero through 0x4d9f4, and stores
 * the distance at `+0x484`.  A distance below the contact distance calls the
 * collision response at 0x1d200 with the record in `eax` and the candidate in
 * `edx`.
 *
 * After the scan, a drop from the saved speed at `+0x178` to the current speed
 * at `+0x418` above 5000.0 sets the crash state at `+0x2f8` to 2 or 3 when the
 * state is zero.  The low bit of the quarter of the x and z positions at
 * `+0x498` and `+0x4a0` selects the state.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * module converts the distance to an integer and takes the square root, so the
 * local `car_collision_pass_fpu.asm` supplies `sqrt` and a truncating `__CHP`.
 */
extern unsigned char DAT_00110ab8[];
extern int DAT_0010f8ec;
extern float collision_speed;
extern double sqrt(double value);
extern void collision_response(char *self, char *other);

void car_collision_pass(char *self)
{
    int index;
    int first;
    int second;
    int dx;
    int dz;
    int squared;
    char *other;

    index = *(int *)self + 1;
    other = (char *)DAT_00110ab8 + index * 0x590;
    while (index < DAT_0010f8ec) {
        *(int *)(self + 0x488) = *(int *)(self + 0x44c)
            + *(int *)(other + 0x44c);
        if ((*(int *)(self + 0xc) & (1 << (index & 31))) != 0
            && *(int *)(other + 0x414) != 0) {
            for (first = 0; first < 2; first++) {
                for (second = 0; second < 2; second++) {
                    dx = *(int *)(self + 0x434 + first * 12)
                        - *(int *)(other + 0x434 + second * 12);
                    *(int *)(self + 0x48c) = dx;
                    dz = *(int *)(self + 0x43c + first * 12)
                        - *(int *)(other + 0x43c + second * 12);
                    *(int *)(self + 0x494) = dz;
                    squared = dx * dx + dz * dz;
                    *(int *)(self + 0x484) = (int)sqrt((double)squared);
                    if (*(int *)(self + 0x484) < *(int *)(self + 0x488)) {
                        collision_response(self, other);
                    }
                }
            }
        }
        other += 0x590;
        index = index + 1;
    }
    if (*(float *)(self + 0x178) - *(float *)(self + 0x418)
        > collision_speed) {
        if (*(int *)(self + 0x2f8) == 0) {
            *(int *)(self + 0x2f8) = 2
                + (((*(int *)(self + 0x498)
                     + *(int *)(self + 0x4a0)) >> 2) & 1);
        }
    }
}
