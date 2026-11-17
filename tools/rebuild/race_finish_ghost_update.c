/* The rebuilt `race_finish_ghost_update` module.
 *
 * The original sits at linear 0x23f50 in STARTH.EXE and holds 32 instructions
 * and 117 bytes up to and including the `ret` at 0x23fc4.  The next function starts at
 * 0x23fc8, so the store record keeps the 120-byte extent and sets no separate
 * function size.  The identity check uses 32 instructions.
 *
 * The only direct caller is `race_finish_laps_update` at 0x20ec8 with the call
 * site at 0x20f37.  That function selects one finish handler from the mode byte
 * DAT_0011d260: this handler runs on mode 4.  The handler sits in the
 * `Program functions` section.
 *
 * The mode 4 handler repeats the replay compare of `race_position_update` at
 * 0x20428.  When the replay flag DAT_001d264 is set, it selects the other ghost
 * record at DAT_0018e8c plus the index DAT_0018ed4 ^ 1 times 0x24.  It
 * compares the clock DAT_00100a8 with the ghost time at +0x1c and writes the
 * position 1 or 2 to DAT_000ffa4.
 *
 * The function then tests the position.  Position 1 sets bit 3 of the control
 * byte DAT_0018e1d and enqueues the effect 0xb with the variant mode 3 and the
 * delay 0.  Any other position enqueues the effect 0xc with the mode 3 and the
 * delay 0.  The enqueue goes through `sound_enqueue` at 0x247fc.
 *
 * The caller, the repeated ghost compare, and the win or lose effects support
 * the name race_finish_ghost_update.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses the floating-point compare, so the recipe adds the FPU run-time stubs.
 * The module uses no assembly body.
 */
extern int DAT_001d264;
extern int DAT_0018ed4;
extern float DAT_00100a8;
extern int DAT_000ffa4;
extern unsigned char DAT_0018e1d;
extern char DAT_0018e8c[];
extern void sound_enqueue(int effect, int mode, float delay);

void race_finish_ghost_update(void)
{
    int index;

    if (DAT_001d264 != 0) {
        index = (DAT_0018ed4 ^ 1) * 0x24;
        DAT_000ffa4 = (
            DAT_00100a8 >= *(float *)(DAT_0018e8c + index + 0x1c)
        ) ? 2 : 1;
    }
    if (DAT_000ffa4 == 1) {
        DAT_0018e1d |= 8;
        sound_enqueue(0xb, 3, 0.0f);
    } else {
        sound_enqueue(0xc, 3, 0.0f);
    }
}
