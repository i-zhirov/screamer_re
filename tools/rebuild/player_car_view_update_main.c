/* The test harness for the rebuilt `player_car_view_update` module.
 *
 * The harness holds one player object, the frame delta, the input flags, the
 * view counter, the two filter constants, the filter table, and the five view
 * constants.  The `filter_update` stub writes the value into the view scale.
 *
 * The harness runs eight paths:
 *
 *     0  the full path with the zero angles and the zero speed
 *     1  the input bit 0x10 filters the view scale and the pitch path projects
 *     2  the flag at +0x108 selects the roll path
 *     3  the non-zero state at +0x148 selects the flat yaw path
 *     4  the three angles normalize to the range 0 to 2 pi
 *     5  the heading rate and the shake terms stay, and the small lateral rate
 *        clears
 *     6  the lateral rate adds the two offsets and stays above the epsilon
 *     7  the render flags skip both bits for the non-zero flag and the high
 *        speed
 *     D  the exit marker
 */
#pragma off (check_stack)

void player_car_view_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_0011d260;
int DAT_0011d254;
float DAT_00118e04;
float *DAT_0010cf08;
float view_angle_wrap;
float view_angle_unwrap;
double view_rate_epsilon;
double view_shake_scale;
float view_speed_threshold;
float view_filter_scale;
float view_filter_base;

static char object[0x800];
static float filter_table[0x60];
static char text[8];
static int filter_calls;
static int filter_index;
static float filter_value;

static void put(char a)
{
    text[0] = a;
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

static void set_float(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *(float *)(object + offset) = v.f;
}

static unsigned int get_float_bits(int offset)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = *(float *)(object + offset);
    return v.u;
}

static float get_float(int offset)
{
    return *(float *)(object + offset);
}

static void set_float_global(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static void set_double_global(double *p, unsigned long long bits)
{
    union {
        double d;
        unsigned long long u;
    } v;

    v.u = bits;
    *p = v.d;
}

void filter_update(float *p, float f, int i)
{
    *p = f;
    filter_calls = filter_calls + 1;
    filter_index = i;
    filter_value = f;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x800; index++) {
        object[index] = 0;
    }
    for (index = 0; index < 0x60; index++) {
        filter_table[index] = 0.0f;
    }
    filter_table[0x50] = 1.0f;
    filter_table[0x5b] = 1.0f;
    DAT_0010cf08 = filter_table;
    DAT_0011d260 = 0;
    DAT_0011d254 = 2;
    set_float_global(&DAT_00118e04, 0x3f800000);
    set_float_global(&view_angle_wrap, 0x40c90fdb);
    set_float_global(&view_angle_unwrap, 0xc0c90fdb);
    set_double_global(&view_rate_epsilon, 0x406f400000000000ull);
    set_double_global(&view_shake_scale, 0x4000000000000000ull);
    set_float_global(&view_speed_threshold, 0x457a0000);
    set_float_global(&view_filter_scale, 0x40400000);
    set_float_global(&view_filter_base, 0x40e00000);
    filter_calls = 0;
    filter_index = -1;
    filter_value = 0.0f;
}

