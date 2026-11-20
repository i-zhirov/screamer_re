/* The test harness for the rebuilt `player_car_effect_update` module.
 *
 * The harness holds the input byte `DAT_00118e1c` and one stub for each of
 * the seven callees.  Each stub appends a letter to the trace and checks the
 * forwarded object pointer.  The car sound stub also counts its calls.
 *
 * The five paths:
 *
 *     0  bit 0 set, one object: all seven calls, return
 *     1  bit 0 clear: the six steps, no car sound
 *     2  the other bits set: the six steps, no car sound
 *     3  bit 0 with other bits set: all seven calls
 *     4  bit 1 set, another object: the six steps, no car sound
 *     D  the exit marker
 *
 * The harness builds the expected trace one letter at a time, because the
 * `print` helper and the data reads of this harness do not reach a string
 * literal.  The markers go through the data buffer `text`.
 */
#pragma off (check_stack)

unsigned char DAT_00118e1c;

void player_car_effect_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char trace[128];
static char expect[128];
static int trace_length;
static int expect_length;
static int sound_calls;
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

static void expect_reset(void)
{
    expect_length = 0;
    expect[0] = 0;
}

static void expect_steps(int with_sound)
{
    expect_add('A');
    expect_add('B');
    expect_add('C');
    expect_add('D');
    expect_add('E');
    expect_add('F');
    if (with_sound) {
        expect_add('G');
    }
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

static void reset(void)
{
    trace_length = 0;
    trace[0] = 0;
    expect_reset();
    sound_calls = 0;
    bad_object = 0;
    expect_object = 0x1234;
    DAT_00118e1c = 0;
}

void player_car_suspension_update(void *object)
{
    record('A', object);
}

void player_car_skid_update(void *object)
{
    record('B', object);
}

void player_car_surface_effect_update(void *object)
{
    record('C', object);
}

void player_car_reset_update(void *object)
{
    record('D', object);
}

void player_car_spin_update(void *object)
{
    record('E', object);
}

void player_car_wheel_effect_update(void *object)
{
    record('F', object);
}

void car_sound_update(void *object)
{
    record('G', object);
    sound_calls++;
}

int main(void)
{
    reset();
    DAT_00118e1c = 1;
    expect_steps(1);
    player_car_effect_update((void *)0x1234);
    put(sound_calls == 1 && bad_object == 0 && same() ? '0' : 'x');

    reset();
    DAT_00118e1c = 0;
    expect_steps(0);
    player_car_effect_update((void *)0x1234);
    put(sound_calls == 0 && bad_object == 0 && same() ? '1' : 'x');

    reset();
    DAT_00118e1c = 0xfe;
    expect_steps(0);
    player_car_effect_update((void *)0x1234);
    put(sound_calls == 0 && bad_object == 0 && same() ? '2' : 'x');

    reset();
    DAT_00118e1c = 0x81;
    expect_steps(1);
    player_car_effect_update((void *)0x1234);
    put(sound_calls == 1 && bad_object == 0 && same() ? '3' : 'x');

    reset();
    DAT_00118e1c = 2;
    expect_object = 0x5678;
    expect_steps(0);
    player_car_effect_update((void *)0x5678);
    put(sound_calls == 0 && bad_object == 0 && same() ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
