/* The rebuilt `race_session_run` module.
 *
 * The original sits at linear 0x1fd70 in STARTH.EXE and holds 112 instructions
 * and 419 bytes up to the `ret` at 0x1ff12.  The next function starts at
 * 0x1ff14, so the store record keeps the 420-byte extent and sets no function
 * size.  The identity check uses 112 instructions.
 *
 * The function is the outer race-session driver.  `loading_screen` at 0x2a364
 * calls it at 0x2a514 and treats a non-zero result as a fatal error.
 * `race_results_screen` at 0x2a558 calls it at 0x2a753.  The function saves the
 * mode word `DAT_001d274`, moves bit 0x80 of the mode byte `DAT_001d260` to
 * `DAT_0018e30` and `DAT_0000cf54`, and clears that bit.  It then runs
 * `race_session_start(1)` and stops on a non-zero result.
 *
 * The session loop runs the frame loop at 0x1ff14.  A negative frame result in
 * the replay modes reads the replay record through 0x1b974 with the map name
 * `DAT_001d294`.  A read error returns 0x16 at once and skips the mode restore
 * and the two cleanup calls.  A successful read sets `DAT_001d264`.
 *
 * A set `DAT_0018e30` reloads the controls through `controls_load` and ends the
 * loop.  Otherwise, a failed session with the mode bits 0 and 1, the replay flag
 * `DAT_0018e60`, and no input bits 0x1020 restores a replay through 0x1b738
 * with 0xc88; a mode bit 0 also restarts the session with 0 and runs one frame
 * loop.  The function then restores the input word `DAT_00118e1c`, runs the
 * quit confirmation at 0x219ec behind the mode checks, saves a record through
 * `race_record_save` at 0x2e82c, and restarts the session with 0.
 *
 * The loop repeats while the frame result is not 1.  At the end the function
 * restores `DAT_001d274`, calls 0x21da8 with 1, calls 0x156a8, and returns the
 * last session result.
 *
 * The callers, the session start, the frame loop, the replay reads, the quit
 * screen, and the record save support the name `race_session_run`.  The original
 * has no symbol, so the record keeps the name with the confidence inferred.
 *
 * The function uses no floating-point instruction.  The original has the EBP
 * frame, so the module builds with `-of+`.
 */
extern int DAT_001d274;
extern int DAT_001d260;
extern int DAT_0018e30;
extern int DAT_0000cf54;
extern int DAT_0018e1c;
extern int DAT_0018e60;
extern int DAT_001d264;
extern int DAT_000203bc;
extern char DAT_001d294[];

extern int race_session_start(int value);
extern int race_frame_loop(void);
extern int replay_meta_read(char *name);
extern void controls_load(void);
extern int replay_load(int value);
extern int quit_confirm_screen(void);
extern void race_record_save(int value);
extern void view_transition(int value);
extern void engine_teardown(void);

int race_session_run(void)
{
    int saved_mode;
    int result;
    int exit_code;
    int input;

    saved_mode = DAT_001d274;
    DAT_0018e30 = DAT_001d260 & 0x80;
    DAT_0000cf54 = DAT_0018e30;
    DAT_001d260 = DAT_001d260 & ~0x80;

    result = race_session_start(1);
    if (result != 0) {
        goto finish;
    }

    for (;;) {
        exit_code = race_frame_loop();
        input = DAT_0018e1c;

        if ((DAT_001d260 & 4) != 0 && exit_code < 0 && (input & 0x800) != 0) {
            result = replay_meta_read(DAT_001d294);
            if (result != 0) {
                return 0x16;
            }
            DAT_001d264 = 1;
        }

        if (DAT_0018e30 != 0) {
            exit_code = 1;
            controls_load();
        } else {
            if ((DAT_001d260 & 3) != 0 && DAT_0018e60 != 0
                && (input & 0x1020) == 0 && exit_code < 0) {
                replay_load(0xc88);
                if ((DAT_001d260 & 1) != 0) {
                    DAT_0000cf54 = 1;
                    result = race_session_start(0);
                    if (result != 0) {
                        goto finish;
                    }
                    race_frame_loop();
                }
                DAT_0000cf54 = 0;
            }

            DAT_0018e1c = input;
            exit_code = 1;
            if (DAT_001d260 == 2 && (DAT_0018e1c & 0x1020) == 0) {
                goto test_exit;
            }
            if ((DAT_001d260 & 0x10) != 0) {
                goto test_exit;
            }
            if (quit_confirm_screen() != 0) {
                goto test_exit;
            }
            exit_code = 0;
            if ((DAT_001d260 & 3) != 0) {
                race_record_save(DAT_000203bc);
            }
            result = race_session_start(0);
            if (result != 0) {
                goto finish;
            }
        }

    test_exit:
        if (exit_code != 1) {
            continue;
        }
        break;
    }

finish:
    DAT_001d274 = saved_mode;
    view_transition(1);
    engine_teardown();
    return result;
}
