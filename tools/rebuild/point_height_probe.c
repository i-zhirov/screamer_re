/* The rebuilt `point_height_probe` module.
 *
 * The original sits at linear 0x1bfd4 in STARTH.EXE.  The body holds 46
 * instructions up to the first `ret` at 0x1c07e.  The body sweep stops there,
 * so the identity check uses 46 instructions.  A second block at 0x1c080 holds
 * the hit path, and the full body ends at the `ret` at 0x1c0aa.  The next
 * record starts at 0x1c0ac, so the store record keeps the 216-byte extent and
 * sets no separate function size.
 *
 * The only direct caller at 0x11dd5 is collision_cell_test at 0x11ccc.  That
 * function has a mode branch.  The mode 1 branch calls the plain hit probe at
 * 0x1c0ac.  Every other mode calls this function.  The caller walks the
 * collision records of a spatial cell and probes three points ten units apart.
 * The record takes the section The vehicle state chain.
 *
 * The global DAT_000c740 holds the collision shape, the global DAT_000c744
 * holds the current collision record, and the global DAT_000c748 holds the
 * object.  The shape holds the x base at +0x00, the height base at +0x04, the
 * y base at +0x08, the entry count at +0x14, the vertex array offset at +0x1c,
 * and the entry index table offset at +0x28.
 *
 * The function builds the entry index table at DAT_000c724 and the vertex array
 * base at DAT_000c71c.  It loops over the entry count.  Each unsigned short in
 * the index table selects one polygon entry.  A vertex count word of 4 selects
 * quad_point_test.  Any other count selects tri_point_test.  The callee takes
 * the polygon entry and the current record.
 *
 * A hit with a zero edge divisor at entry +0x18 moves to the next entry.  A hit
 * with a non-zero divisor stores the three entry words into DAT_000c734,
 * DAT_000c738, and DAT_000c73c.  It then stores the surface height into
 * DAT_000c72c and returns 1.  The height is the shape height base plus the
 * truncated quotient of the negated point dot product and the divisor.  A full
 * sweep returns 0.
 *
 * The hit path, the height, and the surface direction support the name
 * `point_height_probe`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The function uses no floating-point instruction.  The original uses no EBP
 * frame, so the module builds without `-of+`.
 */
struct polygon_entry {
    char pad00[6];
    short vertex_count;         /* +0x06 */
    char pad08[0x14 - 0x08];
    int field14;                /* +0x14 */
    int field18;                /* +0x18 */
    int field1c;                /* +0x1c */
};

struct collision_shape {
    int x;                      /* +0x00 */
    int height;                 /* +0x04 */
    int y;                      /* +0x08 */
    char pad0c[0x14 - 0x0c];
    int count;                  /* +0x14 */
    char pad18[0x1c - 0x18];
    int vertex_offset;          /* +0x1c */
    char pad20[0x28 - 0x20];
    int entry_offset;           /* +0x28 */
};

extern char *DAT_000c740;
extern char *DAT_000c744;
extern char *DAT_000c748;
extern int *DAT_000c71c;
extern int DAT_000c714;
extern int DAT_000c718;
extern char *DAT_000c720;
extern char *DAT_000c724;
extern int DAT_000c728;
extern int DAT_000c72c;
extern int DAT_000c730;
extern int DAT_000c734;
extern int DAT_000c738;
extern int DAT_000c73c;

extern int tri_point_test(void *poly, void *record);
extern int quad_point_test(void *poly, void *record);

int point_height_probe(void)
{
    struct collision_shape *shape;
    struct polygon_entry *entry;
    int index;
    int count;
    int result;
    int dx;
    int dy;
    int a;
    int b;
    int c;

    shape = (struct collision_shape *)DAT_000c740;
    DAT_000c724 = (char *)shape + shape->entry_offset;
    DAT_000c71c = (int *)(DAT_000c748 + shape->vertex_offset);
    count = shape->count;
    DAT_000c718 = count;
    index = 0;
    DAT_000c714 = index;
    for (;;) {
        entry = (struct polygon_entry *)((char *)shape
            + *(unsigned short *)(DAT_000c724 + index * 2));
        DAT_000c720 = (char *)entry;
        if (entry->vertex_count == 4) {
            result = quad_point_test(entry, DAT_000c744);
        } else {
            result = tri_point_test(entry, DAT_000c744);
        }
        if (result != 0) {
            dx = DAT_000c728 - shape->x;
            dy = DAT_000c730 - shape->y;
            a = entry->field14;
            b = entry->field18;
            c = entry->field1c;
            if (b != 0) {
                DAT_000c734 = a;
                DAT_000c738 = b;
                DAT_000c73c = c;
                DAT_000c72c = shape->height + (-(dx * a + dy * c)) / b;
                return 1;
            }
        }
        index++;
        DAT_000c714 = index;
        if (index != count) {
            continue;
        }
        return 0;
    }
}
