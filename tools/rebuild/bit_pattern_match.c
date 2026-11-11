/* The rebuilt `bit_pattern_match` module.
 *
 * The original sits at linear 0x24bfc in STARTH.EXE and holds 32 instructions
 * and 78 bytes up to the `ret` at 0x24c49.  The next entry `sound_effect_delay`
 * starts at 0x24c4c, so the store record keeps the 80-byte extent and sets no
 * separate function size.  The identity check uses 32 instructions.
 *
 * The function has no direct caller, no indirect call, and no relocation slot
 * that points to it.  It sits between `sound_variant_next` at 0x24b48 and
 * `sound_effect_delay` at 0x24c4c, but the body reads only the global
 * DAT_000007fd, so the record takes the section Program functions.  The
 * original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The argument arrives in EAX.  The loop doubles the mask and sets the low bit
 * for each bit of the argument, so the mask becomes all ones up to the highest
 * set bit of the argument.  The function then returns 1 when the low bits of
 * DAT_000007fd equal the argument, and 0 otherwise.  A zero argument returns 1
 * with an empty mask.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no call, so the module needs
 * no FPU stub and no assembly implementation.
 */
extern int DAT_000007fd;

int bit_pattern_match(int value)
{
    int mask = 0;
    int remaining = value;

    while (remaining != 0) {
        mask = mask * 2 + 1;
        remaining >>= 1;
    }
    return (DAT_000007fd & mask) == value;
}
