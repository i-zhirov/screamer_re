/* The rebuilt `point_rotation_flags` module.
 *
 * The original sits at linear 0x224dc in STARTH.EXE and holds 41 instructions
 * and 110 bytes up to the `ret 4` at 0x22547.  The next record starts at
 * 0x2254c, so the store record keeps the 112-byte extent and sets no separate
 * function size.  The identity check uses 41 instructions.
 *
 * The two point arguments arrive in the Watcom register order: `eax` holds the
 * first point and `edx` holds the second point.  The float angle makes the
 * compiler pass the remaining argument on the stack, so the callee cleans it
 * with `ret 4`.
 *
 * A point holds the x value at `+0x00` and the z value at `+0x08`.  The
 * function forms the ground-plane delta `second - first`, turns the delta by
 * the angle through the integer rotation helper at 0x2218c, and returns the
 * sign flags of the turned pair: bit 0 is set when the turned x value is
 * positive, and bit 1 is set when the turned z value is positive.
 *
 * The only callers are `car_collision_response` at 0x1790e and 0x17991, which
 * use the flags to choose the collision turn and the turn rate.  The caller
 * sits in the section The vehicle state chain, so this module takes the same
 * section.  The caller harness already names the helper `angle_flags`, and the
 * delta rotation and the two sign tests support the name
 * `point_rotation_flags`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses the FPU only to pass the float angle, so it needs the shared
 * `fpu_stub.asm` and no truncating helper.  The module uses no assembly body.
 */
extern void point_rotate_y(int *out, int *in, float angle);

int point_rotation_flags(int *first, int *second, float angle)
{
    int delta[3];
    int rotated[3];
    int flags;

    delta[0] = second[0] - first[0];
    delta[2] = second[2] - first[2];
    point_rotate_y(rotated, delta, angle);
    flags = rotated[0] > 0;
    if (rotated[2] > 0) {
        flags = flags | 2;
    }
    return flags;
}
