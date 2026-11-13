/* The rebuilt `func_27f80` module.
 *
 * The original sits at linear 0x27f80 in STARTH.EXE.  The body holds 7
 * instructions and 18 bytes up to the `ret` at 0x27f91.  Two pad bytes follow
 * the `ret`, and the next recorded function starts at 0x27f94, so the store
 * record keeps the 20-byte extent and sets no separate function size.  The
 * identity check uses 7 instructions.
 *
 * The body reads the race context pointer DAT_00002dec, adds 0x18, and stores
 * the sum in the global DAT_00002e88.  The context field at `+0x18` holds the
 * mode word that `race_menu_handler` at 0x2a214 tests, so the global receives
 * the address of the mode field.
 *
 * The only relocation to the body is the pointer slot at object-3 offset
 * 0x6278 (linear 0x106278), so the body has no direct call.  The slot sits in
 * the 6-byte record table at 0x106272.  The body has no confirmed name, so the
 * record uses the fallback name `func_27f80`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original has no floating-point instruction.  The module needs no assembly
 * source and no FPU stub.
 */
extern void *DAT_00002dec;
extern void *DAT_00002e88;

void func_27f80(void)
{
    DAT_00002e88 = (char *)DAT_00002dec + 0x18;
}
