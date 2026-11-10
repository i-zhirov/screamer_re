/* The rebuilt `basis_rotation_build` module.
 *
 * The original sits at linear 0x81fd0 in STARTH.EXE and holds 57 instructions
 * and 162 bytes up to the `ret` at 0x82071.  The next recorded function starts
 * at 0x82074, so the store record keeps the 164-byte extent and sets no
 * separate function size.  The identity check uses 57 instructions.
 *
 * The only direct caller at 0x8b664 is `basis_transform_build` at 0x8b610.
 * It passes the destination pointer in `edx`, the three cosine values in
 * `ecx`, `ebx`, and `ebp`, and the three sine values in `esi`, `edi`, and
 * `eax`.  The caller runs this function on the real angles, so the result is
 * the forward basis at object `+0x18`.  The second call at 0x8b6c7 runs the
 * sibling at 0x82074 on the complement angles and builds the inverse basis at
 * object `+0x48`.  The caller sits in the `object_update_all` group, so the
 * record takes the section `object_update_all callees`.
 *
 * The function builds the nine Q14 dwords of the 3x3 matrix at `+0x00` through
 * `+0x20`.  It shifts the two inner products `c0 * c1` and `c0 * s1` before the
 * outer product, so the two stages match the original.  Some elements negate
 * the sum before the shift, and the arithmetic shift rounds toward negative
 * infinity.  The element at `+0x1c` holds `-c0` without a shift.
 *
 * The sibling builder at 0x82074 is the `basis_rotation_mirror` module.  It
 * builds the complement basis from the same register layout.  This builder
 * keeps the name `basis_rotation_build` with the confidence inferred.  The
 * original has no symbol.
 *
 * The original has no EBP frame.  It spills `c2` and `s2` with `push`, uses
 * `ebp` as the destination pointer, and does not restore `ebp`, so the module
 * builds without `-of+`.  The function uses no floating-point instruction and
 * calls no function, so it needs no FPU stub.  The rebuilt C uses the standard
 * Watcom calling convention, so the identity check differs in the prologue and
 * in the register use.  The behavior check covers the arithmetic.
 */
#define FIXED_SHIFT 14

void basis_rotation_build(int *matrix, int cos0, int sin0,
    int cos1, int sin1, int cos2, int sin2)
{
    int cos0_cos1;
    int cos0_sin1;

    cos0_cos1 = (cos0 * cos1) >> FIXED_SHIFT;
    matrix[0] = ((sin1 * sin2) - (cos0_cos1 * cos2)) >> FIXED_SHIFT;
    matrix[1] = (cos2 * sin0) >> FIXED_SHIFT;
    matrix[4] = (sin0 * sin2) >> FIXED_SHIFT;
    matrix[8] = (sin0 * sin1) >> FIXED_SHIFT;
    matrix[6] = (-(sin0 * cos1)) >> FIXED_SHIFT;

    cos0_sin1 = (cos0 * sin1) >> FIXED_SHIFT;
    matrix[2] = ((cos0_sin1 * cos2) + (cos1 * sin2)) >> FIXED_SHIFT;
    matrix[5] = ((cos0_sin1 * sin2) - (cos1 * cos2)) >> FIXED_SHIFT;

    matrix[3] = (-((sin2 * cos0_cos1) + (sin1 * cos2))) >> FIXED_SHIFT;
    matrix[7] = -cos0;
}
