/* The rebuilt `replay_playback_begin` module.
 *
 * The original sits at linear 0x22db8 in STARTH.EXE.  The body holds 19
 * instructions and 64 bytes up to the `ret` at 0x22df4.  The next function
 * starts at 0x22df8, so the store record keeps the 64-byte extent and sets no
 * separate function size.  The identity check uses 19 instructions.
 *
 * The only direct caller is `game_mode_1_start` at 0x12c0c, at the call site
 * 0x12ce1.  The caller loads the replay member with the flag 1 through
 * `replay_load` at 0x1b840 and then runs this function, so the function belongs
 * to the mode 1 replay path.
 *
 * The function runs only when the frame buffer gate DAT_00018e60 is not zero.
 * It selects the replay read mode by the dword DAT_00018e6c = 1, clears the
 * replay index DAT_00018e64, sets the two shared view cells DAT_00001df0 and
 * DAT_0010a9c to 1, and sets the playback bit 0x40 of the byte DAT_00018e1c.
 *
 * The sibling at 0x22d88 selects the replay write mode with the same gate.  The
 * `timing_update` at 0x22df8 reads DAT_00018e6c: a non-zero value reads a record
 * through 0x22e94, and a zero value writes a record through 0x22f54.  The caller,
 * the read mode, and the cleared index support the name `replay_playback_begin`.
 * The original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The original has the EBP frame.  The C form with `-of+` saves the frame
 * first and reorders the stores, so the recipe uses the exact
 * `replay_playback_begin.asm` body.  This C source stays as the readable
 * behavioral reference.  The function holds no floating-point instruction and
 * calls no helper, so it needs no FPU stub.
 */
extern unsigned int DAT_00018e60;
extern unsigned char DAT_00018e1c;
extern unsigned int DAT_00018e6c;
extern unsigned int DAT_00018e64;
extern unsigned int DAT_00001df0;
extern unsigned int DAT_0010a9c;

void replay_playback_begin(void)
{
    unsigned char flags;

    if (DAT_00018e60 != 0) {
        flags = DAT_00018e1c;
        DAT_00018e6c = 1;
        DAT_00018e64 = 0;
        DAT_00001df0 = 1;
        DAT_00018e1c = flags | 0x40;
        DAT_0010a9c = 1;
    }
}
