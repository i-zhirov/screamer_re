/* The rebuilt `race_menu_handler` module.
 *
 * The original sits at linear 0x2a214 in STARTH.EXE.  The record keeps the
 * 336-byte extent up to the next record at 0x2a364 and sets no separate
 * function size.  The dispatch head holds 31 instructions and 103 bytes up to
 * the default `ret` at 0x2a27a, so the identity check uses 31 instructions.
 * The case blocks after the head hold 75 more instructions and 13 calls.
 *
 * The function reads the context dword at 0x2dec and its field `+0x18`.  A
 * value 3 moves to 4, so the menu skips the value 3.  It then reads the mode
 * word `DAT_0001d260` and the counter `DAT_000203c8`.  The switch selects the
 * race-menu path:
 *
 *     mode 1        the loading screen from the table `DAT_000021e4` at the
 *                   context field `+0x28`, the menu space, the menu frame, and
 *                   the record save at the track index `DAT_000203bc`
 *     mode 4        the sprintf path, the member check, the loading screen
 *                   with the screen 0, the menu space, and the menu frame
 *     mode 0x10     the race results screen
 *     mode 0x20     the loading screen with the screen 0, the menu space, and
 *     mode 0x40     the menu frame
 *     other         no path
 *
 * The member check clears `DAT_0001d264` when it fails and the flag is set.
 * The three menu paths pass the context value as the third argument and the
 * counter plus one as the second argument.
 *
 * The callers `net_race_screen` at 0x28ba4 and `net_race_lobby` at 0x2d7c4
 * belong to the race-frame group, so the record takes the section
 * `race_frame callees`.  The mode switch, the menu calls, the results screen,
 * and the record save support the name race_menu_handler.  The original has
 * no symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point operation, so the module needs no FPU stub.
 * The module uses no assembly implementation.
 */
extern void *DAT_00002dec;
extern int DAT_0001d260;
extern int DAT_0001d264;
extern int DAT_000203c8;
extern int DAT_000203bc;
extern int DAT_000021e4[];
extern char DAT_00000ff5[];
extern char DAT_00007edc[];

extern void loading_screen(int screen, int counter, int value);
extern void menu_space_init(void);
extern void menu_frame(void);
extern void race_record_save(int track);
extern void race_results_screen(void);
extern int sprintf(char *buffer, const char *format, ...);
extern int member_check(char *path, void *record);

void race_menu_handler(void)
{
    int value;
    int mode;
    int counter;
    char buffer[0x100];
    char member[0x44];

    value = *(int *)((char *)DAT_00002dec + 0x18);
    if (value == 3) {
        value = value + 1;
    }
    mode = DAT_0001d260;
    counter = DAT_000203c8 + 1;
    switch (mode) {
    case 1:
        loading_screen(DAT_000021e4[*(int *)((char *)DAT_00002dec + 0x28)],
                       counter, value);
        menu_space_init();
        menu_frame();
        race_record_save(DAT_000203bc);
        break;
    case 4:
        sprintf(buffer, DAT_00000ff5, DAT_00007edc, counter);
        if (member_check(buffer, member) == -1 && DAT_0001d264 != 0) {
            DAT_0001d264 = 0;
        }
        loading_screen(0, DAT_000203c8 + 1, value);
        menu_space_init();
        menu_frame();
        break;
    case 0x10:
        race_results_screen();
        break;
    case 0x20:
    case 0x40:
        loading_screen(0, counter, value);
        menu_space_init();
        menu_frame();
        break;
    default:
        break;
    }
}
