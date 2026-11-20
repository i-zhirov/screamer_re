/* The rebuilt `race_finish_sla_update` module.
 *
 * The original sits at linear 0x23fc8 in STARTH.EXE and holds 24 instructions
 * and 83 bytes up to the `ret` at 0x2401a.  The one padding byte at 0x2401b
 * and the next function at 0x2401c give the 84-byte extent, so the store
 * record keeps that extent and sets no separate function size.  The identity
 * check uses 24 instructions.
 *
 * The only direct caller is `race_finish_laps_update` at 0x20ec8 with the call
 * site at 0x20f3e.  That function selects one finish handler from the mode
 * byte DAT_0011d260: this handler runs on mode 0x40.  The game mode 0x40 is
 * the SLA mode, and the handler sits in the `Program functions` section.
 *
 * The handler sets bit 3 of the control byte DAT_00118e1d, writes the finish
 * position 1 to DAT_0000ffa4, and enqueues the effect 0xb with the variant
 * mode 3 and the delay 0.  The enqueue goes through `sound_enqueue` at
 * 0x247fc.
 *
 * When the SLA marker count DAT_00007ed4 equals the hit count DAT_001100dc
 * and the state cell DAT_001100d4 is zero, the handler sets bit 7 (0x80)
 * of the control byte.  The finish overlay at 0x2420c reads that bit and adds
 * the extra text, so the bit marks the all-marker result.
 *
 * The caller, the SLA marker count, the hit count, and the extra-text bit
 * support the name `race_finish_sla_update`.  The original has no symbol, so
 * the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original function holds no floating-point instruction, but the
 * `sound_enqueue` call passes a float argument, so the recipe adds the shared
 * `fpu_stub.asm` for the FPU run-time symbols.  The module uses no assembly
 * implementation.
 */
extern unsigned char DAT_00118e1d;
extern int DAT_00007ed4;
extern int DAT_001100dc;
extern int DAT_001100d4;
extern int DAT_0000ffa4;

extern void sound_enqueue(int effect, int mode, float delay);

void race_finish_sla_update(void)
{
    unsigned char control;
    int markers;

    control = DAT_00118e1d | 8;
    DAT_00118e1d = control;
    markers = DAT_00007ed4;
    DAT_0000ffa4 = 1;
    if (markers == DAT_001100dc && DAT_001100d4 == 0) {
        DAT_00118e1d |= 0x80;
    }
    sound_enqueue(0xb, 3, 0.0f);
}
