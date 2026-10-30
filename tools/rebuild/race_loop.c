/* The rebuilt `race_loop` module.
 *
 * The original sits at linear 0x27700 in STARTH.EXE and holds 34 instructions
 * and 110 bytes: the three register saves, the frame, the start call at
 * `0x27770`, the loop body, the exit test on `DAT_0011ce44`, the end call at
 * `0x279a4`, and the epilogue.
 *
 * The function starts the race.  A non-zero start result is returned at once.
 * Otherwise the function runs the loop body until the session state reaches 4:
 * the motion update, the default data copy, the joystick update, the gated
 * input poll, the frame timer, the input map, the object update, and the video
 * fill.  Then it runs the race end and returns 0.
 */
extern unsigned int DAT_001d2c0;
extern unsigned int DAT_001d2cc;
extern unsigned int DAT_0011ce44;
extern int race_start(void);
extern void motion_update(void);
extern void default_data_copy(void);
extern void joystick_update(void);
extern int input_poll(void);
extern void frame_timer(void);
extern void input_map(void);
extern void object_update_all(void);
extern void video_fill(void);
extern void race_end(void);

int race_loop(void)
{
    int result;

    result = race_start();
    if (result != 0) {
        return result;
    }
    do {
        motion_update();
        default_data_copy();
        joystick_update();
        if (DAT_001d2c0 != 0 && DAT_001d2cc != 0) {
            input_poll();
        }
        frame_timer();
        input_map();
        object_update_all();
        video_fill();
    } while (DAT_0011ce44 != 4);
    race_end();
    return 0;
}
