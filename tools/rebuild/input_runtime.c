/* The interim input subsystem cluster.
 *
 * The whole-image link reports these symbols as undefined because the shipped
 * image holds the keyboard, joystick, and mouse input layer.  This module
 * defines the input entry points with the calling convention of their callers
 * and the input state globals that the rebuilt modules reference:
 *
 *   input_apply(state, device)        -> apply one input state
 *   input_apply_release()             -> release the applied state
 *   input_clear(key), input_reset(n)  -> clear or reset the input map
 *   input_read(), input_refresh()     -> poll and refresh the state
 *   input_overlay_update()            -> update the overlay input
 *   joystick_axis_read(mask, port)    -> the joystick axis value
 *   joystick_button_read(port)        -> the joystick button state
 *   mouse_system_init/shutdown()      -> the mouse driver
 *   mouse_handler_install/remove()    -> the mouse callback slots
 *   wait_for_input()                  -> wait for the next event
 *
 * The functions are a compatibility layer, not a byte-faithful recovery.  The
 * input entry points are inert and return 0, and the state globals start at 0.
 * The module has no function record and no behavior check, so it does not
 * enter the verified count.
 */
unsigned char input_flags_low;
unsigned char input_flags_high;
int input_selector;
int joystick_axis0;
int joystick_axis1;
int joystick_axis4;
int joystick_axis5;
int joystick_center4;
int joystick_center5;

int input_apply(unsigned int state, unsigned int device)
{
    (void)state;
    (void)device;
    return 0;
}

void input_apply_release(void)
{
}

void input_clear(int key)
{
    (void)key;
}

void input_overlay_update(void)
{
}

void input_read(void)
{
}

void input_refresh(void)
{
}

void input_reset(int value)
{
    (void)value;
}

int joystick_axis_read(int mask, int port)
{
    (void)mask;
    (void)port;
    return 0;
}

int joystick_button_read(int port)
{
    (void)port;
    return 0;
}

int mouse_system_init(int a, int b, int c)
{
    (void)a;
    (void)b;
    (void)c;
    return 0;
}

int mouse_system_shutdown(void)
{
    return 0;
}

int mouse_handler_install(int setup, void *handler, void *state,
                          void *handler_id)
{
    (void)setup;
    (void)handler;
    (void)state;
    (void)handler_id;
    return 0;
}

int mouse_handler_remove(int handler_id, int a, int b)
{
    (void)handler_id;
    (void)a;
    (void)b;
    return 0;
}

void wait_for_input(void)
{
}
