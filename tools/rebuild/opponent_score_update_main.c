/* The test harness for the rebuilt `opponent_score_update` module.
 *
 * The harness holds one player object with the score float at `+0x790`, the
 * started opponent counter at `+0x7b8`, and the stored counter at `+0x7bc`.
 * The guard cells at `+0x78c`, `+0x794`, and `+0x7b4` check that the function
 * touches no neighbour field.
 *
 * The paths:
 *
 *     0  the counter equals the stored counter: no change
 *     1  the counter rises by one: the score adds 1.0 and the counter is stored
 *     2  the counter stays below the stored counter: no change
 *     3  the counter rises by several: the score adds only 1.0
 *     4  a negative counter crosses a more negative stored counter
 *     5  a fractional score keeps its fraction
 *     6  a second call at the same counter adds nothing
 *     7  the guard cells stay unchanged
 *     D  the exit marker
 *
 * The float values and the expected values come from their bit patterns, so
 * the harness does not depend on a float literal or on the constant segment.
 */
#pragma off (check_stack)

void opponent_score_update(void *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define OBJECT_SIZE 0x7c0

static char object[OBJECT_SIZE];
static char text[8];

static void set_f(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(object + offset) = value.f;
}

static unsigned int f_bits(int offset)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = *(float *)(object + offset);
    return value.u;
}

static int int_at(int offset)
{
    return *(int *)(object + offset);
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
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

static void reset(void)
{
    int index;

    for (index = 0; index < OBJECT_SIZE; index++) {
        object[index] = 0;
    }
}

static int guards_hold(void)
{
    return f_bits(0x78c) == 0 && int_at(0x7b4) == 0
        && f_bits(0x794) == 0;
}

int main(void)
{
    reset();
    set_f(0x790, 0x40a00000);
    set_int(0x7b8, 3);
    set_int(0x7bc, 3);
    opponent_score_update(object);
    put(f_bits(0x790) == 0x40a00000 && int_at(0x7bc) == 3
        && guards_hold() ? '0' : 'x');

    reset();
    set_f(0x790, 0x40200000);
    set_int(0x7b8, 4);
    set_int(0x7bc, 3);
    opponent_score_update(object);
    put(f_bits(0x790) == 0x40600000 && int_at(0x7bc) == 4
        && guards_hold() ? '1' : 'x');

    reset();
    set_f(0x790, 0x40200000);
    set_int(0x7b8, 2);
    set_int(0x7bc, 3);
    opponent_score_update(object);
    put(f_bits(0x790) == 0x40200000 && int_at(0x7bc) == 3
        && guards_hold() ? '2' : 'x');

    reset();
    set_f(0x790, 0x00000000);
    set_int(0x7b8, 10);
    set_int(0x7bc, 1);
    opponent_score_update(object);
    put(f_bits(0x790) == 0x3f800000 && int_at(0x7bc) == 10
        && guards_hold() ? '3' : 'x');

    reset();
    set_f(0x790, 0x00000000);
    set_int(0x7b8, -1);
    set_int(0x7bc, -3);
    opponent_score_update(object);
    put(f_bits(0x790) == 0x3f800000 && int_at(0x7bc) == -1
        && guards_hold() ? '4' : 'x');

    reset();
    set_f(0x790, 0x3fc00000);
    set_int(0x7b8, 2);
    set_int(0x7bc, 1);
    opponent_score_update(object);
    put(f_bits(0x790) == 0x40200000 && int_at(0x7bc) == 2
        && guards_hold() ? '5' : 'x');

    reset();
    set_f(0x790, 0x00000000);
    set_int(0x7b8, 5);
    set_int(0x7bc, 4);
    opponent_score_update(object);
    opponent_score_update(object);
    put(f_bits(0x790) == 0x3f800000 && int_at(0x7bc) == 5
        && guards_hold() ? '6' : 'x');

    reset();
    set_f(0x78c, 0x3f800000);
    set_f(0x794, 0x40000000);
    set_int(0x7b4, 0x1234);
    set_f(0x790, 0x00000000);
    set_int(0x7b8, 1);
    set_int(0x7bc, 0);
    opponent_score_update(object);
    put(f_bits(0x790) == 0x3f800000 && int_at(0x7bc) == 1
        && f_bits(0x78c) == 0x3f800000 && int_at(0x7b4) == 0x1234
        && f_bits(0x794) == 0x40000000 ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
