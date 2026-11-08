/* The test harness for the rebuilt `race_simulation_update` module.
 *
 * The harness holds the two gate bytes, the record count, the shared loop
 * counter, the record storage, and the fourteen callee stubs.  Each player
 * stub records its letter and checks the fixed player pointer 0xf91c.  Each
 * object stub records its pass letter and the record index, so the sequence
 * proves the interleaving, the record order, and the 0x590-byte stride.
 *
 * The harness runs five paths:
 *
 *     0  the input gate is clear: no call runs and the counter keeps its value
 *     1  the mode holds neither bit 0 nor bit 1: no call runs
 *     2  the gates pass with a zero count: the eight single calls run in order
 *     3  the gates pass with one record and mode bit 1: the single calls and
 *        the six object passes run in order
 *     4  the gates pass with two records and mode bit 0: every pass runs on
 *        records 0 and 1 in order
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void race_simulation_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x590
#define MAX_RECORDS 2
#define SEQUENCE_MAX 64
#define COUNTER_SENTINEL 0x12345678

unsigned char DAT_00118e1d;
unsigned char DAT_0011d260;
int DAT_0010f8ec;
int DAT_0010634c;
unsigned char DAT_00110ab8[STRIDE * MAX_RECORDS + 16];

static char text[8];
static char sequence[SEQUENCE_MAX];
static int sequence_length;
static char expected[SEQUENCE_MAX];
static int expected_length;
static int bad_pointer;

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
    offset = (unsigned int)(pointer - DAT_00110ab8);
    index = (int)(offset / STRIDE);
    if (pointer < DAT_00110ab8
        || pointer >= DAT_00110ab8 + STRIDE * DAT_0010f8ec
        || offset != (unsigned int)index * STRIDE
        || index < 0
        || index >= DAT_0010f8ec) {
        bad_pointer = 1;
        append_actual('x');
        return;
    }
    append_actual(letter);
    append_actual((char)('0' + index));
}

void func_20428(void) { append_actual('F'); }
void player_car_view_update(void *player) { record_player('V', player); }
void player_physics_update(void *player) { record_player('P', player); }
void player_effect_update(void *player) { record_player('E', player); }
void player_step_update(void *player) { record_player('S', player); }
void player_proximity_update(void *player) { record_player('X', player); }
void player_update(void) { append_actual('U'); }
void player_car_link(void *player) { record_player('L', player); }

void object_transform(void *object) { record_object('t', object); }
void object_player_input(void *object) { record_object('i', object); }
void object_physics(void *object) { record_object('h', object); }
void object_prepare(void *object) { record_object('r', object); }
void object_collision_step(void *object) { record_object('c', object); }
void object_car_link(void *object) { record_object('k', object); }

static void build_expected(int count)
{
    int index;

    expected_length = 0;
    append_expected('F');
    append_expected('V');
    for (index = 0; index < count; index++) {
        append_expected('t');
        append_expected((char)('0' + index));
    }
    append_expected('P');
    for (index = 0; index < count; index++) {
        append_expected('i');
        append_expected((char)('0' + index));
    }
    append_expected('E');
    for (index = 0; index < count; index++) {
        append_expected('h');
        append_expected((char)('0' + index));
    }
    append_expected('S');
    for (index = 0; index < count; index++) {
        append_expected('r');
        append_expected((char)('0' + index));
    }
    append_expected('X');
    for (index = 0; index < count; index++) {
        append_expected('c');
        append_expected((char)('0' + index));
    }
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
    DAT_0010634c = COUNTER_SENTINEL;
}

static int untouched_ok(void)
{
    return sequence_length == 0 && bad_pointer == 0
        && DAT_0010634c == COUNTER_SENTINEL;
}

int main(void)
{
    reset();
    DAT_00118e1d = 0xfe;
    DAT_0011d260 = 1;
    DAT_0010f8ec = 1;
    race_simulation_update();
    put(untouched_ok() ? '0' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 0x0c;
    DAT_0010f8ec = 1;
    race_simulation_update();
    put(untouched_ok() ? '1' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 2;
    DAT_0010f8ec = 0;
    race_simulation_update();
    put(sequence_ok(0) && bad_pointer == 0
        && DAT_0010634c == 0 ? '2' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 2;
    DAT_0010f8ec = 1;
    race_simulation_update();
    put(sequence_ok(1) && bad_pointer == 0
        && DAT_0010634c == 1 ? '3' : 'x');

    reset();
    DAT_00118e1d = 0x01;
    DAT_0011d260 = 1;
    DAT_0010f8ec = MAX_RECORDS;
    race_simulation_update();
    put(sequence_ok(MAX_RECORDS) && bad_pointer == 0
        && DAT_0010634c == MAX_RECORDS ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
