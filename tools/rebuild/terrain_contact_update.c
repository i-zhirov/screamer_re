/* The rebuilt `terrain_contact_update` module.
 *
 * The original sits at linear 0x1cbac in STARTH.EXE and holds 81 instructions
 * and 297 bytes up to the `ret` at 0x1ccd4.  The next function starts at
 * 0x1ccd8, so the store record keeps the 300-byte extent and sets no separate
 * function size.  The identity check uses 81 instructions.
 *
 * The only direct caller is `object_prepare` at 0x1c79c through the call at
 * 0x1c7c4.  `object_prepare` runs on each car record from `car_setup` at
 * 0x142ec, `race_simulation_update` at 0x2003c, and `object_update_all` at
 * 0x2786c, so the record takes the section object_prepare callees.
 *
 * The function publishes the world position from `+0x258 + +0x454` and
 * `+0x260 + +0x45c` through the globals 0x8038 and 0x8040, then calls the map
 * collision check at 0x11c00 with the argument 0.
 *
 * On a hit, the ground height `+0x238` takes the global 0xc72c.  The surface
 * direction in the globals 0xc734 and 0xc73c is rotated by the heading
 * `+0x4bc` plus -pi/2 (object-3 offset 0xb04) through the helper at 0x2218c.
 * The point scales by pi (object-3 offset 0xb08) and divides by 46080.0
 * (object-3 offset 0xb0c) into `+0x23c` and `+0x244`.  Each value adds 2*pi
 * (object-3 offset 0xb10) while it stays negative.
 *
 * On a clear cell, when the speed `+0x418` passes 3000.0 (object-3 offset
 * 0xb14) the function stores zero at `+0x418`.
 *
 * `object_transform` at 0x1cd44 reads the height `+0x238` into `+0x49c` and
 * adds `+0x23c` to `+0x314` and `+0x244` to `+0x31c`, so `+0x23c` and `+0x244`
 * are the terrain pitch and roll.  The caller, these fields, and the constants
 * support the name `terrain_contact_update`.  The original has no symbol, so
 * the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no float-to-integer conversion and calls no math routine, so
 * the shared `fpu_stub.asm` supplies the FPU run-time symbols and the
 * float-used marker.  The constants arrive through globals, because the
 * minimal image does not load the constant segment.
 */
struct rotate_point {
    int x;
    int pad;
    int y;
};

extern void rotate_pair(struct rotate_point *out, struct rotate_point *in,
    float angle);
extern int map_collision_check(int value);

extern int DAT_00108038;
extern int DAT_00108040;
extern int DAT_0010c72c;
extern int DAT_0010c734;
extern int DAT_0010c73c;
extern float DAT_00100b04;
extern float DAT_00100b08;
extern float DAT_00100b0c;
extern float DAT_00100b10;
extern float DAT_00100b14;

void terrain_contact_update(char *self)
{
    struct rotate_point center;
    struct rotate_point point;

    DAT_00108038 = *(int *)(self + 0x258) + *(int *)(self + 0x454);
    DAT_00108040 = *(int *)(self + 0x260) + *(int *)(self + 0x45c);
    if (map_collision_check(0) != 0) {
        *(int *)(self + 0x238) = DAT_0010c72c;
        center.x = DAT_0010c734;
        center.y = DAT_0010c73c;
        rotate_pair(&point, &center, *(float *)(self + 0x4bc) + DAT_00100b04);
        *(float *)(self + 0x23c) =
            (float)point.x * DAT_00100b08 / DAT_00100b0c;
        *(float *)(self + 0x244) =
            (float)point.y * DAT_00100b08 / DAT_00100b0c;
        while (*(float *)(self + 0x244) < 0.0f) {
            *(float *)(self + 0x244) = *(float *)(self + 0x244)
                + DAT_00100b10;
        }
        while (*(float *)(self + 0x23c) < 0.0f) {
            *(float *)(self + 0x23c) = *(float *)(self + 0x23c)
                + DAT_00100b10;
        }
    } else {
        if (*(float *)(self + 0x418) > DAT_00100b14) {
            *(int *)(self + 0x418) = 0;
        }
    }
}
