/* The test harness for the rebuilt `race_loop` module.
 *
 * The harness holds the two input flags, the session state, and the ten stubs.
 * Each stub records its call order.  The harness runs the start failure, the
 * one-iteration loop, the gated input poll, and the two-iteration loop:
 *
 *     0  a non-zero start result returns at once and runs no other call
 *     1  the state 4 runs the loop once and then the race end
 *     2  a clear input flag skips the input poll in the loop
 *     3  the state turns to 4 in the second iteration, so the loop runs twice
 *     D  the exit marker
 *
 * The order checks use explicit comparisons, because the initialized arrays of
 * the harness are not reliable under the link model of this project.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int race_loop(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_001d2c0;
unsigned int DAT_001d2cc;
unsigned int DAT_0011ce44;

#define STEP(index, value) (order[index] == (value))

static char text[8];
static char order[24];
static int order_count;
static int start_result;
static int video_calls;
static int set_state_on_video_call;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void record(char step)
{
    if (order_count < 24) {
        order[order_count] = step;
    }
    order_count = order_count + 1;
}

int race_start(void) { record(1); return start_result; }
void motion_update(void) { record(2); }
void default_data_copy(void) { record(3); }
void joystick_update(void) { record(4); }
int input_poll(void) { record(5); return 0; }
void frame_timer(void) { record(6); }
void input_map(void) { record(7); }
void object_update_all(void) { record(8); }
void video_fill(void)
{
    record(9);
    video_calls = video_calls + 1;
    if (set_state_on_video_call != 0 && video_calls == set_state_on_video_call) {
        DAT_0011ce44 = 4;
    }
}
void race_end(void) { record(10); }

static void reset(void)
{
    int index;

    for (index = 0; index < 24; ++index) {
        order[index] = 0;
    }
    order_count = 0;
    video_calls = 0;
    set_state_on_video_call = 0;
    DAT_001d2c0 = 1;
    DAT_001d2cc = 1;
    DAT_0011ce44 = 4;
}

int main(void)
{
    int result;

    reset();
    start_result = 0x1234;
    result = race_loop();
    put(result == 0x1234 && order_count == 1 && STEP(0, 1) ? '0' : 'x');

    reset();
    start_result = 0;
    result = race_loop();
    put(result == 0 && order_count == 10
        && STEP(0, 1) && STEP(1, 2) && STEP(2, 3) && STEP(3, 4)
        && STEP(4, 5) && STEP(5, 6) && STEP(6, 7) && STEP(7, 8)
        && STEP(8, 9) && STEP(9, 10) ? '1' : 'x');

    reset();
    start_result = 0;
    DAT_001d2c0 = 0;
    result = race_loop();
    put(result == 0 && order_count == 9
        && STEP(0, 1) && STEP(1, 2) && STEP(2, 3) && STEP(3, 4)
        && STEP(4, 6) && STEP(5, 7) && STEP(6, 8) && STEP(7, 9)
        && STEP(8, 10) ? '2' : 'x');

    reset();
    start_result = 0;
    DAT_0011ce44 = 0;
    set_state_on_video_call = 2;
    result = race_loop();
    put(result == 0 && video_calls == 2 && order_count == 18
        && STEP(0, 1) && STEP(1, 2) && STEP(2, 3) && STEP(3, 4)
        && STEP(4, 5) && STEP(5, 6) && STEP(6, 7) && STEP(7, 8)
        && STEP(8, 9) && STEP(9, 2) && STEP(10, 3) && STEP(11, 4)
        && STEP(12, 5) && STEP(13, 6) && STEP(14, 7) && STEP(15, 8)
        && STEP(16, 9) && STEP(17, 10) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
