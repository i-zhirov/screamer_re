/* The rebuilt `traffic_body_point_place` module.
 *
 * The original sits at linear 0x1f108 in STARTH.EXE.  The plan row holds 45
 * instructions and 147 bytes up to the `ret` at 0x1f19a.  Five pad bytes
 * follow the `ret`, and the next function starts at 0x1f1a0, so the store
 * record keeps the 152-byte extent and sets no separate function size.  The
 * identity check uses 45 instructions.
 *
 * The only direct caller at 0x20257 is the traffic view group at 0x201d0,
 * which walks the 0x2a8-byte traffic records at DAT_00113cd8 while the index
 * stays below the count DAT_00115218 and first runs this record update on each
 * record.  The frame update at 0x1ff70 selects the group through the mode byte
 * DAT_0011d260 bit 0x10, so the module takes the section The vehicle state
 * chain, like the sibling traffic_object_link at 0x1ef68.
 *
 * The function rotates the two local traffic body points at `+0x274` and
 * `+0x280` by the negative heading at `+0x4c` through the rotation helper at
 * 0x2218c.  The helper takes the output point in `eax`, the input point in
 * `edx`, and the angle as a float on the stack.  The function then adds the
 * traffic world position at `+0x3c` and `+0x44` to the two results.  It stores
 * the two world traffic points at `+0x28c` and `+0x298`, so
 * `traffic_collision_pass` at 0x175dc can compare them against the player body
 * points.
 *
 * A point holds x at `+0x00`, y at `+0x04`, and z at `+0x08`.  The function
 * writes only x and z, because the comparison uses the ground plane.  The
 * original loads the heading and negates it with `fchs`, so the module keeps
 * the plain floating-point operations and needs no math stub.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * module uses no assembly implementation.
 */
struct rotate_point {
    int x;
    int pad;
    int z;
};

extern void rotate_pair(struct rotate_point *out, struct rotate_point *in,
    float angle);

void traffic_body_point_place(char *self)
{
    rotate_pair((struct rotate_point *)(self + 0x28c),
        (struct rotate_point *)(self + 0x274),
        -*(float *)(self + 0x4c));
    rotate_pair((struct rotate_point *)(self + 0x298),
        (struct rotate_point *)(self + 0x280),
        -*(float *)(self + 0x4c));
    *(int *)(self + 0x28c) += *(int *)(self + 0x3c);
    *(int *)(self + 0x294) += *(int *)(self + 0x44);
    *(int *)(self + 0x298) += *(int *)(self + 0x3c);
    *(int *)(self + 0x2a0) += *(int *)(self + 0x44);
}
