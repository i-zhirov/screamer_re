/* The rebuilt `game_mode_40_start` module.
 *
 * The original sits at linear 0x130a0 in STARTH.EXE and holds 44 instructions
 * and 127 bytes up to the `ret` at 0x1311e.  The next record
 * `game_mode_20_start` starts at 0x13120, so the store record keeps the
 * 128-byte extent and sets no separate function size.  The identity check uses
 * 44 instructions.
 *
 * `race_session_start` at 0x128e0 dispatches the game mode 0x40 to this
 * function through the call at 0x12a1a and passes the mode in eax.  The caller
 * takes the section The timing and filter layer, so this module takes the same
 * section.  The program map already holds the name `game_mode_40_start` with
 * the confidence inferred.
 *
 * The game mode 0x40 is the SLA mode.  The function clears the per-marker hit
 * flag array at `sla_marker_flags` (object field +0x694) for every entry below
 * the marker count `sla_marker_count` at 0x7ed4.  It then clears the state
 * cells DAT_001100a4, DAT_001100dc, and DAT_001100d4, and sets DAT_0000cf40 to
 * 1.
 *
 * The function loads the player car through `player_car_init` at 0x136cc with
 * the object offset 0xf91c and the mode argument.  A non-zero result returns at
 * once.  Otherwise the function runs `scene_setup` at 0x15984 with the mode
 * argument and returns 0.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub and no
 * assembly.
 */
extern int sla_marker_count;
extern int sla_marker_flags[];
extern int DAT_001100a4;
extern int DAT_001100dc;
extern int DAT_001100d4;
extern int DAT_0000cf40;

extern int player_car_init(void *car, int load);
extern void scene_setup(int value);

int game_mode_40_start(int index)
{
    int position;
    int result;

    for (position = 0; position < sla_marker_count; position++) {
        sla_marker_flags[position] = 0;
    }
    DAT_001100a4 = 0;
    DAT_001100dc = 0;
    DAT_001100d4 = 0;
    DAT_0000cf40 = 1;
    result = player_car_init((void *)0xf91c, index);
    if (result != 0) {
        return result;
    }
    scene_setup(index);
    return 0;
}
