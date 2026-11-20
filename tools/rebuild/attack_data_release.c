/* The rebuilt `attack_data_release` module.
 *
 * The original sits at linear 0x22d64 in STARTH.EXE and holds 12 instructions
 * and 36 bytes up to the `ret` at 0x22d87.  The next recorded function
 * `replay_write_begin` starts at 0x22d88, so the store record keeps the
 * 36-byte extent and sets no separate function size.  The identity check uses
 * 12 instructions.
 *
 * The only direct caller is `race_data_release` at 0x156a8.  It calls this
 * function after the `REPLAY` release at 0x22d44 and before the `GAMESPLINE`
 * release at 0x1b180.  The function frees the `ATTACK` resource at object-3
 * offset 0xdc1 from the archive `DAT_000062ac` through `resource_free_name` at
 * 0x2f326.  It then clears the two attack replay cells `DAT_0018ed0` and
 * `DAT_0018eac`.
 *
 * `replay_buffer_init` at 0x22c88 loads the same `ATTACK` resource and sets
 * the two cells, so the two functions form the attack replay buffer load and
 * release pair.  The module sits with the replay records, so it takes the
 * section The replay records.
 *
 * The `ATTACK` resource, the two cells, and the replay buffer loader support
 * the name `attack_data_release`.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly body.
 */
extern int DAT_000062ac;
extern int DAT_0018ed0;
extern int DAT_0018eac;

extern void resource_free_name(int context, char *name);

void attack_data_release(void)
{
    resource_free_name(DAT_000062ac, "ATTACK");
    DAT_0018ed0 = 0;
    DAT_0018eac = 0;
}
