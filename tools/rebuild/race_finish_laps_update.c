/* The rebuilt `race_finish_laps_update` module.
 *
 * The original sits at linear 0x20ec8 in STARTH.EXE and holds 58 instructions
 * and 208 bytes up to the `ret` at 0x20f97.  The next function starts at
 * 0x20f98, so the store record keeps the 208-byte extent and sets no separate
 * function size.  The identity check uses 58 instructions.
 *
 * The only direct caller at 0x20029 is the frame update at 0x1ff70.  The frame
 * update runs this function after the race intro update at 0x20d20 and before
 * the sibling finish handler at 0x20f98 and the finish delay at 0x21058.  No
 * named function sits between the frame update and the target, so the module
 * takes the section Program functions.
 *
 * The gate is bit 3 of DAT_00118e1c.  race_lap_complete at 0x19678 sets the
 * bit when the lap index passes the race distance DAT_0011d2b0, so the function
 * runs after the player completes the last lap.
 *
 * The function releases the sound handle DAT_00100e8 through 0x1f880.  It
 * copies the float cells DAT_00100a8 and DAT_00100b4 into the race time
 * DAT_0011d268 and the lap time DAT_0011d26c.  It copies the player slot
 * DAT_000ffa4 into the car number DAT_0011d270.  The mode DAT_0011d260 then
 * selects the finish handler: 0x23ed8 for 1, 0x23f14 for 2, 0x23f50 for 4,
 * 0x23ed0 for 0x10, 0x2401c for 0x20, and 0x23fc8 for 0x40.
 *
 * The function sets DAT_0011df0 to 1, clears the finish timer DAT_00118e28, and
 * clears bits 0 through 3 and bit 14 of the control word DAT_00118e1c.  It then
 * sets bits 2 and 14, so the finish delay at 0x21058 runs on the next frame.
 *
 * The caller, the completed-lap gate, the result stores, and the per-mode finish
 * handlers support the name `race_finish_laps_update`.  The original has no
 * symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses no assembly.
 */
extern float DAT_00100a8;
extern float DAT_00100b4;
extern int DAT_00100e8;
extern int DAT_000ffa4;
extern int DAT_0011d260;
extern float DAT_0011d268;
extern float DAT_0011d26c;
extern int DAT_0011d270;
extern int DAT_0011df0;
extern unsigned short DAT_00118e1c;
extern int DAT_00118e28;

extern void sound_handle_release(int handle);
extern void finish_mode_1(void);
extern void finish_mode_2(void);
extern void finish_mode_4(void);
extern void finish_mode_16(void);
extern void finish_mode_32(void);
extern void finish_mode_64(void);

void race_finish_laps_update(void)
{
    unsigned int flags;

    if ((DAT_00118e1c & 8) == 0) {
        return;
    }

    sound_handle_release(DAT_00100e8);

    DAT_0011d268 = DAT_00100a8;
    DAT_0011d26c = DAT_00100b4;
    DAT_0011d270 = DAT_000ffa4;

    switch (DAT_0011d260) {
    case 1:
        finish_mode_1();
        break;
    case 2:
        finish_mode_2();
        break;
    case 4:
        finish_mode_4();
        break;
    case 0x10:
        finish_mode_16();
        break;
    case 0x20:
        finish_mode_32();
        break;
    case 0x40:
        finish_mode_64();
        break;
    default:
        break;
    }

    DAT_0011df0 = 1;

    flags = DAT_00118e1c;
    flags &= 0xffffbff0;
    DAT_00118e28 = 0;
    DAT_00118e1c = (unsigned short)flags;
    flags |= 0x4004;
    DAT_00118e1c = (unsigned short)flags;
}
