/* The rebuilt `menu_enter` module.
 *
 * The original sits at linear 0x28b5c in STARTH.EXE and holds 19 instructions
 * and 54 bytes: the register saves, the frame, the saved value of
 * `DAT_0011fb58`, the two state stores, the menu frame call at `0x29f80`, the
 * restore, and the epilogue.
 *
 * The function saves the value of `DAT_0011fb58`, sets `DAT_0011fb64` to -1
 * and clears the fade `DAT_0011d224`, runs the menu frame, and restores the
 * saved value.
 */
extern unsigned int DAT_0011fb58;
extern unsigned int DAT_0011fb64;
extern unsigned int DAT_0011d224;
extern void menu_frame(void);

void menu_enter(void)
{
    unsigned int saved;

    saved = DAT_0011fb58;
    DAT_0011fb64 = 0xffffffff;
    DAT_0011d224 = 0;
    menu_frame();
    DAT_0011fb58 = saved;
}
