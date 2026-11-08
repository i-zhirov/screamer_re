/* The test harness for the rebuilt `race_session_run` module.
 *
 * The harness holds the mode word, the mode byte, the input word, the replay
 * flag, the two frame flags, the record index, and the map name.  The callee
 * stubs follow a small script, so the session loop can reach each branch and
 * its exit.  The screen stubs record their arguments.
 *
 * The harness runs ten paths:
 *
 *     0  the session start returns 5: the function restores the mode, calls the
 *        transition with 1, calls the teardown, and returns 5; the mode bit
 *        0x80 moves to the two frame flags
 *     1  the replay mode reads the record and the quit screen answers 1: the
 *        record flag is set, the quit screen runs, and the record is not saved
 *     2  the replay record read fails: the function returns 0x16 at once and
 *        skips the transition and the teardown
 *     3  the saved mode bit 0x80 is set: the function reloads the controls and
 *        ends the loop without the quit screen
 *     4  a failed session in mode 1 restores the replay and restarts the
 *        session: the replay loader takes 0xc88, the start runs twice, the frame
 *        loop runs twice, and the frame flag clears
 *     5  the replay restart fails: the function returns the restart result and
 *        leaves the frame flag set
 *     6  the quit screen answers 0 and the record saves: the loop runs a second
 *        pass, and the second quit answer ends it
 *     7  the mode bit 0x10 skips the quit screen and the record save
 *     8  the mode 2 with no input bits 0x1020 ends the loop before the quit
 *        screen and restores the input word
 *     9  the mode 2 with an input bit 0x1020 runs the quit screen and ends the
 *        loop on its answer
 *     D  the exit marker
 */
#pragma off (check_stack)

int race_session_run(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];

int DAT_001d274;
int DAT_001d260;
int DAT_0018e30;
int DAT_0000cf54;
int DAT_0018e1c;
int DAT_0018e60;
int DAT_001d264;
int DAT_000203bc;
char DAT_001d294[8] = "map1";

static int start_calls;
static int start_arg[8];
static int start_return[8];
static int frame_calls;
static int frame_return[8];
static int meta_calls;
static int meta_match;
static int meta_return;
static int controls_calls;
static int load_calls;
static int load_arg;
static int quit_calls;
static int quit_return[8];
static int record_calls;
static int record_arg;
static int view_calls;
static int view_arg;
static int teardown_calls;

int race_session_start(int value)
{
    int result;

    result = 0;
    if (start_calls < 8) {
        start_arg[start_calls] = value;
        result = start_return[start_calls];
    }
    start_calls = start_calls + 1;
    return result;
}

int race_frame_loop(void)
{
    int result;

    result = 0;
    if (frame_calls < 8) {
        result = frame_return[frame_calls];
    }
    frame_calls = frame_calls + 1;
    return result;
}

int replay_meta_read(char *name)
{
    meta_calls = meta_calls + 1;
    meta_match = (name == DAT_001d294);
    return meta_return;
}

void controls_load(void)
{
    controls_calls = controls_calls + 1;
}

int replay_load(int value)
{
    load_calls = load_calls + 1;
    load_arg = value;
    return 0;
}

int quit_confirm_screen(void)
{
    int result;

    result = 0;
    if (quit_calls < 8) {
        result = quit_return[quit_calls];
    }
    quit_calls = quit_calls + 1;
    return result;
}

void race_record_save(int value)
{
    record_calls = record_calls + 1;
    record_arg = value;
}

void view_transition(int value)
{
    view_calls = view_calls + 1;
    view_arg = value;
}

void engine_teardown(void)
{
    teardown_calls = teardown_calls + 1;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    int index;

    DAT_001d274 = 0x40;
    DAT_001d260 = 0;
    DAT_0018e30 = 0x11;
    DAT_0000cf54 = 0x22;
    DAT_0018e1c = 0;
    DAT_0018e60 = 0;
    DAT_001d264 = 0;
    DAT_000203bc = 0x1234;

    start_calls = 0;
    frame_calls = 0;
    meta_calls = 0;
    meta_match = 0;
    meta_return = 0;
    controls_calls = 0;
    load_calls = 0;
    load_arg = 0;
    quit_calls = 0;
    record_calls = 0;
    record_arg = 0;
    view_calls = 0;
    view_arg = 0;
    teardown_calls = 0;
    for (index = 0; index < 8; index++) {
        start_arg[index] = 0;
        start_return[index] = 0;
        frame_return[index] = 0;
        quit_return[index] = 0;
    }
}

