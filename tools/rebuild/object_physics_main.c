/* The test harness for the rebuilt `object_physics` module.
 *
 * The harness holds the object record and the four step stubs.  Each stub
 * counts its calls.  The harness runs the module with the state at `+0x414`
 * cleared and then set, and checks the call set:
 *
 *     0  with the state cleared: the gear step and the collision step ran
 *     1  with the state set: all four steps ran
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_physics(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];
static int slide_calls;
static int gear_state_calls;
static int gear_step_calls;
static int collision_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void object_slide_step(void *obj) { slide_calls = slide_calls + 1; }
void object_gear_state(void *obj) { gear_state_calls = gear_state_calls + 1; }
void object_gear_step(void *obj) { gear_step_calls = gear_step_calls + 1; }
void object_collision(void *obj) { collision_calls = collision_calls + 1; }

int main(void)
{
    *(unsigned int *)(object + 0x414) = 0;
    slide_calls = 0;
    gear_state_calls = 0;
    gear_step_calls = 0;
    collision_calls = 0;
    object_physics(object);
    put(slide_calls == 0 && gear_state_calls == 0
        && gear_step_calls == 1 && collision_calls == 1 ? '0' : 'x');
    *(unsigned int *)(object + 0x414) = 1;
    slide_calls = 0;
    gear_state_calls = 0;
    gear_step_calls = 0;
    collision_calls = 0;
    object_physics(object);
    put(slide_calls == 1 && gear_state_calls == 1
        && gear_step_calls == 1 && collision_calls == 1 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
