/* The test harness for the rebuilt `race_frame` module.
 *
 * The harness holds the two buffers, the swap flags, the race state, the frame
 * value, the threshold, the two state words, and the fade value.  The twelve
 * callee stubs record their arguments and count their calls.  The harness runs
 * five paths:
 *
 *     0  the state 5 path skips the switch, skips both swaps, and keeps the
 *        frame flag clear
 *     1  the state 0 path runs `race_panel_0`, runs the swap A behind the flag
 *        0x10, and sets the frame flag because the frame value is above the
 *        threshold and the overlay state is 0
 *     2  the state 2 path runs `race_panel_2` and the swap B behind the flag
 *        0x20, and keeps the frame flag clear because the overlay state is 1
 *     3  the state 3 path runs both swaps and `race_panel_3`, and keeps the
 *        frame flag clear because the frame value is below the threshold
 *     4  the state 4 path runs `race_panel_4` and sets the frame flag
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void race_frame(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_0011fad0;
void *DAT_00121528;
unsigned char DAT_0011d274;
float DAT_0011d24c;
unsigned int DAT_0011fac4;
unsigned int DAT_0011d234;
float DAT_00120378;
unsigned int DAT_001203b8;
float frame_limit;

static char text[8];
static void *copy_destination;
static void *copy_source;
static unsigned int copy_count;
static int copy_calls;
static int swap_a_calls;
static int swap_b_calls;
static int overlay_calls;
static int heading_calls;
static int state_calls;
static void *camera_argument;
static int camera_calls;
static void *sound_argument;
static int sound_calls;
static int fade_calls;
static unsigned int fade_bits;
static int panel_calls[5];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static unsigned int bits(float value)
{
    return *(unsigned int *)&value;
}

void *memcpy(void *destination, const void *source, unsigned int count)
{
    copy_destination = destination;
    copy_source = (void *)source;
    copy_count = count;
    copy_calls = copy_calls + 1;
    return destination;
}

void video_block_swap_a(void) { swap_a_calls = swap_a_calls + 1; }
void video_block_swap_b(void) { swap_b_calls = swap_b_calls + 1; }
void overlay_dispatch(void) { overlay_calls = overlay_calls + 1; }
void race_heading_update(void) { heading_calls = heading_calls + 1; }
void race_state_update(void) { state_calls = state_calls + 1; }

void camera_update(void *object)
{
    camera_argument = object;
    camera_calls = camera_calls + 1;
}

void sound_update(void *device)
{
    sound_argument = device;
    sound_calls = sound_calls + 1;
}

void race_panel_0(void) { panel_calls[0] = panel_calls[0] + 1; }
void race_panel_2(void) { panel_calls[2] = panel_calls[2] + 1; }
void race_panel_3(void) { panel_calls[3] = panel_calls[3] + 1; }
void race_panel_4(void) { panel_calls[4] = panel_calls[4] + 1; }

void fade_update(float delta)
{
    fade_bits = bits(delta);
    fade_calls = fade_calls + 1;
}

static void reset(void)
{
    int index;

    copy_destination = 0;
    copy_source = 0;
    copy_count = 0;
    copy_calls = 0;
    swap_a_calls = 0;
    swap_b_calls = 0;
    overlay_calls = 0;
    heading_calls = 0;
    state_calls = 0;
    camera_argument = 0;
    camera_calls = 0;
    sound_argument = 0;
    sound_calls = 0;
    fade_calls = 0;
    fade_bits = 0;
    for (index = 0; index < 5; ++index) {
        panel_calls[index] = 0;
    }
    DAT_0011fad0 = (void *)0x2222;
    DAT_00121528 = (void *)0x1111;
    DAT_0011d274 = 0;
    DAT_0011d24c = 0.0f;
    frame_limit = 1.0f;
    DAT_0011fac4 = 0;
    DAT_0011d234 = 0;
    DAT_00120378 = 2.0f;
    DAT_001203b8 = 0;
}

static int common(void)
{
    return copy_calls == 1
        && copy_destination == (void *)0x1111
        && copy_source == (void *)0x2222
        && copy_count == 0x3e800
        && overlay_calls == 1
        && heading_calls == 1
        && state_calls == 1
        && camera_calls == 1
        && camera_argument == (void *)0x109a0
        && sound_calls == 1
        && sound_argument == (void *)0x6350
        && fade_calls == 1
        && fade_bits == 0x40000000u;
}

static int panel_only(int index)
{
    int other;

    for (other = 0; other < 5; ++other) {
        if (other != index && panel_calls[other] != 0) {
            return 0;
        }
    }
    return panel_calls[index] == 1;
}

static int no_panel(void)
{
    int index;

    for (index = 0; index < 5; ++index) {
        if (panel_calls[index] != 0) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    reset();
    DAT_001203b8 = 5;
    race_frame();
    put(common() && swap_a_calls == 0 && swap_b_calls == 0
        && no_panel() && DAT_0011d234 == 0 ? '0' : 'x');

    reset();
    DAT_0011d274 = 0x10;
    DAT_001203b8 = 0;
    DAT_0011d24c = 2.0f;
    race_frame();
    put(common() && swap_a_calls == 1 && swap_b_calls == 0
        && panel_only(0) && DAT_0011d234 == 1 ? '1' : 'x');

    reset();
    DAT_0011d274 = 0x20;
    DAT_001203b8 = 2;
    DAT_0011d24c = 2.0f;
    DAT_0011fac4 = 1;
    race_frame();
    put(common() && swap_a_calls == 0 && swap_b_calls == 1
        && panel_only(2) && DAT_0011d234 == 0 ? '2' : 'x');

    reset();
    DAT_0011d274 = 0x30;
    DAT_001203b8 = 3;
    DAT_0011d24c = 0.5f;
    race_frame();
    put(common() && swap_a_calls == 1 && swap_b_calls == 1
        && panel_only(3) && DAT_0011d234 == 0 ? '3' : 'x');

    reset();
    DAT_001203b8 = 4;
    DAT_0011d24c = 2.0f;
    race_frame();
    put(common() && swap_a_calls == 0 && swap_b_calls == 0
        && panel_only(4) && DAT_0011d234 == 1 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
