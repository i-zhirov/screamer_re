/* The rebuilt `sound_voice_sample_set` module.
 *
 * The original sits at linear 0x1f764 in STARTH.EXE and holds 26 instructions
 * and 71 bytes up to the `ret` at 0x1f7aa.  One padding byte at 0x1f7ab
 * precedes the next function at 0x1f7ac, so the store record keeps the 72-byte
 * extent and sets no separate function size.  The identity check uses 26
 * instructions.
 *
 * The only direct caller is `player_car_surface_effect_update` at 0x1814c with
 * the call site at 0x1820f.  The caller passes the voice index in `eax` and the
 * sample index in `edx`.
 *
 * The setup word `DAT_00008058` gates the store: a zero word returns at once.
 * With a set word the function passes the driver handle `DAT_00018d94`, the
 * voice index, and the address of the sample record in the table at 0x16034 to
 * the driver record loader at 0xca477.  Each record holds 116 bytes, so the
 * index scales by 116.
 *
 * The sibling function `table_lookup` at 0x1f718 reads the same setup word and
 * the same table, and `sound_voice_pitch_set` at 0x1f7e4 writes another field
 * of the same voice.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module needs no FPU stub
 * and no assembly.
 */
extern unsigned int DAT_00008058;
extern int DAT_00018d94;

extern int voice_sample_write(unsigned int driver, int voice, unsigned int record);

void sound_voice_sample_set(int voice, int sample)
{
    if (DAT_00008058 != 0) {
        voice_sample_write(DAT_00018d94, voice, 0x16034 + sample * 116);
    }
}
