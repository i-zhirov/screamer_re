/* The rebuilt `sound_voice_stop` module.
 *
 * The original sits at linear 0x1f880 in STARTH.EXE.  The identity window holds
 * 15 instructions and 42 bytes up to the `ret` at 0x1f8a9.  A zero setup word
 * branches to the second exit at 0x1f8aa, and that exit returns 0.  The next
 * recorded function starts at 0x1f8b4, so the store record keeps the extent and
 * sets no separate function size.
 *
 * The direct callers sit in several sections.  `race_finish_laps_update` at
 * 0x20ec8 and `race_finish_time_update` at 0x20f98 release the sound handle
 * `DAT_00100e8`.  `race_finish_update` at 0x21058 releases `DAT_001100ec`.
 * `sound_emitter_update` at 0x21b9c stops the sound at record offset +0x38.
 * `player_car_skid_update` at 0x17d14 releases the effect at object offset
 * +0x148.  `object_collision` at 0x1e00c releases the hit sound.  The record
 * takes the section Program functions.
 *
 * The setup word `DAT_00008058` gates the call.  When the word is zero, the
 * function returns 0 and calls nothing.  Otherwise it passes the driver handle
 * `DAT_00018d94` and the voice index to the driver voice release at 0xca3c9
 * and returns -1.  The callers store the -1 result as the empty sound handle,
 * so the name `sound_voice_stop` fits the data flow.  The original has no
 * symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame and one local, so the module builds with
 * `-of+`.  The function holds no floating-point instruction, so the module
 * needs no FPU stub.  The module uses no assembly implementation.
 */
extern int DAT_00008058;
extern int DAT_00018d94;
extern int driver_voice_release(int handle, int voice);

int sound_voice_stop(int voice)
{
    if (DAT_00008058 != 0) {
        driver_voice_release(DAT_00018d94, voice);
        return -1;
    }
    return 0;
}
