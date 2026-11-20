/* The rebuilt `music_select` module.
 *
 * The original sits at linear 0x2febc in STARTH.EXE and holds 20 instructions
 * and 54 bytes up to the `ret` at 0x2fef1.  The next function starts at
 * 0x2fef4 after the two padding bytes at 0x2fef2 and 0x2fef3, so the store
 * record keeps the 56-byte extent and sets no separate function size.  The
 * identity check uses 20 instructions.
 *
 * Three rebuilt callers declare the target as `music_select`.  `music_play` at
 * 0x2ff14 calls it with the music record and the value 1 when the word at
 * `+0x0c` is 1.  `menu_space_init` at 0x29d08 calls it at 0x29f6f with the
 * record 0x62b0 and the value 1, after `music_load`.  `race_session_start` at
 * 0x128e0 calls it with the record 0x62b0 and the value 1.  The record takes
 * the name `music_select` with the confidence inferred, because the original
 * has no symbol.
 *
 * The function returns at once when the success flag at `+0x04` is negative.
 * Otherwise it frees the driver voice of the channel index at `+0x00` through
 * `sound_channel_free` at 0xd38e6, starts the voice again through 0xd3857, and
 * writes the argument to `+0x0c`.  The same record gate and the same free call
 * sit in `music_bank_stop` at 0x2fe68, so the record takes the section The
 * sound loaders.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module needs no FPU
 * stub, and the module uses no assembly.
 */
extern void sound_channel_free(int index);
extern void sound_channel_start(int index);

void music_select(char *record, int value)
{
    if (*(int *)(record + 4) < 0) {
        return;
    }
    sound_channel_free(*(int *)record);
    sound_channel_start(*(int *)record);
    *(int *)(record + 0xc) = value;
}
