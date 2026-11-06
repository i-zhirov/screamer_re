/* The test harness for the rebuilt `object_prepare` module.
 *
 * The harness holds the three preparation step stubs.  Each stub counts its
 * calls.  The harness runs the module with the state at `+0x414` cleared and
 * then set, and checks the call set:
 *
 *     0  with the state cleared: the steps 0 and 2 ran, the step 1 did not
 *     1  with the state set: the steps 0, 1, and 2 ran
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_prepare(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];
static int step_0_calls;
static int step_1_calls;
static int step_2_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void object_prepare_0(void *obj)
{
    step_0_calls = step_0_calls + 1;
}

void object_prepare_1(void *obj)
{
    step_1_calls = step_1_calls + 1;
}

void object_prepare_2(void *obj)
{
    step_2_calls = step_2_calls + 1;
}

int main(void)
{
    *(unsigned int *)(object + 0x414) = 0;
    step_0_calls = 0;
    step_1_calls = 0;
    step_2_calls = 0;
    object_prepare(object);
    put(step_0_calls == 1 && step_1_calls == 0 && step_2_calls == 1 ? '0' : 'x');
    *(unsigned int *)(object + 0x414) = 1;
    step_0_calls = 0;
    step_1_calls = 0;
    step_2_calls = 0;
    object_prepare(object);
    put(step_0_calls == 1 && step_1_calls == 1 && step_2_calls == 1 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
