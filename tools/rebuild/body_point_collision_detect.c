/* The rebuilt `body_point_collision_detect` module.
 *
 * The original sits at linear 0x11e74 in STARTH.EXE and holds 85 instructions
 * and 292 bytes up to the `ret` at 0x11f97.  The next function starts at
 * 0x11f98, so the store record keeps the 292-byte extent and sets no separate
 * function size.  The identity check uses 85 instructions.
 *
 * The only direct caller at 0x15ad4 is the second player step group at
 * 0x15ab8.  The group runs the player car steps on the player object at
 * object-3 offset 0xf91c after `player_lap_progress_update` at 0x194a0 and
 * before `player_segment_update` at 0x11f98 and `wheel_contact_update` at
 * 0x11a30, so the module takes the section The vehicle state chain.
 *
 * The function starts with the absolute cosine at `+0x4d0` and the absolute
 * cosine at `+0x4c8`.  It clears the body contact flag `+0x40` and walks the
 * four 0x2c-byte body slots at `+0x54` through `+0x104`.
 *
 * A slot holds the two local offsets at `+0x00` and `+0x08`, the screen
 * position at `+0x0c` and `+0x14`, and the contact flag at `+0x18`.  For each
 * slot the function scales the two offsets by the two cosines, truncates them
 * through the helper at 0x4d9f4, and rotates the pair through 0x2218c by the
 * negative yaw `+0x4cc`.  It adds the view offsets `+0x4b0` and `+0x4b8` and
 * stores the screen position.
 *
 * The map collision check at 0x11c00 reads the position through the globals
 * 0x8038 and 0x8040.  A zero return marks a contact, so the slot flag takes
 * the compare result and the body flag `+0x40` takes 1.
 *
 * The conversion to an integer uses the Watcom helper at 0x4d9f4, which
 * truncates toward zero, so the module uses the local truncating __CHP stub.
 * The calls at 0x4da66 are the cosines, so the local FPU stub also supplies
 * `cos` and `fabs`.  The original has the EBP frame, so the module builds
 * with `-of+`.
 */
struct rotate_point {
    int x;
    int pad;
    int z;
};

extern double cos(double value);
extern double fabs(double value);
extern void rotate_point(struct rotate_point *out, struct rotate_point *in,
    float angle);
extern int map_collision_check(int mode);

extern int DAT_00108038;
extern int DAT_00108040;

void body_point_collision_detect(char *self)
{
    float scale_x;
    float scale_z;
    struct rotate_point in;
    struct rotate_point out;
    char *slot;
    char *end;

    scale_x = (float)fabs(cos((double)*(float *)(self + 0x4d0)));
    scale_z = (float)fabs(cos((double)*(float *)(self + 0x4c8)));
    *(int *)(self + 0x40) = 0;
    slot = self + 0x54;
    end = self + 0x104;
    while (slot < end) {
        in.x = (int)(*(int *)(slot + 0x00) * scale_x);
        in.z = (int)(*(int *)(slot + 0x08) * scale_z);
        rotate_point(&out, &in, -*(float *)(self + 0x4cc));
        *(int *)(slot + 0x0c) = *(int *)(self + 0x4b0) + out.x;
        DAT_00108038 = *(int *)(slot + 0x0c);
        *(int *)(slot + 0x14) = *(int *)(self + 0x4b8) + out.z;
        DAT_00108040 = *(int *)(slot + 0x14);
        *(int *)(slot + 0x18) = (map_collision_check(1) == 0);
        if (*(int *)(slot + 0x18) != 0) {
            *(int *)(self + 0x40) = 1;
        }
        slot = slot + 0x2c;
    }
}
