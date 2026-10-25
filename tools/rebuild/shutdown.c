/* The rebuilt `engine_shutdown` module.
 *
 * The original sits at linear 0x128ac in STARTH.EXE and holds 14 instructions and 52
 * bytes: the EBP frame `push ebp` and `mov ebp,esp`, eight calls with the argument
 * setup `xor eax,eax` for the value 0 and `mov eax,0x62b0` for the fourth call, and
 * the epilogue `pop ebp` and `ret`.
 *
 * The frame comes from the Watcom stack-frame option `-of+`, which the original build
 * passed to the game files (see the findings).  OpenWatcom V2 adds `mov esp,ebp` to the
 * epilogue, so the rebuild holds 15 instructions and 54 bytes until that difference is
 * resolved.  No compiler option suppresses the restore: the option list holds `-of` for
 * `as needed`, `-of+` for `always`, `-ok` for the flow graph, and `-oc` for the
 * call-to-jump form, and each one keeps the restore.
 */
extern void f1(void);
extern void f2(void);
extern void f3(int value);
extern void f4(int value);
extern void f5(void);
extern void f6(void);
extern void f7(void);
extern void f8(void);

void engine_shutdown(void)
{
    f1();
    f2();
    f3(0);
    f4(0x62b0);
    f5();
    f6();
    f7();
    f8();
}
