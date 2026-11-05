/* The rebuilt `object_transform` module.
 *
 * The original sits at linear 0x1cd44 in STARTH.EXE and holds 61 instructions
 * and 244 bytes up to the first `ret`: the register save, the frame, the
 * 4-byte local slot, the `object_integrate` call at 0x1d6bc, the
 * `object_state_save` call at 0x1ccd8, the two integer position sums, the
 * conditional height copy, the three float sums, the height minus two store,
 * and the vertical sum.
 *
 * The function integrates the object and saves the previous state.  It then
 * writes the world position at `+0x498` and `+0x4a0` from the base position and
 * the integrated offset, copies the height at `+0x238` to `+0x49c` while the
 * flag at `+0x2f8` is zero, writes the three float sums at `+0x4a4`, `+0x4a8`,
 * and `+0x4ac`, stores the height minus two at `+0x2c4`, and stores the
 * vertical sum at `+0x4bc` from the new `+0x4a8` value and `+0x4c8`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern void object_integrate(char *object);
extern void object_state_save(char *object);

void object_transform(char *object)
{
    object_integrate(object);
    object_state_save(object);
    *(unsigned int *)(object + 0x498) =
        *(unsigned int *)(object + 0x258) + *(unsigned int *)(object + 0x454);
    *(unsigned int *)(object + 0x4a0) =
        *(unsigned int *)(object + 0x260) + *(unsigned int *)(object + 0x45c);
    if (*(unsigned int *)(object + 0x2f8) == 0) {
        *(unsigned int *)(object + 0x49c) = *(unsigned int *)(object + 0x238);
    }
    *(float *)(object + 0x4a4) = *(float *)(object + 0x23c) + *(float *)(object + 0x314);
    *(float *)(object + 0x4a8) = *(float *)(object + 0x27c) + *(float *)(object + 0x318);
    *(float *)(object + 0x4ac) = *(float *)(object + 0x244) + *(float *)(object + 0x31c);
    *(unsigned int *)(object + 0x2c4) = *(unsigned int *)(object + 0x238) - 2;
    *(float *)(object + 0x4bc) = *(float *)(object + 0x4a8) + *(float *)(object + 0x4c8);
}
