/* The test harness for the rebuilt `player_physics_update` module.
 *
 * The harness holds one stub for each of the five callees.  Each stub appends
 * a letter to the trace and checks the forwarded object pointer.
 *
 * The three paths:
 *
 *     0  object 0x1234: the five steps in order, return
 *     1  object 0x5678: the same order with the other object
 *     2  object 0: the same order with the null object
 *     D  the exit marker
 *
 * The harness builds the expected trace one letter at a time, because the
 * `print` helper and the data reads of this harness do not reach a string
 * literal.  The markers go through the data buffer `text`.
 */
#pragma off (check_stack)

void player_physics_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char trace[16];
static char expect[16];
static int trace_length;
static int expect_length;
static int bad_object;
static unsigned int expect_object;

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

static void expect_steps(void)
{
    expect_add('A');
    expect_add('B');
    expect_add('C');
    expect_add('D');
    expect_add('E');
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

static void reset(unsigned int object)
{
    trace_length = 0;
    trace[0] = 0;
    expect_length = 0;
    expect[0] = 0;
    bad_object = 0;
    expect_object = object;
}

void race_clock_update(void *object)
{
    record('A', object);
}

void engine_rpm_update(void *object)
{
    record('B', object);
}

void player_throttle_update(void *object)
{
    record('C', object);
}

void player_brake_update(void *object)
{
    record('D', object);
}

void player_steering_update(void *object)
{
    record('E', object);
}

int main(void)
{
    reset(0x1234);
    expect_steps();
    player_physics_update((void *)0x1234);
    put(bad_object == 0 && same() ? '0' : 'x');

    reset(0x5678);
    expect_steps();
    player_physics_update((void *)0x5678);
    put(bad_object == 0 && same() ? '1' : 'x');

    reset(0);
    expect_steps();
    player_physics_update((void *)0);
    put(bad_object == 0 && same() ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
