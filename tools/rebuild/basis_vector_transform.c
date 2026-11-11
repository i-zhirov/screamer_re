/* The rebuilt `basis_vector_transform` module.
 *
 * The original sits at linear 0x81f80 in STARTH.EXE and holds 28 instructions
 * and 78 bytes up to the `ret` at 0x81fcd.  The next function starts at
 * 0x81fd0, so the store record keeps the 80-byte extent and sets no separate
 * function size.  The identity check uses 28 instructions.
 *
 * The three direct call sites are 0x8b607 in the second basis builder at
 * 0x8b51c, 0x8b6e8 in `basis_transform_build` at 0x8b610, and 0x8b7c8 in the
 * next builder at 0x8b6ee.  Each caller passes the matrix pointer in esi, the
 * three vector components in eax, ebx, and ecx, and stores the result in the
 * same record.
 * The callers sit in the object update chain, so the record takes the section
 * object_update_all callees.
 *
 * The record holds a 3x3 matrix of nine Q14 dwords at `+0x00` through
 * `+0x20`.  The function multiplies the matrix by the vector `(x, y, z)` and
 * stores the three Q14 results at `+0x24`, `+0x28`, and `+0x2c`:
 *
 *     result[0] = (matrix[0] * x + matrix[3] * y + matrix[6] * z) >> 14
 *     result[1] = (matrix[1] * x + matrix[4] * y + matrix[7] * z) >> 14
 *     result[2] = (matrix[2] * x + matrix[5] * y + matrix[8] * z) >> 14
 *
 * The shift is arithmetic.  `basis_transform_build` calls this function with
 * the negated position, so the result is the inverse translation at `+0x24`,
 * `+0x28`, and `+0x2c` of the inverse basis.  The caller data and the existing
 * `basis_vector_transform` reference in the `basis_transform_build` module
 * support the name with the confidence inferred.  The original has no symbol.
 *
 * The original has no EBP frame and uses ebp and edi as scratch registers, so
 * the module builds without `-of+`.  The original uses no floating-point
 * instruction, so the module needs no FPU stub.  The module uses no assembly
 * implementation.
 */

void basis_vector_transform(int *matrix, int x, int y, int z)
{
    matrix[9] = (matrix[0] * x + matrix[3] * y + matrix[6] * z) >> 14;
    matrix[10] = (matrix[1] * x + matrix[4] * y + matrix[7] * z) >> 14;
    matrix[11] = (matrix[2] * x + matrix[5] * y + matrix[8] * z) >> 14;
}
