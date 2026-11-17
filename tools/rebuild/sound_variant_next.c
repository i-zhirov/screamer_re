/* The rebuilt `sound_variant_next` module.
 *
 * The original sits at linear 0x24b48 in STARTH.EXE and holds 47 instructions
 * and 152 bytes up to the `ret` at 0x24bdf.  The zero-quality return path is
 * out of line at 0x24be0 through the `ret` at 0x24bfa.  The next function
 * starts at 0x24bfc, so the store record keeps the 180-byte extent and sets no
 * separate function size.  The identity check uses 47 instructions.
 *
 * The only direct caller is `sound_variant_select` at 0x24a84, which calls this
 * function for the variant mode 3 at 0x24af5.  The function returns the next
 * sample id of a sound effect and skips the -1 entries.  The effect index
 * arrives in EAX.
 *
 * The table at object 3 offset 0x1fb4 holds 0x1b effects and three sample ids
 * per effect, each entry 0xc bytes.  The value -1 marks a missing sample id.
 * The rotation counter of each effect lives in the counter array at 0x191c0.
 *
 * When `0x19064` is zero, the function returns the first sample id of the
 * effect and leaves the counter unchanged.  When `0x19064` is not zero, the
 * function advances the counter with the wrap 2 to 0, skips the -1 entries, and
 * returns at most three times the same value when every entry is -1.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no call, so the module
 * needs no FPU stub and no assembly.
 */
extern int sound_variant_table[27][3];
extern int sound_quality_enabled;
extern int sound_rotation[27];

int sound_variant_next(int effect)
{
    int slot;
    int attempt;

    if (sound_quality_enabled == 0) {
        return sound_variant_table[effect][0];
    }
    for (attempt = 0; attempt < 3; ++attempt) {
        slot = sound_rotation[effect] + 1;
        if (slot == 3) {
            slot = 0;
        }
        sound_rotation[effect] = slot;
        if (sound_variant_table[effect][slot] != -1) {
            return sound_variant_table[effect][slot];
        }
    }
    return sound_variant_table[effect][sound_rotation[effect]];
}
