/* The rebuilt `race_data_release` module.
 *
 * The original sits at linear 0x156a8 in STARTH.EXE and holds 29 instructions
 * and 117 bytes up to the `ret` at 0x1571c.  The next function starts at
 * 0x15720, so the store record keeps the 120-byte extent and sets no separate
 * function size.  The identity check uses 29 instructions.
 *
 * The only direct caller is `race_session_run` at 0x1fd70 with the call site at
 * 0x1ff02.  `race_session_run` calls the function after the fade-out at
 * 0x21da8, so the function releases the race data at the end of a session.  The
 * caller sits in the section `race_frame callees`, so the record takes that
 * section.
 *
 * The function stops the music bank record at `DAT_000062b0` and frees the
 * `COCKPIT` resource from the archive `DAT_000062ac`.  It then releases the
 * object list at `0x6350` for the table at `0x6530`, unloads the object table
 * at `0x6530`, and frees the `MATERIALS`, `OBJ_LST`, and `UNR_FILE` resources.
 *
 * The function then frees the `SOUND_FX`, `TEXT`, `SPRITES`, `FONTS`, `REPLAY`,
 * `ATTACK`, and `GAMESPLINE` resources.  The replay and attack helpers also
 * clear the ghost cells `DAT_0018e5c`, `DAT_0018ed0`, and `DAT_0018eac`.
 *
 * The tail clears the CD flag `DAT_000166ac` and stops the CD audio.  The
 * function resets the mouse pointer at the start.
 *
 * The caller, the resource frees, and the symmetric `race_data_load` at
 * 0x131a8 support the name `race_data_release`; the original has no symbol, so
 * the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub, and
 * the module uses no assembly body.
 */
extern int DAT_000062ac;
extern char DAT_000062b0[];
extern char DAT_00006350[];
extern char DAT_00006530[];
extern int DAT_000166ac;

extern void mouse_helper(int value);
extern void music_bank_stop(void *record);
extern void resource_free_name(int context, char *name);
extern void object_list_release_all(void *table, void *list);
extern void object_unload(void *table);
extern void library_free(void);
extern void free_sound_fx(void);
extern void free_text(void);
extern void free_sprites(void);
extern void free_fonts(void);
extern void free_replay(void);
extern void free_attack(void);
extern void free_gamespline(void);
extern void cd_audio_stop(void);

void race_data_release(void)
{
    mouse_helper(0);
    music_bank_stop(DAT_000062b0);
    resource_free_name(DAT_000062ac, "COCKPIT");
    object_list_release_all(DAT_00006530, DAT_00006350);
    object_unload(DAT_00006530);
    library_free();
    free_sound_fx();
    free_text();
    free_sprites();
    free_fonts();
    free_replay();
    free_attack();
    free_gamespline();
    DAT_000166ac = 0;
    cd_audio_stop();
}
