/* The test harness for the rebuilt `replay_view_update` module.
 *
 * The harness holds the two gate bytes, the ghost index, the ghost record
 * array, the playback block, and the eleven callee stubs.  Every player stub
 * checks the fixed player pointer 0xf91c.  The keyframe stub checks the record
 * pointer `DAT_00118e8c + index * 0x24`, and the playback stubs check the
 * block `DAT_00118ed8`.  The sequence buffer proves the call order.
 *
 * The harness runs five paths:
 *
 *     0  the input gate is clear: no call runs
 *     1  the mode gate is clear: no call runs
 *     2  the gates pass with ghost index 0: the eleven calls run in order
 *     3  the gates pass with ghost index 1: the record pointer adds 0x24
 *     4  the gates pass with ghost index 2 and other mode bits set: the
 *        record pointer adds 0x48 and the mode bit 4 still selects the group
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void replay_view_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define RECORD_SIZE 0x24
#define RECORD_COUNT 4
#define SEQUENCE_MAX 32
#define PLAYER_POINTER 0xf91c

unsigned char DAT_00118e1d;
unsigned char DAT_0011d260;
int DAT_00118ed4;
unsigned char DAT_00118e8c[RECORD_SIZE * RECORD_COUNT];
unsigned char DAT_00118ed8[0x40];

static char text[8];
static char sequence[SEQUENCE_MAX];
static int sequence_length;
static char expected[SEQUENCE_MAX];
static int expected_length;
static int bad_pointer;
static int expected_index;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void append_actual(char letter)
{
    if (sequence_length < SEQUENCE_MAX - 1) {
        sequence[sequence_length] = letter;
        sequence_length = sequence_length + 1;
    }
}

static void append_expected(char letter)
{
    if (expected_length < SEQUENCE_MAX - 1) {
        expected[expected_length] = letter;
        expected_length = expected_length + 1;
    }
}

static void check_player(char letter, void *player)
{
    if (player != (void *)PLAYER_POINTER) {
        bad_pointer = 1;
    }
    append_actual(letter);
}

void race_position_update(void) { append_actual('R'); }
void player_car_view_update(void *player) { check_player('V', player); }
void path_keyframe_append(void *player, void *record)
{
    unsigned char *pointer;

    if (player != (void *)PLAYER_POINTER) {
        bad_pointer = 1;
    }
    pointer = (unsigned char *)record;
    if (pointer != DAT_00118e8c + expected_index * RECORD_SIZE) {
        bad_pointer = 1;
        append_actual('x');
        return;
    }
    append_actual('K');
    append_actual((char)('0' + expected_index));
}
void path_playback_update(void *block, int sample)
{
    if (block != (void *)DAT_00118ed8 || sample != 0) {
        bad_pointer = 1;
    }
    append_actual('U');
}
void player_physics_update(void *player) { check_player('P', player); }
void player_effect_update(void *player) { check_player('E', player); }
void player_step_update(void *player) { check_player('S', player); }
void player_opponent_update(void *player) { check_player('O', player); }
void player_update(void) { append_actual('T'); }
void player_car_link(void *player) { check_player('L', player); }
void path_playback_apply(void *block)
{
    if (block != (void *)DAT_00118ed8) {
        bad_pointer = 1;
    }
    append_actual('A');
}

static void build_expected(int index)
{
    expected_length = 0;
    append_expected('R');
    append_expected('V');
    append_expected('K');
    append_expected((char)('0' + index));
    append_expected('U');
    append_expected('P');
    append_expected('E');
    append_expected('S');
    append_expected('O');
    append_expected('T');
    append_expected('L');
    append_expected('A');
}

static int sequence_ok(int index)
{
    int position;

    build_expected(index);
    if (sequence_length != expected_length) {
        return 0;
    }
    for (position = 0; position < expected_length; position++) {
        if (sequence[position] != expected[position]) {
            return 0;
        }
    }
    return 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < SEQUENCE_MAX; index++) {
        sequence[index] = 0;
        expected[index] = 0;
    }
    sequence_length = 0;
    expected_length = 0;
    bad_pointer = 0;
    expected_index = 0;
}

static int untouched_ok(void)
{
    return sequence_length == 0 && bad_pointer == 0;
}

int main(void)
{
    reset();
    DAT_00118e1d = 0xfe;
    DAT_0011d260 = 4;
    DAT_00118ed4 = 0;
    replay_view_update();
    put(untouched_ok() ? '0' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 0x10;
    DAT_00118ed4 = 0;
    replay_view_update();
    put(untouched_ok() ? '1' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 4;
    DAT_00118ed4 = 0;
    expected_index = 0;
    replay_view_update();
    put(sequence_ok(0) && bad_pointer == 0 ? '2' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 4;
    DAT_00118ed4 = 1;
    expected_index = 1;
    replay_view_update();
    put(sequence_ok(1) && bad_pointer == 0 ? '3' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 0x0c;
    DAT_00118ed4 = 2;
    expected_index = 2;
    replay_view_update();
    put(sequence_ok(2) && bad_pointer == 0 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
