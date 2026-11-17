/* The rebuilt `basis_rotation_mirror` module.
 *
 * The original sits at linear 0x82074 in STARTH.EXE and holds 58 instructions
 * and 163 bytes up to the `ret` at 0x82116.  The next recorded function starts
 * at 0x82258, so the store record keeps the 484-byte extent and sets no
 * separate function size.  The identity check uses 58 instructions.
 *
 * The only direct caller at 0x8b6c7 is `basis_transform_build` at 0x8b610.
 * It passes the destination pointer in `edx`, the three cosine values in
 * `ecx`, `ebx`, and `ebp`, and the three sine values in `esi`, `edi`, and
 * `eax`.  The caller runs this function on the complement angles `0x7ff -
 * angle`, so the result is the inverse basis.  The record takes the section
 * object_update_all callees.
 *
 * The function builds the nine Q14 dwords of the 3x3 matrix at `+0x00` through
 * `+0x20`.  It multiplies two Q14 values, sums the two products, and shifts the
 * sum to the right by 14.  It shifts the two inner products `c0 * c1` and
 * `c0 * s1` before the outer product, so the two stages match the original.
 * The negative values use the arithmetic shift, which rounds toward negative
 * infinity.
 *
 * The sibling builder at 0x81fd0 builds the forward basis.  The existing
 * `basis_transform_build` module names this caller-side function
 * `basis_rotation_mirror`, so the record keeps that name with the confidence
 * inferred.  The original has no symbol.
 *
 * The original has no EBP frame, so the module builds without `-of+`.  The
 * function uses no floating-point instruction, so it needs no FPU stub.  The
 * rebuilt C uses the standard Watcom calling convention, so the identity check
 * differs in the prologue and in the register use.  The behavior check covers
 * the arithmetic.
 */
#define FIXED_SHIFT 14

void basis_rotation_mirror(int *matrix, int cos0, int sin0,
    int cos1, int sin1, int cos2, int sin2)
{
    int cos0_cos1;
    int cos0_sin1;

    matrix[3] = -((sin0 * cos2) >> FIXED_SHIFT);
    matrix[4] = (sin0 * sin2) >> FIXED_SHIFT;
    matrix[5] = cos0;
    matrix[2] = (cos1 * sin0) >> FIXED_SHIFT;
    matrix[8] = (sin0 * sin1) >> FIXED_SHIFT;

    cos0_cos1 = (cos0 * cos1) >> FIXED_SHIFT;
    matrix[0] = ((sin1 * sin2) + (cos0_cos1 * cos2)) >> FIXED_SHIFT;
    matrix[1] = ((sin1 * cos2) - (cos0_cos1 * sin2)) >> FIXED_SHIFT;

    cos0_sin1 = (cos0 * sin1) >> FIXED_SHIFT;
    matrix[6] = ((cos0_sin1 * cos2) - (cos1 * sin2)) >> FIXED_SHIFT;
    matrix[7] = ((-(cos1 * cos2)) - (cos0_sin1 * sin2)) >> FIXED_SHIFT;
}
