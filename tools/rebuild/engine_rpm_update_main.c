/* The test harness for the rebuilt `engine_rpm_update` module.
 *
 * The harness holds the player object, the input flags, the six joystick axes,
 * the frame delta, the engine constants, the filter response table, and the
 * three callee stubs.  The `filter_update` stub applies the one-pole filter and
 * records the index.  The `vehicle_state_set` stub mirrors the original state
 * store.  The `engine_sound_update` stub records the sound pointer and the
 * value.  The `sin` and `sqrt` stubs come from the local FPU assembly file.
 *
 * The harness runs eleven paths:
 *
 *     0  manual upshift: gear 2 to 3, the shift flag, the speed, and the sound
 *     1  manual downshift: gear 3 to 2 with the second axis
 *     2  manual neutral: the shift flag clears and the gear holds
 *     3  automatic upshift: the low engine-speed threshold
 *     4  automatic downshift: the high engine-speed threshold
 *     5  state 5: the gear reset and the speed step
 *     6  input bit 0x40: the axis store, the index 0x3d, and the filter target
 *     7  input bit 0 clear: the filter target and index 0x4b
 *     8  neutral: the speed decay
 *     9  one gear: the square-root torque path
 *    10  over the maximum engine speed: the clamp and the acceleration flag
 *     D  the exit marker
 *
 * The constants and the expected values are set from their exact bit patterns,
 * because a float literal in this minimal image becomes a read of the constant
 * segment, which this harness does not load.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void engine_rpm_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

double sin(double value);
double sqrt(double value);

static char text[8];
static unsigned char object[0x800];
static float response[0x100];
static int filter_calls;
static int filter_index[8];
static float filter_target[8];
static int state_calls;
static int state_value;
static int sound_calls;
static void *sound_pointer;
static int sound_value;
static int sound_marker;

unsigned char input_flags_low;
unsigned char input_flags_high;
int joystick_axis0;
int joystick_axis1;
int joystick_axis4;
int joystick_axis5;
int joystick_center4;
int joystick_center5;
int throttle_center;
int input_selector;
float engine_frame_delta;
double engine_sin_scale;
double engine_limit_base;
double engine_two;
double engine_half;
double engine_minus_one;
double engine_gravity_scale;
double engine_gravity_base;
float engine_rpm_threshold;
float engine_speed_low;
float engine_decay;
float engine_quarter_pi;
float engine_state_speed;
float engine_sound_scale;
float engine_sound_base;
float engine_ratio_min;
float engine_gear_scale[8];

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

static int close_to(float a, float b, float tolerance)
{
    float d;

    d = a - b;
    return d < tolerance && d > -tolerance;
}

void filter_update(float *p, float f, int i)
{
    if (filter_calls < 8) {
        filter_index[filter_calls] = i;
        filter_target[filter_calls] = f;
    }
    filter_calls = filter_calls + 1;
    *p = (*p - f) * response[i] + f;
}

void vehicle_state_set(void *value, int state)
{
    state_calls = state_calls + 1;
    state_value = state;
    if (*(int *)((char *)value + 0x148) == 0) {
        *(int *)((char *)value + 0x148) = state;
    }
}

void engine_sound_update(void *sound, int value)
{
    sound_calls = sound_calls + 1;
    sound_pointer = sound;
    sound_value = value;
}

static void base_setup(void)
{
    int index;

    set_bits_float(&engine_frame_delta, 0x3d4ccccd);
    set_bits_double(&engine_sin_scale, 0x00000000, 0x40100000);
    set_bits_double(&engine_limit_base, 0x00000000, 0x408f4000);
    set_bits_double(&engine_two, 0x00000000, 0x40000000);
    set_bits_double(&engine_half, 0x00000000, 0x3fe00000);
    set_bits_double(&engine_minus_one, 0x00000000, 0xbff00000);
    set_bits_double(&engine_gravity_scale, 0x00000000, 0x40590000);
    set_bits_double(&engine_gravity_base, 0x00000000, 0x40af4000);
    set_bits_float(&engine_rpm_threshold, 0x45bb8000);
    set_bits_float(&engine_speed_low, 0x44fa0000);
    set_bits_float(&engine_decay, 0x43fa0000);
    set_bits_float(&engine_quarter_pi, 0x3fc90fdb);
    set_bits_float(&engine_state_speed, 0x447a0000);
    set_bits_float(&engine_sound_scale, 0x41600000);
    set_bits_float(&engine_sound_base, 0x47000000);
    set_bits_float(&engine_ratio_min, 0x3e4ccccd);

    set_bits_float(&engine_gear_scale[0], 0x41300000);
    set_bits_float(&engine_gear_scale[1], 0x40a00000);
    set_bits_float(&engine_gear_scale[2], 0x40600000);
    set_bits_float(&engine_gear_scale[3], 0x40000000);
    set_bits_float(&engine_gear_scale[4], 0x3f733333);
    set_bits_float(&engine_gear_scale[5], 0x3f47ae14);
    set_bits_float(&engine_gear_scale[6], 0x3f3851ec);
    set_bits_float(&engine_gear_scale[7], 0x00000000);

    for (index = 0; index < 0x100; index++) {
        set_bits_float(&response[index], 0x3f800000);
    }
}

static void reset_case(void)
{
    int index;

    for (index = 0; index < 0x800; index++) {
        object[index] = 0;
    }
    input_flags_low = 0;
    input_flags_high = 0;
    joystick_axis0 = 0;
    joystick_axis1 = 0;
    joystick_axis4 = 0;
    joystick_axis5 = 0;
    joystick_center4 = 0;
    joystick_center5 = 0;
    throttle_center = 0;
    input_selector = 0;

    set_int(0x494, 6);
    set_float(0x498, 0x40000000);
    set_float(0x49c, 0x3f800000);
    set_float(0x4a0, 0x3f000000);
    set_float(0x4a4, 0x3e800000);
    set_float(0x4a8, 0x3e000000);
    set_float(0x4ac, 0x3dcccccd);
    set_float(0x480, 0x44fa0000);
    set_float(0x484, 0x461c4000);
    set_float(0x488, 0x3f800000);
    set_float(0x48c, 0x00000000);
    set_float(0x4c8, 0x00000000);
    set_float(0x144, 0x00000000);
    set_float(0x174, 0x40000000);
    set_float(0x178, 0x00000000);
    set_float(0x18c, 0x00000000);
    set_float(0x50c, 0x00000000);
    set_float(0x510, 0x00000000);
    set_float(0x7a8, 0x00000000);
    set_int(0x7ac, 0);
    set_int(0x490, 0);
    set_int(0x140, 0);
    set_int(0x148, 0);
    set_int(0x110, 0);
    *(void **)(object + 0x7cc) = &sound_marker;

    for (index = 0; index < 0x100; index++) {
        set_bits_float(&response[index], 0x3f800000);
    }
    filter_calls = 0;
    state_calls = 0;
    state_value = 0;
    sound_calls = 0;
    sound_pointer = 0;
    sound_value = 0;
}

int main(void)
{
    base_setup();

    reset_case();
    input_flags_low = 1;
    set_float(0x144, 0x3e4ccccd);
    set_int(0x7ac, 2);
    set_float(0x7a8, 0x447a0000);
    set_float(0x50c, 0x42c80000);
    joystick_axis4 = 100;
    joystick_center4 = 1;
    engine_rpm_update(object);
    put(get_int(0x7ac) == 3 && get_int(0x140) == 1
        && close_to(get_float(0x50c), 1000.0f, 0.01f)
        && sound_calls == 1 && sound_value == 46768 ? '0' : 'x');

    reset_case();
    input_flags_low = 1;
    set_float(0x144, 0x3e4ccccd);
    set_int(0x7ac, 3);
    set_float(0x7a8, 0x447a0000);
    set_float(0x50c, 0x42c80000);
    joystick_axis5 = -100;
    joystick_center5 = -1;
    engine_rpm_update(object);
    put(get_int(0x7ac) == 2 && get_int(0x140) == 1
        && close_to(get_float(0x50c), 500.0f, 0.01f)
        && sound_value == 46768 ? '1' : 'x');

    reset_case();
    input_flags_low = 1;
    set_float(0x144, 0x3e4ccccd);
    set_int(0x7ac, 2);
    set_float(0x7a8, 0x447a0000);
    set_float(0x50c, 0x42c80000);
    engine_rpm_update(object);
    put(get_int(0x7ac) == 2 && get_int(0x140) == 0
        && close_to(get_float(0x50c), 500.0f, 0.01f) ? '2' : 'x');

    reset_case();
    input_flags_low = 1;
    set_float(0x144, 0x3f800000);
    set_int(0x490, 1);
    set_int(0x7ac, 2);
    set_float(0x7a8, 0x46147800);
    set_float(0x50c, 0x42c80000);
    engine_rpm_update(object);
    put(get_int(0x7ac) == 3
        && close_to(get_float(0x50c), 1000.0f, 0.01f)
        && close_to(get_float(0x7a8), 9502.0f, 0.02f) ? '3' : 'x');

    reset_case();
    input_flags_low = 1;
    set_float(0x144, 0x3e4ccccd);
    set_int(0x490, 1);
    set_int(0x7ac, 4);
    set_float(0x7a8, 0x447a0000);
    set_float(0x50c, 0x42c80000);
    engine_rpm_update(object);
    put(get_int(0x7ac) == 3
        && close_to(get_float(0x50c), 1000.0f, 0.01f) ? '4' : 'x');

    reset_case();
    input_flags_low = 1;
    set_int(0x490, 1);
    set_int(0x7ac, 3);
    set_int(0x148, 5);
    set_float(0x7a8, 0x447a0000);
    engine_rpm_update(object);
    put(get_int(0x7ac) == 1 && get_int(0x148) == 5
        && close_to(get_float(0x50c), 50.0f, 0.01f) ? '5' : 'x');

    reset_case();
    input_flags_high = 0x40;
    input_selector = -5;
    set_int(0x7ac, 2);
    set_float(0x7a8, 0x459c4000);
    set_float(0x50c, 0x42c80000);
    set_bits_float(&response[0x3d], 0x00000000);
    engine_rpm_update(object);
    put(joystick_axis1 == -512 && filter_calls == 1
        && filter_index[0] == 0x3d && close_to(get_float(0x7a8), 0.0f, 0.01f)
        && get_int(0x7ac) == 2 && sound_value == 32768 ? '6' : 'x');

    reset_case();
    set_int(0x7ac, 3);
    set_float(0x7a8, 0x459c4000);
    set_float(0x50c, 0x42c80000);
    set_bits_float(&response[0x4b], 0x00000000);
    engine_rpm_update(object);
    put(filter_calls == 1 && filter_index[0] == 0x4b
        && close_to(get_float(0x7a8), 0.0f, 0.01f)
        && get_int(0x7ac) == 3 && sound_value == 32768 ? '7' : 'x');

    reset_case();
    input_flags_low = 1;
    set_float(0x50c, 0x42c80000);
    engine_rpm_update(object);
    put(get_int(0x7ac) == 0 && get_int(0x140) == 0
        && close_to(get_float(0x50c), 75.0f, 0.01f) ? '8' : 'x');

    reset_case();
    input_flags_low = 1;
    set_float(0x144, 0x3e4ccccd);
    set_int(0x490, 1);
    set_int(0x494, 1);
    set_int(0x7ac, 1);
    set_float(0x7a8, 0x447a0000);
    engine_rpm_update(object);
    put(get_int(0x7ac) == 1
        && close_to(get_float(0x7a8), 1000.0f, 0.01f)
        && close_to(get_float(0x50c), 250.0f, 0.01f)
        && sound_value == 46768 ? '9' : 'x');

    reset_case();
    input_flags_low = 1;
    set_int(0x490, 1);
    set_int(0x7ac, 2);
    set_float(0x7a8, 0x463b8000);
    set_float(0x50c, 0x42c80000);
    engine_rpm_update(object);
    put(get_int(0x7ac) == 0
        && close_to(get_float(0x7a8), 0.0f, 0.01f)
        && get_float(0x110) == 1.0f && sound_value == 32768 ? 'a' : 'x');

    put('D');
    quit();
    return 0;
}
