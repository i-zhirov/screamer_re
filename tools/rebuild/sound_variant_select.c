/* The rebuilt `sound_variant_select` module.
 *
 * The original sits at linear 0x24a84 in STARTH.EXE and holds 58 instructions
 * and 194 bytes up to the `ret` at 0x24b45.  Two pad bytes follow the `ret`,
 * and the next function starts at 0x24b48, so the store record keeps the
 * 196-byte extent and sets no separate function size.  The identity check uses
 * 58 instructions.
 *
 * The argument arrives in EAX and points at a sound-queue record.  The field
 * +0x00 holds the effect index and the field +0x04 holds the variant mode.  The
 * mode selects one of the three sample ids of the effect in the table at object
 * 3 offset 0x1fb4:
 *
 *     0  the first sample id
 *     1  the second sample id
 *     2  the third sample id
 *     3  the rotating sample id through 0x24b48
 *
 * The table holds 0x1b effects and three sample ids per effect.  The value -1
 * marks a missing sample id.
 *
 * When `0x19064` is zero, the function clamps the result to the sample count at
 * `0x19068`: a value at or above the count falls back to the first sample id,
 * and a still-too-high value becomes -1.  When `0x19064` is not zero, the
 * function skips the clamp, so the high-quality mode keeps every sample id.
 *
 * The sound-effect timers at 0x24c4c, 0x24cc8, and 0x24d30 enqueue an entry
 * with the mode 3, and the intro at 0x20d20 plays an effect with the mode 0.
 * The queue resolver at 0x24a28 passes the head record at 0x19080, and the
 * player at 0x247ac passes a two-field stack record.
 *
 * The original dispatches the mode through the jump table at `0x24a74`.  The
 * rebuild uses an if-else chain, because the V2 jump table at the start of the
 * code object faults in the harness.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub, and
 * the module uses no assembly.
 */
struct sound_entry {
    int effect;
    int mode;
};

extern int sound_variant_table[27][3];
extern int sound_quality_enabled;
extern int sound_sample_count;
extern int sound_variant_next(int effect);

int sound_variant_select(struct sound_entry *entry)
{
    int result;

    if (entry->mode == 0) {
        result = sound_variant_table[entry->effect][0];
    } else if (entry->mode == 1) {
        result = sound_variant_table[entry->effect][1];
    } else if (entry->mode == 2) {
        result = sound_variant_table[entry->effect][2];
    } else if (entry->mode == 3) {
        result = sound_variant_next(entry->effect);
    }
    if (sound_quality_enabled == 0) {
        if (result >= sound_sample_count)
            result = sound_variant_table[entry->effect][0];
        if (result >= sound_sample_count)
            result = -1;
    }
    return result;
}
