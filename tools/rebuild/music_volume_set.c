/* The rebuilt `music_volume_set` module.
 *
 * The original sits at linear 0x2ff84 in STARTH.EXE.  The identity window holds
 * 15 instructions and 40 bytes up to the `ret` at 0x2ffab.  The next recorded
 * function starts at 0x2ffc4, so the store record keeps the 64-byte extent and
 * sets no separate function size.  The identity check uses 15 instructions.
 *
 * The only direct caller is `view_move` at 0x1f94c.  It calls this function at
 * 0x1f96e with the music bank record 0x62b0 in `eax` and the shifted third
 * argument in `edx`.  The caller sits in the section motion_update callees, so
 * the record takes the same section.  The caller also passes the first argument
 * to `cd_audio_volume_set` at 0x265b4, so this function applies the music side
 * of the same volume change.
 *
 * The record gate sits at `+0x14`.  `music_bank_load` at 0x2f9e0 sets this flag
 * to 1 on success, and `music_load` at 0x2fcec uses the same gate.  A clear
 * gate returns without a call.  A set gate passes the low seven bits of the
 * value to the MIDI master volume helper at 0xd3d14.  That helper stores the
 * master volume and re-sends controller 7 on the active MIDI channels, so the
 * record gate, the record, and the callee support the name `music_volume_set`.
 * The original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The original has the EBP frame and two locals, so the module builds with
 * `-of+`.  The function holds no floating-point instruction, so the module
 * needs no FPU stub.  The module uses no assembly implementation.
 */
struct music_bank {
    unsigned char pad[0x14];
    int active;
};

extern void midi_master_volume_set(unsigned int volume);

void music_volume_set(struct music_bank *bank, unsigned int volume)
{
    if (bank->active != 0) {
        midi_master_volume_set(volume & 0x7f);
    }
}
