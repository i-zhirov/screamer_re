/* The rebuilt `race_frame_loop` module.
 *
 * The original sits at linear 0x1ff14 in STARTH.EXE and holds 27 instructions
 * and 90 bytes up to the `ret` at 0x1ff6d.  Two padding bytes follow the `ret`
 * and the next function starts at 0x1ff70, so the store record keeps the
 * 92-byte extent and sets no separate function size.  The identity check uses
 * 27 instructions.
 *
 * The only direct caller is the race session driver `race_session_run` at
 * 0x1fd70, which calls this function at 0x1fdb5 and 0x1fe7a.  The driver runs
 * this loop once per race session and treats a negative result as the replay
 * mode.  The neighbouring record `race_frame_update` at 0x1ff70 calls this
 * function the frame loop, so the module keeps the name `race_frame_loop`.
 *
 * The loop runs one frame: `motion_update`, `default_data_copy`,
 * `joystick_update`, the optional `input_poll`, `music_play` with the request
 * record 0x62b0, `frame_timer`, `input_map`, `race_frame_update`, and
 * `video_fill`.  It then reads the exit state `DAT_0018e18` and repeats while
 * the state stays zero.  It returns the exit state.
 *
 * The input poll runs only when `DAT_001d2c0` and `DAT_001d2cc` are not zero,
 * the same gate as `quit_confirm_screen` at 0x219ec.
 *
 * The caller, the per-frame callees, and the exit state support the name
 * `race_frame_loop`.  The original has no symbol, so the record keeps the name
 * with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction, so the module needs no FPU
 * stub, and the module uses no assembly.
 */
extern int DAT_001d2c0;
extern int DAT_001d2cc;
extern int DAT_0018e18;

extern void motion_update(void);
extern void default_data_copy(void);
extern void joystick_update(void);
extern int input_poll(void);
extern int music_play(char *record);
extern void frame_timer(void);
extern void input_map(void);
extern void race_frame_update(void);
extern void video_fill(void);

int race_frame_loop(void)
{
    int result;

    do {
        motion_update();
        default_data_copy();
        joystick_update();
        if (DAT_001d2c0 != 0 && DAT_001d2cc != 0) {
            input_poll();
        }
        music_play((char *)0x62b0);
        frame_timer();
        input_map();
        race_frame_update();
        video_fill();
        result = DAT_0018e18;
    } while (result == 0);
    return result;
}
