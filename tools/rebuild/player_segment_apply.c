/* The rebuilt `player_segment_apply` module.
 *
 * The original sits at linear 0x16b98 in STARTH.EXE and holds 95 instructions
 * and 262 bytes up to the `ret` at 0x16c9d.  The next function starts at
 * 0x16ca0, so the store record keeps the 264-byte extent and sets no separate
 * function size.  The identity check uses 95 instructions.
 *
 * The only direct caller at 0x15ae4 is the second player step group at
 * 0x15ab8, between `player_segment_update` at 0x11f98 and
 * `wheel_contact_update` at 0x11a30.  The group runs on the player object at
 * object-3 offset 0xf91c, so the module takes the section The vehicle state
 * chain.
 *
 * The function runs only when the flag `+0x40` is set.  It clears the two
 * lateral terms `+0x44` and `+0x4c` and walks the four 0x2c-byte slots at
 * `+0x54`.  A slot with a clear flag `+0x18` is skipped.  For each active slot
 * the function keeps the larger absolute value of `+0x1c` in `+0x44` and the
 * larger absolute value of `+0x24` in `+0x4c`.  Each replacement copies the
 * slot angle `+0x28` to `+0x50`.
 *
 * The function adds `+0x44` to the x position `+0x4b0` and `+0x4c` to the y
 * position `+0x4b8`.  It then calls `wheel_contact_update`.  The cornering
 * step at 0x16ca0 adds the selected angle `+0x50` to the body yaw `+0x4cc`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * module assigns a float value and calls no math routine, so the shared
 * `fpu_stub.asm` supplies the FPU run-time symbols and the float-used marker.
 */
extern void wheel_contact_update(char *self);

void player_segment_apply(char *self)
{
    char *unit;
    int index;
    int current;
    int candidate;

    if (*(int *)(self + 0x40) == 0) {
        return;
    }
    *(int *)(self + 0x4c) = 0;
    *(int *)(self + 0x44) = *(int *)(self + 0x4c);
    unit = self + 0x54;
    for (index = 0; index <= 3; index++) {
        if (*(int *)(unit + 0x18) != 0) {
            current = *(int *)(self + 0x44);
            if (current < 0) {
                current = -current;
            }
            candidate = *(int *)(unit + 0x1c);
            if (candidate < 0) {
                candidate = -candidate;
            }
            if (current < candidate) {
                *(int *)(self + 0x44) = *(int *)(unit + 0x1c);
                *(float *)(self + 0x50) = *(float *)(unit + 0x28);
            }
            current = *(int *)(self + 0x4c);
            if (current < 0) {
                current = -current;
            }
            candidate = *(int *)(unit + 0x24);
            if (candidate < 0) {
                candidate = -candidate;
            }
            if (current < candidate) {
                *(int *)(self + 0x4c) = *(int *)(unit + 0x24);
                *(float *)(self + 0x50) = *(float *)(unit + 0x28);
            }
        }
        unit = unit + 0x2c;
    }
    *(int *)(self + 0x4b0) += *(int *)(self + 0x44);
    *(int *)(self + 0x4b8) += *(int *)(self + 0x4c);
    wheel_contact_update(self);
}
