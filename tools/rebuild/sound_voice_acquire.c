/* The rebuilt `sound_voice_acquire` module.
 *
 * The original sits at linear 0x1f7ac in STARTH.EXE.  The first block holds 17
 * instructions and 48 bytes up to the `ret` at 0x1f7db.  A branch at 0x1f7ce
 * enters the second exit block at 0x1f7dc, and that block ends with the `ret`
 * at 0x1f7e2.  The next function starts at 0x1f7e4, so the store record keeps
 * the 56-byte extent and sets no separate function size.  The identity check
 * uses 17 instructions.
 *
 * The only direct caller at 0x16f1b is `player_car_cornering_update` at
 * 0x16ca0.  The caller passes the constant 4 in eax and the object cell at
 * +0x7d4 in edx, then stores the result back in the cell.  The caller sits in
 * The vehicle state chain, so the record takes that section.
 *
 * The function returns the cached voice when the cell is not -1 and the driver
 * word at offset 0x34 of that voice equals the sample.  Otherwise it returns
 * the free voice from `table_lookup` at 0x1f718 for the sample.  The driver
 * word comes from 0xc6bde with the driver handle `DAT_00018d94`.  The sibling
 * `sound_voice_sample_set` at 0x1f764 and `sound_voice_finished` at 0x1f834
 * read the same driver fields, so the name `sound_voice_acquire` fits the data
 * flow.  The original has no symbol, so the record keeps the name with the
 * confidence inferred.
 *
 * The original has the EBP frame and two locals, so the module builds with
 * `-of+`.  The function holds no floating-point instruction, so the module
 * needs no FPU stub and no assembly implementation.
 */
extern int DAT_00018d94;
extern int driver_voice_sample(int handle, int voice);
extern int table_lookup(int sample);

int sound_voice_acquire(int sample, int voice)
{
    if (voice != -1 && driver_voice_sample(DAT_00018d94, voice) == sample) {
        return voice;
    }
    return table_lookup(sample);
}
