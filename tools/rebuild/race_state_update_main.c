/* The test harness for the rebuilt `race_state_update` module.
 *
 * The harness holds the state words and the eight state step stubs.  Each stub
 * counts its calls.  The harness runs the states 0, 2, 7, and 8, and checks
 * the call set:
 *
 *     0  the state 0: `state_step_0` ran
 *     2  the state 2: `state_step_2` ran
 *     7  the state 7: `state_step_7` ran
 *     D  the exit marker, after the state 8 ran nothing
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void race_state_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0011fac4;
unsigned int DAT_0011fac8;
unsigned int DAT_0011facc;
unsigned int DAT_000203a4;
unsigned int DAT_000203b0;
unsigned int DAT_000203b8;
float DAT_0002038c;
float DAT_00018e08;
float DAT_0000f2f;

static char text[8];
static int step_calls[8];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void step(unsigned index)
{
    step_calls[index] = step_calls[index] + 1;
}

void state_step_0(void) { step(0); }
void state_step_1(void) { step(1); }
void state_step_2(void) { step(2); }
void state_step_3(void) { step(3); }
void state_step_4(void) { step(4); }
void state_step_5(void) { step(5); }
void state_step_6(void) { step(6); }
void state_step_7(void) { step(7); }

static void run_state(unsigned int state)
{
    unsigned i;

    DAT_0011fac4 = state;
    for (i = 0; i < 8; i = i + 1) {
        step_calls[i] = 0;
    }
    race_state_update();
}

static int only_step(unsigned int index)
{
    unsigned i;
    int ok;

    ok = step_calls[index] == 1;
    for (i = 0; i < 8; i = i + 1) {
        if (i != index && step_calls[i] != 0) {
            ok = 0;
        }
    }
    return ok;
}

int main(void)
{
    run_state(0);
    put(only_step(0) ? '0' : 'x');
    run_state(2);
    put(only_step(2) ? '2' : 'x');
    run_state(7);
    put(only_step(7) ? '7' : 'x');
    run_state(8);
    put(step_calls[0] == 0 && step_calls[1] == 0 && step_calls[2] == 0
        && step_calls[3] == 0 && step_calls[4] == 0 && step_calls[5] == 0
        && step_calls[6] == 0 && step_calls[7] == 0 ? 'D' : 'x');
    quit();
    return 0;
}
