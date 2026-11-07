/* The rebuilt `collision_cell_test` module.
 *
 * The original sits at linear 0x11ccc in STARTH.EXE and holds 127 instructions
 * and 413 bytes up to the first `ret` at 0x11e68.  The normal exit jumps to the
 * second epilogue at 0x11e69, and the full body ends at the `ret` at 0x11e73.
 * The next function starts at 0x11e74, so the store record keeps the 424-byte
 * extent and sets no function size.  The identity check uses 127 instructions.
 *
 * The only direct caller at 0x11caa is the map collision check at 0x11c00.  That
 * check walks the three-by-three cell neighborhood of the spatial grid at
 * `DAT_00102fa4` and calls this function for each cell with the mode in `eax`.
 * The cell slot address arrives through `DAT_00108034`, so the function takes
 * the section `The vehicle state chain`.
 *
 * The function walks the 0x40-byte collision records of the cell while the
 * current pointer `DAT_0010c744` stays below the end at the count `cell + 0x14`.
 * The first record field selects the object from the handle table
 * `DAT_00021540`.  The object field `+0x18` is the offset to the collision
 * shape, which the function keeps at `DAT_0010c740`.
 *
 * The shape holds the radius at `+0x00`, the centre x as a signed 16-bit value
 * at `+0x04`, and the centre y as a signed 16-bit value at `+0x08`.  The
 * function forms the offset from the world position `DAT_00108038` and
 * `DAT_00108040` to the record position at `+0x08` and `+0x10`.  A squared
 * offset above the squared radius skips the record.
 *
 * Inside the radius the function scales the record angle at `+0x18` by the
 * float 1/1024 at object-3 offset 0x404 and pi at object-3 offset 0x408.  It
 * rotates the offset through 0x2218c and forms the world position at
 * `DAT_0010c728` and `DAT_0010c730`.  It then probes three points ten units
 * apart.  The mode 1 calls 0x1c0ac; any other mode calls 0x1bfd4.  A hit returns
 * 1.  The mode 1 returns at once.  The other mode rotates the pair
 * `DAT_0010c734` and `DAT_0010c73c` back by the negative angle, stores the
 * result, copies the shape byte `+0x6b` to `DAT_00108024`, and returns 1.
 *
 * The map collision check, the record table, and the shape test support the
 * name `collision_cell_test`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The module converts an int to a float only, so the shared `fpu_stub.asm`
 * supplies the FPU run-time symbols.  The original has the EBP frame, so the
 * module builds with `-of+`.
 */
struct rotate_pair {
    int first;
    int pad;
    int second;
};

struct collision_cell {
    int *records;
    int unknown[4];
    int count;
};

extern int *DAT_00108034;
extern int DAT_00108038;
extern int DAT_00108040;
extern int DAT_00108024;
extern int *DAT_00021540;
extern char *DAT_0010c740;
extern char *DAT_0010c744;
extern char *DAT_0010c748;
extern int DAT_0010c728;
extern int DAT_0010c730;
extern int DAT_0010c734;
extern int DAT_0010c73c;
extern float DAT_00100404;
extern float DAT_00100408;

extern int sub_1bfd4(void);
extern int sub_1c0ac(void);
extern void rotate_pair(struct rotate_pair *out, struct rotate_pair *in,
                        float angle);

int collision_cell_test(int mode)
{
    struct collision_cell *cell;
    char *record;
    char *end;
    char *object;
    char *shape;
    struct rotate_pair in;
    struct rotate_pair out;
    struct rotate_pair back_in;
    struct rotate_pair back_out;
    int dx;
    int dy;
    int a;
    int b;
    int radius;
    int k;
    float angle;

    cell = (struct collision_cell *)*DAT_00108034;
    DAT_0010c744 = (char *)cell->records;
    end = DAT_0010c744 + cell->count * 0x40;
    while (DAT_0010c744 < end) {
        record = DAT_0010c744;
        object = (char *)DAT_00021540[*(int *)record];
        DAT_0010c748 = object;
        shape = object + *(int *)(object + 0x18);
        DAT_0010c740 = shape;
        dx = DAT_00108038 - *(int *)(record + 8);
        dy = DAT_00108040 - *(int *)(record + 0x10);
        a = *(int *)(shape + 2) >> 16;
        b = *(int *)(shape + 6) >> 16;
        radius = *(int *)shape;
        if ((dx - a) * (dx - a) + (dy - b) * (dy - b) > radius * radius) {
            DAT_0010c744 = DAT_0010c744 + 0x40;
            continue;
        }
        angle = (float)*(int *)(record + 0x18) * DAT_00100404
            * DAT_00100408;
        in.first = dx;
        in.second = dy;
        rotate_pair(&out, &in, angle);
        DAT_0010c728 = out.first + *(int *)(record + 8);
        DAT_0010c730 = out.second + *(int *)(record + 0x10);
        for (k = 3; k > 0; k--) {
            if (mode == 1) {
                if (sub_1c0ac() != 0) {
                    return 1;
                }
            } else {
                if (sub_1bfd4() != 0) {
                    back_in.first = DAT_0010c734;
                    back_in.second = DAT_0010c73c;
                    rotate_pair(&back_out, &back_in, -angle);
                    DAT_0010c734 = back_out.first;
                    DAT_0010c73c = back_out.second;
                    DAT_00108024 = (unsigned char)shape[0x6b];
                    return 1;
                }
            }
            DAT_0010c728 = DAT_0010c728 + 10;
            DAT_0010c730 = DAT_0010c730 + 10;
        }
        DAT_0010c744 = DAT_0010c744 + 0x40;
    }
    return 0;
}
