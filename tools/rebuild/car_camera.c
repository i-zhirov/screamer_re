/* The rebuilt `car_camera` module.
 *
 * The original sits at linear 0x1a930 in STARTH.EXE and holds 396 instructions
 * and 1425 bytes up to the `ret` at 0x1aec0.  The record keeps the 1428-byte
 * extent and sets no function size.
 *
 * The only callers are `player_view_setup` at 0x19ae8 and the camera group at
 * 0x1aec4.  The group at 0x1aec4 runs this function on every 0x590-byte car
 * record at `DAT_00110ab8`, then copies the selected view block at `+0x4f8`.
 * The name `car_camera` comes from that call graph and from the view data.
 *
 * The function walks the 0x38-byte route records at `DAT_0010f90c`.  Each
 * record holds the position at `+0x04` (x), `+0x08` (z), and `+0x0c` (y).  The
 * loop picks the nearest route record by the horizontal distance from the car
 * position at `+0x498` and `+0x4a0`.  A zero distance does not take part.
 *
 * The function forms the yaw at `+0x52c` and the pitch at `+0x528` from the
 * chosen record.  It copies the record position into the view and counts the
 * horizontal distance at `+0x55c` when the chosen record changes.  The flag at
 * `+0x568` comes from the absolute height modulo 10 and the saved distance.
 * When the flag changes while the view is idle, the function holds the old
 * flag and runs the setup path once.  When the total distance exceeds 1500
 * units, the function moves the view along the yaw and the pitch.
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
extern void player_view_setup(char *view);
extern void view_render(char *view);

extern char *DAT_0010f90c;
extern float DAT_0010092c;
extern float DAT_00100930;
extern float DAT_00100934;
extern int DAT_00101e04;
extern float car_camera_zero;
extern float car_camera_far_limit;

static int node_field(char *node, int offset)
{
    return *(int *)(node + offset);
}

void car_camera(char *car)
{
    char *nodes;
    char *node;
    int count;
    int i;
    int index;
    int dx;
    int dz;
    int dy;
    int node_y;
    int rem;
    int old_flag;
    int new_flag;
    int saved_new;
    float dxf;
    float dzf;
    float dyf;
    float dxx;
    float dzz;
    float dyy;
    float d;
    float h;
    float total;
    float a1;
    float a2;
    float best;
    float f;

    *(int *)(car + 0x58c) = *(int *)car;
    *(int *)(car + 0x554) = 0;
    best = car_camera_far_limit;
    nodes = *(char **)DAT_0010f90c;
    count = *(int *)(DAT_0010f90c + 0x1c);

    i = 0;
    node = nodes;
    while (i < count) {
        dxf = (float)(node_field(node, 4) - *(int *)(car + 0x498));
        dzf = (float)(node_field(node, 8) - *(int *)(car + 0x4a0));
        dxx = dxf * dxf;
        dzz = dzf * dzf;
        d = (float)sqrt((double)(dxx + dzz));
        if ((*(unsigned int *)&d & 0x7fffffff) != 0) {
            if (d < best) {
                best = (float)(int)d;
                *(int *)(car + 0x554) = i;
            }
        }
        node += 0x38;
        i = i + 1;
    }

    index = *(int *)(car + 0x554);
    node = nodes + index * 0x38;

    dx = node_field(node, 4) - *(int *)(car + 0x498);
    dxf = (float)dx;
    dxx = dxf * dxf;
    dz = node_field(node, 8) - *(int *)(car + 0x4a0);
    dzf = (float)dz;
    dzz = dzf * dzf;
    dy = node_field(node, 0xc) - *(int *)(car + 0x49c);
    dyf = (float)dy;
    dyy = dyf * dyf;

    h = (float)sqrt((double)(dxx + dzz));
    a1 = (float)asin((double)(dxf / h));
    if ((dzf / h) < car_camera_zero) {
        a1 = DAT_0010092c - a1;
    }
    *(float *)(car + 0x52c) = a1 + DAT_00100930;

    total = (float)sqrt((double)(dxx + dzz + dyy));
    a2 = (float)asin((double)(h / total));
    if (a2 < car_camera_zero) {
        a2 = -a2;
    }
    *(float *)(car + 0x528) = DAT_0010092c - a2;

    *(int *)(car + 0x530) = 0;
    *(int *)(car + 0x4f8) = node_field(node, 4);
    *(int *)(car + 0x504) = *(int *)(car + 0x4f8);
    *(int *)(car + 0x4fc) = node_field(node, 0xc);
    *(int *)(car + 0x500) = node_field(node, 8);
    *(int *)(car + 0x50c) = *(int *)(car + 0x500);

    if (*(int *)(car + 0x554) != *(int *)(car + 0x558)) {
        dx = node_field(node, 4) - *(int *)(car + 0x16c);
        dz = node_field(node, 8) - *(int *)(car + 0x174);
        dxf = (float)dx;
        dzf = (float)dz;
        *(double *)(car + 0x55c) =
            sqrt((double)(dxf * dxf + dzf * dzf));
        *(int *)(car + 0x558) = *(int *)(car + 0x554);
        *(int *)(car + 0x578) = 1;
        *(int *)(car + 0x564) = *(int *)(car + 0x578);
        *(int *)(car + 0x550) = 0;
        *(int *)(car + 0x54c) = *(int *)(car + 0x550);
        *(int *)(car + 0x56c) = *(int *)(car + 0x54c);
    } else {
        *(int *)(car + 0x56c) = 0;
        *(int *)(car + 0x578) = *(int *)(car + 0x56c);
        *(int *)(car + 0x564) = *(int *)(car + 0x578);
    }

    node_y = node_field(node, 0xc);
    if (node_y < 0) {
        node_y = -node_y;
    }
    rem = node_y % 10;

    old_flag = *(int *)(car + 0x568);
    if ((rem & 1) != 0) {
        new_flag = 1;
    } else if ((rem & 2) != 0) {
        if (*(double *)(car + 0x55c) <= (double)h) {
            new_flag = 1;
        } else if ((rem & 4) == 0) {
            new_flag = 0;
        } else if (*(double *)(car + 0x55c) < (double)h) {
            new_flag = 0;
        } else {
            new_flag = 1;
        }
    } else if ((rem & 4) == 0) {
        new_flag = 0;
    } else if (*(double *)(car + 0x55c) < (double)h) {
        new_flag = 0;
    } else {
        new_flag = 1;
    }
    *(int *)(car + 0x568) = new_flag;

    saved_new = new_flag;
    if (new_flag != old_flag && *(int *)(car + 0x564) == 0) {
        saved_new = new_flag;
        *(int *)(car + 0x568) = old_flag;
        *(int *)(car + 0x56c) = 1;
    }

    if (*(int *)(car + 0x568) != 0) {
        f = (float)(int)(total + DAT_00100934);
        if (f > car_camera_zero) {
            double yaw;
            double pitch;
            double sine_yaw;
            double cosine_yaw;
            double cosine_pitch;
            double sine_pitch;

            yaw = (double)*(float *)(car + 0x52c);
            pitch = (double)*(float *)(car + 0x528);
            sine_yaw = sin(yaw);
            cosine_pitch = cos(pitch);
            sine_pitch = sin(pitch);
            cosine_yaw = cos(yaw);
            *(int *)(car + 0x4f8) = (int)(
                (double)*(int *)(car + 0x4f8)
                + cosine_pitch * sine_yaw * (double)f);
            *(int *)(car + 0x4fc) = (int)(
                (double)*(int *)(car + 0x4fc)
                + sine_pitch * (double)f);
            *(int *)(car + 0x500) = (int)(
                (double)*(int *)(car + 0x500)
                - cosine_pitch * cosine_yaw * (double)f);
        }
    }

    if (*(int *)(car + 0x56c) != 0) {
        *(int *)(car + 0x568) = saved_new;
        *(int *)(car + 0x574) = 0;
        DAT_00101e04 = 2;
        player_view_setup(car + 0x4f8);
    } else {
        *(int *)(car + 0x574) = 1;
        *(double *)(car + 0x55c) = (double)h;
    }
    view_render(car + 0x4f8);
}
