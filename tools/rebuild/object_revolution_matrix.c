/* The rebuilt function at linear 0x1d55c in STARTH.EXE.
 *
 * The original body holds 103 instructions and 350 bytes up to the `ret` at
 * 0x1d6b9.  The next function starts at 0x1d6bc, so the store record keeps the
 * 352-byte extent and sets no function size.  The identity check uses 103
 * instructions.
 *
 * The function takes the object pointer in eax.  It reads the gear index at
 * `+0x248` and the entry index at `+0x24c`.  The index selects a 36-byte record
 * in the table at `DAT_0010f8e4`.  The first dword of the record points to an
 * entry array with a 56-byte stride.  The entry holds a signed integer array at
 * `+0x14` with a three-int stride, three two-dimensional points, and the next
 * entry index at `+0x10`.
 *
 * The function builds three two-dimensional control points from the current
 * entry and the next entry.  The gear flag at `+0x254` selects the window:
 *
 *     flag 0:  first point of the current entry, second point of the current
 *              entry, first point of the next entry
 *     flag 1:  second point of the current entry, first point of the next
 *              entry, second point of the next entry
 *
 * Each selected x and y pair converts from a signed integer to a float and
 * lands at `+0x284` with a three-float stride.  The z slots stay unchanged.
 * The function then calls the two-dimensional spline sampler at 0x1e7b4 with
 * the output at `+0x2a8`, the control points at `+0x284`, and the fraction at
 * `+0x250`.
 *
 * The only direct caller is `object_revolution_step` at 0x1d30c, which calls
 * this function to build the matrix.  The caller names the call the matrix
 * build, so the record takes the name `object_revolution_matrix` and the
 * section `The vehicle state chain`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned int DAT_0010f8e4;
extern void func_1e7b4(float *output, float *points, float fraction);

void object_revolution_matrix(char *object)
{
    char *table;
    char *current;
    char *next;
    int *first;
    int *second;
    int index;
    int step;

    table = (char *)DAT_0010f8e4;
    index = *(int *)(object + 0x248);
    step = *(int *)(object + 0x24c);
    current = *(char **)(table + index * 36) + step * 56;
    first = (int *)(current + 0x14);
    step = *(int *)(current + 0x10);
    next = *(char **)(table + index * 36) + step * 56;
    second = (int *)(next + 0x14);

    if (*(int *)(object + 0x254) == 0) {
        *(float *)(object + 0x284) = (float)first[0];
        *(float *)(object + 0x288) = (float)first[1];
        *(float *)(object + 0x290) = (float)first[3];
        *(float *)(object + 0x294) = (float)first[4];
        *(float *)(object + 0x29c) = (float)second[0];
        *(float *)(object + 0x2a0) = (float)second[1];
    } else {
        *(float *)(object + 0x284) = (float)first[3];
        *(float *)(object + 0x288) = (float)first[4];
        *(float *)(object + 0x290) = (float)second[0];
        *(float *)(object + 0x294) = (float)second[1];
        *(float *)(object + 0x29c) = (float)second[3];
        *(float *)(object + 0x2a0) = (float)second[4];
    }

    func_1e7b4((float *)(object + 0x2a8), (float *)(object + 0x284),
               *(float *)(object + 0x250));
}
