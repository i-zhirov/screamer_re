/* The test harness for the rebuilt `object_drive_step` module.
 *
 * The harness holds the object record, the drive-mode global, and the frame
 * delta global.  It runs eight paths:
 *
 *     0  the force mode is active and the speed stays below the target after
 *        the acceleration
 *     1  the acceleration reaches past the target, so the speed clamps to it
 *     2  the speed is above the target and the bound, so the brake flag sets
 *        and the deceleration stays above the target
 *     3  a negative throttle puts the bound above the target, so the speed
 *        decelerates without a brake set
 *     4  the deceleration reaches below the target, so the speed clamps to it
 *     5  the speed equals the target, so the function keeps the speed and the
 *        brake flag
 *     6  the force mode is inactive and the speed accelerates to the target
 *     7  the force mode is inactive and the speed decelerates but clamps to
 *        the target
 *     D  the exit marker
 *
 * The float values and the expected values are set from their exact bit
 * patterns, so the harness does not depend on a float literal or on the
 * constant segment.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_drive_step(char *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];

int DAT_0000ffa4;
float DAT_00118e04;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_float(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *(float *)(object + offset) = v.f;
}

static void set_bits_global(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int float_is(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } x;
    union {
        float f;
        unsigned int u;
    } y;

    x.f = *(float *)(object + offset);
    y.u = bits;
    return x.u == y.u;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x500; index++) {
        object[index] = 0;
    }
    DAT_0000ffa4 = 0;
    set_bits_global(&DAT_00118e04, 0x3f000000);
}

static void setup(unsigned int force, unsigned int drag,
                  unsigned int throttle, unsigned int accel,
                  unsigned int decel, int mode, int state,
                  unsigned int speed, int brake)
{
    reset();
    DAT_0000ffa4 = mode;
    set_int(0x410, state);
    set_bits_float(0x2e4, force);
    set_bits_float(0x2f0, drag);
    set_bits_float(0x3fc, throttle);
    set_bits_float(0x2e8, accel);
    set_bits_float(0x2ec, decel);
    set_bits_float(0x418, speed);
    set_int(0x2c0, brake);
}

static int result(int brake)
{
    return *(int *)(object + 0x2c0) == brake;
}

int main(void)
{
    setup(0x40000000, 0x3f800000, 0x40400000, 0x3f800000,
          0x40800000, 0, 10, 0x3f800000, 0x1111);
    object_drive_step((char *)object);
    put(float_is(0x418, 0x3fc00000) && result(0) ? '0' : 'x');

    setup(0x40000000, 0x3f800000, 0x40400000, 0x3f800000,
          0x40800000, 0, 10, 0x410c0000, 0x1111);
    object_drive_step((char *)object);
    put(float_is(0x418, 0x41100000) && result(0) ? '1' : 'x');

    setup(0x40000000, 0x3f800000, 0x40400000, 0x3f800000,
          0x3f800000, 0, 10, 0x41200000, 0x1111);
    object_drive_step((char *)object);
    put(float_is(0x418, 0x41180000) && result(1) ? '2' : 'x');

    setup(0x40000000, 0x3f800000, 0xc0400000, 0x3f800000,
          0x3f800000, 0, 10, 0xc1000000, 0x2222);
    object_drive_step((char *)object);
    put(float_is(0x418, 0xc1080000) && result(0x2222) ? '3' : 'x');

    setup(0x40000000, 0x3f800000, 0x40400000, 0x3f800000,
          0x40800000, 0, 10, 0x41140000, 0x3333);
    object_drive_step((char *)object);
    put(float_is(0x418, 0x41100000) && result(1) ? '4' : 'x');

    setup(0x40000000, 0x3f800000, 0x40400000, 0x3f800000,
          0x40800000, 0, 10, 0x41100000, 0x12345678);
    object_drive_step((char *)object);
    put(float_is(0x418, 0x41100000) && result(0x12345678) ? '5' : 'x');

    setup(0x40000000, 0x3f800000, 0x40400000, 0x3f800000,
          0x40800000, 10, 5, 0x3f800000, 0x4444);
    object_drive_step((char *)object);
    put(float_is(0x418, 0x3fc00000) && result(0) ? '6' : 'x');

    setup(0x40000000, 0x3f800000, 0x40400000, 0x3f800000,
          0x40800000, 10, 5, 0x40e00000, 0x5555);
    object_drive_step((char *)object);
    put(float_is(0x418, 0x40c00000) && result(1) ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
