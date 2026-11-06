/* The test harness for the rebuilt `player_car_suspension_update` module.
 *
 * The harness holds the player object, the four suspension units, the filter
 * table, the frame delta, and the constants.  The `cos`, `atan`, and `fabs`
 * stubs live in the local FPU assembly file.  The harness runs five paths:
 *
 *     0  the speed change and the lateral load set opposite forces on the
 *        left and the right units; each travel clamps at -18 or 18, the body
 *        height is 0, and the first body angle is pi/4
 *     1  the stored forces pass the new forces: no unit resets; the travel is
 *        the force sum, the lateral force keeps 2.0, and the phase advances
 *     2  the two forces stay below the dead zone: both clear
 *     3  the speed change sets the longitudinal force through the filter; the
 *        four travel values are -1, 3, -1, and 3, and the body height is 1
 *     4  the stored forces pass the zero new forces: the negative travel sum
 *        truncates toward zero to -7, and the two body angles are 0.4636 and
 *        -0.7854
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.  The float comparisons use
 * the raw bits, because the constant pool of this toolchain reads at a low
 * address.
 */
#pragma off (check_stack)

struct suspension_unit {
    unsigned char pad0[0x30];
    float phase;
    int travel;
    float lateral;
    int pad3c;
    float longitudinal;
    unsigned char pad44[0x10];
};

void player_car_suspension_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float suspension_frame_delta;
float *suspension_filter_table;
double suspension_speed_scale;
double suspension_lateral_scale;
float suspension_wheel_scale;
float suspension_wave_scale;
float suspension_wave_offset;
double suspension_dead_zone;

void filter_update(float *value, float target, int index)
{
    *value = (*value - target) * suspension_filter_table[index] + target;
}

static float filter_data[0x80];
static unsigned char object[0x900];
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

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int get_int(int offset)
{
    return *(int *)(object + offset);
}

static void set_float(int offset, float value)
{
    *(float *)(object + offset) = value;
}

static unsigned int get_bits(int offset)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = *(float *)(object + offset);
    return v.u;
}

static struct suspension_unit *unit_at(int index)
{
    return (struct suspension_unit *)(object + 0x194 + index * 0x54);
}

static void clear_object(void)
{
    int index;

    for (index = 0; index < 0x900; index++) {
        object[index] = 0;
    }
}

static void set_forces(int index, float lateral, float longitudinal)
{
    unit_at(index)->lateral = lateral;
    unit_at(index)->longitudinal = longitudinal;
}

static void set_phase(int index, float phase)
{
    unit_at(index)->phase = phase;
}

int main(void)
{
    int index;

    suspension_speed_scale = 1.0;
    suspension_lateral_scale = 1.0;
    suspension_wheel_scale = 3.0f;
    suspension_wave_scale = 0.0f;
    suspension_wave_offset = 0.0f;
    suspension_dead_zone = 2.0;
    suspension_frame_delta = 0.25f;
    suspension_filter_table = filter_data;
    for (index = 0; index < 0x80; index++) {
        filter_data[index] = 0.0f;
    }
    filter_data[0x3d] = 0.5f;
    filter_data[0x47] = 1.0f;

    clear_object();
    set_float(0x50c, 10.0f);
    set_float(0x588, 0.0f);
    set_float(0x2f4, 0.25f);
    set_int(0x2e4, 36);
    set_int(0x2ec, 1);
    player_car_suspension_update(object);
    put(unit_at(0)->travel == -18 && unit_at(1)->travel == -18
            && unit_at(2)->travel == 18 && unit_at(3)->travel == 18
            && get_int(0x4c0) == 0
            && get_bits(0x4e8) == 0x3f490fdb
            && get_bits(0x4e0) == 0
        ? '0' : 'x');

    clear_object();
    for (index = 0; index < 4; index++) {
        set_forces(index, 4.0f, 4.0f);
        set_phase(index, 0.5f);
    }
    player_car_suspension_update(object);
    put(unit_at(0)->travel == 8 && unit_at(1)->travel == 8
            && unit_at(2)->travel == 8 && unit_at(3)->travel == 8
            && get_bits(0x1cc) == 0x40000000
            && get_bits(0x1d4) == 0x40800000
            && get_bits(0x1c4) == 0x3f400000
        ? '1' : 'x');

    clear_object();
    for (index = 0; index < 4; index++) {
        set_forces(index, 3.0f, 1.0f);
        set_phase(index, 0.5f);
    }
    player_car_suspension_update(object);
    put(unit_at(0)->travel == 4 && unit_at(1)->travel == 4
            && unit_at(2)->travel == 4 && unit_at(3)->travel == 4
            && get_bits(0x1cc) == 0
            && get_bits(0x1d4) == 0
        ? '2' : 'x');

    clear_object();
    set_float(0x50c, 0.5f);
    set_float(0x588, 0.0f);
    set_float(0x2f4, 0.0f);
    set_int(0x2e4, 1);
    set_int(0x2ec, 4);
    player_car_suspension_update(object);
    put(unit_at(0)->travel == -1 && unit_at(1)->travel == 3
            && unit_at(2)->travel == -1 && unit_at(3)->travel == 3
            && get_int(0x4c0) == 1
            && get_bits(0x4e8) == 0x80000000
            && get_bits(0x4e0) == 0xbf490fdb
        ? '3' : 'x');

    clear_object();
    set_int(0x2e4, 8);
    set_int(0x2ec, 1);
    set_forces(0, -10.0f, 0.0f);
    set_forces(1, -8.0f, 0.0f);
    set_forces(2, -6.0f, 0.0f);
    set_forces(3, -5.0f, 0.0f);
    for (index = 0; index < 4; index++) {
        set_phase(index, 0.5f);
    }
    player_car_suspension_update(object);
    put(unit_at(0)->travel == -10 && unit_at(1)->travel == -8
            && unit_at(2)->travel == -6 && unit_at(3)->travel == -5
            && get_int(0x4c0) == -7
            && get_bits(0x4e8) == 0x3eed6338
            && get_bits(0x4e0) == 0xbf490fdb
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
