/* The test harness for the rebuilt `traffic_simulation_update` module.
 *
 * The harness holds the two gate bytes, the traffic count, the shared loop
 * counter, the traffic record storage, and the eleven callee stubs.  Each
 * player stub records its letter and checks the fixed player pointer 0xf91c.
 * The traffic step stub checks the retry limit 3.  Each record stub records its
 * pass letter and the record index, so the sequence proves the two loops, the
 * record order, and the 0x2a8-byte stride.
 *
 * The harness runs five paths:
 *
 *     0  the input gate is clear: no call runs and the counter keeps its value
 *     1  the mode holds no bit 0x10: no call runs
 *     2  the gates pass with a zero count: the nine single calls run in order
 *     3  the gates pass with one record: the single calls and both record
 *        passes run in order
 *     4  the gates pass with two records: both loops run on records 0 and 1 in
 *        order
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void traffic_simulation_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x2a8
#define MAX_RECORDS 2
#define SEQUENCE_MAX 64
#define COUNTER_SENTINEL 0x12345678

unsigned char DAT_00118e1d;
unsigned char DAT_0011d260;
int DAT_00115218;
int DAT_0000634c;
unsigned char DAT_00113cd8[STRIDE * MAX_RECORDS + 16];

static char text[8];
static char sequence[SEQUENCE_MAX];
static int sequence_length;
static char expected[SEQUENCE_MAX];
static int expected_length;
static int bad_pointer;
static int bad_limit;

static void put(char a)
{
    text[0] = a;
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

static void record_player(char letter, void *player)
{
    if (player != (void *)0xf91c) {
        bad_pointer = 1;
    }
    append_actual(letter);
}

static void record_object(char letter, void *object)
{
    unsigned char *pointer;
    unsigned int offset;
    int index;

    pointer = (unsigned char *)object;
    offset = (unsigned int)(pointer - DAT_00113cd8);
    index = (int)(offset / STRIDE);
    if (pointer < DAT_00113cd8
        || pointer >= DAT_00113cd8 + STRIDE * DAT_00115218
        || offset != (unsigned int)index * STRIDE
        || index < 0
        || index >= DAT_00115218) {
        bad_pointer = 1;
        append_actual('x');
        return;
    }
    append_actual(letter);
    append_actual((char)('0' + index));
}

void player_car_view_update(void *player) { record_player('V', player); }
void player_physics_update(void *player) { record_player('P', player); }
void player_effect_update(void *player) { record_player('E', player); }
void player_step_update(void *player) { record_player('S', player); }

void traffic_step(int limit)
{
    if (limit != 3) {
        bad_limit = 1;
    }
    append_actual('T');
}

void traffic_record_update(void *record) { record_object('r', record); }
void race_position_update(void) { append_actual('F'); }
void traffic_effect_update(void *player) { record_player('X', player); }
void player_update(void) { append_actual('U'); }
void player_car_link(void *player) { record_player('L', player); }
void traffic_object_link(void *record) { record_object('k', record); }

static void build_expected(int count)
{
    int index;

    expected_length = 0;
    append_expected('V');
    append_expected('P');
    append_expected('E');
    append_expected('S');
    append_expected('T');
    for (index = 0; index < count; index++) {
        append_expected('r');
        append_expected((char)('0' + index));
    }
    append_expected('F');
    append_expected('X');
    append_expected('U');
    append_expected('L');
    for (index = 0; index < count; index++) {
        append_expected('k');
        append_expected((char)('0' + index));
    }
}

static int sequence_ok(int count)
{
    int index;

    build_expected(count);
    if (sequence_length != expected_length) {
        return 0;
    }
    for (index = 0; index < expected_length; index++) {
        if (sequence[index] != expected[index]) {
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
    bad_limit = 0;
    DAT_0000634c = COUNTER_SENTINEL;
}

static int untouched_ok(void)
{
    return sequence_length == 0 && bad_pointer == 0 && bad_limit == 0
        && DAT_0000634c == COUNTER_SENTINEL;
}

int main(void)
{
    reset();
    DAT_00118e1d = 0xfe;
    DAT_0011d260 = 0x10;
    DAT_00115218 = 1;
    traffic_simulation_update();
    put(untouched_ok() ? '0' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 0x0c;
    DAT_00115218 = 1;
    traffic_simulation_update();
    put(untouched_ok() ? '1' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 0x10;
    DAT_00115218 = 0;
    traffic_simulation_update();
    put(sequence_ok(0) && bad_pointer == 0 && bad_limit == 0
        && DAT_0000634c == 0 ? '2' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 0x10;
    DAT_00115218 = 1;
    traffic_simulation_update();
    put(sequence_ok(1) && bad_pointer == 0 && bad_limit == 0
        && DAT_0000634c == 1 ? '3' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 0x10;
    DAT_00115218 = MAX_RECORDS;
    traffic_simulation_update();
    put(sequence_ok(MAX_RECORDS) && bad_pointer == 0 && bad_limit == 0
        && DAT_0000634c == MAX_RECORDS ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
