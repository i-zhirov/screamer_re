/* The rebuilt `race_cleanup` module.
 *
 * The original sits at linear 0x2a1b0 in STARTH.EXE and holds 24 instructions
 * and 97 bytes: the two register saves, the frame, the twelve cleanup calls,
 * and the epilogue.  The function releases the race state.  Two calls take the
 * two imported data addresses, one call takes the pointer in `DAT_000062ac`,
 * and the last call takes the address `DAT_000062b0`.
 *
 * The call targets have no recovered names yet, so the module names them by
 * their order and records the addresses in the comments below.
 *
 *     01  0x264ec
 *     02  0x1be6c
 *     03  0x25640
 *     04  0x258b8
 *     05  0x115f8  (DAT_00006530, DAT_00006350)
 *     06  0x11304  (DAT_00006530, DAT_00000fed)
 *     07  0x11a08
 *     08  0x4cecc
 *     09  0x1b58c
 *     10  0x2f326  (DAT_000062ac)
 *     11  0x1f6f8
 *     12  0x2fe68  (DAT_000062b0)
 */
extern void *DAT_000062ac;
extern char DAT_000062b0[];
extern char DAT_00006530[];
extern char DAT_00006350[];
extern char DAT_00000fed[];
extern void cleanup_01(void);
extern void cleanup_02(void);
extern void cleanup_03(void);
extern void cleanup_04(void);
extern void cleanup_05(void *a, void *b);
extern void cleanup_06(void *a, void *b);
extern void cleanup_07(void);
extern void cleanup_08(void);
extern void cleanup_09(void);
extern void cleanup_10(void *a);
extern void cleanup_11(void);
extern void cleanup_12(void *a);

void race_cleanup(void)
{
    cleanup_01();
    cleanup_02();
    cleanup_03();
    cleanup_04();
    cleanup_05(DAT_00006530, DAT_00006350);
    cleanup_06(DAT_00006530, DAT_00000fed);
    cleanup_07();
    cleanup_08();
    cleanup_09();
    cleanup_10(DAT_000062ac);
    cleanup_11();
    cleanup_12(DAT_000062b0);
}
