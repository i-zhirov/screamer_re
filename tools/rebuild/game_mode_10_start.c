/* The rebuilt `game_mode_10_start` module.
 *
 * The original sits at linear 0x12d0c in STARTH.EXE and holds 76 instructions
 * and 249 bytes up to the first `ret` at 0x12e04.  The loop-exit path ends at
 * the second `ret` at 0x12e16, and the next function starts at 0x12e18, so the
 * store record keeps the 268-byte extent and sets no separate function size.
 * The identity check uses 76 instructions.
 *
 * The only direct caller is the game mode dispatch at 0x12996 in
 * race_session_start at 0x128e0, which sends the mode 0x10 to this function.
 * The caller takes the section The timing and filter layer, so this module
 * takes the same section.  The caller and the program map already hold the
 * name game_mode_10_start with the confidence inferred.
 *
 * The function starts the mode 0x10 race session.  It stores the player index
 * plus one at DAT_0000cf40 and loads the player car through player_car_init at
 * 0x136cc with the object offset 0xf91c and the mode argument.  A failed load
 * returns the result at once.
 *
 * The traffic count DAT_0011d254 selects two float cells.  The cell
 * DAT_0000d6e0 takes the square root of the count times 0.1, minus 0.3, times
 * 0.7.  The cell DAT_0000d6e4 takes 1.0 minus that value.  The count then
 * lands at DAT_001100a4 and DAT_00115218.
 *
 * The function clears the 0x2a8-byte traffic records at DAT_00113cd8 through
 * the memset helper at 0x8c160, runs traffic_step at 0x1eae0 with the retry
 * limit 0x50, and waits through the timing loop at 0x1ec48.  The record loop
 * prepares each traffic record through the record init at 0x14970 and returns
 * the first non-zero result.  When every record is ready, it runs the scene
 * setup at 0x15984 and returns 0.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The three
 * constants 0.1, -0.3, and 0.7 arrive through globals, because the harness
 * image does not load the constant segment.  The local FPU stub file supplies
 * the square root at 0x22bec and the FPU run-time symbols.
 */
extern int DAT_0011d250;
extern int DAT_0011d254;
extern int DAT_0000cf40;
extern int DAT_001100a4;
extern int DAT_00115218;
extern float DAT_0000d6e0;
extern float DAT_0000d6e4;
extern char DAT_00113cd8[];
extern int DAT_0000634c;

extern double traffic_time_scale;
extern double traffic_time_bias;
extern double traffic_time_factor;

extern int player_car_init(int offset, int index);
extern double sqrt(double value);
extern void memory_clear(void *dest, unsigned int count, int value);
extern int traffic_step(int limit);
extern void traffic_wait(void);
extern int traffic_record_init(void *record);
extern void scene_setup(int value);

int game_mode_10_start(int index)
{
    int result;
    int count;
    int position;

    DAT_0000cf40 = DAT_0011d250 + 1;
    result = player_car_init(0xf91c, index);
    if (result != 0) {
        return result;
    }
    count = DAT_0011d254;
    DAT_0000d6e0 = (float)((sqrt((double)count * traffic_time_scale)
                            + traffic_time_bias) * traffic_time_factor);
    DAT_001100a4 = count;
    DAT_00115218 = count;
    DAT_0000d6e4 = (float)(1.0 - DAT_0000d6e0);
    memory_clear(DAT_00113cd8, count * 0x2a8, 0);
    traffic_step(0x50);
    traffic_wait();
    DAT_0000634c = 0;
    while (DAT_0000634c < DAT_00115218) {
        position = DAT_0000634c;
        result = traffic_record_init(DAT_00113cd8 + position * 0x2a8);
        if (result != 0) {
            return result;
        }
        DAT_0000634c = DAT_0000634c + 1;
    }
    scene_setup(index);
    return 0;
}
