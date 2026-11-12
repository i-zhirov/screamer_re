/* The test harness for the rebuilt `opponent_effect_update` module.
 *
 * The harness holds the player object buffers and one stub for each of the two
 * callees.  Each stub appends a letter to the trace and checks the forwarded
 * object pointer.  The state field at `+0x148` decides the gate.
 *
 * The four paths:
 *
 *     0  state 0, the first object: both calls in order
 *     1  state 1: only the body point place call
 *     2  state -1: only the body point place call
 *     3  state 0, the second object: both calls with the other pointer
 *     D  the exit marker
 *
 * The harness builds the expected trace one letter at a time, because the
 * `print` helper and the data reads of this harness do not reach a string
 * literal.  The markers go through the data buffer `text`.
 */
#pragma off (check_stack)

void opponent_effect_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define OBJECT_SIZE 0x14c

static char text[8];
static char trace[32];
static char expect[32];
static int trace_length;
static int expect_length;
static int bad_object;
static unsigned int expect_object;
static unsigned char object_one[OBJECT_SIZE];
static unsigned char object_two[OBJECT_SIZE];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void add(char letter)
{
    if (trace_length < (int)sizeof(trace) - 1) {
        trace[trace_length] = letter;
        trace_length++;
    }
}

static void record(char letter, void *object)
{
    if ((unsigned int)object != expect_object) {
        bad_object = 1;
    }
    add(letter);
}

static void expect_add(char letter)
{
    if (expect_length < (int)sizeof(expect) - 1) {
        expect[expect_length] = letter;
        expect_length++;
        expect[expect_length] = 0;
    }
}

static void expect_reset(void)
{
    expect_length = 0;
    expect[0] = 0;
}

static int same(void)
{
    int index;

    index = 0;
    while (expect[index] != 0) {
        if (index >= trace_length || trace[index] != expect[index]) {
            return 0;
        }
        index++;
    }
    return index == trace_length;
}

static void set_state(unsigned char *object, int state)
{
    *(int *)(object + 0x148) = state;
}

static void reset(unsigned char *object, int state)
{
    trace_length = 0;
    trace[0] = 0;
    expect_reset();
    bad_object = 0;
    expect_object = (unsigned int)object;
    set_state(object, state);
}

void player_body_point_place(void *object)
{
    record('A', object);
}

void opponent_start(void *object)
{
    record('B', object);
}

int main(void)
{
    reset(object_one, 0);
    expect_add('A');
    expect_add('B');
    opponent_effect_update((void *)object_one);
    put(bad_object == 0 && same() ? '0' : 'x');

    reset(object_one, 1);
    expect_add('A');
    opponent_effect_update((void *)object_one);
    put(bad_object == 0 && same() ? '1' : 'x');

    reset(object_one, -1);
    expect_add('A');
    opponent_effect_update((void *)object_one);
    put(bad_object == 0 && same() ? '2' : 'x');

    reset(object_two, 0);
    expect_add('A');
    expect_add('B');
    opponent_effect_update((void *)object_two);
    put(bad_object == 0 && same() ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
