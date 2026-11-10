/* The rebuilt `player_body_point_place` module.
 *
 * The original sits at linear 0x17c6c in STARTH.EXE.  The plan row holds 45
 * instructions and 165 bytes up to the `ret` at 0x17d10.  The next function
 * starts at 0x17d14, so the store record keeps the 168-byte extent and sets no
 * separate function size.  The identity check uses 45 instructions.
 *
 * The player group at 0x15b08 calls this function first at 0x15b14, then
 * `player_nearby_mark` at 0x12378, `car_proximity_pass` at 0x17408, and
 * `opponent_start` at 0x124c0.  The player update at 0x2003c runs the group at
 * 0x20154, so the record takes the section `The vehicle state chain`, like the
 * other player steps.
 *
 * The function rotates the two local player body points at `+0x514` and
 * `+0x520` by the negative heading at `+0x4cc` through the rotation helper at
 * 0x2218c.  The helper takes the output point in `eax`, the input point in
 * `edx`, and the angle as a float on the stack.  The function then adds the
 * player world position at `+0x4b0` and `+0x4b8` to the two results.  It stores
 * the player body points at `+0x52c` and `+0x538`, so `car_proximity_pass` at
 * 0x17408 and `opponent_avoid_player` at 0x1e414 can compare them against the
 * car records.
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

void player_body_point_place(char *self)
{
    rotate_pair((struct rotate_point *)(self + 0x52c),
        (struct rotate_point *)(self + 0x514),
        -*(float *)(self + 0x4cc));
    rotate_pair((struct rotate_point *)(self + 0x538),
        (struct rotate_point *)(self + 0x520),
        -*(float *)(self + 0x4cc));
    *(int *)(self + 0x52c) += *(int *)(self + 0x4b0);
    *(int *)(self + 0x534) += *(int *)(self + 0x4b8);
    *(int *)(self + 0x538) += *(int *)(self + 0x4b0);
    *(int *)(self + 0x540) += *(int *)(self + 0x4b8);
}
