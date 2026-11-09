/* The rebuilt `race_finish_time_update` module.
 *
 * The original sits at linear 0x20f98 in STARTH.EXE and holds 55 instructions
 * and 189 bytes up to the `ret` at 0x21054.  The next function starts at
 * 0x21058 after three padding bytes, so the store record keeps the 192-byte
 * extent and sets no separate function size.  The identity check uses 55
 * instructions.
 *
 * The only direct caller at 0x2002e is the frame update at 0x1ff70.  The frame
 * update runs this function after the last-lap handler at 0x20ec8 and before
 * the finish delay at 0x21058.  No named function sits between the frame update
 * and the target, so the module takes the section Program functions.
 *
 * The gate is bit 4 of DAT_00118e1c.  race_clock_update at 0x18a14 sets the bit
 * at 0x18b45 when the time limit expires, so the function runs after the clock
 * reaches zero.
 *
 * The function releases the sound handle DAT_00100e8 through 0x1f880 and queues
 * the effect 0x19 with the subtype 3 and the value 0.0 through 0x247fc.  The
 * mode DAT_0011d260 then selects the finish handler: 0x24070 for 1, 0x24078 for
 * 2, 0x24080 for 4, 0x24068 for 0x10, 0x24098 for 0x20, and 0x24090 for 0x40.
 * The mode 4 handler at 0x24080 sets the player slot DAT_000ffa4 to 2.
 *
 * The function sets DAT_0011df0 to 1, clears the finish timer DAT_00118e28, and
 * clears bits 0, 1, 2, 4, 11, and 12 of the control word DAT_00118e1c.  It then
 * sets bits 2 and 12, so the finish delay at 0x21058 runs on the next frame.
 *
 * The caller, the time-limit gate, the effect push, and the per-mode finish
 * handlers support the name `race_finish_time_update`.  The original has no
 * symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses no assembly and no floating-point instruction.
 */
extern int DAT_00100e8;
extern int DAT_0011d260;
extern int DAT_0011df0;
extern unsigned short DAT_00118e1c;
extern int DAT_00118e28;

extern void sound_handle_release(int handle);
extern void effect_push(int type, int subtype, int value);
extern void finish_time_mode_1(void);
extern void finish_time_mode_2(void);
extern void finish_time_mode_4(void);
extern void finish_time_mode_16(void);
extern void finish_time_mode_32(void);
extern void finish_time_mode_64(void);

void race_finish_time_update(void)
{
    unsigned int flags;

    if ((DAT_00118e1c & 0x10) == 0) {
        return;
    }

    sound_handle_release(DAT_00100e8);
    effect_push(0x19, 3, 0);

    switch (DAT_0011d260) {
    case 1:
        finish_time_mode_1();
        break;
    case 2:
        finish_time_mode_2();
        break;
    case 4:
        finish_time_mode_4();
        break;
    case 0x10:
        finish_time_mode_16();
        break;
    case 0x20:
        finish_time_mode_32();
        break;
    case 0x40:
        finish_time_mode_64();
        break;
    default:
        break;
    }

    DAT_0011df0 = 1;

    flags = DAT_00118e1c;
    flags &= 0xffffe7e8;
    DAT_00118e28 = 0;
    DAT_00118e1c = (unsigned short)flags;
    flags |= 0x1004;
    DAT_00118e1c = (unsigned short)flags;
}
