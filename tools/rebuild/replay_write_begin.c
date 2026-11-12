/* The rebuilt `replay_write_begin` module.
 *
 * The original sits at linear 0x22d88 in STARTH.EXE.  The body holds 14
 * instructions and 45 bytes up to the `ret` at 0x22db4.  The next function
 * starts at 0x22db8, so the store record keeps the 48-byte extent and sets no
 * separate function size.  The identity check uses 14 instructions.
 *
 * The only direct caller is `game_mode_1_start` at 0x12c0c, at the call site
 * 0x12cf9.  The caller runs this function on the no-replay path, so the
 * function belongs to the mode 1 race start.
 *
 * The function runs only when the frame buffer gate DAT_00018e60 is not zero.
 * It clears the replay index DAT_00018e64 and selects the replay write mode by
 * the dword DAT_00018e6c = 0.  It then sets the playback bit 0x40 of the byte
 * DAT_00018e1c.
 *
 * The sibling `replay_playback_begin` at 0x22db8 selects the replay read mode
 * with the same gate.  The `timing_update` at 0x22df8 reads DAT_00018e6c: a
 * non-zero value reads a record through 0x22e94, and a zero value writes a
 * record through 0x22f54.  The caller, the write mode, and the cleared index
 * support the name `replay_write_begin`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and calls no helper, so it
 * needs no FPU stub and no assembly implementation.
 */
extern unsigned int DAT_00018e60;
extern unsigned char DAT_00018e1c;
extern unsigned int DAT_00018e64;
extern unsigned int DAT_00018e6c;

void replay_write_begin(void)
{
    unsigned char flags;

    if (DAT_00018e60 != 0) {
        flags = DAT_00018e1c;
        DAT_00018e64 = 0;
        DAT_00018e6c = 0;
        DAT_00018e1c = flags | 0x40;
    }
}
