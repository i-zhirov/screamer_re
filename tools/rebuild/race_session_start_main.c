/* The test harness for the rebuilt `race_session_start` module.
 *
 * The harness holds the state globals, the two lookup tables, and the twenty-four
 * stubs.  A stub records its call and its arguments.  The harness runs the two
 * error paths, every mode handler, the default path, the handler error path, and
 * the input and table branches:
 *
 *     0  a non-zero race data load returns at once and sets the flag
 *     1  a non-zero track load returns after the seven constant stores
 *     2  the mode 1 runs the first handler and the whole default path
 *     3  the mode 2 runs the same handler, and the final words clear
 *     4  the mode 4 runs the second handler
 *     5  the mode 0x10 runs the third handler and the four mode steps
 *     6  the mode 0x20 runs the fourth handler
 *     7  the mode 0x40 runs the fifth handler
 *     8  the mode 0x100 runs the sixth handler and skips the panel block
 *     9  the mode 3 runs no handler and takes the default path
 *     A  a non-zero handler result returns at once and skips the default path
 *     B  the input branch applies the table value and selects the music
 *     C  the input branch sets the table word when the flag is clear
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int race_session_start(int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int _DAT_00118e1c;
int _DAT_00118e18;
int DAT_0011cb24;
int DAT_00113cc8;
int _DAT_00118e38;
int DAT_00119060;
int DAT_0011521c;
int _DAT_0011cb14;
int DAT_0011d260;
char DAT_0011d294[4];
int DAT_0011d2a0;
int DAT_0011d2a8;
int DAT_0011d2cc;
int DAT_000203bc;
int DAT_0000cf40;
int DAT_0000cf54;
char DAT_0000cf5c[8];
int DAT_0000ff88;
int DAT_0000ffa4;
int DAT_00001df0;
int DAT_00001d7c[8];
int DAT_00001db4[16];
int DAT_00001f00[8];
int DAT_000100e8;
int DAT_000007f9;
int DAT_00110a9c;
void *DAT_00021528;
int _DAT_00118e04;
int _DAT_00118e10;
int _DAT_00118e24;
int _DAT_00118e28;

static char text[8];
static char fill_buffer[16];

static int data_load_calls;
static int data_load_arg;
static int track_load_calls;
static int track_load_arg;
static int objects_reset_calls;
static int handler_calls[6];
static int handler_arg[6];
static int view_calls;
static int view_x;
static int view_y;
static int view_z;
static int flag_calls;
static int flag_arg;
static int format_calls;
static int show_calls;
static int memset_calls;
static void *memset_target;
static unsigned int memset_count;
static int memset_value;
static int video_calls;
static int panel_calls;
static int lookup_calls;
static int lookup_index;
static int refresh_calls;
static int apply_calls;
static unsigned int apply_state;
static void *apply_device;
static int select_calls;
static char *select_record;
static int select_value;
static int link_calls;
static char *link_first;
static char *link_second;
static int player_calls;
static int step_a_calls;
static int step_a_arg;
static int step_b_calls;

static int load_result;
static int track_result;
static int handler_result[6];
static int lookup_result;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int race_data_load(int value)
{
    data_load_calls = data_load_calls + 1;
    data_load_arg = value;
    return load_result;
}

void race_objects_reset(void)
{
    objects_reset_calls = objects_reset_calls + 1;
}

int race_track_load(int value)
{
    track_load_calls = track_load_calls + 1;
    track_load_arg = value;
    return track_result;
}

static int handler(int index, int value)
{
    handler_calls[index] = handler_calls[index] + 1;
    handler_arg[index] = value;
    return handler_result[index];
}

int game_mode_1_start(int value) { return handler(0, value); }
int game_mode_4_start(int value) { return handler(1, value); }
int game_mode_10_start(int value) { return handler(2, value); }
int game_mode_20_start(int value) { return handler(3, value); }
int game_mode_40_start(int value) { return handler(4, value); }
int game_mode_100_start(int value) { return handler(5, value); }

void view_move(int x, int y, int z)
{
    view_calls = view_calls + 1;
    view_x = x;
    view_y = y;
    view_z = z;
}

void mode_flag_set(int value)
{
    flag_calls = flag_calls + 1;
    flag_arg = value;
}

void message_format(char *format, char *buffer)
{
    (void)format;
    (void)buffer;
    format_calls = format_calls + 1;
}

void message_show(char *buffer)
{
    (void)buffer;
    show_calls = show_calls + 1;
}

void *memset(void *target, unsigned int count, int value)
{
    memset_calls = memset_calls + 1;
    memset_target = target;
    memset_count = count;
    memset_value = value;
    return target;
}

void video_fill(void)
{
    video_calls = video_calls + 1;
}

void panel_reset(void)
{
    panel_calls = panel_calls + 1;
}

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_index = index;
    return lookup_result;
}

void input_refresh(void)
{
    refresh_calls = refresh_calls + 1;
}

int input_apply(unsigned int state, void *device)
{
    apply_calls = apply_calls + 1;
    apply_state = state;
    apply_device = device;
    return 0;
}

void music_select(char *record, int value)
{
    select_calls = select_calls + 1;
    select_record = record;
    select_value = value;
}

void object_list_link(char *first, char *second)
{
    link_calls = link_calls + 1;
    link_first = first;
    link_second = second;
}

void player_update(void)
{
    player_calls = player_calls + 1;
}

void mode10_step_a(int value)
{
    step_a_calls = step_a_calls + 1;
    step_a_arg = value;
}

void mode10_step_b(void)
{
    step_b_calls = step_b_calls + 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 8; ++index) {
        DAT_00001d7c[index] = 0;
        DAT_00001f00[index] = 0;
    }
    for (index = 0; index < 16; ++index) {
        DAT_00001db4[index] = 0;
    }
    for (index = 0; index < 6; ++index) {
        handler_calls[index] = 0;
        handler_arg[index] = 0;
        handler_result[index] = 0;
    }
    data_load_calls = 0;
    data_load_arg = 0;
    track_load_calls = 0;
    track_load_arg = 0;
    objects_reset_calls = 0;
    view_calls = 0;
    view_x = -1;
    view_y = -1;
    view_z = -1;
    flag_calls = 0;
    flag_arg = -1;
    format_calls = 0;
    show_calls = 0;
    memset_calls = 0;
    memset_target = 0;
    memset_count = 0;
    memset_value = -1;
    video_calls = 0;
    panel_calls = 0;
    lookup_calls = 0;
    lookup_index = -1;
    refresh_calls = 0;
    apply_calls = 0;
    apply_state = 0;
    apply_device = 0;
    select_calls = 0;
    select_record = 0;
    select_value = -1;
    link_calls = 0;
    link_first = 0;
    link_second = 0;
    player_calls = 0;
    step_a_calls = 0;
    step_a_arg = -1;
    step_b_calls = 0;
    load_result = 0;
    track_result = 0;
    lookup_result = 0x55;
    DAT_0011d260 = 0;
    DAT_0011d2a0 = 0;
    DAT_0011d2a8 = 0;
    DAT_0011d2cc = 0;
    DAT_000203bc = 0;
    DAT_0000cf40 = 0;
    DAT_0000cf54 = 0;
    DAT_0000ff88 = 0;
    DAT_0000ffa4 = 0;
    DAT_00001df0 = 0;
    DAT_000100e8 = 0;
    DAT_000007f9 = -1;
    DAT_00110a9c = 7;
    DAT_00021528 = fill_buffer;
    _DAT_00118e1c = 0;
    _DAT_00118e18 = -1;
    DAT_0011cb24 = -1;
    DAT_00113cc8 = -1;
    _DAT_00118e38 = -1;
    DAT_00119060 = -1;
    DAT_0011521c = 0;
    _DAT_0011cb14 = 0;
    _DAT_00118e04 = -1;
    _DAT_00118e10 = -1;
    _DAT_00118e24 = -1;
    _DAT_00118e28 = -1;
}

static int no_handlers(void)
{
    int index;

    for (index = 0; index < 6; ++index) {
        if (handler_calls[index] != 0) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int result;

    reset();
    load_result = 0x1234;
    result = race_session_start(0x21);
    put(result == 0x1234 && data_load_calls == 1 && data_load_arg == 0x21
        && _DAT_00118e1c == 0x102 && objects_reset_calls == 0
        && track_load_calls == 0 && no_handlers() && view_calls == 0
        ? '0' : 'x');

    reset();
    track_result = 0x5678;
    result = race_session_start(0x33);
    put(result == 0x5678 && data_load_calls == 1 && data_load_arg == 0x33
        && objects_reset_calls == 1 && track_load_calls == 1
        && track_load_arg == 0x33 && _DAT_00118e18 == 0
        && DAT_0011cb24 == 0 && DAT_00113cc8 == 0 && _DAT_00118e38 == 0
        && DAT_00119060 == 0 && DAT_0011521c == 0x1234
        && _DAT_0011cb14 == 1 && no_handlers() && view_calls == 0
        ? '1' : 'x');

    reset();
    DAT_0011d260 = 1;
    DAT_000203bc = 3;
    DAT_0000cf40 = 0x2468;
    DAT_00001d7c[0] = 0x123;
    lookup_result = 0x77;
    result = race_session_start(0x11);
    put(result == 0 && handler_calls[0] == 1 && handler_arg[0] == 0x11
        && view_calls == 1 && view_x == 0 && view_y == 0 && view_z == 0
        && flag_calls == 1 && flag_arg == 0x10
        && format_calls == 1 && show_calls == 1
        && memset_calls == 1 && memset_target == fill_buffer
        && memset_count == 0x3e800 && memset_value == 0
        && video_calls == 1 && panel_calls == 1 && lookup_calls == 1
        && lookup_index == 0x123 && DAT_000100e8 == 0x77
        && DAT_0000ffa4 == 0x2468 && refresh_calls == 1
        && apply_calls == 0 && select_calls == 0
        && link_calls == 1 && link_first == (char *)0x6530
        && link_second == (char *)0x6350 && player_calls == 1
        && DAT_00110a9c == 7 && step_a_calls == 0 ? '2' : 'x');

    reset();
    DAT_0011d260 = 2;
    result = race_session_start(0);
    put(result == 0 && handler_calls[0] == 1 && format_calls == 0
        && show_calls == 0 && _DAT_00118e04 == 0 && _DAT_00118e10 == 0
        && _DAT_00118e24 == 0 && DAT_000007f9 == 0
        && _DAT_00118e28 == 0 ? '3' : 'x');

    reset();
    DAT_0011d260 = 4;
    result = race_session_start(0);
    put(result == 0 && handler_calls[1] == 1 && handler_calls[0] == 0
        && view_calls == 1 ? '4' : 'x');

    reset();
    DAT_0011d260 = 0x10;
    result = race_session_start(0);
    put(result == 0 && handler_calls[2] == 1 && step_a_calls == 2
        && step_a_arg == 0x50 && step_b_calls == 2 ? '5' : 'x');

    reset();
    DAT_0011d260 = 0x20;
    result = race_session_start(0);
    put(result == 0 && handler_calls[3] == 1 && step_a_calls == 0
        ? '6' : 'x');

    reset();
    DAT_0011d260 = 0x40;
    result = race_session_start(0);
    put(result == 0 && handler_calls[4] == 1 ? '7' : 'x');

    reset();
    DAT_0011d260 = 0x100;
    result = race_session_start(0);
    put(result == 0 && handler_calls[5] == 1 && panel_calls == 0
        && lookup_calls == 0 && refresh_calls == 1 ? '8' : 'x');

    reset();
    DAT_0011d260 = 3;
    result = race_session_start(0);
    put(result == 0 && no_handlers() && view_calls == 1
        && panel_calls == 1 && format_calls == 0 ? '9' : 'x');

    reset();
    DAT_0011d260 = 1;
    handler_result[0] = 0x999;
    result = race_session_start(0);
    put(result == 0x999 && handler_calls[0] == 1 && view_calls == 0
        && refresh_calls == 0 && player_calls == 0 ? 'A' : 'x');

    reset();
    DAT_0011d260 = 1;
    DAT_0011d2cc = 1;
    DAT_0011d2a0 = 0xc;
    DAT_0000cf54 = 1;
    DAT_0000ff88 = 2;
    DAT_000203bc = 3;
    DAT_00001d7c[4] = 0x777;
    DAT_00001f00[5] = 0x40;
    result = race_session_start(0);
    put(result == 0 && lookup_index == 0x777 && DAT_00001df0 == 1
        && DAT_00001db4[0] == 0 && apply_calls == 1
        && apply_state == 0x41 && apply_device == (void *)1
        && select_calls == 1 && select_record == (char *)0x62b0
        && select_value == 1 ? 'B' : 'x');

    reset();
    DAT_0011d260 = 1;
    DAT_0011d2cc = 1;
    DAT_0011d2a8 = 2;
    DAT_00001f00[2] = 0x20;
    result = race_session_start(0);
    put(result == 0 && DAT_00001df0 == 0 && DAT_00001db4[10] == 1
        && apply_state == 0x21 ? 'C' : 'x');

    put('D');
    quit();
    return 0;
}
