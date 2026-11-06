/* The rebuilt `wheel_contact_update` module.
 *
 * The original sits at linear 0x11a30 in STARTH.EXE and holds 141 instructions
 * and 461 bytes up to the `ret` at 0x11bfc.  The next function starts at
 * 0x11c00, so the store record keeps the 464-byte extent and sets no function
 * size.  The identity check uses 141 instructions.
 *
 * The two direct callers are the second player step group at 0x15ab8 through
 * the call at 0x15aec and the cornering body at 0x16b98 through the call at
 * 0x16c92.  The step group runs on the player object at object-3 offset
 * 0xf91c, before player_car_cornering_update at 0x16ca0, and the five view
 * groups call the group, so the module takes the section The vehicle state
 * chain.
 *
 * The function starts with the two absolute cosines at `+0x4d0` and `+0x4c8`.
 * It clears the wheel flag `+0x2f0` and walks the four 0x54-byte wheel units at
 * `+0x194`.  A unit holds the two local offsets at `+0x18` and `+0x20`, the
 * two contact points at `+0x00`, `+0x08`, `+0x0c`, and `+0x14`, the two ground
 * heights at `+0x24` and `+0x28`, and the surface flag at `+0x2c`.
 *
 * For each unit, the function scales the two offsets by the two cosines,
 * truncates them, and rotates the pair through the helper at 0x2218c by the
 * negative yaw `+0x4cc` plus the delta `+0x128`.  It adds the view offsets
 * `+0x4b0` and `+0x4b8` and stores the first contact point pair.  The map
 * collision check at 0x11c00 reads the pair through the globals 0x8038 and
 * 0x8040.  On a hit, the first ground height takes the global 0xc72c, the
 * surface flag takes the compare of the global 0x8024 with 0x65, and the
 * `+0x2f0` flag takes the even-unit surface bit.
 *
 * The function then rotates the same pair by the negative yaw alone and stores
 * the second contact point pair through the same two globals.  On a hit, the
 * second ground height takes the global 0xc72c.
 *
 * The conversions to an integer use the Watcom helper at 0x4d9f4, which
 * truncates toward zero, so the module uses the local truncating __CHP stub.
 * The calls at 0x4da66 are the cosines, so the local FPU stub also supplies
 * `cos` and `fabs`.  The original has the EBP frame, so the module builds with
 * `-of+`.
 */
struct rotate_point {
    int x;
    int pad;
    int y;
};

extern double cos(double value);
extern double fabs(double value);
extern void rotate_pair(struct rotate_point *out, struct rotate_point *in,
    float angle);
extern int map_collision_check(int value);

extern int DAT_0010c72c;
extern int DAT_00108024;
extern int DAT_00108038;
extern int DAT_00108040;

void wheel_contact_update(char *self)
{
    float scale_x;
    float scale_y;
    struct rotate_point center;
    struct rotate_point point;
    char *unit;
    int index;

    scale_x = (float)fabs(cos((double)*(float *)(self + 0x4d0)));
    scale_y = (float)fabs(cos((double)*(float *)(self + 0x4c8)));
    *(int *)(self + 0x2f0) = 0;
    unit = self + 0x194;
    for (index = 0; index < 4; index++) {
        center.x = (int)(*(int *)(unit + 0x18) * scale_x);
        center.y = (int)(*(int *)(unit + 0x20) * scale_y);
        rotate_pair(&point, &center,
            -(*(float *)(self + 0x4cc) + *(float *)(self + 0x128)));
        *(int *)(unit + 0x00) = *(int *)(self + 0x4b0) + point.x;
        DAT_00108038 = *(int *)(unit + 0x00);
        *(int *)(unit + 0x08) = *(int *)(self + 0x4b8) + point.y;
        DAT_00108040 = *(int *)(unit + 0x08);
        if (map_collision_check(0) != 0) {
            *(int *)(unit + 0x24) = DAT_0010c72c;
            *(int *)(unit + 0x2c) = (DAT_00108024 == 0x65);
            *(int *)(self + 0x2f0) |=
                ((index & 1) ^ 1) & *(int *)(unit + 0x2c);
        }
        rotate_pair(&point, &center, -*(float *)(self + 0x4cc));
        *(int *)(unit + 0x0c) = *(int *)(self + 0x4b0) + point.x;
        DAT_00108038 = *(int *)(unit + 0x0c);
        *(int *)(unit + 0x14) = *(int *)(self + 0x4b8) + point.y;
        DAT_00108040 = *(int *)(unit + 0x14);
        if (map_collision_check(0) != 0) {
            *(int *)(unit + 0x28) = DAT_0010c72c;
        }
        unit = unit + 0x54;
    }
}
