/* The rebuilt `race_finish_mode_2_update` module.
 *
 * The original sits at linear 0x23f14 in STARTH.EXE and holds 18 instructions
 * and 60 bytes up to the `ret` at 0x23f4f.  The next function starts at
 * 0x23f50, so the store record keeps the 60-byte extent and sets no separate
 * function size.  The identity check uses 18 instructions.
 *
 * The only direct caller is `race_finish_laps_update` at 0x20ec8 with the call
 * site at 0x20f29.  That function selects one finish handler from the mode
 * byte DAT_0011d260: this handler runs on mode 2.  The handler sits in the
 * `Program functions` section.  The mode 1 handler at 0x23ed8 holds the
 * identical body, so the record names this one by the mode 2 call site.
 *
 * The handler compares the finish position DAT_000ffa4 with 1.  Position 1
 * sets bit 3 of the control byte DAT_0018e1d and enqueues the win effect 0xb.
 * Any other position keeps the control byte and enqueues the lose effect 0xc.
 * Both calls use the variant mode 3 and the delay 0, and the enqueue goes
 * through `sound_enqueue` at 0x247fc.
 *
 * The caller, the mode 2 selection, the position test, and the two effects
 * support the name `race_finish_mode_2_update`.  The original has no symbol,
 * so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original function holds no floating-point instruction, but the
 * `sound_enqueue` call passes a float argument, so the recipe adds the shared
 * `fpu_stub.asm` for the FPU run-time symbols.  The module uses no assembly
 * implementation.
 */
extern unsigned char DAT_0018e1d;
extern int DAT_000ffa4;

extern void sound_enqueue(int effect, int mode, float delay);

void race_finish_mode_2_update(void)
{
    if (DAT_000ffa4 == 1) {
        DAT_0018e1d = DAT_0018e1d | 8;
        sound_enqueue(0xb, 3, 0.0f);
    } else {
        sound_enqueue(0xc, 3, 0.0f);
    }
}
