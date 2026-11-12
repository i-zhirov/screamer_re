/* The test harness for the rebuilt `traffic_event_flush` module.
 *
 * The harness holds the dequeue stub, the pump stub, and a script of pending
 * events.  The dequeue stub records the header length and the two pointers,
 * returns one scripted message per call, and then returns zero.  The pump stub
 * counts its calls.  The script uses BSS state and the reset assigns every
 * value, because the minimal image does not load the initialized data page.
 *
 * The harness runs eight paths:
 *
 *     0  an empty queue: one dequeue and one pump
 *     1  one event: two dequeues and two pumps
 *     2  three events: four dequeues and four pumps
 *     3  five events: six dequeues and six pumps
 *     4  two events: the pump count equals the dequeue count and every call
 *        takes the 0xc length and a non-null header and payload pointer
 *     5  six events: seven dequeues and seven pumps
 *     6  two runs in a row: the first one event and the second empty, so the
 *        counters start from zero at each call
 *     7  four events: the last call keeps the 0xc length and both pointers
 *     D  the exit marker
 */
#pragma off (check_stack)

#define HEADER_SIZE 0x0c
#define SCRIPT_MAX 8
#define PAYLOAD_SIZE 0x20

struct event_header {
    int sequence;
    int owner;
    int index;
};

void traffic_event_flush(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static unsigned char payloads[SCRIPT_MAX][PAYLOAD_SIZE];
static int script_count;
static int script_at;
static int dequeue_calls;
static int pump_calls;
static int last_length;
static struct event_header *last_header;
static void *last_payload;
static int length_ok;
static int pointer_ok;
static char text[8];

int traffic_event_dequeue(
    struct event_header *header, int length, void **payload
)
{
    dequeue_calls++;
    last_length = length;
    last_header = header;
    if (length != HEADER_SIZE) {
        length_ok = 0;
    }
    if (header == 0 || payload == 0) {
        pointer_ok = 0;
        return 0;
    }
    if (script_at >= script_count) {
        return 0;
    }
    header->sequence = script_at;
    header->owner = 0x1234;
    header->index = script_at;
    *payload = payloads[script_at];
    last_payload = payloads[script_at];
    script_at++;
    return 1;
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

static void reset(void)
{
    int slot;
    int index;

    for (slot = 0; slot < SCRIPT_MAX; slot++) {
        for (index = 0; index < PAYLOAD_SIZE; index++) {
            payloads[slot][index] = (unsigned char)(0x40 + slot * 0x10 + index);
        }
    }
    script_count = 0;
    script_at = 0;
    dequeue_calls = 0;
    pump_calls = 0;
    last_length = 0;
    last_header = 0;
    last_payload = 0;
    length_ok = 1;
    pointer_ok = 1;
}

int main(void)
{
    int ok;

    reset();
    traffic_event_flush();
    ok = dequeue_calls == 1 && pump_calls == 1 && length_ok && pointer_ok;
    report(ok, '0');

    reset();
    script_count = 1;
    traffic_event_flush();
    ok = dequeue_calls == 2 && pump_calls == 2 && length_ok && pointer_ok;
    report(ok, '1');

    reset();
    script_count = 3;
    traffic_event_flush();
    ok = dequeue_calls == 4 && pump_calls == 4 && script_at == 3;
    report(ok, '2');

    reset();
    script_count = 5;
    traffic_event_flush();
    ok = dequeue_calls == 6 && pump_calls == 6 && script_at == 5;
    report(ok, '3');

    reset();
    script_count = 2;
    traffic_event_flush();
    ok = dequeue_calls == 3 && pump_calls == 3
        && dequeue_calls == pump_calls
        && length_ok && pointer_ok
        && last_length == HEADER_SIZE
        && last_header != 0;
    report(ok, '4');

    reset();
    script_count = 6;
    traffic_event_flush();
    ok = dequeue_calls == 7 && pump_calls == 7 && script_at == 6;
    report(ok, '5');

    reset();
    script_count = 1;
    traffic_event_flush();
    ok = dequeue_calls == 2 && pump_calls == 2;
    script_at = 0;
    script_count = 0;
    dequeue_calls = 0;
    pump_calls = 0;
    traffic_event_flush();
    ok = ok && dequeue_calls == 1 && pump_calls == 1;
    report(ok, '6');

    reset();
    script_count = 4;
    traffic_event_flush();
    ok = dequeue_calls == 5 && pump_calls == 5
        && last_length == HEADER_SIZE
        && last_header != 0
        && last_payload != 0;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
