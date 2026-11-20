/* The rebuilt `sound_bank_release` module.
 *
 * The original sits at linear 0x1f6f8 in STARTH.EXE.  The body holds 11
 * instructions and 31 bytes up to the `ret` at 0x1f716.  The next function
 * `sound_voice_sample_set` starts at 0x1f764, so the store record keeps the
 * 32-byte extent and sets no separate function size.  The identity check uses
 * 11 instructions.
 *
 * The function tests the setup word `DAT_00008058`.  A zero word returns at
 * once.  A set word frees the `SOUND_FX` resource from the archive
 * `DAT_000062ac` through `resource_free_name` at 0x2f326.
 *
 * `sound_bank_load` at 0x1f2e0 gets the `SOUND_FX` resource from the same
 * archive and returns at once when the setup word is zero.  The two functions
 * form the sound bank load and release pair, so the name `sound_bank_release`
 * fits the pair.  The direct callers are `race_data_release` at 0x156a8 with
 * the call site at 0x156ea, `race_end` at 0x279a4 with the call site at
 * 0x27a03, and `race_cleanup` at 0x2a1b0 with the call site at 0x2a1ff.  All
 * three callers run at race teardown.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module needs no FPU
 * stub.  The module uses no assembly implementation.
 */
extern int DAT_00008058;
extern int DAT_000062ac;

extern void resource_free_name(int context, char *name);

void sound_bank_release(void)
{
    if (DAT_00008058 != 0) {
        resource_free_name(DAT_000062ac, "SOUND_FX");
    }
}
