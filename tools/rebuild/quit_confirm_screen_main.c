/* The test harness for the rebuilt `quit_confirm_screen` module.
 *
 * The harness holds the three text pointers, the mode flags, the two input
 * gates, the highlight phase, and the six object-2 key bytes.  The screen
 * stubs record their arguments, and the `input_poll` stub runs a small key
 * script so the modal loop can reach its exit.  Each text starts with a spare
 * byte and the pointer skips it, because a string at DGROUP offset 0 holds the
 * pointer value 0 and the text stub then reads a null pointer.
 *
 * The harness runs nine paths:
 *
 *     0  the enter key is set: one loop pass returns 0, draws the title and the
 *        two choices, and the selected first choice takes the color 2
 *     1  the up key is set: the branch clears the exit flag, calls
 *        `table_lookup` once with 6, and the enter key returns 0
 *     2  the down key is held and the enter key arrives on the second poll:
 *        the first pass selects the second choice and the second pass returns 1
 *     3  the escape key is set: the function returns 1 without a lookup
 *     4  the first input gate `DAT_001d2c0` is zero: no input poll runs
 *     5  the second input gate `DAT_001d2cc` is zero: no input poll runs
 *     6  the mode flags 0x10 and 0x20 are set: both video swaps run
 *     7  the up key is held and the enter key arrives on the second poll: the
 *        latch calls `table_lookup` once
 *     8  the up key releases and presses again before the enter key: the
 *        latch clears on the release and calls `table_lookup` twice
 *     D  the exit marker
 */
#pragma off (check_stack)

int quit_confirm_screen(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char line_title[] = "XTITLE";
static char line_first[] = "XNO";
static char line_second[] = "XYES";

char *DAT_0018fe0 = line_title + 1;
char *DAT_0018fe4 = line_first + 1;
char *DAT_0018fe8 = line_second + 1;

int DAT_001d274;
int DAT_001d2c0;
int DAT_001d2cc;
unsigned int DAT_000007f9;
unsigned char DAT_0000001e;
unsigned char DAT_00000003;
unsigned char DAT_0000004a;
unsigned char DAT_0000004d;
unsigned char DAT_00000052;
unsigned char DAT_0000004f;

static int mode_calls;
static unsigned int mode_width;
static unsigned int mode_height;
static unsigned int mode_depth;
static int sound_calls;
static int sound_arg;
static int swap_a_calls;
static int swap_b_calls;
static int rect_calls;
static int rect_x;
static int rect_y;
static int rect_width;
static int rect_height;
static int fill_calls;
static int panel_count;
static char *panel_text[16];
static int panel_y[16];
static int panel_font[16];
static int panel_color[16];
static int poll_calls;
static int lookup_calls;
static int lookup_index;

static int poll_press_at;
static unsigned char *poll_press_ptr;
static unsigned char poll_press_value;
static int poll_press2_at;
static unsigned char *poll_press2_ptr;
static unsigned char poll_press2_value;
static int poll_release_at;
static unsigned char *poll_release_ptr;

void screen_mode_set(unsigned int width, unsigned int height,
    unsigned int depth)
{
    mode_calls = mode_calls + 1;
    mode_width = width;
    mode_height = height;
    mode_depth = depth;
}

void sound_effects_update(int driver)
{
    sound_calls = sound_calls + 1;
    sound_arg = driver;
}

void video_block_swap_a(void)
{
    swap_a_calls = swap_a_calls + 1;
}

void video_block_swap_b(void)
{
    swap_b_calls = swap_b_calls + 1;
}

void rect_fill(int x, int y, int width, int height)
{
    rect_calls = rect_calls + 1;
    rect_x = x;
    rect_y = y;
    rect_width = width;
    rect_height = height;
}

void video_fill(void)
{
    fill_calls = fill_calls + 1;
}

void panel_draw_centered(char *value, int y, int font, int color)
{
    if (panel_count < 16) {
        panel_text[panel_count] = value;
        panel_y[panel_count] = y;
        panel_font[panel_count] = font;
        panel_color[panel_count] = color;
    }
    panel_count = panel_count + 1;
}

int input_poll(void)
{
    poll_calls = poll_calls + 1;
    if (poll_press_at == poll_calls && poll_press_ptr != 0) {
        *poll_press_ptr = poll_press_value;
    }
    if (poll_press2_at == poll_calls && poll_press2_ptr != 0) {
        *poll_press2_ptr = poll_press2_value;
    }
    if (poll_release_at == poll_calls && poll_release_ptr != 0) {
        *poll_release_ptr = 0;
    }
    return 0;
}

void default_data_copy(void)
{
}

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_index = index;
    return 0;
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

    DAT_001d274 = 0;
    DAT_001d2c0 = 1;
    DAT_001d2cc = 1;
    DAT_000007f9 = 0x10;
    DAT_0000001e = 0;
    DAT_00000003 = 0;
    DAT_0000004a = 0;
    DAT_0000004d = 0;
    DAT_00000052 = 0;
    DAT_0000004f = 0;

    mode_calls = 0;
    mode_width = 0;
    mode_height = 0;
    mode_depth = 0;
    sound_calls = 0;
    sound_arg = 0;
    swap_a_calls = 0;
    swap_b_calls = 0;
    rect_calls = 0;
    rect_x = 0;
    rect_y = 0;
    rect_width = 0;
    rect_height = 0;
    fill_calls = 0;
    panel_count = 0;
    for (index = 0; index < 16; index++) {
        panel_text[index] = 0;
        panel_y[index] = 0;
        panel_font[index] = 0;
        panel_color[index] = 0;
    }
    poll_calls = 0;
    lookup_calls = 0;
    lookup_index = 0;
    poll_press_at = -1;
    poll_press_ptr = 0;
    poll_press_value = 0;
    poll_press2_at = -1;
    poll_press2_ptr = 0;
    poll_press2_value = 0;
    poll_release_at = -1;
    poll_release_ptr = 0;
}

