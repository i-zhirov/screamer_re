/* The rebuilt `body_point_place` module.
 *
 * The original sits at linear 0x1ce38 in STARTH.EXE.  The plan row holds 45
 * instructions and 165 bytes up to the `ret` at 0x1cedc.  The next function
 * starts at 0x1cee0, so the store record keeps the 168-byte extent and sets no
 * separate function size.  The identity check uses 45 instructions.
 *
 * The only direct caller at 0x1c7bc is `object_prepare` at 0x1c79c, which runs
 * the step when the state at `+0x414` is set.  `object_prepare` is a callee of
 * `object_update_all` at 0x2786c, so the record takes the section
 * `object_prepare callees`, like the two adjacent preparation steps
 * `track_progress_update` at 0x1db94 and `terrain_contact_update` at 0x1cbac.
 *
 * The function rotates the two local body points at `+0x41c` and `+0x428` by
 * the negative heading at `+0x4a8` through the rotation helper at 0x2218c.
 * The helper takes the output point in `eax`, the input point in `edx`, and
 * the angle as a float on the stack.  The function then adds the car world
 * position at `+0x498` and `+0x4a0` to the two results.  It stores the world
 * body points at `+0x434` and `+0x440`, so `car_collision_pass` at 0x1d00c
 * can compare them against the other car records.
 *
 * A point holds x at `+0x00`, y at `+0x04`, and z at `+0x08`.  The function
 * writes only x and z, because the comparison uses the ground plane.  The
 * original loads the heading and negates it with `fchs`, so the module keeps
 * the plain floating-point operations and needs no math stub.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
struct rotate_point {
    int x;
    int pad;
    int z;
};

extern void rotate_pair(struct rotate_point *out, struct rotate_point *in,
    float angle);

void body_point_place(char *self)
{
    rotate_pair((struct rotate_point *)(self + 0x434),
        (struct rotate_point *)(self + 0x41c),
        -*(float *)(self + 0x4a8));
    rotate_pair((struct rotate_point *)(self + 0x440),
        (struct rotate_point *)(self + 0x428),
        -*(float *)(self + 0x4a8));
    *(int *)(self + 0x434) += *(int *)(self + 0x498);
    *(int *)(self + 0x43c) += *(int *)(self + 0x4a0);
    *(int *)(self + 0x440) += *(int *)(self + 0x498);
    *(int *)(self + 0x448) += *(int *)(self + 0x4a0);
}
