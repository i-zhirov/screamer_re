/* The rebuilt `opponent_route_setup` module.
 *
 * The original starts at linear 0x1517c in STARTH.EXE and holds 137
 * instructions and 499 bytes up to the `ret` at 0x1536e.  The function
 * continues through the `ret` at 0x15377, so the record keeps the 508-byte
 * extent to the next function at 0x15378 and sets no function size.  The
 * branch at 0x152f8 flies over the first `ret` to the common block at
 * 0x151ba, so the identity check uses 137 instructions.
 *
 * The only direct caller at 0x1354c is `race_track_load` at 0x13540, which
 * `race_session_start` at 0x128e0 runs at 0x1294e.  The caller passes the
 * level value and drops a non-zero result.
 *
 * The spline table at `DAT_0010d6f4` comes from the level archive through
 * `spline_load` at 0x1af70.  Its first record holds the point count at
 * `+0x1c`, the first node index at `+0x20`, and the point array pointer at
 * `+0x00`.  The count is copied to `DAT_0010d6f8`; a zero spline count
 * `DAT_0010d6fc` clears it.
 *
 * The function walks the 0x38-byte point records from the first node index.
 * A point holds the x position at `+0x04`, the z position at `+0x08`, the
 * model index at `+0x0c`, and the next node index at `+0x10`.
 *
 * A non-zero load flag loads the model name at
 * `0xf000 + model * 0x24 + (DAT_0011d274 & 3) * 0x0c` from the object table
 * at 0x6530 through 0x11060.  The load stores the object slot at `+0x18` of
 * the record, and a non-zero result returns at once.
 *
 * The function fills one 0x40-byte record at `DAT_0010d700 + index * 0x40`: the
 * x and z positions at `+0x00` and `+0x08`, the ground height at `+0x04` from
 * the map collision check at 0x11c00, the track heading at `+0x10` from
 * `track_nearest_node` at 0x120c0, the two angles at `+0x0c` and `+0x14` from
 * the rotation of `DAT_0010c734` and `DAT_0010c73c` through 0x2218c, and the
 * flags `+0x20` and `+0x38` plus the clear word `+0x3c`.  The record proceeds
 * along the next node index.  The function returns 0 after the last record.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * rotation helper at 0x2218c writes two int values 8 bytes apart, so the
 * module uses the pair structure below.  The module converts an int to a
 * float only, so the shared `fpu_stub.asm` supplies the FPU run-time symbols.
 */
struct rotate_pair {
    int first;
    int pad;
    int second;
};

extern void *DAT_0010d6f4;
extern int DAT_0010d6fc;
extern int DAT_0010d6f8;
extern char DAT_0010d700[];
extern int DAT_0011d274;
extern int DAT_00108038;
extern int DAT_00108040;
extern float DAT_00108048;
extern int DAT_0010c72c;
extern int DAT_0010c734;
extern int DAT_0010c73c;
extern float DAT_00100633;
extern float DAT_00100637;
extern float DAT_0010063b;

extern int object_load_named(void *table, char *name, int *slot);
extern int map_collision(int mode);
extern int track_nearest_node(int mode);
extern void rotate_pair(struct rotate_pair *out, struct rotate_pair *in,
                        float angle);

int opponent_route_setup(int load)
{
    char *points;
    char *record;
    char *point;
    struct rotate_pair in;
    struct rotate_pair out;
    int result;
    int node;
    int count;
    int index;

    count = *(int *)((char *)DAT_0010d6f4 + 0x1c);
    DAT_0010d6f8 = count;
    if (DAT_0010d6fc == 0) {
        DAT_0010d6f8 = 0;
    }
    points = *(char **)DAT_0010d6f4;
    node = *(int *)((char *)DAT_0010d6f4 + 0x20);
    index = 0;
    while (index < DAT_0010d6f8) {
        record = DAT_0010d700 + index * 0x40;
        if (load != 0) {
            point = points + node * 0x38;
            result = object_load_named((void *)0x6530,
                (char *)(0xf000 + *(int *)(point + 0xc) * 0x24
                         + (DAT_0011d274 & 3) * 0xc),
                (int *)(record + 0x18));
            if (result != 0) {
                return result;
            }
        }
        point = points + node * 0x38;
        *(int *)record = *(int *)(point + 4);
        DAT_00108038 = *(int *)record;
        *(int *)(record + 8) = *(int *)(point + 8);
        DAT_00108040 = *(int *)(record + 8);
        map_collision(0);
        *(int *)(record + 4) = DAT_0010c72c;
        track_nearest_node(0);
        *(float *)(record + 0x10) = DAT_00108048;
        in.first = DAT_0010c734;
        in.second = DAT_0010c73c;
        rotate_pair(&out, &in, *(float *)(record + 0x10) + DAT_00100633);
        *(float *)(record + 0xc) = (float)out.first
            * DAT_00100637 / DAT_0010063b;
        *(float *)(record + 0x14) = (float)out.second
            * DAT_00100637 / DAT_0010063b;
        *(int *)(record + 0x20) = 1;
        *(int *)(record + 0x38) = 0;
        *(int *)(record + 0x3c) = *(int *)(record + 0x38);
        node = *(int *)(point + 0x10);
        index = index + 1;
    }
    return 0;
}
