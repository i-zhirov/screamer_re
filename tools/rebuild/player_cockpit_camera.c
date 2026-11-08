/* The rebuilt `player_cockpit_camera` module.
 *
 * The original sits at linear 0x1a2bc in STARTH.EXE and holds 71 instructions
 * and 250 bytes up to the `ret` at 0x1a3b5.  The next function starts at
 * 0x1a3b8, so the store record keeps the 252-byte extent and sets no separate
 * function size.  The identity check uses 71 instructions.
 *
 * The only direct caller is the view mode dispatch at 0x19e78, which calls this
 * function at 0x19ea1 for the view mode 0.  The dispatch passes the player
 * object at object-3 offset 0xf91c in `eax` and the player view object at
 * `DAT_00110a20` in `edx`.  The player update at 0x19ab0 and
 * `player_view_setup` at 0x19ae8 both call the dispatch, so the module takes
 * the section `player_view_setup callees`.
 *
 * The function first refreshes the two list slots at `+0x460` and `+0x468`
 * through the helper at 0x11850.  The helper copies the two selected object
 * values and inverts the two top bits of each value.  The sibling
 * `player_view_orbit` uses the helper at 0x117fc, which copies the same values
 * without the inversion.
 *
 * The function then places the view on the player body.  It forms the pitch
 * from the contact angle `+0x4c8` and the body angle `+0x4e0`, the yaw from
 * the heading `+0x4cc` and the view yaw offset `+0x128`, and the roll from the
 * body angle `+0x4e8` times 3.0 plus the contact angle `+0x4d0`.  The position
 * takes the player position `+0x4b0`, `+0x4b4`, and `+0x4b8` plus the body
 * offsets `+0x4bc` and `+0x4c0` and the height bias `DAT_0010cf48`.  The view
 * keeps the two saved copies at `+0x0c` and `+0x14`.
 *
 * The view mode 0 is the default cockpit view, so the module records the name
 * `player_cockpit_camera` with the confidence `inferred`.
 *
 * The roll scale 3.0 arrives through a global, because the minimal image does
 * not load the constant segment.  The shared `fpu_stub.asm` supplies the FPU
 * run-time symbols and the float-used marker.  The original has the EBP frame,
 * so the module builds with `-of+`.
 */
extern void object_list_mirror(void *table, char *name, int *slot);

extern float camera_roll_scale;
extern int camera_height_bias;

void player_cockpit_camera(char *car, char *view)
{
    object_list_mirror((void *)0x6530, (char *)0x6350,
                       (int *)(car + 0x460));
    object_list_mirror((void *)0x6530, (char *)0x6350,
                       (int *)(car + 0x468));

    *(float *)(view + 0x30) = *(float *)(car + 0x4c8)
        + *(float *)(car + 0x4e0);
    *(float *)(view + 0x34) = *(float *)(car + 0x4cc)
        + *(float *)(car + 0x128);
    *(float *)(view + 0x38) = *(float *)(car + 0x4e8) * camera_roll_scale
        + *(float *)(car + 0x4d0);

    *(int *)(view + 0x00) = *(int *)(car + 0x4b0)
        + *(int *)(car + 0x4bc);
    *(int *)(view + 0x0c) = *(int *)(view + 0x00);

    *(int *)(view + 0x04) = *(int *)(car + 0x04)
        + *(int *)(car + 0x4b4)
        + *(int *)(car + 0x4c0)
        + camera_height_bias;

    *(int *)(view + 0x08) = *(int *)(car + 0x4b8);
    *(int *)(view + 0x14) = *(int *)(view + 0x08);
}
