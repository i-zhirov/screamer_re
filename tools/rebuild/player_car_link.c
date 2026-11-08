/* The rebuilt `player_car_link` module.
 *
 * The original sits at linear 0x15c1c in STARTH.EXE and holds 306 instructions
 * and 1160 bytes up to the `ret` at 0x160a3.  The record keeps the 1160-byte
 * extent and sets no function size.
 *
 * The five callers at 0x20195, 0x20279, 0x2034e, 0x203bb, and 0x2041f pass the
 * player object at object-3 offset 0xf91c in `eax`.  The callers are the five
 * view groups that run `player_car_view_update` at 0x16f34 and the player
 * physics group at 0x15a80 on the same object.  The function links the player
 * car to the render arrays, so the module records the name `player_car_link`.
 *
 * The 28-byte entries at `DAT_0010653c` hold the link pointer at field 0 and
 * the render pointer at field 4.  The object field `+0x460` selects the link
 * entry.  The object field `+0x468` selects the target entry; its field 0 is
 * the target pointer.
 *
 * The call at 0x18ccc projects the world position `DAT_00110a20` and
 * `DAT_00110a28` through the screen limit 20.0 and writes the screen offsets
 * at `+0x118`, `+0x11c`, and `+0x120`.
 *
 * The link pointer receives the three screen positions from `+0x4b0`, `+0x4b4`,
 * and `+0x4b8` with the offsets at `+0x130`, `+0x134`, and `+0x138`.  It also
 * receives the three angles from `+0x4c8`, `+0x4cc`, and `+0x4d0` with the
 * offsets at `+0x124`, `+0x128`, and `+0x12c`, scaled by 1024.0 and divided by
 * pi.  The truncation toward zero and the mask 0x7ff give the 11-bit angle.
 *
 * The render arrays take the values from `+0x4e0` scaled by 2.0 and from
 * `+0x4e8` scaled by 4.0 at offset 8 and offset 0x10.  The first masked angle
 * comes from `+0x108` divided by the product of `+0x10c` and 1.5 and by the
 * double pi.  The speed at `+0x50c` divided by 15.0 minus the stored value at
 * offset 8 forms the second masked value with the mask 0x7fe.
 *
 * The target pointer receives the height at `+0x118`, the screen offset at
 * `+0x120`, the masked angle at 0x18, and the value `+0x4ec - 2` at 0xc.  When
 * the state at `+0x148` is not 1, it also receives the two link angles at 0x14
 * and 0x1c.  The link pointer receives `+0x7b0` at 0x3c and `+0x7b4` at 0x20.
 *
 * The original truncates through the Watcom helper at 0x4d9f4.  The local FPU
 * stub makes `__CHP` truncate toward zero, because the shared stub leaves the
 * round-to-nearest mode.  The float constants arrive through globals, because
 * the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned char DAT_0010653c[];
extern void player_screen_offset(char *object);
extern float player_car_link_angle_scale;
extern float player_car_link_pi;
extern float player_car_link_angle_double;
extern float player_car_link_angle_quad;
extern double player_car_link_pitch_ratio;
extern double player_car_link_pi_double;
extern float player_car_link_speed_divisor;

void player_car_link(char *object)
{
    char *link;
    char *render;
    char *target;
    int value;
    float pitch;

    link = *(char **)(DAT_0010653c + *(int *)(object + 0x460) * 28);
    render = *(char **)(DAT_0010653c + *(int *)(object + 0x460) * 28 + 4);
    target = *(char **)(DAT_0010653c + *(int *)(object + 0x468) * 28);

    player_screen_offset(object);

    *(int *)(link + 8) = *(int *)(object + 0x4b0)
        + *(int *)(object + 0x130);
    *(int *)(link + 0xc) = *(int *)(object + 0x4b4)
        + *(int *)(object + 0x134);
    *(int *)(link + 0x10) = *(int *)(object + 0x4b8)
        + *(int *)(object + 0x138);

    *(int *)(link + 0x14) = (int)((*(float *)(object + 0x4c8)
        + *(float *)(object + 0x124))
        * player_car_link_angle_scale / player_car_link_pi) & 0x7ff;
    *(int *)(link + 0x18) = (int)((*(float *)(object + 0x4cc)
        + *(float *)(object + 0x128))
        * player_car_link_angle_scale / player_car_link_pi) & 0x7ff;
    *(int *)(link + 0x1c) = (int)((*(float *)(object + 0x4d0)
        + *(float *)(object + 0x12c))
        * player_car_link_angle_scale / player_car_link_pi) & 0x7ff;

    *(int *)(target + 0x18) = *(int *)(link + 0x18);
    *(int *)(target + 8) = *(int *)(object + 0x118);
    *(int *)(target + 0x10) = *(int *)(object + 0x120);

    value = (int)(*(float *)(object + 0x4e0)
        * player_car_link_angle_scale * player_car_link_angle_double
        / player_car_link_pi) & 0x7ff;
    *(int *)(render + *(int *)(object + 0x45c) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x454) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x44c) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x464) * 32 + 8) = value;

    value = (int)(*(float *)(object + 0x4e8)
        * player_car_link_angle_scale * player_car_link_angle_quad
        / player_car_link_pi) & 0x7ff;
    *(int *)(render + *(int *)(object + 0x45c) * 32 + 0x10) = value;
    *(int *)(render + *(int *)(object + 0x454) * 32 + 0x10) = value;
    *(int *)(render + *(int *)(object + 0x44c) * 32 + 0x10) = value;
    *(int *)(render + *(int *)(object + 0x464) * 32 + 0x10) = value;

    *(int *)(render + *(int *)(object + 0x44c) * 32 + 0x18) =
        *(int *)object - *(int *)(object + 0x4c0);
    *(int *)(render + *(int *)(object + 0x464) * 32 + 0x18) =
        *(int *)(object + 4) - *(int *)(object + 0x4c0);
    *(int *)(render + *(int *)(object + 0x454) * 32 + 0x18) =
        *(int *)(object + 8) - *(int *)(object + 0x4c0);
    *(int *)(render + *(int *)(object + 0x45c) * 32 + 0x18) =
        *(int *)(object + 0xc) - *(int *)(object + 0x4c0);

    pitch = (*(float *)(object + 0x108) * player_car_link_angle_scale)
        / (*(float *)(object + 0x10c) * player_car_link_pitch_ratio);
    value = (int)(pitch / player_car_link_pi_double) & 0x7ff;
    *(int *)(render + *(int *)(object + 0x28c) * 32 + 0xc) = value;
    *(int *)(render + *(int *)(object + 0x284) * 32 + 0xc) = value;
    *(int *)(render + *(int *)(object + 0x1e4) * 32 + 0xc) = value;
    *(int *)(render + *(int *)(object + 0x1dc) * 32 + 0xc) = value;

    value = (int)(*(float *)(object + 0x50c) / player_car_link_speed_divisor
        - (float)*(int *)(render + *(int *)(object + 0x1dc) * 32 + 8))
        & 0x7fe;
    *(int *)(render + *(int *)(object + 0x1dc) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x284) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x230) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x2d8) * 32 + 8) = value;

    *(int *)(target + 0xc) = *(int *)(object + 0x4ec) - 2;
    if (*(int *)(object + 0x148) != 1) {
        *(int *)(target + 0x14) = *(int *)(link + 0x14);
        *(int *)(target + 0x1c) = *(int *)(link + 0x1c);
    }
    *(int *)(link + 0x3c) = *(int *)(object + 0x7b0);
    *(int *)(link + 0x20) = *(int *)(object + 0x7b4);
}
