/* The rebuilt `sound_variant_lookup` module.
 *
 * The original sits at linear 0x247ac in STARTH.EXE and holds 22 instructions
 * and 71 bytes up to the `ret` at 0x247f2.  The early return block at 0x247f3
 * through 0x247fb follows the main `ret`, and the next function starts at
 * 0x247fc, so the store record keeps the 80-byte extent and sets no separate
 * function size.  The identity check uses 22 instructions.
 *
 * The only direct caller is `race_intro_update` at 0x20d20 with the call site
 * at 0x20db7.  The caller passes the half-second bucket in `eax` and the mode 0
 * in `edx`.
 *
 * The state `DAT_0001d2c8` gates the lookup: a zero state returns -1.  The
 * sound table flag `DAT_00008058` gates the lookup too: a zero flag returns -1.
 * With both values set, the function builds a two-field record on the stack
 * with the effect at `+0x00` and the variant mode at `+0x04`, and passes the
 * record to `sound_variant_select` at 0x24a84.
 *
 * The function adds the sample base `DAT_0001906c` to the selected sample id
 * and resolves the sum through `table_lookup` at 0x1f718.  It returns the table
 * entry.  The base comes from `sound_effect_list_init` at 0x24600.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module needs no FPU stub
 * and no assembly.
 */
struct sound_entry {
    int effect;
    int mode;
};

extern int DAT_0001d2c8;
extern int DAT_00008058;
extern int DAT_0001906c;

extern int sound_variant_select(struct sound_entry *entry);
extern int table_lookup(int index);

int sound_variant_lookup(int effect, int mode)
{
    struct sound_entry entry;

    if (DAT_0001d2c8 == 0) {
        return -1;
    }
    if (DAT_00008058 == 0) {
        return -1;
    }
    entry.effect = effect;
    entry.mode = mode;
    return table_lookup(sound_variant_select(&entry) + DAT_0001906c);
}
