/* The test harness for the rebuilt `player_lap_progress_update` module.
 *
 * The harness holds the player object, the node count, and the three callee
 * stubs.  The `effect_push` stub records the effect type, the subtype, the
 * value, and the call count.  The `track_timer_update` and
 * `race_lap_complete` stubs count their calls.
 *
 * The harness runs ten paths:
 *
 *     0  the status 1 without the node flag 2 marks the target node, increments
 *        the lap counter, pushes no effect, updates no timer, and forms the
 *        progress 502.25
 *     1  the status 1 with the flag 2 and the clear latch sets the latch,
 *        updates the timer, and pushes no effect
 *     2  the timer 1.0 pushes the effect 4 with the subtype 2
 *     3  the timer 3.0 pushes the effect 4 with the subtype 0
 *     4  the timer 20.0 pushes the effect 4 with the subtype 1
 *     5  the timer 10.0 pushes the effect 4 with the subtype 0
 *     6  the status 2 at the target 0 with the lap counter equal to the node
 *        count updates the timer and starts the new lap
 *     7  the status 2 at another target does nothing
 *     8  the status 2 at the target 0 with a short lap counter does nothing
 *     9  another status does nothing and keeps the progress from the clear
 *        current node
 *     A  the current node in the second chain forms the progress 704.75
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void player_lap_progress_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int lap_node_count;
float lap_timer_low;
float lap_timer_high;

static unsigned char object[0x800];
static char text[8];
static int effect_count;
static int effect_type;
static int effect_subtype;
static float effect_value;
static int timer_count;
static int complete_count;

void effect_push(int type, int subtype, float value)
{
    effect_type = type;
    effect_subtype = subtype;
    effect_value = value;
    effect_count = effect_count + 1;
}

void track_timer_update(void *state_object)
{
    (void)state_object;
    timer_count = timer_count + 1;
}

void race_lap_complete(void *state_object)
{
    (void)state_object;
    complete_count = complete_count + 1;
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

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void reset(void)
{
    int index;

    for (index = 0; index < (int)sizeof(object); index++) {
        object[index] = 0;
    }
    effect_count = 0;
    effect_type = 0;
    effect_subtype = 0;
    effect_value = 0.0f;
    timer_count = 0;
    complete_count = 0;
}

static void set_int(int offset, int value)
{
    *(int *)((char *)object + offset) = value;
}

static int get_int(int offset)
{
    return *(int *)((char *)object + offset);
}

static void set_float(int offset, float value)
{
    *(float *)((char *)object + offset) = value;
}

static float get_float(int offset)
{
    return *(float *)((char *)object + offset);
}

static void set_float_bits(float *value, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *value = v.f;
}

static int near_value(float left, float right, float tolerance)
{
    float difference;

    difference = left - right;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference < tolerance;
}

static void setup_checkpoint(float timer)
{
    reset();
    set_int(0x780, 1);
    set_int(0x668, 1);
    set_int(0x598, 1);
    set_int(0x59c, 2);
    set_int(0x678, 2);
    set_int(0x784, 1);
    set_int(0x684, 2);
    set_float(0x67c, 0.5f);
    set_float(0x790, timer);
}

int main(void)
{
    set_float_bits(&lap_timer_low, 0x40400000);
    set_float_bits(&lap_timer_high, 0x41700000);

    reset();
    set_int(0x780, 1);
    set_int(0x668, 2);
    set_int(0x598, 2);
    set_int(0x59c, 5);
    set_int(0x678, 0);
    set_int(0x684, 5);
    set_float(0x67c, 0.25f);
    player_lap_progress_update(object);
    check('0',
          get_int(0x5a0 + 2 * 4) == 1
          && get_int(0x59c) == 6
          && get_int(0x780) == 0
          && effect_count == 0
          && timer_count == 0
          && complete_count == 0
          && near_value(get_float(0x680), 502.25f, 0.001f));

    reset();
    set_int(0x780, 1);
    set_int(0x668, 1);
    set_int(0x598, 1);
    set_int(0x59c, 2);
    set_int(0x678, 2);
    set_int(0x784, 0);
    set_int(0x684, 2);
    set_float(0x67c, 0.5f);
    set_float(0x790, 10.0f);
    player_lap_progress_update(object);
    check('1',
          get_int(0x784) == 1
          && timer_count == 1
          && effect_count == 0
          && get_int(0x5a0 + 1 * 4) == 1
          && get_int(0x59c) == 3
          && get_int(0x780) == 0
          && near_value(get_float(0x680), 201.5f, 0.001f));

    setup_checkpoint(1.0f);
    player_lap_progress_update(object);
    check('2',
          effect_count == 1
          && effect_type == 4
          && effect_subtype == 2
          && near_value(effect_value, 0.0f, 0.001f)
          && timer_count == 1
          && complete_count == 0
          && near_value(get_float(0x680), 201.5f, 0.001f));

    setup_checkpoint(3.0f);
    player_lap_progress_update(object);
    check('3',
          effect_count == 1
          && effect_type == 4
          && effect_subtype == 0
          && timer_count == 1);

    setup_checkpoint(20.0f);
    player_lap_progress_update(object);
    check('4',
          effect_count == 1
          && effect_type == 4
          && effect_subtype == 1
          && timer_count == 1);

    setup_checkpoint(10.0f);
    player_lap_progress_update(object);
    check('5',
          effect_count == 1
          && effect_type == 4
          && effect_subtype == 0
          && timer_count == 1);

    reset();
    lap_node_count = 3;
    set_int(0x780, 2);
    set_int(0x668, 0);
    set_int(0x598, 0);
    set_int(0x59c, 3);
    set_float(0x680, 999.0f);
    player_lap_progress_update(object);
    check('6',
          timer_count == 1
          && complete_count == 1
          && effect_count == 0
          && get_int(0x780) == 0
          && near_value(get_float(0x680), 999.0f, 0.001f));

    reset();
    lap_node_count = 3;
    set_int(0x780, 2);
    set_int(0x668, 1);
    set_int(0x59c, 3);
    player_lap_progress_update(object);
    check('7',
          timer_count == 0
          && complete_count == 0
          && get_int(0x780) == 0);

    reset();
    lap_node_count = 3;
    set_int(0x780, 2);
    set_int(0x668, 0);
    set_int(0x59c, 2);
    player_lap_progress_update(object);
    check('8',
          timer_count == 0
          && complete_count == 0
          && get_int(0x780) == 0);

    reset();
    set_int(0x780, 3);
    set_int(0x598, 5);
    set_float(0x680, 42.0f);
    player_lap_progress_update(object);
    check('9',
          timer_count == 0
          && complete_count == 0
          && effect_count == 0
          && get_int(0x780) == 0
          && near_value(get_float(0x680), 42.0f, 0.001f));

    reset();
    set_int(0x780, 0);
    set_int(0x598, 4);
    set_int(0x684, 7);
    set_int(0x5a0 + 4 * 4, 1);
    set_float(0x67c, 0.75f);
    player_lap_progress_update(object);
    check('A',
          effect_count == 0
          && timer_count == 0
          && complete_count == 0
          && get_int(0x780) == 0
          && near_value(get_float(0x680), 704.75f, 0.001f));

    put('D');
    quit();
    return 0;
}