int main(void)
{
    int result;

    reset();
    DAT_001d260 = 0x80;
    start_return[0] = 5;
    result = race_session_run();
    put(result == 5 && start_calls == 1 && start_arg[0] == 1
        && DAT_001d260 == 0 && DAT_0018e30 == 0x80
        && DAT_0000cf54 == 0x80 && frame_calls == 0
        && view_calls == 1 && view_arg == 1 && teardown_calls == 1 ? '0' : 'x');

    reset();
    DAT_001d260 = 6;
    DAT_0018e1c = 0x800;
    frame_return[0] = -1;
    quit_return[0] = 1;
    result = race_session_run();
    put(result == 0 && meta_calls == 1 && meta_match == 1
        && DAT_001d264 == 1 && quit_calls == 1 && record_calls == 0
        && start_calls == 1 && frame_calls == 1
        && view_calls == 1 && teardown_calls == 1 ? '1' : 'x');

    reset();
    DAT_001d260 = 6;
    DAT_0018e1c = 0x800;
    frame_return[0] = -1;
    meta_return = 0x16;
    result = race_session_run();
    put(result == 0x16 && meta_calls == 1 && DAT_001d264 == 0
        && quit_calls == 0 && view_calls == 0 && teardown_calls == 0 ? '2' : 'x');

    reset();
    DAT_001d260 = 0x80;
    frame_return[0] = 0;
    result = race_session_run();
    put(result == 0 && controls_calls == 1 && quit_calls == 0
        && record_calls == 0 && DAT_001d260 == 0 && start_calls == 1
        && view_calls == 1 && teardown_calls == 1 ? '3' : 'x');

    reset();
    DAT_001d260 = 1;
    DAT_0018e60 = 1;
    frame_return[0] = -1;
    frame_return[1] = 0;
    quit_return[0] = 1;
    result = race_session_run();
    put(result == 0 && load_calls == 1 && load_arg == 0xc88
        && start_calls == 2 && start_arg[1] == 0
        && frame_calls == 2 && DAT_0000cf54 == 0
        && quit_calls == 1 && record_calls == 0 ? '4' : 'x');

    reset();
    DAT_001d260 = 1;
    DAT_0018e60 = 1;
    frame_return[0] = -1;
    start_return[1] = 7;
    result = race_session_run();
    put(result == 7 && load_calls == 1 && start_calls == 2
        && start_arg[1] == 0 && frame_calls == 1
        && DAT_0000cf54 == 1 && quit_calls == 0 ? '5' : 'x');

    reset();
    DAT_001d260 = 1;
    quit_return[0] = 0;
    quit_return[1] = 1;
    result = race_session_run();
    put(result == 0 && quit_calls == 2 && record_calls == 1
        && record_arg == 0x1234 && start_calls == 2 && start_arg[1] == 0
        && frame_calls == 2 && view_calls == 1 && teardown_calls == 1 ? '6' : 'x');

    reset();
    DAT_001d260 = 0x10;
    frame_return[0] = 0;
    result = race_session_run();
    put(result == 0 && quit_calls == 0 && record_calls == 0
        && start_calls == 1 && frame_calls == 1 && DAT_0018e30 == 0 ? '7' : 'x');

    reset();
    DAT_001d260 = 2;
    DAT_0018e1c = 0x40;
    frame_return[0] = 0;
    result = race_session_run();
    put(result == 0 && quit_calls == 0 && record_calls == 0
        && DAT_0018e1c == 0x40 && start_calls == 1 ? '8' : 'x');

    reset();
    DAT_001d260 = 2;
    DAT_0018e1c = 0x1020;
    frame_return[0] = 0;
    quit_return[0] = 1;
    result = race_session_run();
    put(result == 0 && quit_calls == 1 && record_calls == 0
        && start_calls == 1 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
