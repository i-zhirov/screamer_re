/* The rebuilt `opponent_avoid_player` module.
 *
 * The original sits at linear 0x1e414 in STARTH.EXE and holds 162 instructions
 * and 590 bytes up to the `ret` at 0x1e661.  The next function starts at
 * 0x1e670, so the store record keeps the 604-byte extent and sets no function
 * size.  The identity check uses 162 instructions.
 *
 * The only direct caller at 0x1c804 is the per-object AI step at 0x1c7d0.
 * The object update at 0x2003c runs that step on each 0x590-byte car record
 * at 0x10ab8 after `object_prepare`, so the record takes the section
 * `object_prepare callees`.  The step first marks the nearby objects at
 * 0x126e4 and resolves the nearest opponent at 0x1d00c, then calls this
 * function.
 *
 * The function forms the look-ahead point three car radii ahead of the
 * opponent from the world position at `+0x498` and `+0x4a0` and the heading at
 * `+0x4a8`.  It returns when the point stays 5000 units or more from the
 * player position.  The player position is the fixed player object at 0xf91c:
 * `+0x4b0` at 0xfdcc and `+0x4b8` at 0xfdd4.
 *
 * Near the player, the function sets the avoidance radius at `+0x488` to twice
 * the opponent radius `+0x44c` plus the player radius at `+0x544`.  It then
 * checks the two player body points at `+0x52c` and `+0x534`, and at `+0x538`
 * and `+0x540`, which the player update at 0x17c6c rotates from the body
 * offsets and translates by the player position.
 *
 * When a point stays inside the avoidance radius and the opponent speed
 * `+0x418` is above the player speed at `+0x50c`, the function pushes the
 * opponent local position `+0x454` and `+0x45c` to the avoidance radius along
 * the offset from that point.  The distance goes through the square-root
 * helper at 0x22bec, and the increments `+0x484`, `+0x48c`, and `+0x494` hold
 * the distance and the two offsets.
 *
 * The original calls `sin` at 0x4da72, `cos` at 0x4da66, `sqrt` at 0x22bec, and
 * the truncating helper at 0x4d9f4.  This harness links no math library, so the
 * local assembly stub supplies `sin`, `cos`, and `sqrt` from the hardware and
 * makes `__CHP` truncate toward zero.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sin(double value);
extern double cos(double value);
extern double sqrt(double value);

extern int player_world_x;
extern int player_world_z;
extern float player_speed;
extern int player_radius;
extern int player_point0_x;
extern int player_point0_z;
extern int player_point1_x;
extern int player_point1_z;

void opponent_avoid_player(char *object)
{
    int lookahead;
    int look_x;
    int look_z;
    int point_x[2];
    int point_z[2];
    int index;
    int threshold;
    int dx;
    int dz;
    int distance;
    float unit_x;
    float unit_z;

    lookahead = *(int *)(object + 0x44c) * 3;
    look_x = (int)((double)*(int *)(object + 0x498)
        + sin((double)*(float *)(object + 0x4a8)) * (double)lookahead);
    look_z = (int)((double)*(int *)(object + 0x4a0)
        - cos((double)*(float *)(object + 0x4a8)) * (double)lookahead);

    dx = look_x - player_world_x;
    if (dx < 0) {
        dx = -dx;
    }
    if (dx >= 0x1388) {
        return;
    }
    dz = look_z - player_world_z;
    if (dz < 0) {
        dz = -dz;
    }
    if (dz >= 0x1388) {
        return;
    }

    threshold = *(int *)(object + 0x44c) * 2 + player_radius;
    *(int *)(object + 0x488) = threshold;

    point_x[0] = player_point0_x;
    point_z[0] = player_point0_z;
    point_x[1] = player_point1_x;
    point_z[1] = player_point1_z;

    for (index = 0; index < 2; index++) {
        dx = look_x - point_x[index];
        *(int *)(object + 0x48c) = dx;
        dz = look_z - point_z[index];
        *(int *)(object + 0x494) = dz;
        distance = (int)sqrt((double)(dx * dx + dz * dz));
        *(int *)(object + 0x484) = distance;
        if (distance >= threshold) {
            continue;
        }
        if (!(*(float *)(object + 0x418) > player_speed)) {
            continue;
        }
        distance = distance + 1;
        *(int *)(object + 0x484) = distance;
        unit_x = (float)dx / (float)distance;
        unit_z = (float)dz / (float)distance;
        *(int *)(object + 0x454) = (int)((double)*(int *)(object + 0x454)
            + ((double)threshold * (double)unit_x - (double)dx));
        *(int *)(object + 0x45c) = (int)((double)*(int *)(object + 0x45c)
            + ((double)threshold * (double)unit_z - (double)dz));
    }
}
