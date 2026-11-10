/* The rebuilt `view_mode_dispatch` module.
 *
 * The original sits at linear 0x19e78 in STARTH.EXE and holds 41 instructions
 * and 134 bytes up to the `ret` at 0x19efd.  The next function starts at
 * 0x19f00, so the store record keeps the 136-byte extent and sets no separate
 * function size.  The identity check uses 41 instructions.
 *
 * The only direct caller is the player update at 0x19ab0, which calls this
 * function at 0x19ac2 on the player view object `DAT_00110a20` after
 * `player_view_setup` at 0x19ae8.  `player_view_setup` writes the render mode
 * at view field `+0x88` from the chosen view slot, and the car camera at
 * 0x1a930 also starts the renderer at 0x19e78, so the module takes the section
 * `player_view_setup callees`.
 *
 * The function reads the render mode at view field `+0x88`.  A value above 9
 * takes the default path and returns.  The jump table at 0x19e50 selects the
 * camera routine for each mode:
 *
 *     mode 0   player_cockpit_camera at 0x1a2bc
 *     mode 1   player_view_orbit at 0x1a3b8 with the mode 1
 *     mode 2   no routine
 *     mode 3   player_route_camera at 0x1a630
 *     mode 4   no routine
 *     mode 5   no routine
 *     mode 6   player_view_orbit at 0x1a3b8 with the mode 2
 *     mode 7   no routine
 *     mode 8   car_camera_select at 0x1aec4
 *     mode 9   player_view_orbit at 0x1a3b8 with the mode 0
 *
 * Every camera routine except `car_camera_select` takes the player object at
 * object-3 offset 0xf91c in `eax` and the view object in `edx`.
 * `player_view_orbit` also takes the orbit mode in `ebx`.  `car_camera_select`
 * takes the view object in `eax`.
 *
 * The mode field, the jump table, and the four camera routines support the
 * name `view_mode_dispatch`; the original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no direct data reference
 * beyond the mode field, so the module needs no FPU stub and no assembly.
 */

struct player_view {
    int field_00[0x22];
    int mode;
};

extern void player_cockpit_camera(char *car, char *view);
extern void player_view_orbit(char *car, char *view, int mode);
extern void player_route_camera(char *car, char *view);
extern void car_camera_select(char *view);

void view_mode_dispatch(struct player_view *view)
{
    switch (view->mode) {
    case 0:
        player_cockpit_camera((char *)0xf91c, (char *)view);
        break;
    case 1:
        player_view_orbit((char *)0xf91c, (char *)view, 1);
        break;
    case 3:
        player_route_camera((char *)0xf91c, (char *)view);
        break;
    case 6:
        player_view_orbit((char *)0xf91c, (char *)view, 2);
        break;
    case 8:
        car_camera_select((char *)view);
        break;
    case 9:
        player_view_orbit((char *)0xf91c, (char *)view, 0);
        break;
    }
}