int main(void)
{
    reset();
    set_int(0x10, 5);
    set_int(0x18, 7);
    set_int(0x4b0, 10);
    set_int(0x4b8, 20);
    set_int(0x148, 0);
    set_int(0x108, 0);
    set_int(0x114, 0);
    set_int(0x47c, 0x100);
    set_float(0x50c, 0x00000000);
    set_float(0x4c8, 0x00000000);
    set_float(0x4cc, 0x00000000);
    set_float(0x4d0, 0x00000000);
    set_float(0x548, 0x00000000);
    set_float(0x54c, 0x00000000);
    set_float(0x550, 0x00000000);
    set_float(0x558, 0x00000000);
    set_float(0x4fc, 0x00000000);
    set_float(0x500, 0x00000000);
    set_float(0x508, 0x00000000);
    set_float(0x300, 0x00000000);
    player_car_view_update(object);
    put(get_int(0x4b0) == 15 && get_int(0x4b8) == 27
        && get_float_bits(0x588) == 0x00000000
        && filter_calls == 0
        && get_int(0x7b4) == 0x6100
        && get_int(0x10) == 0 && get_int(0x18) == 0 ? '0' : 'x');

    reset();
    set_int(0x148, 0);
    set_int(0x108, 0);
    set_int(0x114, 0);
    set_int(0x47c, 0);
    set_int(0x688, 4);
    DAT_0011d260 = 0x10;
    set_float(0x50c, 0x3f800000);
    set_float(0x4c8, 0x00000000);
    set_float(0x4cc, 0x00000000);
    set_float(0x4d0, 0x00000000);
    set_float(0x548, 0x00000000);
    set_float(0x54c, 0x00000000);
    set_float(0x550, 0x00000000);
    set_float(0x558, 0x00000000);
    set_float(0x4fc, 0x00000000);
    set_float(0x500, 0x00000000);
    set_float(0x508, 0x00000000);
    set_float(0x300, 0x00000000);
    player_car_view_update(object);
    put(filter_calls == 1 && filter_index == 0x58
        && get_float_bits(0x300) == 0x41500000
        && get_int(0x4b0) == 0 && get_int(0x4b8) == -13
        && get_float_bits(0x588) == 0x3f800000
        && get_int(0x7b4) == 0x6000 ? '1' : 'x');

    reset();
    set_int(0x148, 0);
    set_int(0x108, 1);
    set_int(0x114, 0);
    set_int(0x47c, 0);
    set_int(0x1c, 2);
    set_int(0x24, 3);
    set_float(0x50c, 0x00000000);
    set_float(0x4c8, 0x00000000);
    set_float(0x4cc, 0x00000000);
    set_float(0x4d0, 0x00000000);
    set_float(0x548, 0x00000000);
    set_float(0x54c, 0x00000000);
    set_float(0x550, 0x00000000);
    set_float(0x558, 0x00000000);
    set_float(0x4fc, 0x00000000);
    set_float(0x500, 0x00000000);
    set_float(0x508, 0x00000000);
    set_float(0x300, 0x3f800000);
    player_car_view_update(object);
    put(get_int(0x4b0) == 2 && get_int(0x4b8) == 3
        && get_int(0x7b4) == 0x6000 ? '2' : 'x');

    reset();
    set_int(0x148, 1);
    set_int(0x108, 0);
    set_int(0x114, 0);
    set_int(0x47c, 0);
    set_float(0x50c, 0x40000000);
    set_float(0x4c8, 0x00000000);
    set_float(0x4cc, 0x00000000);
    set_float(0x4d0, 0x00000000);
    set_float(0x548, 0x00000000);
    set_float(0x54c, 0x00000000);
    set_float(0x550, 0x00000000);
    set_float(0x558, 0x00000000);
    set_float(0x4fc, 0x00000000);
    set_float(0x500, 0x00000000);
    set_float(0x508, 0x00000000);
    set_float(0x300, 0x00000000);
    player_car_view_update(object);
    put(get_int(0x4b0) == 0 && get_int(0x4b8) == -2
        && get_float_bits(0x588) == 0x40000000
        && get_int(0x7b4) == 0x6000 ? '3' : 'x');

    reset();
    set_int(0x148, 0);
    set_int(0x108, 0);
    set_int(0x114, 0);
    set_int(0x47c, 0);
    set_float(0x50c, 0x00000000);
    set_float(0x4c8, 0xbf800000);
    set_float(0x4cc, 0x40e00000);
    set_float(0x4d0, 0xbf800000);
    set_float(0x548, 0x00000000);
    set_float(0x54c, 0x00000000);
    set_float(0x550, 0x00000000);
    set_float(0x558, 0x00000000);
    set_float(0x4fc, 0x00000000);
    set_float(0x500, 0x00000000);
    set_float(0x508, 0x00000000);
    set_float(0x300, 0x00000000);
    player_car_view_update(object);
    put(get_float(0x4c8) > 0.0f && get_float(0x4c8) < view_angle_wrap
        && get_float(0x4d0) > 0.0f && get_float(0x4d0) < view_angle_wrap
        && get_float(0x4cc) > 0.0f && get_float(0x4cc) < view_angle_wrap
        && get_int(0x4b0) == 0 && get_int(0x4b8) == 0 ? '4' : 'x');

    reset();
    set_int(0x148, 0);
    set_int(0x108, 0);
    set_int(0x114, 0);
    set_int(0x47c, 0);
    set_float(0x50c, 0x00000000);
    set_float(0x4c8, 0x00000000);
    set_float(0x4cc, 0x00000000);
    set_float(0x4d0, 0x00000000);
    set_float(0x548, 0x3f800000);
    set_float(0x54c, 0x42c80000);
    set_float(0x550, 0x00000000);
    set_float(0x558, 0x00000000);
    set_float(0x4fc, 0x3f800000);
    set_float(0x500, 0x40000000);
    set_float(0x508, 0x40400000);
    set_float(0x300, 0x3f800000);
    filter_table[0x50] = 2.0f;
    player_car_view_update(object);
    put(get_float_bits(0x548) == 0x40000000
        && get_float_bits(0x4cc) == 0x40000000
        && get_float_bits(0x54c) == 0x00000000
        && get_int(0x4b0) == 2 && get_int(0x4b8) == 4
        && get_int(0x7b4) == 0x6000 ? '5' : 'x');

    reset();
    set_int(0x148, 0);
    set_int(0x108, 0);
    set_int(0x114, 0);
    set_int(0x47c, 0);
    set_float(0x50c, 0x00000000);
    set_float(0x4c8, 0x00000000);
    set_float(0x4cc, 0x00000000);
    set_float(0x4d0, 0x00000000);
    set_float(0x548, 0x00000000);
    set_float(0x54c, 0x43960000);
    set_float(0x550, 0x40000000);
    set_float(0x558, 0x40400000);
    set_float(0x4fc, 0x00000000);
    set_float(0x500, 0x00000000);
    set_float(0x508, 0x00000000);
    set_float(0x300, 0x00000000);
    player_car_view_update(object);
    put(get_int(0x4b0) == 600 && get_int(0x4b8) == 900
        && get_float_bits(0x54c) == 0x43960000
        && get_float_bits(0x4cc) == 0x00000000
        && get_int(0x7b4) == 0x6000 ? '6' : 'x');

    reset();
    set_int(0x148, 0);
    set_int(0x108, 0);
    set_int(0x114, 1);
    set_int(0x47c, 0x55);
    set_int(0x4b0, 1);
    set_int(0x4b8, 2);
    set_float(0x50c, 0x459c4000);
    set_float(0x4c8, 0x00000000);
    set_float(0x4cc, 0x00000000);
    set_float(0x4d0, 0x00000000);
    set_float(0x548, 0x00000000);
    set_float(0x54c, 0x00000000);
    set_float(0x550, 0x00000000);
    set_float(0x558, 0x00000000);
    set_float(0x4fc, 0x00000000);
    set_float(0x500, 0x00000000);
    set_float(0x508, 0x00000000);
    set_float(0x300, 0x00000000);
    player_car_view_update(object);
    put(get_int(0x4b0) == 1 && get_int(0x4b8) == 2
        && get_float_bits(0x588) == 0x459c4000
        && get_int(0x7b4) == 0x55
        && get_int(0x10) == 0 && get_int(0x18) == 0 ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
