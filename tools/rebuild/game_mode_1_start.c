/* The rebuilt `game_mode_1_start` module.
 *
 * The original sits at linear 0x12c0c in STARTH.EXE.  The plan row holds the
 * first 49 instructions and 145 bytes up to the early `ret` at 0x12c9c.  That
 * `ret` is the error return of the player car load.  The full body continues
 * through the shared epilogue at 0x12d00 to the `ret` at 0x12d08.  The next
 * function starts at 0x12d0c after three pad bytes, so the store record keeps
 * the 256-byte extent and sets no separate function size.  The identity check
 * uses 49 instructions.
 *
 * `race_session_start` at 0x128e0 dispatches the modes 1 and 2 to this
 * function.  The caller takes the section The timing and filter layer, so this
 * module takes the same section.  The program map already holds the name
 * `game_mode_1_start` with the confidence inferred.
 *
 * The function starts the race session.  It clears the car slot counter
 * DAT_0000f8f0 and the shared car counter DAT_0000634c, stores the car count
 * plus one at DAT_0000cf40, and copies the car count to DAT_001100a4.
 *
 * The car loop walks the car count DAT_0000f8ec.  For each 0x590-byte car
 * record at DAT_00010ab8 it calls `car_setup` with the record, the index, and
 * the mode argument.  A non-zero result returns at once.
 *
 * After the car loop the function loads the player car through
 * `player_car_init` at 0x136cc with the object pointer 0xf91c and the mode
 * argument.  A non-zero result returns at once.  It then runs `scene_setup` at
 * 0x15984 with the mode argument.
 *
 * The replay flag DAT_0018e30 and the control flag DAT_0000cf54 select the
 * tail.  When either flag is set, a set replay flag prepares the frame buffer
 * through 0x22c30 with a non-zero mode argument, loads the replay name
 * DAT_0001d278 through `replay_load` at 0x1b840 with the flag 1, and runs
 * 0x22db8.  When both flags are clear, a non-zero mode argument prepares the
 * frame buffer, and the function runs 0x22d88.  The success path returns 0.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub and no
 * assembly.
 */
extern int DAT_0000f8ec;
extern int DAT_0000f8f0;
extern int DAT_0000634c;
extern int DAT_0000cf40;
extern int DAT_001100a4;
extern int DAT_0000cf54;
extern int DAT_0018e30;
extern int DAT_0018e60;
extern char DAT_00010ab8[];
extern char DAT_0001d278[];

extern int car_setup(void *record, int index, int load);
extern int player_car_init(void *car, int load);
extern void scene_setup(int value);
extern int replay_load(char *name, int load_frames);
extern int func_22c30(void);
extern void func_22d88(void);
extern void func_22db8(void);

int game_mode_1_start(int index)
{
    int result;

    DAT_0000634c = 0;
    DAT_0000cf40 = DAT_0000f8ec + 1;
    DAT_0000f8f0 = 0;
    DAT_001100a4 = DAT_0000f8ec;
    while (DAT_0000634c < DAT_0000f8ec) {
        result = car_setup((void *)(DAT_00010ab8 + DAT_0000634c * 0x590),
                           DAT_0000634c, index);
        if (result != 0) {
            return result;
        }
        DAT_0000634c = DAT_0000634c + 1;
    }
    result = player_car_init((void *)0xf91c, index);
    if (result != 0) {
        return result;
    }
    scene_setup(index);
    if (DAT_0018e30 != 0 || DAT_0000cf54 != 0) {
        if (DAT_0018e30 != 0) {
            if (index != 0) {
                DAT_0018e60 = func_22c30();
            }
            replay_load(DAT_0001d278, 1);
        }
        func_22db8();
    } else {
        if (index != 0) {
            DAT_0018e60 = func_22c30();
        }
        func_22d88();
    }
    return 0;
}
