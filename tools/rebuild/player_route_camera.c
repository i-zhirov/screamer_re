/* The rebuilt `player_route_camera` module.
 *
 * The original sits at linear 0x1a630 in STARTH.EXE and holds 243 instructions
 * and 768 bytes up to the `ret` at 0x1a92f.  The record keeps the 768-byte
 * extent and sets no function size.  The extent ends at the next function,
 * `car_camera` at 0x1a930.
 *
 * The only direct caller at 0x19ec4 is the view mode dispatch at 0x19e78.  The
 * dispatch selects this function for the render mode 3 and passes the player
 * object at object-3 offset 0xf91c in `eax` and the player view object at
 * `DAT_00110a20` in `edx`.  The player update at 0x19ab0 and `player_view_setup`
 * at 0x19ae8 both call the dispatch, so the module takes the section
 * `player_view_setup callees`.
 *
 * The function first refreshes the two list slots at `+0x460` and `+0x468`
 * through the helper at 0x117fc.  It then walks the 0x38-byte route records
 * below the pointer `DAT_0010f910`, the RPL route.  The count is at `+0x1c`,
 * and each record holds the position at `+0x04` (x), `+0x08` (z), and `+0x0c`
 * (y).  The loop picks the nearest record by the horizontal distance from the
 * car position at `+0x4b0` and `+0x4b8`.  A zero distance does not take part.
 *
 * The function forms the yaw at `+0x34` and the pitch at `+0x30` of the view
 * from the chosen record.  It copies the record position into the view at
 * `+0x00`, `+0x04`, and `+0x08` and the two saved copies at `+0x0c` and
 * `+0x14`.  When the total distance exceeds 1500 units, it moves the view
 * point along the yaw and the pitch by the excess.
 *
 * The original calls `asin` through the Watcom helper at 0xc5e42, `sqrt`
 * through 0x22bec, `sin` through 0x4da72, and `cos` through 0x4da66.  This
 * harness links no math library, so the local assembly stub supplies `asin`,
 * `sqrt`, `sin`, and `cos` from the hardware.  The original truncates through
 * the helper at 0x4d9f4, so the local stub also makes `__CHP` truncate toward
 * zero.  The float constants arrive through globals, because the minimal image
 * does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double asin(double value);
extern double sqrt(double value);
extern double sin(double value);
extern double cos(double value);

extern void object_list_refresh(void *table, char *name, int *slot);

extern char *DAT_0010f910;

extern float player_route_two_pi;
extern float player_route_minus_half_pi;
extern double player_route_minus_1500;

void player_route_camera(char *car, char *view)
{
    char *route;
    char *node;
    char *best;
    int count;
    int index;
    int best_distance;
    int dx;
    int dz;
    int dy;
    double dxd;
    double dzd;
    double dyd;
    double dx2;
    double dz2;
    double dy2;
    double distance;
    double horizontal;
    double total;
    float a;
    float b;
    float angle;
    float c;
    float v;
    int move;

    object_list_refresh((void *)0x6530, (char *)0x6350,
                        (int *)(car + 0x460));
    object_list_refresh((void *)0x6530, (char *)0x6350,
                        (int *)(car + 0x468));

    best = 0;
    best_distance = 0x7fffffff;
    route = DAT_0010f910;
    count = *(int *)(route + 0x1c);
    node = *(char **)route;
    index = 0;
    while (index < count) {
        dx = *(int *)(node + 4) - *(int *)(car + 0x4b0);
        dz = *(int *)(node + 8) - *(int *)(car + 0x4b8);
        dxd = (double)dx;
        dzd = (double)dz;
        dx2 = dxd * dxd;
        dz2 = dzd * dzd;
        distance = sqrt(dx2 + dz2);
        if (distance != 0.0 && (double)best_distance > distance) {
            best_distance = (int)distance;
            best = node;
        }
        node = node + 0x38;
        index = index + 1;
    }

    node = best;
    dx = *(int *)(node + 4) - *(int *)(car + 0x4b0);
    dz = *(int *)(node + 8) - *(int *)(car + 0x4b8);
    dy = *(int *)(node + 0xc) - *(int *)(car + 0x4b4);
    dxd = (double)dx;
    dzd = (double)dz;
    dyd = (double)dy;
    dx2 = dxd * dxd;
    dz2 = dzd * dzd;
    dy2 = dyd * dyd;

    horizontal = sqrt(dx2 + dz2);
    a = (float)(dxd / horizontal);
    b = (float)(dzd / horizontal);
    angle = (float)asin((double)a);
    if (b < 0.0f) {
        angle = player_route_two_pi - angle;
    }
    *(float *)(view + 0x34) = angle + player_route_minus_half_pi;

    total = sqrt(dy2 + dz2 + dx2);
    c = (float)(horizontal / total);
    v = (float)asin((double)c);
    if (v < 0.0f) {
        v = -v;
    }
    *(float *)(view + 0x30) = player_route_two_pi - v;

    *(int *)(view + 0x38) = 0;
    *(int *)(view + 0x00) = *(int *)(node + 4);
    *(int *)(view + 0x0c) = *(int *)(view + 0x00);
    *(int *)(view + 0x04) = *(int *)(node + 0xc);
    *(int *)(view + 0x08) = *(int *)(node + 8);
    *(int *)(view + 0x14) = *(int *)(view + 0x08);

    move = (int)(total + player_route_minus_1500);
    if (move > 0) {
        double yaw;
        double sine_yaw;
        double pitch;

        yaw = (double)*(float *)(view + 0x34);
        sine_yaw = sin(yaw);
        pitch = (double)*(float *)(view + 0x30);
        *(int *)(view + 0x00) = (int)((double)*(int *)(view + 0x00)
            + cos(pitch) * sine_yaw * (double)move);
        *(int *)(view + 0x04) = (int)((double)*(int *)(view + 0x04)
            + sin(pitch) * (double)move);
        *(int *)(view + 0x08) = (int)((double)*(int *)(view + 0x08)
            - cos(pitch) * cos(yaw) * (double)move);
    }
}
