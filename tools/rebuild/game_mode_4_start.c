/* The rebuilt `game_mode_4_start` module.
 *
 * The original sits at linear 0x12e18 in STARTH.EXE.  The plan row holds the
 * first 54 instructions and 169 bytes up to the `ret` at 0x12ec0.  That `ret`
 * is the error return of the ghost loader.  The full body continues through
 * the shared epilogue at 0x12f46 to the `ret` at 0x12f4e.  The next entry
 * `game_mode_100_start` starts at 0x12f50 after one `nop` pad byte, so the
 * store record keeps the 312-byte extent and sets no separate function size.
 * The identity check uses 54 instructions.
 *
 * `race_session_start` at 0x128e0 dispatches the mode 4 to this function
 * through the call at 0x129d8.  The caller takes the section The timing and
 * filter layer, so this module takes the same section.  The program map already
 * holds the name `game_mode_4_start` with the confidence inferred.
 *
 * The function loads the player car through `player_car_init` at 0x136cc with
 * the object pointer 0xf91c and the load flag.  A non-zero result returns at
 * once.  The function then runs `scene_setup` at 0x15984 with the load flag.
 *
 * When the load flag is set, the function loads the `ATTACK_RES` resource
 * through `resource_get` at 0x2eed0 with the name at object-3 offset 0x465 and
 * the size 0x1f400.  A zero result reports the fault through
 * `memory_error_report` at 0x2edad.  The function runs `replay_buffer_init` at
 * 0x22c88, then frees the second `ATTACK_RES` name at object-3 offset 0x470
 * through `resource_free_name` at 0x2f326.
 *
 * The function copies the player index `DAT_0011d2a0` to `DAT_0018ec8` and
 * `DAT_0018ea4`.  When the replay flag `DAT_0011d264` is set, it loads the
 * ghost record through `replay_load_atk` at 0x1ba4c with the map name
 * `DAT_0011d294`, and a non-zero result returns at once.  It then starts the
 * playback through `replay_playback_start` at 0x14d2c with the playback block
 * `DAT_0018ed8` and the flipped ghost index `DAT_0018ed4 ^ 1`, and sets
 * `DAT_001100a4` to 1.  When the flag is clear, `DAT_001100a4` takes 0.
 *
 * The tail clears `DAT_0018eb4` and `DAT_0018e90`, sets `DAT_0000cf40` to 1,
 * sets `DAT_0018e98` and `DAT_0018ebc` to 1 when each is set, then loads the
 * player car again through `player_car_init` with the zero load flag.  A
 * non-zero result returns at once, and the success path returns 0.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub and no
 * assembly.
 */
extern void *DAT_000062ac;
extern int DAT_0011d2a0;
extern int DAT_0011d264;
extern int DAT_0018ec8;
extern int DAT_0018ea4;
extern char DAT_0011d294[];
extern int DAT_0018ed4;
extern char DAT_0018ed8[];
extern int DAT_001100a4;
extern int DAT_0018eb4;
extern int DAT_0018e90;
extern int DAT_0000cf40;
extern int DAT_0018e98;
extern int DAT_0018ebc;

extern int player_car_init(void *car, int load);
extern void scene_setup(int value);
extern void *resource_get(void *archive, char *name, unsigned int size);
extern void memory_error_report(void *archive);
extern void replay_buffer_init(void);
extern void resource_free_name(void *archive, char *name);
extern int replay_load_atk(char *name);
extern int replay_playback_start(char *block, int index);

int game_mode_4_start(int load)
{
    int result;

    result = player_car_init((void *)0xf91c, load);
    if (result != 0) {
        return result;
    }
    scene_setup(load);
    if (load != 0) {
        if (resource_get(DAT_000062ac, (char *)0x465, 0x1f400) == 0) {
            memory_error_report(DAT_000062ac);
        }
        replay_buffer_init();
        resource_free_name(DAT_000062ac, (char *)0x470);
    }
    DAT_0018ec8 = DAT_0011d2a0;
    DAT_0018ea4 = DAT_0011d2a0;
    if (DAT_0011d264 != 0) {
        result = replay_load_atk(DAT_0011d294);
        if (result != 0) {
            return result;
        }
        replay_playback_start(DAT_0018ed8, DAT_0018ed4 ^ 1);
        DAT_001100a4 = 1;
    } else {
        DAT_001100a4 = 0;
    }
    DAT_0018eb4 = 0;
    DAT_0018e90 = 0;
    DAT_0000cf40 = 1;
    if (DAT_0018e98 != 0) {
        DAT_0018e98 = 1;
    }
    if (DAT_0018ebc != 0) {
        DAT_0018ebc = 1;
    }
    result = player_car_init((void *)0xf91c, 0);
    if (result != 0) {
        return result;
    }
    return 0;
}
