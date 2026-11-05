/* The test harness for the rebuilt `player_steering_update` module.
 *
 * The harness holds the player object, the control globals, the six control
 * values, the one-pole filter table, and the `filter_update` stub.  The stub
 * applies the filter with the table value of the index, so the selected index
 * stays visible.  The `sqrt`, `tan`, `sin`, and `cos` stubs come from the local
 * FPU assembly file.
 *
 * The harness runs ten paths:
 *
 *     0  mode 2 with the direct axis: index 0x50, a negative angle, the turn
 *        radius, the yaw step, the heading, and the position delta
 *     1  mode 1 with the second axis: index 0x55 and a positive angle
 *     2  mode 0 with index 0x50: the direct assignment and no filter call
 *     3  the keyboard ramp above the limit: the clamp sets the limit
 *     4  a positive angle with the stop flag `+0x6c`: the reset sets zero
 *     5  a negative angle with the stop flag `+0xc4`: the reset sets zero
 *     6  state 1 with a non-zero angle: the state reset sets zero
 *     7  mode 0 with index 0x55: the filtered assignment
 *     8  the epsilon guard: the small filtered value sets zero
 *     D  the exit marker
 *
 * The constants and the expected values are set from their exact bit patterns,
 * because a float literal in this minimal image becomes a read of the constant
 * segment, which this harness does not load.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void player_steering_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

double sqrt(double value);
double tan(double value);

static char text[8];
static unsigned char object[0x600];
static float response[0x100];
static int filter_calls;
static int filter_index[4];
static float filter_target[4];

double steering_angle_range;
double steering_angle_base;
float steering_axis_scale;
double steering_epsilon;
double steering_key_scale;
int steering_mode;
int steering_axis;
int steering_axis_secondary;
unsigned char steering_flags;
float steering_frame_delta;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static void set_bits_double(double *p, unsigned int low, unsigned int high)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.u[0] = low;
    v.u[1] = high;
    *p = v.d;
}

static void set_float(int offset, unsigned int bits)
{
    set_bits_float((float *)(object + offset), bits);
}

static int get_int(int offset)
{
    return *(int *)(object + offset);
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static float get_float(int offset)
{
    return *(float *)(object + offset);
}

static unsigned int get_float_bits(int offset)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = get_float(offset);
    return v.u;
}

void filter_update(float *p, float f, int i)
{
    if (filter_calls < 4) {
        filter_index[filter_calls] = i;
        filter_target[filter_calls] = f;
    }
    filter_calls = filter_calls + 1;
    *p = (*p - f) * response[i] + f;
}

static double close_to(double a, double b, double tolerance)
{
    double d;

    d = a - b;
    return d < tolerance && d > -tolerance;
}

static void clear_object(void)
{
    int index;

    for (index = 0; index < 0x600; index++) {
        object[index] = 0;
    }
}

static void set_response(unsigned int bits)
{
    int index;

    for (index = 0; index < 0x100; index++) {
        set_bits_float(&response[index], bits);
    }
}

static void base_setup(void)
{
    set_bits_double(&steering_angle_range, 0x20000000, 0x3fd2fd3b);
    set_bits_double(&steering_angle_base, 0xa0000000, 0x3f91df46);
    set_bits_float(&steering_axis_scale, 0x3a800000);
    set_bits_double(&steering_epsilon, 0x88e368f1, 0x3ee4f8b5);
    set_bits_double(&steering_key_scale, 0xeb1c432d, 0x3f4a36e2);
    set_bits_float(&steering_frame_delta, 0x3c23d70a);

    clear_object();
    set_float(0x50c, 0x42c80000);
    set_float(0x480, 0x43c80000);
    set_int(0x2ec, 2500);
    set_response(0x00000000);

    steering_mode = 0;
    steering_axis = 0;
    steering_axis_secondary = 0;
    steering_flags = 0;
    filter_calls = 0;
}

static float expected_limit(void)
{
    double ratio;

    ratio = (double)get_float(0x50c) / (double)get_float(0x480);
    return (float)((1.0 - sqrt(ratio)) * steering_angle_range +
                   steering_angle_base);
}

static float expected_target(int axis, int sign, float limit)
{
    float value;

    value = (float)axis * steering_axis_scale;
    value = value * limit;
    value = -value;
    value = value * (float)sign;
    return value;
}

int main(void)
{
    float heading;
    float limit;
    float target;
    float extra;

    base_setup();
    steering_mode = 2;
    steering_axis = 512;
    set_float(0x4cc, 0x3f000000);
    heading = get_float(0x4cc);
    player_steering_update(object);
    limit = expected_limit();
    target = expected_target(512, 1, limit);
    put(filter_calls == 1 && filter_index[0] == 0x50 &&
                close_to(get_float(0x10c), limit, 1e-6) &&
                close_to(get_float(0x108), target, 1e-6) &&
                close_to((double)get_float(0x2f8) *
                             tan((double)get_float(0x108)),
                         2500.0, 0.5) &&
                close_to((double)get_float(0x2f4) *
                             (double)get_float(0x2f8),
                         1.0, 1e-4) &&
                close_to(get_float(0x4cc),
                         (double)(heading + get_float(0x2f4)), 1e-6) &&
                !(get_int(0x1c) == 0 && get_int(0x24) == 0)
            ? '0'
            : 'x');

    base_setup();
    steering_mode = 1;
    steering_axis = 512;
    steering_axis_secondary = 256;
    set_int(0x110, 1);
    player_steering_update(object);
    limit = expected_limit();
    target = expected_target(512, 1, limit);
    extra = -expected_target(256, 1, limit);
    target = -(target + extra);
    put(filter_calls == 1 && filter_index[0] == 0x55 &&
                close_to(get_float(0x108), target, 1e-6) &&
                close_to(get_float(0x10c), limit, 1e-6)
            ? '1'
            : 'x');

    base_setup();
    steering_mode = 0;
    steering_axis = 512;
    player_steering_update(object);
    limit = expected_limit();
    target = expected_target(512, 1, limit);
    put(filter_calls == 0 && close_to(get_float(0x108), target, 1e-6) &&
                close_to(get_float(0x10c), limit, 1e-6)
            ? '2'
            : 'x');

    base_setup();
    steering_mode = 3;
    set_float(0x108, 0x41200000);
    set_int(0x2f0, 1);
    set_int(0x4b0, 1);
    player_steering_update(object);
    limit = expected_limit();
    put(filter_calls == 0 && close_to(get_float(0x108), limit, 1e-6)
            ? '3'
            : 'x');

    base_setup();
    steering_mode = 0;
    steering_axis = 512;
    steering_flags = 0x10;
    set_int(0x6c, 1);
    player_steering_update(object);
    put(get_float_bits(0x108) == 0 ? '4' : 'x');

    base_setup();
    steering_mode = 0;
    steering_axis = 512;
    set_int(0xc4, 1);
    player_steering_update(object);
    put(get_float_bits(0x108) == 0 ? '5' : 'x');

    base_setup();
    steering_mode = 3;
    set_int(0x148, 1);
    set_float(0x108, 0x3f800000);
    player_steering_update(object);
    put(get_float_bits(0x108) == 0 ? '6' : 'x');

    base_setup();
    steering_mode = 0;
    steering_axis = 512;
    set_int(0x110, 1);
    player_steering_update(object);
    limit = expected_limit();
    target = expected_target(512, 1, limit);
    put(filter_calls == 1 && filter_index[0] == 0x55 &&
                close_to(get_float(0x108), target, 1e-6)
            ? '7'
            : 'x');

    base_setup();
    steering_mode = 2;
    set_float(0x108, 0x358637bd);
    set_bits_float(&response[0x50], 0x3f000000);
    player_steering_update(object);
    put(filter_calls == 1 && get_float_bits(0x108) == 0 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
