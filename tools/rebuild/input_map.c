/* The rebuilt `input_map` module.
 *
 * The original sits at linear 0x211e0 in STARTH.EXE and holds 62 instructions
 * and 239 bytes up to the `ret` at 0x212ce, plus the `nop` at 0x212cf.  The
 * function tests the control byte at `DAT_00118e1d`, the exit flags, the input
 * word at `DAT_00118e1c`, the overlay state, the gear release, and the reset
 * gate.
 *
 * The control byte is the high byte of the input word.  Its bit 0x20 requests
 * the race exit.  When one of the four flags at `DAT_0000003b`,
 * `DAT_0000001e`, `DAT_00000003`, and `DAT_00019880` is set, the function sets
 * bit 1 of the exit byte `DAT_001ce44`.  Otherwise the function returns.
 *
 * The input word bit 0x401c blocks the rest of the function.  The overlay
 * update runs when the overlay flag `DAT_0000001b` is set and the mode
 * `DAT_001d260` is not 0x10.  The gear counter `DAT_001d2a4` advances from 0
 * through 3 while the release flag `DAT_00000034` is set.  The last block runs
 * the reset helper and sets the input bits 0x220 when the control byte holds
 * bit 0.
 *
 * The original has a dead local store of 0xffffff before the reset call.  The
 * rebuild keeps the local to hold the instruction count.
 */
typedef union {
    unsigned short bits;
    unsigned char bytes[2];
} control_flags;

extern control_flags DAT_00118e1c;
extern unsigned char DAT_0000003b;
extern unsigned char DAT_0000001e;
extern unsigned char DAT_00000003;
extern unsigned int DAT_00019880;
extern unsigned char DAT_001ce44;
extern unsigned char DAT_0000001b;
extern unsigned int DAT_001d260;
extern unsigned int DAT_000007f9;
extern unsigned char DAT_00000034;
extern unsigned int DAT_00118f90;
extern unsigned int DAT_001d2a4;

extern void input_overlay_update(void);
extern void input_reset(int value);

void input_map(void)
{
    if ((DAT_00118e1c.bytes[1] & 0x20) != 0) {
        if (DAT_0000003b == 0 && DAT_0000001e == 0
            && DAT_00000003 == 0 && DAT_00019880 == 0) {
            return;
        }
        DAT_001ce44 = DAT_001ce44 | 2;
        return;
    }
    if ((DAT_00118e1c.bits & 0x401c) != 0) {
        return;
    }
    if (DAT_0000001b != 0 && DAT_001d260 != 0x10) {
        input_overlay_update();
        DAT_000007f9 = 0;
    }
    if (DAT_00000034 != 0) {
        if (DAT_00118f90 == 0) {
            DAT_001d2a4 = DAT_001d2a4 + 1;
            if (DAT_001d2a4 > 3) {
                DAT_001d2a4 = 0;
            }
        }
        DAT_00118f90 = 1;
    } else {
        DAT_00118f90 = 0;
    }
    if (DAT_00000003 != 0) {
        volatile int guard;
        guard = 0xffffff;
        input_reset(1);
        if ((DAT_00118e1c.bytes[1] & 1) != 0) {
            DAT_00118e1c.bits = DAT_00118e1c.bits | 0x220;
        }
    }
}
