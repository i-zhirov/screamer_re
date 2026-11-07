/* The rebuilt `opponent_curve_eval` module.
 *
 * The original sits at linear 0x22378 in STARTH.EXE and holds 121 instructions
 * and 353 bytes up to the `ret` at 0x224d8.  The next function starts at
 * 0x224dc after the three padding bytes at 0x224d9, so the store record keeps
 * the 356-byte extent and sets no separate function size.  The identity check
 * uses 121 instructions.
 *
 * The only callers are `opponent_update` at 0x20954 and `opponent_path_update`
 * at 0x20b1c.  Both pass the state base `DAT_0010f174` and the path record.
 * They sit in the object_update_all group, so the module takes the section
 * object_update_all callees.
 *
 * The path record holds the track index at `+0`, the node index at `+4`, the
 * segment phase at `+8`, and the direction flag at `+0xc`.  The state base
 * holds a pointer at `track * 36` to an array of 56-byte nodes.  A node holds
 * the scalar value at `+0xc`, the next node index at `+0x10`, and two integer
 * points: the first at `+0x14`, `+0x18`, and `+0x1c`, and the second at
 * `+0x20`, `+0x24`, and `+0x28`.
 *
 * The function selects three control points from the two nodes.  A clear
 * direction flag takes the first and the second point of the first node and
 * the first point of the second node.  A set direction flag takes the second
 * point of the first node and the first and the second point of the second
 * node.  The function converts the x and the y value of each point to a float
 * at `+0x3c`, `+0x48`, and `+0x54`, and it calls the 2D curve sampler at
 * 0x1e7b4.  The sampler writes the target position at `+0x60` and `+0x64` and
 * the forward tangent at `+0x6c` and `+0x70`.
 *
 * The function then interpolates the node scalar value:
 *
 *     ty = scalar1 + 0.5 * (scalar2 - scalar1) * (flag + phase)
 *
 * and writes the result to `+0x68`.  The double 0.5 arrives through a global,
 * because the minimal image does not load the constant segment.  The original
 * has the EBP frame, so the module builds with `-of+`.
 */
extern double opponent_curve_basis_scale;

extern void opponent_curve_sample(float *out, float *points, float t);

void opponent_curve_eval(unsigned int state, void *body)
{
    char *path = (char *)body;
    char *entry;
    char *base;
    char *node1;
    char *node2;
    int *first;
    int *second;
    float *points;
    int flag;
    float value;

    entry = (char *)(state + (unsigned int)(*(int *)path * 36));
    base = *(char **)entry;
    node1 = base + *(int *)(path + 4) * 56;
    node2 = base + *(int *)(node1 + 0x10) * 56;
    first = (int *)(node1 + 0x14);
    second = (int *)(node2 + 0x14);
    points = (float *)(path + 0x3c);
    flag = *(int *)(path + 0xc);

    if (flag == 0) {
        points[0] = (float)first[0];
        points[1] = (float)first[1];
        points[3] = (float)first[3];
        points[4] = (float)first[4];
        points[6] = (float)second[0];
        points[7] = (float)second[1];
    } else {
        points[0] = (float)first[3];
        points[1] = (float)first[4];
        points[3] = (float)second[0];
        points[4] = (float)second[1];
        points[6] = (float)second[3];
        points[7] = (float)second[4];
    }

    opponent_curve_sample((float *)(path + 0x60), points,
        *(float *)(path + 8));

    value = (float)(*(int *)(node2 + 0xc) - *(int *)(node1 + 0xc))
        * ((float)flag + *(float *)(path + 8));
    *(float *)(path + 0x68) = (float)((double)value
        * opponent_curve_basis_scale + (double)*(int *)(node1 + 0xc));
}
