/* The test harness for the rebuilt `cng_view_update` module.
 *
 * The harness holds the two gate bytes and the eight callee stubs.  Every
 * player stub checks the fixed player pointer 0xf91c.  The sequence buffer
 * proves the call order.
 *
 * The harness runs five paths:
 *
 *     0  the input gate is clear: no call runs
 *     1  the mode gate is clear: no call runs
 *     2  the gates pass with the mode 0x20: the eight calls run in order
 *     3  the gates pass with the extra mode bit 1 set: the group still runs
 *     4  the gates pass with the extra input bits set: the group still runs
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void cng_view_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define SEQUENCE_MAX 32

unsigned char DAT_00118e1d;
unsigned char DAT_0011d260;
extern char player_object;

static char text[8];
static char sequence[SEQUENCE_MAX];
static char expected[SEQUENCE_MAX];
static int sequence_length;
static int expected_length;
static int bad_pointer;

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

static void check_player(char letter, void *player)
{
    if (player != (void *)&player_object) {
        bad_pointer = 1;
    }
    append_actual(letter);
}

void opponent_score_update(void *player) { check_player('G', player); }
void player_car_view_update(void *player) { check_player('V', player); }
void player_physics_update(void *player) { check_player('P', player); }
void player_car_effect_update(void *player) { check_player('E', player); }
void player_step_update(void *player) { check_player('S', player); }
void player_opponent_update(void *player) { check_player('O', player); }
void player_update(void) { append_actual('T'); }
void player_car_link(void *player) { check_player('L', player); }

static void append_expected(char letter)
{
    if (expected_length < SEQUENCE_MAX - 1) {
        expected[expected_length] = letter;
        expected_length = expected_length + 1;
    }
}

static char letter_at(int position)
{
    if (position == 0) {
        return 'G';
    }
    if (position == 1) {
        return 'V';
    }
    if (position == 2) {
        return 'P';
    }
    if (position == 3) {
        return 'E';
    }
    if (position == 4) {
        return 'S';
    }
    if (position == 5) {
        return 'O';
    }
    if (position == 6) {
        return 'T';
    }
    return 'L';
}

static void build_expected(void)
{
    int position;

    expected_length = 0;
    for (position = 0; position < 8; position++) {
        append_expected(letter_at(position));
    }
}

static int sequence_ok(void)
{
    int position;

    build_expected();
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
}

static int untouched_ok(void)
{
    return sequence_length == 0 && bad_pointer == 0;
}

int main(void)
{
    reset();
    DAT_00118e1d = 0xfe;
    DAT_0011d260 = 0x20;
    cng_view_update();
    put(untouched_ok() ? '0' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 0x10;
    cng_view_update();
    put(untouched_ok() ? '1' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 0x20;
    cng_view_update();
    put(sequence_ok() && bad_pointer == 0 ? '2' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 0x21;
    cng_view_update();
    put(sequence_ok() && bad_pointer == 0 ? '3' : 'x');

    reset();
    DAT_00118e1d = 0xff;
    DAT_0011d260 = 0x20;
    cng_view_update();
    put(sequence_ok() && bad_pointer == 0 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
