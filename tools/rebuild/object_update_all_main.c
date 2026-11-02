/* The test harness for the rebuilt `object_update_all` module.
 *
 * The harness holds the record count, the shared loop counter, the record
 * storage, and the twelve callee stubs.  Each object stub records the pass
 * number and the record index, so the sequence proves the five passes, the
 * record order, and the 0x590-byte stride.  Each subsystem stub records its
 * letter, and the argument stubs record the value.
 *
 * The harness runs three paths:
 *
 *     0  a zero count: no object step runs, and the seven subsystem calls run
 *        in order
 *     1  one record: the five passes run on record 0, then the seven calls
 *     2  three records: every pass runs on records 0, 1, and 2 in order
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_update_all(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x590
#define MAX_RECORDS 3

int DAT_0010f8ec;
int DAT_0010634c;
unsigned char DAT_00110ab8[STRIDE * MAX_RECORDS + 16];

static char text[8];
static char sequence[48];
static int sequence_length;
static int bad_pointer;
static int pass_calls[6];
static int opponent_arg;
static void *camera_arg;
static int sound_arg;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void record(char letter)
{
    if (sequence_length < 47) {
        sequence[sequence_length] = letter;
        sequence_length = sequence_length + 1;
        sequence[sequence_length] = 0;
    }
}

static int sequence_ok(int count)
{
    int pass;
    int index;
    int position;

    if (sequence_length != count * 10 + 7) {
        return 0;
    }
    position = 0;
    for (pass = 1; pass <= 5; pass++) {
        for (index = 0; index < count; index++) {
            if (sequence[position] != (char)('0' + pass)) {
                return 0;
            }
            position = position + 1;
            if (sequence[position] != (char)('0' + index)) {
                return 0;
            }
            position = position + 1;
        }
    }
    if (sequence[position] != 'T') {
        return 0;
    }
    position = position + 1;
    if (sequence[position] != 'P') {
        return 0;
    }
    position = position + 1;
    if (sequence[position] != 'A') {
        return 0;
    }
    position = position + 1;
    if (sequence[position] != 'O') {
        return 0;
    }
    position = position + 1;
    if (sequence[position] != 'C') {
        return 0;
    }
    position = position + 1;
    if (sequence[position] != 'S') {
        return 0;
    }
    position = position + 1;
    return sequence[position] == 'H';
}

static void record_object(int pass, void *object)
{
    unsigned char *pointer;
    unsigned int offset;
    int index;

    pointer = (unsigned char *)object;
    offset = (unsigned int)(pointer - DAT_00110ab8);
    index = (int)(offset / STRIDE);
    record((char)('0' + pass));
    if (pointer < DAT_00110ab8
        || pointer >= DAT_00110ab8 + STRIDE * DAT_0010f8ec
        || offset != (unsigned int)index * STRIDE
        || index < 0
        || index >= DAT_0010f8ec) {
        bad_pointer = 1;
        record('x');
        return;
    }
    record((char)('0' + index));
    pass_calls[pass] = pass_calls[pass] + 1;
}

void object_transform(void *object) { record_object(1, object); }
void object_player_input(void *object) { record_object(2, object); }
void object_physics(void *object) { record_object(3, object); }
void object_prepare(void *object) { record_object(4, object); }
void object_car_link(void *object) { record_object(5, object); }

void race_timeout(void) { record('T'); }
void player_update(void) { record('P'); }
void ai_update(void) { record('A'); }
void opponent_update(int value) { record('O'); opponent_arg = value; }
void camera_update(void *camera) { record('C'); camera_arg = camera; }
void sound_effects_update(int driver) { record('S'); sound_arg = driver; }
void hud_update(void) { record('H'); }

static int common_ok(void)
{
    return bad_pointer == 0
        && DAT_0010634c == DAT_0010f8ec
        && opponent_arg == 1
        && camera_arg == (void *)0x109a0
        && sound_arg == 0x6350;
}

static int passes_ok(int count)
{
    int pass;

    for (pass = 1; pass <= 5; pass++) {
        if (pass_calls[pass] != count) {
            return 0;
        }
    }
    return 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 48; index++) {
        sequence[index] = 0;
    }
    sequence_length = 0;
    bad_pointer = 0;
    for (index = 0; index < 6; index++) {
        pass_calls[index] = 0;
    }
    opponent_arg = 0;
    camera_arg = 0;
    sound_arg = 0;
    DAT_0010634c = 0x12345678;
}

int main(void)
{
    reset();
    DAT_0010f8ec = 0;
    object_update_all();
    put(common_ok() && passes_ok(0) && sequence_ok(0) ? '0' : 'x');

    reset();
    DAT_0010f8ec = 1;
    object_update_all();
    put(common_ok() && passes_ok(1)
        && sequence_ok(1) ? '1' : 'x');

    reset();
    DAT_0010f8ec = 3;
    object_update_all();
    put(common_ok() && passes_ok(3)
        && sequence_ok(3) ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
