/* The rebuilt `sound_fx_volume_set` module.
 *
 * The original sits at linear 0x1f8b4 in STARTH.EXE.  The body holds 14
 * instructions and 37 bytes up to the `ret` at 0x1f8d8.  The next recorded
 * function starts at 0x1f8dc, so the store record keeps the 40-byte extent and
 * sets no separate function size.  The identity check uses 14 instructions.
 *
 * The function saves `eax` in its one local and tests the setup word
 * `DAT_00008058`.  A zero word returns at once.  A set word passes the driver
 * handle `DAT_00018d94` in `eax` and the saved value in `edx` to the driver
 * word store at 0xc6c73 and returns.
 *
 * `view_move` at 0x1f94c calls this function with the second of its three
 * values.  `motion_update` and `credits_fade_out` pass `DAT_0011d2bc` in that
 * slot.  The options screen maps the label `FX` to `DAT_0011d2bc`, while it
 * maps `CD` to `DAT_0011d2c0` and `MU` to `DAT_0011d2c4`.  `view_move` passes
 * the `CD` slot to `cd_audio_volume_set` and the `MU` slot to
 * `music_volume_set`.  The third slot and the parallel volume setters support
 * the name `sound_fx_volume_set`.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame and one local, so the module builds with
 * `-of+`.  The function holds no floating-point instruction, so the module
 * needs no FPU stub.  The module uses no assembly implementation.
 */
extern int DAT_00008058;
extern int DAT_00018d94;
extern int driver_volume_store(int handle, int value);

void sound_fx_volume_set(int volume)
{
    int volatile saved_volume = volume;

    if (DAT_00008058 != 0) {
        driver_volume_store(DAT_00018d94, saved_volume);
    }
}
