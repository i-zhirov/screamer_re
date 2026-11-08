/* The rebuilt `matrix_compose` module.
 *
 * The original sits at linear 0x82258 in STARTH.EXE and holds 127 instructions
 * and 365 bytes up to the `ret` at 0x823c4.  The next recorded function starts
 * at 0x82c14, so the store record keeps the 2492-byte extent and sets no
 * separate function size.  The identity check uses 127 instructions.
 *
 * The two direct callers at 0x8b088 and 0x8b174 compose the matrix at an
 * object `+0x28` with the basis at `+0x48` into the block at `+0xa0`.  The
 * second pass composes the result with the camera basis at `+0x18`, and the
 * other two calls compose the remaining render blocks.  The record takes the
 * name `matrix_compose` with the confidence inferred.
 *
 * The function composes two 4x3 affine matrices in Q14 fixed point.  The
 * source matrix `source` holds four columns of three rows: the first three
 * columns are the rotation and the fourth column is the translation.  The
 * matrix `matrix` holds four rows of three columns: the first three rows are
 * the rotation and the fourth row is the translation.  The result
 * `destination` takes the same layout as `source`.
 *
 * For each column `k` the function forms the three rotated components and the
 * translation component:
 *
 *     destination[k]     = (m0  * a + m1  * b + m2  * c) >> 14
 *     destination[3 + k] = (m3  * a + m4  * b + m5  * c) >> 14
 *     destination[6 + k] = (m6  * a + m7  * b + m8  * c) >> 14
 *     destination[9 + k] = (m9 * a + m10 * b + m11 * c) >> 14 + source[9 + k]
 *
 * where `a`, `b`, and `c` are `source[k]`, `source[3 + k]`, and
 * `source[6 + k]`.  The shift is arithmetic.
 *
 * The callers pass the source pointer in esi, the matrix pointer in ebp, and
 * the destination pointer in edi.  The original has no EBP frame and no saved
 * register.  The C compiler cannot use ebp as a parameter register, so the
 * rebuild uses the standard argument passing.  The identity check stays open
 * on the register use.
 */

#define COMPOSE(k)                                                          \
    do {                                                                    \
        int a = source[(k)];                                                \
        int b = source[3 + (k)];                                            \
        int c = source[6 + (k)];                                            \
        destination[(k)] =                                                  \
            (matrix[0] * a + matrix[1] * b + matrix[2] * c) >> 14;          \
        destination[3 + (k)] =                                              \
            (matrix[3] * a + matrix[4] * b + matrix[5] * c) >> 14;          \
        destination[6 + (k)] =                                              \
            (matrix[6] * a + matrix[7] * b + matrix[8] * c) >> 14;          \
        destination[9 + (k)] =                                              \
            ((matrix[9] * a + matrix[10] * b + matrix[11] * c) >> 14)       \
            + source[9 + (k)];                                              \
    } while (0)

/* Keep one initialized data byte in the image.  The DOS4G loader leaves the
 * BSS object unmapped when the data object holds no page, and this module holds
 * only BSS state.  The harness reads this marker only to keep the page. */
int matrix_compose_marker = 0;

void matrix_compose(int *source, int *matrix, int *destination)
{
    COMPOSE(0);
    COMPOSE(1);
    COMPOSE(2);
}
