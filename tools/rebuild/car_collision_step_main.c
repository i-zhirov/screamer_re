/* The test harness for the rebuilt `car_collision_step` module.
 *
 * The harness holds the car record and the three pass stubs.  Each stub
 * counts its calls, records its order, and checks the record pointer.  The
 * harness sets the crash state at `+0x2f8` and the physics gate at `+0x414`,
 * runs the step, and checks the call set.
 *
 * The paths:
 *
 *     0  crash state 0, gate 1: all three passes ran once in order
 *     1  crash state 1, gate 1: no pass ran
 *     2  crash state 0, gate 0: no pass ran
 *     3  crash state 2, gate 0: no pass ran
 *     4  two calls with crash state 0, gate 1: every pass ran twice in order
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void car_collision_step(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];
static int nearby_calls;
static int collision_calls;
static int avoid_calls;
static int sequence;
static int nearby_sequence;
static int collision_sequence;
static int avoid_sequence;
static int pointer_ok;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void car_nearby_mark(void *obj)
{
    nearby_calls = nearby_calls + 1;
    nearby_sequence = sequence;
    sequence = sequence + 1;
    if (obj != (void *)object) {
        pointer_ok = 0;
    }
}

void car_collision_pass(void *obj)
{
    collision_calls = collision_calls + 1;
    collision_sequence = sequence;
    sequence = sequence + 1;
    if (obj != (void *)object) {
        pointer_ok = 0;
    }
}

void opponent_avoid_player(void *obj)
{
    avoid_calls = avoid_calls + 1;
    avoid_sequence = sequence;
    sequence = sequence + 1;
    if (obj != (void *)object) {
        pointer_ok = 0;
    }
}

static void reset(void)
{
    nearby_calls = 0;
    collision_calls = 0;
    avoid_calls = 0;
    sequence = 0;
    nearby_sequence = -1;
    collision_sequence = -1;
    avoid_sequence = -1;
    pointer_ok = 1;
}

static int no_calls(void)
{
    return nearby_calls == 0 && collision_calls == 0 && avoid_calls == 0;
}

static int one_ordered_call(void)
{
    return nearby_calls == 1 && collision_calls == 1 && avoid_calls == 1
        && nearby_sequence < collision_sequence
        && collision_sequence < avoid_sequence
        && pointer_ok == 1;
}

int main(void)
{
    *(unsigned int *)(object + 0x2f8) = 0;
    *(unsigned int *)(object + 0x414) = 1;
    reset();
    car_collision_step(object);
    put(one_ordered_call() == 1 ? '0' : 'x');

    *(unsigned int *)(object + 0x2f8) = 1;
    *(unsigned int *)(object + 0x414) = 1;
    reset();
    car_collision_step(object);
    put(no_calls() == 1 ? '1' : 'x');

    *(unsigned int *)(object + 0x2f8) = 0;
    *(unsigned int *)(object + 0x414) = 0;
    reset();
    car_collision_step(object);
    put(no_calls() == 1 ? '2' : 'x');

    *(unsigned int *)(object + 0x2f8) = 2;
    *(unsigned int *)(object + 0x414) = 0;
    reset();
    car_collision_step(object);
    put(no_calls() == 1 ? '3' : 'x');

    *(unsigned int *)(object + 0x2f8) = 0;
    *(unsigned int *)(object + 0x414) = 1;
    reset();
    car_collision_step(object);
    car_collision_step(object);
    put(nearby_calls == 2 && collision_calls == 2 && avoid_calls == 2
        && nearby_sequence < collision_sequence
        && collision_sequence < avoid_sequence
        && pointer_ok == 1 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
