/* The test harness for the rebuilt `frame_step` module.
 *
 * The harness holds the three callee stubs.  Each stub prints its marker, and the
 * program prints the exit marker, so the output shows the call order:
 *
 *     D  `default_data_copy` ran
 *     J  `joystick_update` ran
 *     F  `frame_timer` ran
 *     T  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void frame_step(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void default_data_copy(void)
{
    put('D');
}

void joystick_update(void)
{
    put('J');
}

void frame_timer(void)
{
    put('F');
}

int main(void)
{
    frame_step();
    put('T');
    quit();
    return 0;
}