static int panel_is(int index, int y, int color)
{
    return index < panel_count
        && panel_y[index] == y
        && panel_color[index] == color;
}

static int rect_is(int x, int y, int width, int height)
{
    return rect_x == x && rect_y == y
        && rect_width == width && rect_height == height;
}

int main(void)
{
    int result;

    reset();
    DAT_0000001e = 1;
    result = quit_confirm_screen();
    put(result == 0
        && mode_calls == 1 && mode_width == 0x140 && mode_height == 0x190
        && mode_depth == 0x104
        && sound_calls == 1 && sound_arg == 0x6350
        && swap_a_calls == 0 && swap_b_calls == 0
        && rect_calls == 3 && rect_is(0, 0, 0x280, 0x190)
        && fill_calls == 2 && poll_calls == 1 && lookup_calls == 0
        && panel_count == 3
        && panel_is(0, 0x34, 2) && panel_text[0] == DAT_0018fe0
        && panel_is(1, 0x5c, 2) && panel_text[1] == DAT_0018fe4
        && panel_is(2, 0x7a, 0xe) && panel_text[2] == DAT_0018fe8 ? '0' : 'x');

    reset();
    DAT_0000004a = 1;
    DAT_0000001e = 1;
    result = quit_confirm_screen();
    put(result == 0 && lookup_calls == 1 && lookup_index == 6
        && poll_calls == 1 && fill_calls == 2 && panel_count == 3
        && panel_is(1, 0x5c, 2) && panel_is(2, 0x7a, 0xe) ? '1' : 'x');

    reset();
    DAT_00000052 = 1;
    poll_press_at = 2;
    poll_press_ptr = &DAT_0000001e;
    poll_press_value = 1;
    result = quit_confirm_screen();
    put(result == 1 && lookup_calls == 1 && lookup_index == 6
        && poll_calls == 2 && fill_calls == 3 && panel_count == 5
        && panel_is(1, 0x5c, 2) && panel_is(2, 0x7a, 0xe)
        && panel_is(3, 0x5c, 0xe) && panel_is(4, 0x7a, 2) ? '2' : 'x');

    reset();
    DAT_00000003 = 1;
    result = quit_confirm_screen();
    put(result == 1 && lookup_calls == 0 && poll_calls == 1
        && fill_calls == 2 && panel_count == 3 ? '3' : 'x');

    reset();
    DAT_001d2c0 = 0;
    DAT_0000001e = 1;
    result = quit_confirm_screen();
    put(result == 0 && poll_calls == 0 && panel_count == 3 ? '4' : 'x');

    reset();
    DAT_001d2cc = 0;
    DAT_0000001e = 1;
    result = quit_confirm_screen();
    put(result == 0 && poll_calls == 0 && panel_count == 3 ? '5' : 'x');

    reset();
    DAT_001d274 = 0x30;
    DAT_0000001e = 1;
    result = quit_confirm_screen();
    put(result == 0 && swap_a_calls == 1 && swap_b_calls == 1 ? '6' : 'x');

    reset();
    DAT_0000004a = 1;
    poll_press_at = 2;
    poll_press_ptr = &DAT_0000001e;
    poll_press_value = 1;
    result = quit_confirm_screen();
    put(result == 0 && lookup_calls == 1 && poll_calls == 2
        && fill_calls == 3 && panel_count == 5
        && panel_is(1, 0x5c, 2) && panel_is(3, 0x5c, 2) ? '7' : 'x');

    reset();
    DAT_0000004a = 1;
    poll_release_at = 2;
    poll_release_ptr = &DAT_0000004a;
    poll_press2_at = 3;
    poll_press2_ptr = &DAT_0000004a;
    poll_press2_value = 1;
    poll_press_at = 4;
    poll_press_ptr = &DAT_0000001e;
    poll_press_value = 1;
    result = quit_confirm_screen();
    put(result == 0 && lookup_calls == 2 && poll_calls == 4
        && fill_calls == 5 && panel_count == 9
        && panel_is(1, 0x5c, 2) && panel_is(3, 0x5c, 2)
        && panel_is(5, 0x5c, 2) && panel_is(7, 0x5c, 2) ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
