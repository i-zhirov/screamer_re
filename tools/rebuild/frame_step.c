/* The rebuilt `frame_step` module.
 *
 * The original sits at linear 0x2aa84 in STARTH.EXE and holds 7 instructions and
 * 20 bytes: the frame `push ebp` and `mov ebp,esp`, the call to
 * `default_data_copy` at `0x10193`, the call to `joystick_update` at `0x24da0`,
 * the call to `frame_timer` at `0x1fc38`, and the epilogue `pop ebp` and `ret`.
 *
 * The three calls run in this order, so the frame step copies the default data,
 * updates the joystick, and then waits for the frame.
 *
 * The game files passed the Watcom stack-frame option `-of+`, so the rebuild adds
 * it too.
 */
extern void default_data_copy(void);
extern void joystick_update(void);
extern void frame_timer(void);

void frame_step(void)
{
    default_data_copy();
    joystick_update();
    frame_timer();
}
