/* The test harness for the rebuilt `traffic_event_read` module.
 *
 * The harness holds four 0x2a8-byte traffic records at 0x13cd8, the race
 * signature at 0x1521c, the local player index at 0x1d250, and three callee
 * stubs.  The dequeue stub reads a scripted header and returns a payload
 * pointer, the copy stub moves the payload bytes, and the pump stub counts its
 * calls.  The script and the payloads use BSS state and the setup assigns every
 * value, because the minimal image does not load the initialized data page.
 *
 * The harness runs eight paths:
 *
 *     0  an empty queue: no copy and no pump
 *     1  a matching event with index 1: the payload lands in record 1, the
 *        state takes 1, the finished field takes 1, and the call copies 0x4c
 *        bytes from the payload to record offset +0x14
 *     2  a foreign owner: the call pumps the queue and copies nothing
 *     3  the local player index: the call pumps the queue and copies nothing
 *     4  a foreign event and then a matching event index 2: one pump and one
 *        copy
 *     5  a matching event index 0: only record 0 changes
 *     6  a matching event index 3: the last record changes
 *     7  two matching events index 0 and index 1: two copies
 *     D  the exit marker
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x2a8
#define RECORD_COUNT 4
#define STATE_OFFSET 0x00
#define STATE_COPY 0x4c
#define STATE_DEST 0x14
#define FINISHED_OFFSET 0x60
#define HEADER_SIZE 0x0c
#define SCRIPT_MAX 4
#define PAYLOAD_SIZE 0x60

struct event_header {
    int sequence;
    int owner;
    int index;
};

void traffic_event_read(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011521c;
int DAT_0011d250;
char DAT_00113cd8[RECORD_SIZE * RECORD_COUNT];

static int script_owner[SCRIPT_MAX];
static int script_index[SCRIPT_MAX];
static int script_count;
static int script_at;
static unsigned char payloads[SCRIPT_MAX][PAYLOAD_SIZE];
static int dequeue_calls;
static int copy_calls;
static int pump_calls;
static void *copy_source;
static void *copy_destination;
static int copy_count;
static char text[8];

int traffic_event_dequeue(
    struct event_header *header, int length, void **payload
)
{
    (void)length;
    dequeue_calls++;
    if (script_at >= script_count) {
        return 0;
    }
    header->sequence = script_at;
    header->owner = script_owner[script_at];
    header->index = script_index[script_at];
    *payload = payloads[script_at];
    script_at++;
    return 1;
}

void traffic_event_copy(void *source, void *destination, int count)
{
    unsigned char *from = (unsigned char *)source;
    unsigned char *to = (unsigned char *)destination;
    int index;

    for (index = 0; index < count; index++) {
        to[index] = from[index];
    }
    copy_calls++;
    copy_source = source;
    copy_destination = destination;
    copy_count = count;
}

void traffic_event_pump(void)
{
    pump_calls++;
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

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

static unsigned char *record_at(int index)
{
    return (unsigned char *)(DAT_00113cd8 + index * RECORD_SIZE);
}

static void fill_payload(int slot)
{
    int index;

    for (index = 0; index < PAYLOAD_SIZE; index++) {
        payloads[slot][index] = (unsigned char)(0x30 + slot * 0x20 + index);
    }
}

static void set_event(int slot, int owner, int index)
{
    script_owner[slot] = owner;
    script_index[slot] = index;
    fill_payload(slot);
}

static void reset(void)
{
    int index;

    for (index = 0; index < RECORD_SIZE * RECORD_COUNT; index++) {
        DAT_00113cd8[index] = 0;
    }
    for (index = 0; index < SCRIPT_MAX; index++) {
        script_owner[index] = 0;
        script_index[index] = 0;
        fill_payload(index);
    }
    DAT_0011521c = 0x1234;
    DAT_0011d250 = 5;
    script_count = 0;
    script_at = 0;
    dequeue_calls = 0;
    copy_calls = 0;
    pump_calls = 0;
    copy_source = 0;
    copy_destination = 0;
    copy_count = 0;
}

static int record_ok(int index, int state, unsigned char *expected)
{
    unsigned char *record = record_at(index);
    int offset;

    if (*(int *)(record + STATE_OFFSET) != state) {
        return 0;
    }
    if (*(int *)(record + FINISHED_OFFSET) != 1) {
        return 0;
    }
    for (offset = 0; offset < STATE_COPY; offset++) {
        if (record[STATE_DEST + offset] != expected[offset]) {
            return 0;
        }
    }
    return 1;
}

static int record_blank(int index)
{
    unsigned char *record = record_at(index);
    int offset;

    if (*(int *)(record + STATE_OFFSET) != 0) {
        return 0;
    }
    if (*(int *)(record + FINISHED_OFFSET) != 0) {
        return 0;
    }
    for (offset = 0; offset < STATE_COPY; offset++) {
        if (record[STATE_DEST + offset] != 0) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int ok;

    reset();
    traffic_event_read();
    ok = dequeue_calls == 1 && copy_calls == 0 && pump_calls == 0
        && record_blank(0) && record_blank(1);
    report(ok, '0');

    reset();
    set_event(0, 0x1234, 1);
    script_count = 1;
    traffic_event_read();
    ok = dequeue_calls == 2 && copy_calls == 1 && pump_calls == 0
        && copy_source == (void *)payloads[0]
        && copy_destination == (void *)(record_at(1) + STATE_DEST)
        && copy_count == STATE_COPY
        && record_ok(1, 1, payloads[0])
        && record_blank(0) && record_blank(2);
    report(ok, '1');

    reset();
    set_event(0, 0x1235, 1);
    script_count = 1;
    traffic_event_read();
    ok = dequeue_calls == 2 && copy_calls == 0 && pump_calls == 1
        && record_blank(1);
    report(ok, '2');

    reset();
    DAT_0011d250 = 2;
    set_event(0, 0x1234, 2);
    script_count = 1;
    traffic_event_read();
    ok = dequeue_calls == 2 && copy_calls == 0 && pump_calls == 1
        && record_blank(2);
    report(ok, '3');

    reset();
    set_event(0, 0x0001, 0);
    set_event(1, 0x1234, 2);
    script_count = 2;
    traffic_event_read();
    ok = dequeue_calls == 3 && copy_calls == 1 && pump_calls == 1
        && record_ok(2, 2, payloads[1])
        && record_blank(0);
    report(ok, '4');

    reset();
    set_event(0, 0x1234, 0);
    script_count = 1;
    traffic_event_read();
    ok = dequeue_calls == 2 && copy_calls == 1 && pump_calls == 0
        && record_ok(0, 0, payloads[0])
        && record_blank(1);
    report(ok, '5');

    reset();
    set_event(0, 0x1234, 3);
    script_count = 1;
    traffic_event_read();
    ok = dequeue_calls == 2 && copy_calls == 1 && pump_calls == 0
        && record_ok(3, 3, payloads[0])
        && record_blank(2);
    report(ok, '6');

    reset();
    set_event(0, 0x1234, 0);
    set_event(1, 0x1234, 1);
    script_count = 2;
    traffic_event_read();
    ok = dequeue_calls == 3 && copy_calls == 2 && pump_calls == 0
        && record_ok(0, 0, payloads[0])
        && record_ok(1, 1, payloads[1]);
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
