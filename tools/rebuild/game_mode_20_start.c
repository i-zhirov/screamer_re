/* The rebuilt `game_mode_20_start` module.
 *
 * The original sits at linear 0x13120 in STARTH.EXE.  The body holds 44
 * instructions and 133 bytes up to the `ret` at 0x131a4.  The three pad bytes
 * at 0x131a5 through 0x131a7 and the next function `race_data_load` at 0x131a8
 * give the 136-byte extent.  The store record keeps that extent and sets no
 * separate function size.  The identity check uses 44 instructions.
 *
 * `race_session_start` at 0x128e0 dispatches the game mode 0x20 to this
 * function.  The caller takes the section The timing and filter layer, so this
 * module takes the same section.  The program map already holds the name
 * `game_mode_20_start` with the confidence inferred.
 *
 * The function walks the opponent records at `DAT_0010d700`.  The count
 * `DAT_0010d6f8` sets the end of the walk, and each record is 0x40 bytes.  The
 * function clears the started flag at record offset 0x3c.
 *
 * The function then clears `DAT_001100d4`, sets `DAT_0000cf40` to 1, and clears
 * `DAT_001100a4`.  It loads the player car through `player_car_init` at 0x136cc
 * with the object pointer 0xf91c and the mode argument.  A non-zero result
 * returns at once.  Otherwise the function runs `scene_setup` at 0x15984 with
 * the mode argument and returns 0.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub and no
 * assembly.
 */
extern int DAT_0010d6f8;
extern char DAT_0010d700[];
extern int DAT_001100d4;
extern int DAT_0000cf40;
extern int DAT_001100a4;

extern int player_car_init(void *car, int load);
extern void scene_setup(int value);

int game_mode_20_start(int index)
{
    char *ptr;
    char *end;
    int result;

    ptr = DAT_0010d700;
    end = ptr + DAT_0010d6f8 * 0x40;
    while (ptr < end) {
        *(int *)(ptr + 0x3c) = 0;
        ptr = ptr + 0x40;
    }
    DAT_001100d4 = 0;
    DAT_0000cf40 = 1;
    DAT_001100a4 = 0;
    result = player_car_init((void *)0xf91c, index);
    if (result != 0) {
        return result;
    }
    scene_setup(index);
    return 0;
}
