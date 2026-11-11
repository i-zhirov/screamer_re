/* The rebuilt `race_finish_cng_update` module.
 *
 * The original sits at linear 0x2401c in STARTH.EXE and holds 22 instructions
 * and 74 bytes up to the `ret` at 0x24065.  The next function starts at
 * 0x24068, so the store record keeps the 76-byte extent and sets no separate
 * function size.  The identity check uses 22 instructions.
 *
 * The only direct caller is `race_finish_laps_update` at 0x20ec8 with the call
 * site at 0x20f45.  That function selects one finish handler from the mode
 * byte DAT_0011d260: this handler runs on mode 0x20.  The game mode 0x20 is
 * the CNG mode, and the handler sits in the `Program functions` section.
 *
 * The handler sets bit 3 of the control byte DAT_00118e1d, writes the finish
 * position 1 to DAT_0000ffa4, and enqueues the effect 0xb with the variant
 * mode 3 and the delay 0.  The enqueue goes through `sound_enqueue` at
 * 0x247fc.
 *
 * When the opponent count DAT_0010d6f8 equals the state cell DAT_001100d4,
 * the handler sets bit 7 (0x80) of the control byte.  The mode 0x40 finish
 * handler at 0x23fc8 sets the same bit for the SLA result.  The game mode 0x20
 * start at 0x13120 clears the state cell, so the bit marks the CNG result.
 *
 * The caller, the CNG mode, the opponent count, and the state cell support the
 * name `race_finish_cng_update`.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original function holds no floating-point instruction, but the
 * `sound_enqueue` call passes a float argument, so the recipe adds the shared
 * `fpu_stub.asm` for the FPU run-time symbols.  The module uses no assembly
 * implementation.
 */
extern unsigned char DAT_00118e1d;
extern int DAT_0010d6f8;
extern int DAT_001100d4;
extern int DAT_0000ffa4;

extern void sound_enqueue(int effect, int mode, float delay);

void race_finish_cng_update(void)
{
    unsigned char control;
    int opponents;
    int state;

    control = DAT_00118e1d | 8;
    DAT_00118e1d = control;
    state = DAT_001100d4;
    opponents = DAT_0010d6f8;
    DAT_0000ffa4 = 1;
    if (opponents == state) {
        DAT_00118e1d |= 0x80;
    }
    sound_enqueue(0xb, 3, 0.0f);
}
