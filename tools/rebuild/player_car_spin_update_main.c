/* The test harness for the rebuilt `player_car_spin_update` module.
 *
 * The harness holds one player object, the frame delta, and the four
 * constants.  It runs nine paths:
 *
 *     0  state 1 with a negative yaw step: the counter becomes 4 and the
 *        offset rises by one full turn 2*pi
 *     1  state 1 with a positive yaw step: the counter becomes 2 and the
 *        offset falls by one full turn 2*pi
 *     2  state 2 with the offset -1 and the frame delta 1: the offset crosses
 *        zero, so it clears and the counter clears
 *     3  state 2 with the offset -1 and the frame delta 0.1: the offset rises
 *        to -0.885 and the counter stays 2
 *     4  state 4 with the offset 1 and the frame delta 1: the offset crosses
 *        zero, so it clears and the counter clears
 *     5  state 4 with the offset 1 and the frame delta 0.1: the offset falls
 *        to 0.885 and the counter stays 4
 *     6  state 3: the offset and the counter stay unchanged
 *     7  state 0: the function returns at once
 *     8  state -5: the function returns at once
 *     D  the exit marker
 *
 * The harness uses only BSS state and assigns the four constants at run time,
 * because the minimal image does not load the initialized data page.
 */
#pragma off (check_stack)

void player_car_spin_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_00118e04;
float player_car_spin_turn;
float player_car_spin_scale;
double player_car_spin_bias;
double player_car_spin_pi;

static char object[0x200];
static char text[8];

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

    for (index = 0; index < 0x200; index++) {
        object[index] = 0;
    }
    DAT_00118e04 = 1.0f;
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int int_is(int offset, int value)
{
    return *(int *)(object + offset) == value;
}

static void set_float(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(object + offset) = value.f;
}

static void set_float_value(int offset, float value)
{
    *(float *)(object + offset) = value;
}

static int near_float(int offset, float expected, float tolerance)
{
    float actual;
    float difference;

    actual = *(float *)(object + offset);
    difference = actual - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference <= tolerance;
}

int main(void)
{
    player_car_spin_turn = 3.0f;
    player_car_spin_scale = 6.2831855f;
    player_car_spin_bias = 0.2;
    player_car_spin_pi = 3.1415927410125732;

    reset();
    set_int(0x170, 1);
    set_float(0x2f4, 0xbf800000);
    player_car_spin_update(object);
    put(int_is(0x170, 4) && near_float(0x128, 6.2831855f, 0.001f)
        ? '0' : 'x');

    reset();
    set_int(0x170, 1);
    set_float(0x2f4, 0x3f800000);
    player_car_spin_update(object);
    put(int_is(0x170, 2) && near_float(0x128, -6.2831855f, 0.001f)
        ? '1' : 'x');

    reset();
    set_int(0x170, 2);
    set_float(0x128, 0xbf800000);
    player_car_spin_update(object);
    put(int_is(0x170, 0) && near_float(0x128, 0.0f, 0.001f) ? '2' : 'x');

    reset();
    DAT_00118e04 = 0.1f;
    set_int(0x170, 2);
    set_float(0x128, 0xbf800000);
    player_car_spin_update(object);
    put(int_is(0x170, 2) && near_float(0x128, -0.8854084f, 0.001f)
        ? '3' : 'x');

    reset();
    set_int(0x170, 4);
    set_float(0x128, 0x3f800000);
    player_car_spin_update(object);
    put(int_is(0x170, 0) && near_float(0x128, 0.0f, 0.001f) ? '4' : 'x');

    reset();
    DAT_00118e04 = 0.1f;
    set_int(0x170, 4);
    set_float(0x128, 0x3f800000);
    player_car_spin_update(object);
    put(int_is(0x170, 4) && near_float(0x128, 0.8854084f, 0.001f)
        ? '5' : 'x');

    reset();
    set_int(0x170, 3);
    set_float(0x128, 0x3f000000);
    player_car_spin_update(object);
    put(int_is(0x170, 3) && near_float(0x128, 0.5f, 0.001f) ? '6' : 'x');

    reset();
    set_int(0x170, 0);
    set_float(0x128, 0x3e800000);
    player_car_spin_update(object);
    put(int_is(0x170, 0) && near_float(0x128, 0.25f, 0.001f) ? '7' : 'x');

    reset();
    set_int(0x170, -5);
    set_float(0x128, 0x3e800000);
    player_car_spin_update(object);
    put(int_is(0x170, -5) && near_float(0x128, 0.25f, 0.001f) ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
