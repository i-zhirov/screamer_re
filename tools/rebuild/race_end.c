/* The rebuilt `race_end` module.
 *
 * The original sits at linear 0x279a4 in STARTH.EXE and holds 36 instructions
 * and 141 bytes: the three register saves, the frame, the one local, the
 * thirteen teardown calls, the gated call at `0x15874`, the two final copies,
 * and the epilogue.
 *
 * The function tears the race down.  It calls the thirteen targets in order.
 * The first call receives the value 0.  The call at `0x2fe68` receives the two
 * imported data addresses.  The two calls at `0x115f8` and `0x11304` receive
 * the address `DAT_00006530`.  The call at `0x15874` runs only when
 * `DAT_0018e3c` is zero, and then sets `DAT_0018e3c` to 1.
 *
 * The last two stores copy `DAT_001d22c` to `DAT_001d274` and `DAT_0018e3c`
 * to `DAT_001d230`.
 *
 * The call targets have no recovered names yet, so the module names them by
 * their order and records the addresses in the comments below.
 *
 *     01  0x1f8b4  (0)
 *     02  0x1f8dc
 *     03  0x2fe68  (DAT_000062b0, DAT_00006350)
 *     04  0x264ec
 *     05  0x1b180
 *     06  0x115f8  (DAT_00006530)
 *     07  0x11304  (DAT_00006530)
 *     08  0x15874  (gated on DAT_0018e3c == 0)
 *     09  0x4cf2c
 *     10  0x1f6f8
 *     11  0x1b58c
 *     12  0x25640
 *     13  0x258b8
 */
extern char DAT_000062b0[];
extern char DAT_00006350[];
extern char DAT_00006530[];
extern int DAT_0018e3c;
extern unsigned int DAT_001d22c;
extern unsigned int DAT_001d274;
extern unsigned int DAT_001d230;
extern void teardown_01(int value);
extern void teardown_02(void);
extern void teardown_03(void *a, void *b);
extern void teardown_04(void);
extern void teardown_05(void);
extern void teardown_06(void *a);
extern void teardown_07(void *a);
extern void teardown_08(void);
extern void teardown_09(void);
extern void teardown_10(void);
extern void teardown_11(void);
extern void teardown_12(void);
extern void teardown_13(void);

void race_end(void)
{
    int flag;

    teardown_01(0);
    teardown_02();
    teardown_03(DAT_000062b0, DAT_00006350);
    teardown_04();
    teardown_05();
    teardown_06(DAT_00006530);
    teardown_07(DAT_00006530);
    if (DAT_0018e3c == 0) {
        flag = 1;
        teardown_08();
        DAT_0018e3c = flag;
    }
    teardown_09();
    teardown_10();
    teardown_11();
    teardown_12();
    teardown_13();
    DAT_001d274 = DAT_001d22c;
    DAT_001d230 = DAT_0018e3c;
}
