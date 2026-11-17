/* The test harness for the rebuilt `race_frame_update` module.
 *
 * The harness holds the two input bytes, the exit state, the replay byte, and
 * the video flags.  The nineteen callee stubs append one marker to a trace, so
 * the harness can check the call order and the call count.  The three stubs
 * with arguments record their argument.  The harness runs nine paths:
 *
 *     0  the input bit 0x20 sets the exit state 1 and skips the frame
 *     1  the input bit 4 sets the exit state -1 and skips the frame
 *     2  the input bit 2 calls the wait helper, clears DAT_00118e6c, and runs
 *        the full frame with the clear video flags
 *     3  a non-zero exit state skips the frame and the wait helper
 *     4  the full frame with the video flag 0x10 runs the swap A
 *     5  the full frame with the video flag 0x20 runs the swap B
 *     6  the full frame with the video flags 0x30 runs both swaps
 *     7  the two input bits set the exit state 1 and then -1, and the frame
 *        skips
 *     8  the input bit 2 with a non-zero exit state calls the wait helper,
 *        clears DAT_00118e6c, and skips the frame
 *     9  the full frame with no input bit and the clear video flags
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void race_frame_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_00118e1c;
unsigned char DAT_00118e1d;
int DAT_00118e18;
int DAT_00118e6c;
unsigned char DAT_0011d274;

static char text[8];
static char trace[32];
static int trace_count;
static int opponent_argument;
static void *camera_argument;
static int sound_argument;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void log(char marker)
{
    if (trace_count < 32) {
        trace[trace_count] = marker;
        trace_count = trace_count + 1;
    }
}

static int trace_is(int wait, int swap_a, int swap_b)
{
    char expect[24];
    int index;
    int length;

    length = 0;
    if (wait != 0) {
        expect[length] = 'W';
        length = length + 1;
    }
    expect[length + 0] = 'T';
    expect[length + 1] = 'A';
    expect[length + 2] = 'B';
    expect[length + 3] = 'C';
    expect[length + 4] = 'R';
    expect[length + 5] = 'F';
    expect[length + 6] = 'I';
    expect[length + 7] = 'O';
    expect[length + 8] = 'M';
    expect[length + 9] = 'S';
    length = length + 10;
    if (swap_a != 0) {
        expect[length] = 'X';
        length = length + 1;
    }
    if (swap_b != 0) {
        expect[length] = 'Y';
        length = length + 1;
    }
    expect[length + 0] = 'H';
    expect[length + 1] = 'N';
    expect[length + 2] = 'Q';
    expect[length + 3] = 'L';
    expect[length + 4] = 'P';
    expect[length + 5] = 'Z';
    length = length + 6;

    if (trace_count != length) {
        return 0;
    }
    for (index = 0; index < length; index++) {
        if (trace[index] != expect[index]) {
            return 0;
        }
    }
    return 1;
}

void func_1f8dc(void) { log('W'); }
void timing_update(void) { log('T'); }
void func_202c0(void) { log('A'); }
void func_20360(void) { log('B'); }
void func_203c4(void) { log('C'); }
void race_simulation_update(void) { log('R'); }
void traffic_simulation_update(void) { log('F'); }
void ai_update(void) { log('I'); }
void camera_update(void *object)
{
    camera_argument = object;
    log('M');
}
void sound_effects_update(int driver)
{
    sound_argument = driver;
    log('S');
}
void opponent_update(int value)
{
    opponent_argument = value;
    log('O');
}
void video_block_swap_a(void) { log('X'); }
void video_block_swap_b(void) { log('Y'); }
void race_hud_draw(void) { log('H'); }
void race_intro_update(void) { log('N'); }
void func_24880(void) { log('Q'); }
void race_finish_laps_update(void) { log('L'); }
void func_20f98(void) { log('P'); }
void race_finish_update(void) { log('Z'); }

static void reset(void)
{
    int index;

    DAT_00118e1c = 0;
    DAT_00118e1d = 0;
    DAT_00118e18 = 0;
    DAT_00118e6c = 0x55;
    DAT_0011d274 = 0;

    trace_count = 0;
    opponent_argument = -1;
    camera_argument = 0;
    sound_argument = -1;
    for (index = 0; index < 32; index++) {
        trace[index] = 0;
    }
}

int main(void)
{
    reset();
    DAT_00118e1c = 0x20;
    race_frame_update();
    put(DAT_00118e18 == 1 && trace_count == 0 ? '0' : 'x');

    reset();
    DAT_00118e1d = 4;
    race_frame_update();
    put(DAT_00118e18 == -1 && trace_count == 0 ? '1' : 'x');

    reset();
    DAT_00118e1d = 2;
    race_frame_update();
    put(DAT_00118e6c == 0 && trace_is(1, 0, 0)
        && opponent_argument == 0 && camera_argument == (void *)0x109a0
        && sound_argument == 0x6350 ? '2' : 'x');

    reset();
    DAT_00118e18 = 5;
    race_frame_update();
    put(DAT_00118e18 == 5 && trace_count == 0 ? '3' : 'x');

    reset();
    DAT_0011d274 = 0x10;
    race_frame_update();
    put(trace_is(0, 1, 0) ? '4' : 'x');

    reset();
    DAT_0011d274 = 0x20;
    race_frame_update();
    put(trace_is(0, 0, 1) ? '5' : 'x');

    reset();
    DAT_0011d274 = 0x30;
    race_frame_update();
    put(trace_is(0, 1, 1) ? '6' : 'x');

    reset();
    DAT_00118e1c = 0x20;
    DAT_00118e1d = 4;
    race_frame_update();
    put(DAT_00118e18 == -1 && trace_count == 0 ? '7' : 'x');

    reset();
    DAT_00118e18 = 7;
    DAT_00118e1d = 2;
    race_frame_update();
    put(DAT_00118e18 == 7 && DAT_00118e6c == 0 && trace_count == 1
        && trace[0] == 'W' ? '8' : 'x');

    reset();
    race_frame_update();
    put(trace_is(0, 0, 0) ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
