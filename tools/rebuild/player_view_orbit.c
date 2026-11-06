/* The rebuilt `player_view_orbit` module.
 *
 * The original sits at linear 0x1a3b8 in STARTH.EXE and holds 179 instructions
 * and 631 bytes up to the `ret` at 0x1a62e.  The next function starts at
 * 0x1a630, so the store record keeps the 632-byte extent and sets no function
 * size.  The identity check uses 179 instructions.
 *
 * The three direct callers at 0x19eb5, 0x19ed8, and 0x19ef3 are the view mode
 * dispatch at 0x19e78.  The dispatch selects this function for the view modes
 * 1, 2, and 0, and it passes the player object at object-3 offset 0xf91c in
 * `eax`, the player view object at `DAT_00110a20` in `edx`, and the view mode
 * in `ebx`.  The player update at 0x19ab0 and `player_view_setup` at 0x19ae8
 * both call the dispatch, so the module takes the section
 * `player_view_setup callees`.
 *
 * The function refreshes the two list slots at `+0x460` and `+0x468` through
 * the helper at 0x117fc.  It wraps the player yaw at `+0x4cc` into the range
 * zero to 2 pi, and it selects the view height offset, the orbit radius, and
 * the pitch offset for the mode.  It then places the view point on the orbit
 * around the player position at `+0x4b0`, `+0x4b4`, and `+0x4b8`, with the
 * yaw and the pitch of the view at `+0x34` and `+0x30`.
 *
 * A set flag `+0x7c` writes the computed height, the yaw, the pitch, and the
 * second angle directly.  Otherwise the function approaches the values through
 * the helpers at 0x22bb4 and 0x22018 with the step table index 0x51.  The
 * function then stores the two horizontal coordinates and the two saved
 * copies at `+0x0c` and `+0x14`.
 *
 * The original calls `sin` through the Watcom helper at 0x4da72 and `cos`
 * through 0x4da66.  This harness links no math library, so the local assembly
 * stub supplies `sin` and `cos` from the hardware.  The original truncates
 * through the helper at 0x4d9f4, so the local stub also makes `__CHP` truncate
 * toward zero.  The two float constants and the two double constants arrive through
 * globals, because the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sin(double value);
extern double cos(double value);

extern void object_list_copy(void *table, char *name, int *slot);
extern void object_value_approach(int *target, float value, int index);
extern void object_angle_approach(float *target, float value, int index);

extern float player_view_two_pi;
extern float player_view_minus_two_pi;
extern double player_view_offset_short;
extern double player_view_offset_long;

void player_view_orbit(char *car, char *view, int mode)
{
    float angle;
    int base_offset;
    int radius;
    int base;
    int value;
    double yaw;
    double pitch;
    double horizontal;

    object_list_copy((void *)0x6530, (char *)0x6350, (int *)(car + 0x460));
    object_list_copy((void *)0x6530, (char *)0x6350, (int *)(car + 0x468));

    if (*(float *)(car + 0x4cc) > player_view_two_pi) {
        *(float *)(car + 0x4cc) = *(float *)(car + 0x4cc)
            + player_view_minus_two_pi;
    } else if (*(float *)(car + 0x4cc) < 0.0f) {
        *(float *)(car + 0x4cc) = *(float *)(car + 0x4cc)
            + player_view_two_pi;
    }

    switch (mode) {
    case 0:
        angle = (float)(*(float *)(car + 0x4d4) + player_view_offset_short);
        base_offset = 0x12c;
        radius = 0x6a4;
        break;
    case 1:
        angle = (float)(*(float *)(car + 0x4d4) + player_view_offset_long);
        base_offset = 0x12c;
        radius = 0x514;
        break;
    case 2:
        angle = (float)(*(float *)(car + 0x4d4) + player_view_offset_short);
        base_offset = 0x64;
        radius = 0x578;
        break;
    default:
        angle = 0.0f;
        base_offset = 0;
        radius = 0;
        break;
    }

    base = *(int *)(car + 0x4b4) + base_offset;
    value = (int)((double)base
        - (double)radius * sin((double)*(float *)(view + 0x30)));

    if (*(int *)(view + 0x7c) != 0) {
        *(int *)(view + 4) = value;
        *(float *)(view + 0x34) = *(float *)(car + 0x4cc);
        *(float *)(view + 0x38) = *(float *)(car + 0x4d0);
        *(float *)(view + 0x30) = angle;
    } else {
        object_value_approach((int *)(view + 4), (float)value, 0);
        object_angle_approach((float *)(view + 0x34),
                              *(float *)(car + 0x4cc), 0x51);
        object_angle_approach((float *)(view + 0x38),
                              *(float *)(car + 0x4dc), 0x51);
        object_angle_approach((float *)(view + 0x30), angle, 0x51);
    }

    yaw = (double)*(float *)(view + 0x34);
    pitch = (double)*(float *)(view + 0x30);
    horizontal = (double)radius * sin(yaw) * cos(pitch);
    *(int *)(view + 0x00) = (int)((double)*(int *)(car + 0x4b0)
        - horizontal);
    *(int *)(view + 0x0c) = *(int *)(view + 0x00);

    yaw = (double)*(float *)(view + 0x34);
    pitch = (double)*(float *)(view + 0x30);
    horizontal = (double)radius * cos(yaw) * cos(pitch);
    *(int *)(view + 0x08) = (int)((double)*(int *)(car + 0x4b8)
        + horizontal);
    *(int *)(view + 0x14) = *(int *)(view + 0x08);
}
