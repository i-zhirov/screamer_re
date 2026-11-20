/* The rebuilt `music_voice_stop` module.
 *
 * The original sits at linear 0x2fef4 in STARTH.EXE and holds 13 instructions
 * and 32 bytes up to the `ret` at 0x2ff13.  The next function starts at
 * 0x2ff14, so the store record keeps the 32-byte extent and sets no separate
 * function size.  The identity check uses 13 instructions.
 *
 * The only direct caller at 0x214a7 is `options_screen` at 0x21338.  It runs
 * the call when the music volume at DAT_001d2c4 reaches zero and the once flag
 * at `[ebp + 0x56]` is clear.  It passes the music bank record 0x62b0 and then
 * sets the flag, so the function stops the music voice one time.  The two other
 * fades in the same block set the CD audio volume through 0x264b8 and the FX
 * volume through 0x265b4.  The sibling modules `music_select` at 0x2febc and
 * `music_bank_stop` at 0x2fe68 sit in the section The sound loaders, so the
 * record takes that section.
 *
 * The function returns at once when the success flag at `+0x04` is negative.
 * Otherwise it frees the driver voice of the channel index at `+0x00` through
 * `sound_channel_free` at 0xd38e6.  The same gate and the same free call start
 * the body of `music_bank_stop` at 0x2fe68 and of `music_select` at 0x2febc.
 * The zero-volume music stop and the sibling callers support the name
 * `music_voice_stop`.  The original has no symbol, so the record keeps the name
 * with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module needs no FPU stub,
 * and the module uses no assembly implementation.
 */
struct music_bank {
    int channel;
    int success;
};

extern void sound_channel_free(int index);

void music_voice_stop(struct music_bank *bank)
{
    if (bank->success < 0) {
        return;
    }
    sound_channel_free(bank->channel);
}
