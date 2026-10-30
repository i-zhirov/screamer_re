/* The rebuilt `net_race_screen` dispatcher module.
 *
 * The original sits at linear 0x28ba4 in STARTH.EXE.  The first-return body
 * holds 34 instructions and 118 bytes: the six register saves, the frame, the
 * state guard, the menu index and table reads, the menu-id switch, and the
 * epilogue.  The case bodies sit after the first return and extend to
 * `0x293a7`, so the full function is 646 instructions and 2052 bytes.
 *
 * The rebuild keeps the dispatcher and represents every case body and the
 * state 2 overlay body as an external step function.  This is the same shape
 * as the `race_state_update` module: the module rebuilds the switch and the
 * case bodies stay as named functions for a later pass.
 *
 * The case targets:
 *
 *     0x000  0x28c60   0x001  0x28c1a   0x002  0x28f8b
 *     0x003  0x28e08   0x004  0x28e92   0x015  0x28db8
 *     0x018  0x28cef   0x019  0x28d1c   0x01a  0x28d4b
 *     0x01b  0x28d7e   0x01c  0x28c4d or 0x28efd
 *     0x01d  0x28c90   0x01e  0x28cbf   0x01f  0x28deb
 *     0x4d2  0x28ec6
 *
 * The state 2 overlay body starts at `0x28fd6` and runs when `DAT_0011fac4` is
 * not zero.
 */
extern unsigned int DAT_0011fac4;
extern unsigned int DAT_0011fb60;
extern unsigned int DAT_0011fb08[];
extern unsigned int DAT_00102de8[];
extern unsigned int DAT_00120880;
extern void net_race_overlay(void);
extern void case_00(void);
extern void case_01(void);
extern void case_02(void);
extern void case_03(void);
extern void case_04(void);
extern void case_15(void);
extern void case_18(void);
extern void case_19(void);
extern void case_1a(void);
extern void case_1b(void);
extern void case_1c_low(void);
extern void case_1c_high(void);
extern void case_1d(void);
extern void case_1e(void);
extern void case_1f(void);
extern void case_4d2(void);

void net_race_screen(void)
{
    unsigned int index;
    unsigned int *base;
    unsigned int id;

    if (DAT_0011fac4 != 0) {
        net_race_overlay();
        return;
    }
    index = DAT_0011fb60;
    base = (unsigned int *)DAT_00102de8[index];
    id = *(unsigned int *)((char *)base + (DAT_0011fb08[index] << 4) + 4);
    switch (id) {
    case 0x00:
        case_00();
        break;
    case 0x01:
        case_01();
        break;
    case 0x02:
        case_02();
        break;
    case 0x03:
        case_03();
        break;
    case 0x04:
        case_04();
        break;
    case 0x15:
        case_15();
        break;
    case 0x18:
        case_18();
        break;
    case 0x19:
        case_19();
        break;
    case 0x1a:
        case_1a();
        break;
    case 0x1b:
        case_1b();
        break;
    case 0x1c:
        if (DAT_00120880 < 0x7a1200) {
            case_1c_low();
        } else {
            case_1c_high();
        }
        break;
    case 0x1d:
        case_1d();
        break;
    case 0x1e:
        case_1e();
        break;
    case 0x1f:
        case_1f();
        break;
    case 0x4d2:
        case_4d2();
        break;
    default:
        break;
    }
}
