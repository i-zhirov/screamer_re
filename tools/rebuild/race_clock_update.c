/* The rebuilt `race_clock_update` module.
 *
 * The original sits at linear 0x18a14 in STARTH.EXE and holds 92 instructions
 * and 329 bytes up to the `ret` at 0x18b5c.  The next function starts at
 * 0x18b60, so the store record keeps the 332-byte extent and sets no separate
 * function size.  The identity check uses 92 instructions.
 *
 * The only direct caller is the player update group at 0x15a80, which runs
 * this function at 0x15a8c before `engine_rpm_update` at 0x18db0, the throttle
 * step at 0x160a4, `player_brake_update` at 0x16150, and
 * `player_steering_update` at 0x1629c.  The group runs on the player object at
 * object-3 offset 0xf91c, so the module takes the section The vehicle state
 * chain.
 *
 * The function is the player race clock:
 *
 *     gate        input byte 0x118e1c bit 0 must be set and bit 2 must clear
 *     +0x784      when the gate is set, the total time +0x78c and the lap
 *                 time +0x794 both advance by the frame delta 0x118e08
 *     mode 0x10   scan the 0x2a8-byte traffic records at 0x113cd8 up to the
 *                 count 0x115218, stop at the player index 0x11d250, skip a
 *                 record with a negative first field, and copy the time at
 *                 +0x2c of the first open record to the total time +0x78c
 *     +0x790      unless the saved word 0x11d274 holds bit 4, the remaining
 *                 time counts down by the frame delta
 *     expired     below zero the remaining time clamps to zero and the control
 *                 byte 0x118e1d sets bit 0x40, so the throttle step at 0x160a4
 *                 clears the throttle.  A road speed +0x50c below 1.0 also
 *                 clears bit 0x40 and sets input byte 0x118e1c bit 0x10
 *     open        at or above zero the control byte clears bit 0x40
 *
 * The traffic records, the remaining time, and the control bit support the
 * name `race_clock_update`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * shared `fpu_stub.asm` supplies the FPU run-time symbols.  The function
 * converts no value to an integer, so `__CHP` stays a bare return.
 */
extern unsigned char DAT_00118e1c;
extern unsigned char DAT_00118e1d;
extern float DAT_00118e08;
extern int DAT_0011d260;
extern int DAT_0011d274;
extern int DAT_00115218;
extern int DAT_0011d250;
extern char DAT_00113cd8[];

void race_clock_update(char *self)
{
    int index;

    if ((DAT_00118e1c & 1) == 0) {
        return;
    }
    if ((DAT_00118e1c & 4) != 0) {
        return;
    }

    if (*(int *)(self + 0x784) != 0) {
        *(float *)(self + 0x78c) = *(float *)(self + 0x78c)
            + DAT_00118e08;
        *(float *)(self + 0x794) = *(float *)(self + 0x794)
            + DAT_00118e08;
    }

    if ((DAT_0011d260 & 0x10) != 0) {
        for (index = 0; index < DAT_00115218; index++) {
            if (index == DAT_0011d250) {
                break;
            }
            if (*(int *)(DAT_00113cd8 + index * 680) < 0) {
                continue;
            }
            *(float *)(self + 0x78c) =
                *(float *)(DAT_00113cd8 + index * 680 + 0x2c);
            break;
        }
    }

    if ((DAT_0011d274 & 4) == 0) {
        *(float *)(self + 0x790) = *(float *)(self + 0x790)
            - DAT_00118e08;
    }

    if (*(float *)(self + 0x790) < 0.0f) {
        *(int *)(self + 0x790) = 0;
        DAT_00118e1d = DAT_00118e1d | 0x40;
        if (*(float *)(self + 0x50c) < 1.0f) {
            DAT_00118e1d = DAT_00118e1d ^ 0x40;
            DAT_00118e1c = DAT_00118e1c | 0x10;
        }
    } else {
        DAT_00118e1d = DAT_00118e1d & 0xbf;
    }
}
