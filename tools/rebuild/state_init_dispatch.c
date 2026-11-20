/* The rebuilt `state_init_dispatch` module.
 *
 * The original sits at linear 0x2be94 in STARTH.EXE.  The entry path holds 9
 * instructions and 28 bytes up to the `ret` at 0x2beaf.  The other case blocks
 * follow the `ret` and end at the next record at 0x2bedc, so the store record
 * keeps the 72-byte extent and sets no separate function size.  The identity
 * check uses 9 instructions.
 *
 * The only direct caller at 0x295c9 is case 1 of `race_state_update` at
 * 0x29564.  `race_state_update` is a `race_frame` callee at 0x2807c, so the
 * module takes the section `race_frame callees`.
 *
 * The function reads the state word at `DAT_0011fac4`.  A value above 8 takes
 * the default path and returns.  The jump table at linear 0x2be70 selects the
 * case block for each state:
 *
 *     state 0  `menu_frame` at 0x29f80
 *     state 1  no call
 *     state 2  `car_select_screen` at 0x2bedc
 *     state 3  `track_select_screen` at 0x2c188
 *     state 4  `name_entry_screen` at 0x2c430
 *     state 5  `championship_screen_init` at 0x2c7a8
 *     state 6  `race_records_screen_init` at 0x2c954
 *     state 7  no call
 *     state 8  `net_race_lobby_init` at 0x2ca40
 *
 * The state word, the jump table, and the seven screen initializers support
 * the name `state_init_dispatch`.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no direct data reference
 * beyond the state word, so the module needs no FPU stub and no assembly.
 */
extern unsigned int DAT_0011fac4;

extern void menu_frame(void);
extern void car_select_screen(void);
extern void track_select_screen(void);
extern void name_entry_screen(void);
extern void championship_screen_init(void);
extern void race_records_screen_init(void);
extern void net_race_lobby_init(void);

void state_init_dispatch(void)
{
    unsigned int state;

    state = DAT_0011fac4;
    if (state > 8) {
        return;
    }
    if (state == 0) {
        menu_frame();
    } else if (state == 2) {
        car_select_screen();
    } else if (state == 3) {
        track_select_screen();
    } else if (state == 4) {
        name_entry_screen();
    } else if (state == 5) {
        championship_screen_init();
    } else if (state == 6) {
        race_records_screen_init();
    } else if (state == 8) {
        net_race_lobby_init();
    }
}
