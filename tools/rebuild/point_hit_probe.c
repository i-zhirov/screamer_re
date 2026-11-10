/* The rebuilt `point_hit_probe` module.
 *
 * The original sits at linear 0x1c0ac in STARTH.EXE.  The body holds 32
 * instructions and 120 bytes up to the first `ret` at 0x1c123.  The body sweep
 * stops there, so the identity check uses 32 instructions.  The loop tail runs
 * past that `ret` through the second `ret` at 0x1c13a, and the next record
 * starts at 0x1c13c, so the store record keeps the 144-byte extent and sets no
 * separate function size.
 *
 * The only direct caller at 0x11e55 is collision_cell_test at 0x11ccc.  That
 * function has a mode branch.  The mode 1 branch calls this plain hit probe.
 * Every other mode calls point_height_probe at 0x1bfd4.  The caller walks the
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
 * A hit returns 1 at once and stores no height and no direction.  A full sweep
 * returns 0.  The body is the plain form of point_height_probe, which also
 * stores the surface height and direction on a hit.
 *
 * The caller, the shape walk, and the sibling point_height_probe support the
 * name `point_hit_probe`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The function uses no floating-point instruction.  The original uses no EBP
 * frame, so the module builds without `-of+`.
 */
struct polygon_entry {
    char pad00[6];
    short vertex_count;         /* +0x06 */
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

extern int tri_point_test(void *poly, void *record);
extern int quad_point_test(void *poly, void *record);

int point_hit_probe(void)
{
    struct collision_shape *shape;
    struct polygon_entry *entry;
    int index;
    int count;
    int result;

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
            return 1;
        }
        index++;
        DAT_000c714 = index;
        if (index != count) {
            continue;
        }
        return 0;
    }
}
