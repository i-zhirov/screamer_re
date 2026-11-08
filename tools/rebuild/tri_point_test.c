/* The rebuilt function at linear 0x1c38a in STARTH.EXE.
 *
 * The plan row records a 243-byte body of 73 instructions up to the first
 * `ret` at 0x1c47c.  The function continues through seven early exits and ends
 * at the `ret` at 0x1c545.  The next function starts at 0x1c548.  The next
 * record starts at 0x1c760, so the store record keeps the 982-byte extent and
 * sets no function size.  The identity check uses 73 instructions.
 *
 * The two direct callers at 0x1c03c and 0x1c114 sit in the two unnamed point
 * probes at 0x1bfd4 and 0x1c0ac.  Both probes walk the polygon entries of a
 * collision shape and select this function when the vertex count word at
 * `+0x06` is 3.  The sibling quad_point_test at 0x1c13c takes the four-vertex
 * case.  The collision_cell_test module at 0x11ccc reaches the probes, so the
 * record takes the section The vehicle state chain.
 *
 * The function takes the polygon entry in ecx and the object in esi.  The
 * entry holds three unsigned short byte offsets at `+0x08`, `+0x0a`, and
 * `+0x0c`.  The global `DAT_000c71c` points to the vertex array, and each
 * offset selects a signed short triple.  The function adds the object
 * translation at `+0x08`, `+0x0c`, and `+0x10` to the vertex components and
 * stores the three world points at `DAT_000c6d4`, `DAT_000c6e0`, and
 * `DAT_000c6ec`.
 *
 * The function forms the bounding box of the three points in the x and y
 * planes at `DAT_000c704`, `DAT_000c708`, `DAT_000c70c`, and `DAT_000c710`.
 * It then tests the point `DAT_000c728` and `DAT_000c730`.  A point outside
 * the box returns 0.  Inside the box the function tests the point against the
 * three edges 0-1, 1-2, and 2-0.  A negative side term on any edge returns 0.
 * The function returns 1 when the point stays on the same side of every edge.
 *
 * The bbox, the edge side terms, the three-vertex case, and the two callers
 * support the name tri_point_test.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The function uses no call and no floating-point instruction.  The original
 * has no EBP frame, so the module builds without `-of+`.
 */
struct point {
    int x;
    int y;
    int z;
};

struct polygon {
    char pad[6];
    short vertex_count;
    unsigned short index[3];
};

struct transform {
    char pad[8];
    int x;
    int y;
    int z;
};

extern struct point DAT_000c6d4[3];
extern int *DAT_000c71c;
extern int DAT_000c704;
extern int DAT_000c708;
extern int DAT_000c70c;
extern int DAT_000c710;
extern int DAT_000c728;
extern int DAT_000c730;

#define EDGE_SIDE(a, b)                                                     \
    (((b)->y - (a)->y) * (px - (a)->x)                                     \
     + ((a)->x - (b)->x) * (py - (a)->y))

int tri_point_test(struct polygon *poly, struct transform *transform)
{
    short *vertex;
    int i;
    int px;
    int py;

    for (i = 0; i < 3; i++) {
        vertex = (short *)((char *)DAT_000c71c + poly->index[i]);
        DAT_000c6d4[i].x = vertex[0] + transform->x;
        DAT_000c6d4[i].y = vertex[1] + transform->y;
        DAT_000c6d4[i].z = vertex[2] + transform->z;
    }

    DAT_000c704 = DAT_000c6d4[0].x;
    DAT_000c70c = DAT_000c6d4[0].x;
    DAT_000c708 = DAT_000c6d4[0].y;
    DAT_000c710 = DAT_000c6d4[0].y;
    for (i = 1; i < 3; i++) {
        if (DAT_000c6d4[i].x > DAT_000c704) {
            DAT_000c704 = DAT_000c6d4[i].x;
        }
        if (DAT_000c6d4[i].x < DAT_000c70c) {
            DAT_000c70c = DAT_000c6d4[i].x;
        }
        if (DAT_000c6d4[i].y > DAT_000c708) {
            DAT_000c708 = DAT_000c6d4[i].y;
        }
        if (DAT_000c6d4[i].y < DAT_000c710) {
            DAT_000c710 = DAT_000c6d4[i].y;
        }
    }

    px = DAT_000c728;
    py = DAT_000c730;
    if (px > DAT_000c704) {
        return 0;
    }
    if (px < DAT_000c70c) {
        return 0;
    }
    if (py > DAT_000c708) {
        return 0;
    }
    if (py < DAT_000c710) {
        return 0;
    }

    if (EDGE_SIDE(&DAT_000c6d4[0], &DAT_000c6d4[1]) < 0) {
        return 0;
    }
    if (EDGE_SIDE(&DAT_000c6d4[1], &DAT_000c6d4[2]) < 0) {
        return 0;
    }
    if (EDGE_SIDE(&DAT_000c6d4[2], &DAT_000c6d4[0]) < 0) {
        return 0;
    }
    return 1;
}
