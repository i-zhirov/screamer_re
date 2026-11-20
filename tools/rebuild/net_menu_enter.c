/* The rebuilt `net_menu_enter` module.
 *
 * The original sits at linear 0x2df68 in STARTH.EXE and holds 22 instructions
 * and 65 bytes up to the `ret` at 0x2dfa8.  The next function starts at
 * 0x2dfac, so the store record keeps the 68-byte extent and sets no separate
 * function size.  The identity check uses 22 instructions.
 *
 * The function adds the network player 1 through net_player_add at 0x2dddc,
 * runs the network session init at 0xc6f80, requests the NET_SPACE resource at
 * object-3 offset 0x12ca through resource_get at 0x2eed0 with the size 0x2000,
 * reports a zero result through memory_error_report at 0x2edad, and enters the
 * menu through menu_enter at 0x28b5c.
 *
 * The four direct call sites are 0x28365 and 0x283e7 in overlay_dispatch at
 * 0x28160 and 0x2da64 and 0x2dcdf in net_race_lobby at 0x2d7c4.  Both callers
 * run the function to return from a network screen to the menu and then store
 * the mode 0x20 in DAT_0001fb68, so the module takes the name net_menu_enter
 * with the confidence inferred and the section overlay_dispatch callees.
 *
 * The original has the EBP frame, so the module builds with -of+; the function
 * holds no floating-point instruction and makes no floating-point call, so the
 * module needs no FPU stub and no assembly implementation.
 */
extern int net_player_add(int id);
extern int net_session_init(void);
extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern void menu_enter(void);
extern void *DAT_000062ac;

void net_menu_enter(void)
{
    void *resource;

    net_player_add(1);
    net_session_init();
    resource = resource_get(DAT_000062ac, "NET_SPACE", 0x2000);
    if (resource == 0) {
        memory_error_report(DAT_000062ac);
    }
    menu_enter();
}
