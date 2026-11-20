/* The rebuilt `replay_frame_buffer_release` module.
 *
 * The original sits at linear 0x22d44 in STARTH.EXE and holds 11 instructions
 * and 30 bytes up to the `ret` at 0x22d61.  The next recorded function
 * `attack_data_release` starts at 0x22d64, so the store record keeps the
 * 32-byte extent and sets no separate function size.  The identity check uses
 * 11 instructions.
 *
 * The only direct caller is `race_data_release` at 0x156a8 with the call site
 * at 0x156fe.  It calls this function after the `FONTS` release at 0x25640 and
 * before the `ATTACK` release at 0x22d64.  The function frees the `REPLAY`
 * resource at object-3 offset 0xdba from the archive `DAT_000062ac` through
 * `resource_free_name` at 0x2f326.  It then clears the replay frame buffer
 * pointer `DAT_0018e5c`.
 *
 * `replay_frame_buffer_init` at 0x22c30 loads the same `REPLAY` resource and
 * stores the pointer at `DAT_0018e5c`, so the two functions form the replay
 * frame buffer load and release pair.  The module sits with the replay
 * records, so it takes the section The replay records.
 *
 * The `REPLAY` resource, the cleared buffer pointer, and the replay frame
 * buffer loader support the name `replay_frame_buffer_release`.  The original
 * has no symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame.  The C form with `-of+` adds `mov esp,ebp`,
 * reorders the prologue, and keeps the zero in EAX, so the recipe uses the exact
 * `replay_frame_buffer_release.asm` body.  This C source stays as the readable
 * behavioral reference.  The function holds no floating-point instruction, so
 * the module needs no FPU stub.
 */
extern int DAT_000062ac;
extern int DAT_0018e5c;

extern void resource_free_name(int context, char *name);

void replay_frame_buffer_release(void)
{
    resource_free_name(DAT_000062ac, "REPLAY");
    DAT_0018e5c = 0;
}
