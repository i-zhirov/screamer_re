/* The rebuilt `camera_basis_patch` module.
 *
 * The original sits at linear 0x8a9e8 in STARTH.EXE and holds 25 instructions
 * and 102 bytes up to the `ret` at 0x8aa4d.  The next function
 * `sound_mix_prepare` starts at 0x8aa60, so the store record keeps the
 * 120-byte extent and sets no separate function size.  The identity check uses
 * 25 instructions.
 *
 * The only direct caller is `sound_mix_prepare` at 0x8aa60, with the call site
 * at 0x8aa6c.  The caller sets `esi` to the camera record at 0x7a7ac plus
 * 0x48, so `esi` points at the 3x3 camera basis.  On entry the helper copies
 * the basis and the saved camera position at 0x7ad94 through 0x7ad9c into the
 * immediate fields of the caller code at 0x8aaca through 0x8ab18.  The
 * original patches its own code, and the on-disk value 0xaaaabbbb is only the
 * linker fill value.
 *
 * The helper reads the basis in the column order 0, 3, 6, 1, 4, 7, 2, 5, 8
 * into the nine basis slots.  That order equals the order of the `m0` through
 * `m8` values of `sound_mix_prepare`.  Then it copies the three saved position
 * values into the three offset slots.
 *
 * The rebuild writes the same values into the globals `mix_prepare_basis` and
 * `mix_prepare_position`.  `sound_mix_prepare` now reads the camera record
 * directly, so the patch slots are not live code.  The copy is equivalent
 * because the matrix and the position do not change during the mix loop.
 *
 * The caller, the basis read order, and the position copy support the name
 * `camera_basis_patch`.  The original has no symbol, so the record keeps the
 * name with the confidence `inferred`.
 *
 * The original has no EBP frame, so the module builds without `-of+`.  The
 * function uses no floating-point operation, so the module needs no FPU stub.
 * The module uses no assembly implementation.
 */
extern int saved_position[3];

int mix_prepare_basis[9];
int mix_prepare_position[3];

void camera_basis_patch(int *basis)
{
    mix_prepare_basis[0] = basis[0];
    mix_prepare_basis[1] = basis[3];
    mix_prepare_basis[2] = basis[6];
    mix_prepare_basis[3] = basis[1];
    mix_prepare_basis[4] = basis[4];
    mix_prepare_basis[5] = basis[7];
    mix_prepare_basis[6] = basis[2];
    mix_prepare_basis[7] = basis[5];
    mix_prepare_basis[8] = basis[8];
    mix_prepare_position[0] = saved_position[0];
    mix_prepare_position[1] = saved_position[1];
    mix_prepare_position[2] = saved_position[2];
}
