/* The test harness for the rebuilt `race_frame_loop` module.
 *
 * The harness holds the two input gates, the exit state, and one stub for each
 * of the nine callees.  The stubs append a letter to the trace, and the
 * `race_frame_update` stub sets the exit state at the chosen frame.  The
 * `music_play` stub records its argument.
 *
 * The five paths:
 *
 *     0  both gates set, exit 1 at frame 1: one full frame, return 1
 *     1  both gates set, exit -1 at frame 3: three full frames, return -1
 *     2  the first gate clear: no input poll, one frame
 *     3  the second gate clear: no input poll, one frame
 *     4  both gates set, exit 5 at frame 2: two full frames, return 5
 *     D  the exit marker
 *
 * The harness builds the expected trace one letter at a time, because the
 * `print` helper and the data reads of this harness do not reach a string
 * literal.  The markers go through the data buffer `text`.
 */
#pragma off (check_stack)

int DAT_001d2c0;
int DAT_001d2cc;
int DAT_0018e18;

int race_frame_loop(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char trace[128];
static char expect[128];
static int trace_length;
static int expect_length;
static int frame_calls;
static int frame_limit;
static int exit_target;
static unsigned int music_record;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void add(char letter)
{
    if (trace_length < (int)sizeof(trace) - 1) {
        trace[trace_length] = letter;
        trace_length++;
    }
}

static void expect_add(char letter)
{
    if (expect_length < (int)sizeof(expect) - 1) {
        expect[expect_length] = letter;
        expect_length++;
        expect[expect_length] = 0;
    }
}

static void expect_reset(void)
{
    expect_length = 0;
    expect[0] = 0;
}

static void expect_frame(int with_input)
{
    expect_add('A');
    expect_add('B');
    expect_add('C');
    if (with_input) {
        expect_add('D');
    }
    expect_add('E');
    expect_add('F');
    expect_add('G');
    expect_add('H');
    expect_add('I');
}

static int same(void)
{
    int index;

    index = 0;
    while (expect[index] != 0) {
        if (index >= trace_length || trace[index] != expect[index]) {
            return 0;
        }
        index++;
    }
    return index == trace_length;
}

static void reset(void)
{
    trace_length = 0;
    trace[0] = 0;
    expect_reset();
    frame_calls = 0;
    frame_limit = 1;
    exit_target = 1;
    music_record = 0;
    DAT_001d2c0 = 1;
    DAT_001d2cc = 1;
    DAT_0018e18 = 0;
}

void motion_update(void)
{
    add('A');
}

void default_data_copy(void)
{
    add('B');
}

void joystick_update(void)
{
    add('C');
}

int input_poll(void)
{
    add('D');
    return 0;
}

int music_play(char *record)
{
    add('E');
    music_record = (unsigned int)record;
    return 0;
}

void frame_timer(void)
{
    add('F');
}

void input_map(void)
{
    add('G');
}

void race_frame_update(void)
{
    add('H');
    frame_calls++;
    if (frame_calls == frame_limit) {
        DAT_0018e18 = exit_target;
    }
}

void video_fill(void)
{
    add('I');
}

int main(void)
{
    int result;

    reset();
    exit_target = 1;
    expect_frame(1);
    result = race_frame_loop();
    put(result == 1
        && frame_calls == 1
        && music_record == 0x62b0
        && same() ? '0' : 'x');

    reset();
    frame_limit = 3;
    exit_target = -1;
    expect_frame(1);
    expect_frame(1);
    expect_frame(1);
    result = race_frame_loop();
    put(result == -1
        && frame_calls == 3
        && music_record == 0x62b0
        && same() ? '1' : 'x');

    reset();
    DAT_001d2c0 = 0;
    expect_frame(0);
    result = race_frame_loop();
    put(result == 1 && frame_calls == 1 && same() ? '2' : 'x');

    reset();
    DAT_001d2cc = 0;
    expect_frame(0);
    result = race_frame_loop();
    put(result == 1 && frame_calls == 1 && same() ? '3' : 'x');

    reset();
    frame_limit = 2;
    exit_target = 5;
    expect_frame(1);
    expect_frame(1);
    result = race_frame_loop();
    put(result == 5 && frame_calls == 2 && same() ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
