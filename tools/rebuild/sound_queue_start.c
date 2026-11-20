/* The rebuilt `sound_queue_start` module.
 *
 * The original sits at linear 0x24a28 in STARTH.EXE and holds 23 instructions
 * and 67 bytes up to the `ret` at 0x24a6a.  The -1 return path is out of line
 * at 0x24a6b through the `ret` at 0x24a71.  Two pad bytes follow, and the jump
 * table of sound_variant_select starts at 0x24a74.  The next function starts at
 * 0x24a84, so the store record keeps the 92-byte extent and sets no separate
 * function size.  The identity check uses 23 instructions.
 *
 * The only direct caller is sound_queue_update at 0x24880, with the call site
 * at 0x248d7.  The caller passes the head queue record at DAT_0019080.  The
 * callee sound_variant_select at 0x24a84 selects the sample id.  The caller
 * stores the result as the playing sound status, so the record takes the
 * section Program functions.
 *
 * The argument in EAX points at a sound-queue record.  The field +0x00 holds
 * the effect index and the field +0x04 holds the variant mode.  When the
 * selected sample id is -1, the function returns -1 and touches no global.
 *
 * Otherwise the function stores the frame counter DAT_000007fd at
 * DAT_0019078.  It adds the sample base DAT_001906c to the selected sample id,
 * stores the table id at DAT_0019074, and resolves the id through table_lookup
 * at 0x1f718.  It returns the table entry.  The base comes from
 * sound_effect_list_init at 0x24600.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module needs no FPU
 * stub, and the module uses no assembly.
 */
struct sound_entry {
    int effect;
    int mode;
};

extern int DAT_000007fd;
extern int DAT_0001906c;
extern int DAT_00019074;
extern int DAT_00019078;

extern int sound_variant_select(struct sound_entry *entry);
extern int table_lookup(int index);

int sound_queue_start(struct sound_entry *entry)
{
    int sample;
    int result;

    sample = sound_variant_select(entry);
    if (sample == -1) {
        return -1;
    }
    DAT_00019078 = DAT_000007fd;
    DAT_00019074 = sample + DAT_0001906c;
    result = table_lookup(DAT_00019074);
    return result;
}
