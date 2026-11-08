/* The rebuilt function at linear 0x1c13c in STARTH.EXE.
 *
 * The plan row records a 338-byte body of 107 instructions up to the first
 * `ret` at 0x1c28d.  The function continues through three more early exits and
 * ends at the `ret` at 0x1c389.  The next function starts at 0x1c38a, so the
 * store record keeps the 590-byte extent and sets no function size.  The
 * identity check uses 107 instructions.
 *
 * The two direct callers at 0x1c03c and 0x1c114 sit in the two unnamed point
 * probes at 0x1bfd4 and 0x1c0ac.  Both probes walk the polygon entries of a
 * collision shape and select this function when the vertex count word at
 * `+0x06` is 4.  The sibling at 0x1c38a takes the three-vertex case.  The
 * collision_cell_test module at 0x11ccc reaches the probes, so the record
 * takes the section The vehicle state chain.
 *
 * The function takes the polygon entry in ecx and the object in esi.  The
 * entry holds four unsigned byte offsets at `+0x08`, `+0x0a`, `+0x0c`, and
 * `+0x0e`.  The global `DAT_000c71c` points to the vertex array, and each
 * offset selects a signed short triple.  The function adds the object
 * translation at `+0x08`, `+0x0c`, and `+0x10` to the three vertex
 * components and stores the four world points in the scratch array
 * `DAT_000c6d4`.
 *
 * The function forms the bounding box of the four points in the x and y
 * planes at `DAT_000c704`, `DAT_000c708`, `DAT_000c70c`, and `DAT_000c710`.
 * It then tests the point `DAT_000c728` and `DAT_000c730`.  A point outside
 * the box returns 0.  Inside the box the function tests the point against the
 * four edges 0-1, 1-2, 2-3, and 3-0.  A negative side term on any edge returns
 * 0.  The function returns 1 when the point stays on the same side of every
 * edge.
 *
 * The bbox, the edge side terms, and the two callers support the name
 * quad_point_test.  The original has no symbol, so the record keeps the name
 * with the confidence inferred.
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
    unsigned short index[4];
};

struct transform {
    char pad[8];
    int x;
    int y;
    int z;
};

extern struct point DAT_000c6d4[4];
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

int quad_point_test(struct polygon *poly, struct transform *transform)
{
    short *vertex;
    int i;
    int px;
    int py;

    for (i = 0; i < 4; i++) {
        vertex = (short *)((char *)DAT_000c71c + poly->index[i]);
        DAT_000c6d4[i].x = vertex[0] + transform->x;
        DAT_000c6d4[i].y = vertex[1] + transform->y;
        DAT_000c6d4[i].z = vertex[2] + transform->z;
    }

    DAT_000c704 = DAT_000c6d4[0].x;
    DAT_000c70c = DAT_000c6d4[0].x;
    DAT_000c708 = DAT_000c6d4[0].y;
    DAT_000c710 = DAT_000c6d4[0].y;
    for (i = 1; i < 4; i++) {
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
    if (EDGE_SIDE(&DAT_000c6d4[2], &DAT_000c6d4[3]) < 0) {
        return 0;
    }
    if (EDGE_SIDE(&DAT_000c6d4[3], &DAT_000c6d4[0]) < 0) {
        return 0;
    }
    return 1;
}
