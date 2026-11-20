/* The rebuilt `opponent_score_update` module.
 *
 * The original sits at linear 0x18b60 in STARTH.EXE and holds 22 instructions
 * and 70 bytes up to the `ret` at 0x18ba5.  Two padding bytes at 0x18ba6
 * precede the next function at 0x18ba8, so the store record keeps the 72-byte
 * extent and sets no separate function size.  The identity check uses 22
 * instructions.
 *
 * The only direct caller is the CNG view group at 0x20360, at the call site
 * 0x2037a.  The group runs on the player object at object-3 offset 0xf91c, so
 * the module takes the section The vehicle state chain.
 *
 * The player object holds the score float at `+0x790`, the started opponent
 * counter at `+0x7b8`, and the stored counter at `+0x7bc`.  The function reads
 * the counter, and when it rises above the stored counter, it adds `1.0` to
 * the score and copies the counter to the stored counter.  A counter at or
 * below the stored counter changes nothing.
 *
 * The counter, the stored counter, and the score support the name
 * `opponent_score_update`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses the x87 stack for the add, so the module links the shared
 * `fpu_stub.asm` for the run-time symbols `__8087`,
 * `__init_387_emulator`, and `_fltused_`.  The module uses no assembly
 * implementation.
 */
void opponent_score_update(void *self)
{
    int started;

    started = *(int *)((char *)self + 0x7b8);
    if (started > *(int *)((char *)self + 0x7bc)) {
        *(float *)((char *)self + 0x790) =
            *(float *)((char *)self + 0x790) + 1.0f;
        *(int *)((char *)self + 0x7bc) = started;
    }
}
