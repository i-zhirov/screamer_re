/* The rebuilt `race_finish_time_ghost_update` module.
 *
 * The original sits at linear 0x24080 in STARTH.EXE.  The body holds 5
 * instructions and 15 bytes up to the `ret` at 0x2408e.  The next function
 * starts at 0x24090, so the store record keeps the 16-byte extent and sets no
 * separate function size.  The identity check uses 5 instructions.
 *
 * The only direct caller is `race_finish_time_update` at 0x20f98 with the call
 * site at 0x20ff4.  That function selects one finish handler from the mode byte
 * DAT_0011d260 after the time limit expires, and this handler runs on mode 4.
 * The mode 4 handler in the final-lap path is `race_finish_ghost_update` at
 * 0x23f50, so mode 4 is the ghost mode.  No named function sits between the
 * caller and the target, so the record takes the section Program functions.
 *
 * The handler stores the finish position 2 in DAT_000ffa4.  The time limit
 * expires in the ghost mode, so the player loses the race and takes the second
 * position.  The other five mode handlers at 0x24068, 0x24070, 0x24078,
 * 0x24090, and 0x24098 are empty stubs.
 *
 * The caller, the mode 4 selection, and the finish position support the name
 * `race_finish_time_ghost_update`.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub.  The
 * module uses no assembly.
 */
extern int DAT_000ffa4;

void race_finish_time_ghost_update(void)
{
    DAT_000ffa4 = 2;
}
