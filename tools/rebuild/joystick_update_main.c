/* The test harness for the rebuilt `joystick_update` module.
 *
 * The harness holds the six control records, the output array, the axis table,
 * the ten-button port table, and the two read stubs.  The stubs record their
 * arguments.  The harness runs twelve paths:
 *
 *     0  the positive scale: the axis value 150
 *     1  the negative scale: the product -200.5 truncates toward zero to -200
 *     2  the dead zone: the value 25 becomes zero
 *     3  the full-scale high clamp: the value 4000 becomes 1023
 *     4  the mode 2 high clamp: the value 600 becomes 1023
 *     5  the full-scale low clamp: the value -2000 becomes -1023
 *     6  the duplicate search: the second record copies the first output and
 *        the axis read runs once
 *     7  the raw button path with a set mask: the output 1023
 *     8  the raw button path with a clear mask: the output 0
 *    9  the button state pointer with a set byte: the output 1023
 *    A  the button state pointer with a clear byte: the output 0
 *    B  a record of another type: the output stays unchanged
 *    D  the exit marker
 *
 * The scale values and the expected values are set from their exact bit
 * patterns.  A float literal in this minimal image becomes a read of the
 * constant segment, which this harness does not load, so a literal comparison
 * would compare against zero.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void joystick_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct control_record {
    int type;
    int kind;
    int mode;
    int axis;
    int device;
    int field_14;
    int field_18;
};

struct axis_record {
    int field_00;
    int center;
    int field_08;
    int divisor;
    int port;
    float scale_positive;
    float scale_negative;
};

struct control_record DAT_001197d8[6];
int DAT_0019880[6];
struct axis_record DAT_00192a4[40];
int DAT_0019704[10];

static char text[8];
static unsigned char button_state[4];
static int axis_calls;
static int axis_mask;
static int axis_port;
static int axis_result;
static int button_calls;
static int button_port;
static int button_result;

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

int joystick_axis_read(int mask, int port)
{
    axis_calls = axis_calls + 1;
    axis_mask = mask;
    axis_port = port;
    return axis_result;
}

int joystick_button_read(int port)
{
    button_calls = button_calls + 1;
    button_port = port;
    return button_result;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 6; index++) {
        DAT_001197d8[index].type = 0;
        DAT_001197d8[index].kind = 0;
        DAT_001197d8[index].mode = 0;
        DAT_001197d8[index].axis = 0;
        DAT_001197d8[index].device = 0;
        DAT_001197d8[index].field_14 = 0;
        DAT_001197d8[index].field_18 = 0;
        DAT_0019880[index] = 0x5a5a5a5a;
    }
    for (index = 0; index < 40; index++) {
        DAT_00192a4[index].field_00 = 0;
        DAT_00192a4[index].center = 0;
        DAT_00192a4[index].field_08 = 0;
        DAT_00192a4[index].divisor = 0;
        DAT_00192a4[index].port = 0;
        set_bits_float(&DAT_00192a4[index].scale_positive, 0);
        set_bits_float(&DAT_00192a4[index].scale_negative, 0);
    }
    for (index = 0; index < 10; index++) {
        DAT_0019704[index] = 0;
    }
    for (index = 0; index < 4; index++) {
        button_state[index] = 0;
    }
    axis_calls = 0;
    axis_mask = 0;
    axis_port = 0;
    axis_result = 0;
    button_calls = 0;
    button_port = 0;
    button_result = 0;
}

static void set_control(int index, int type, int kind, int mode, int axis,
                        int device, int field_18)
{
    DAT_001197d8[index].type = type;
    DAT_001197d8[index].kind = kind;
    DAT_001197d8[index].mode = mode;
    DAT_001197d8[index].axis = axis;
    DAT_001197d8[index].device = device;
    DAT_001197d8[index].field_14 = 0;
    DAT_001197d8[index].field_18 = field_18;
}

static void set_axis(int index, int center, int divisor, int port,
                     unsigned int positive_bits, unsigned int negative_bits)
{
    DAT_00192a4[index].field_00 = 0;
    DAT_00192a4[index].center = center;
    DAT_00192a4[index].field_08 = 0;
    DAT_00192a4[index].divisor = divisor;
    DAT_00192a4[index].port = port;
    set_bits_float(&DAT_00192a4[index].scale_positive, positive_bits);
    set_bits_float(&DAT_00192a4[index].scale_negative, negative_bits);
}

static int half_bits(void)
{
    return 0x3f000000;
}

int main(void)
{
    reset();
    set_control(0, 1, 1, 0, 0, 0, 0);
    set_axis(0, 100, 10, 0x201, half_bits(), half_bits());
    axis_result = 4000;
    joystick_update();
    put(DAT_0019880[0] == 150 && axis_calls == 1 && axis_mask == 1
        && axis_port == 0x201 ? '0' : 'x');

    reset();
    set_control(0, 1, 1, 0, 0, 0, 0);
    set_axis(0, 701, 10, 0x201, half_bits(), half_bits());
    axis_result = 3000;
    joystick_update();
    put(DAT_0019880[0] == -200 && axis_calls == 1 ? '1' : 'x');

    reset();
    set_control(0, 1, 1, 0, 0, 0, 0);
    set_axis(0, 250, 10, 0x201, half_bits(), half_bits());
    axis_result = 3000;
    joystick_update();
    put(DAT_0019880[0] == 0 ? '2' : 'x');

    reset();
    set_control(0, 1, 1, 0, 0, 0, 0);
    set_axis(0, 0, 1, 0x201, 0x3f800000, 0x3f800000);
    axis_result = 4000;
    joystick_update();
    put(DAT_0019880[0] == 0x3ff ? '3' : 'x');

    reset();
    set_control(0, 1, 1, 2, 0, 0, 0);
    set_axis(0, 0, 5, 0x201, 0x3f800000, 0x3f800000);
    axis_result = 3000;
    joystick_update();
    put(DAT_0019880[0] == 0x3ff ? '4' : 'x');

    reset();
    set_control(0, 1, 1, 0, 0, 0, 0);
    set_axis(0, 2000, 1, 0x201, 0x3f800000, 0x3f800000);
    axis_result = 0;
    joystick_update();
    put(DAT_0019880[0] == -0x3ff ? '5' : 'x');

    reset();
    set_control(0, 1, 1, 0, 0, 0, 0);
    set_control(1, 1, 1, 0, 0, 0, 0);
    set_axis(0, 100, 10, 0x201, half_bits(), half_bits());
    axis_result = 4000;
    joystick_update();
    put(DAT_0019880[0] == 150 && DAT_0019880[1] == 150
        && axis_calls == 1 ? '6' : 'x');

    reset();
    set_control(0, 1, 0, 0, 1, 0, 0x0f);
    DAT_0019704[1] = 0x201;
    button_result = 0x0f;
    joystick_update();
    put(DAT_0019880[0] == 0x3ff && button_calls == 1
        && button_port == 0x201 ? '7' : 'x');

    reset();
    set_control(0, 1, 0, 0, 1, 0, 0x05);
    DAT_0019704[1] = 0x201;
    button_result = 0x0a;
    joystick_update();
    put(DAT_0019880[0] == 0 && button_calls == 1 ? '8' : 'x');

    reset();
    set_control(0, 1, 0, 3, 0, 0, (int)button_state);
    button_state[2] = 1;
    joystick_update();
    put(DAT_0019880[0] == 0x3ff ? '9' : 'x');

    reset();
    set_control(0, 1, 0, 3, 0, 0, (int)button_state);
    button_state[2] = 0;
    joystick_update();
    put(DAT_0019880[0] == 0 ? 'A' : 'x');

    reset();
    set_control(0, 0, 0, 0, 0, 0, 0);
    DAT_0019880[0] = 1234;
    joystick_update();
    put(DAT_0019880[0] == 1234 ? 'B' : 'x');

    put('D');
    quit();
    return 0;
}
