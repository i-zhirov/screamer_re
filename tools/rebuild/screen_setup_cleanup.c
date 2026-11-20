/* The rebuilt `screen_setup_cleanup` module.
 *
 * The original sits at linear 0x15150 in STARTH.EXE.  The body holds 12
 * instructions and 42 bytes up to the `ret` at 0x15179.  The next function
 * starts at 0x1517c, so the store record keeps the 44-byte extent and sets no
 * separate function size.  The identity check uses 12 instructions.
 *
 * The direct callers sit in the screen setup functions.  `cd_check_screen` at
 * 0x15874 calls it after a failed `menu1h.pcx` load.  `menu_space_init` at
 * 0x29d08 and `menu_frame` at 0x29f80 call it after the sprite load and the
 * font load.  `loading_screen` at 0x2a364 calls it after a failed `pcx_load`,
 * and the state init screens at 0x2bf2c, 0x2c1d8, 0x2c497, 0x2c8ec, 0x2c9de,
 * 0x2caa9, and 0x2d9f1 call it on a failed PCX or list load.  The callers span
 * several sections, so the record takes the section Program functions.
 *
 * The function runs the teardown in order: the text mode, the keyboard hook
 * with the value 0, the material release at 0x4cecc, the FONTS release at
 * 0x25640, the SPRITES release at 0x258b8, the sound system shutdown at
 * 0x1f28c, and the DOS clock set at 0x1fa2c.  The short teardown follows a
 * failed screen asset load, so the name `screen_setup_cleanup` fits the data
 * flow.  The original has no symbol, so the record keeps the name with the
 * confidence inferred.
 *
 * The original has the EBP frame.  The C form with `-of+` adds `mov esp,ebp`
 * before the epilogue, so the recipe uses the exact `screen_setup_cleanup.asm`
 * body.  This C source stays as the readable behavioral reference.  The
 * function holds no floating-point instruction and makes no math call, so the
 * module needs no FPU stub.
 */
extern void video_text_mode(void);
extern void keyboard_hook(int enable);
extern void materials_release(void);
extern void fonts_release(void);
extern void sprites_release(void);
extern void sound_system_shutdown(void);
extern void dos_datetime_set(void);

void screen_setup_cleanup(void)
{
    video_text_mode();
    keyboard_hook(0);
    materials_release();
    fonts_release();
    sprites_release();
    sound_system_shutdown();
    dos_datetime_set();
}
