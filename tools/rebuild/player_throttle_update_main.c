/* The test harness for the rebuilt `player_throttle_update` module.
 *
 * The harness holds one player object, the throttle axis and its centre sign,
 * the control byte, and the 1023.0 scale.  The object holds the throttle at
 * +0x144, the four state fields at +0x4f4, +0x148, +0x110, and +0x170.
 *
 * The harness runs eleven paths:
 *
 *     0  the axis 1023 and the centre 1 take the direct path, so the throttle
 *        is 1.0
 *     1  the axis -1023 and the centre -1 keep the sign match, so the absolute
 *        axis gives the throttle 1.0
 *     2  the axis 0 keeps the sign match, so the throttle is 0.0
 *     3  the throttle -1.0 is negative, so the throttle clears
 *     4  the field +0x4f4 is 1, so the throttle clears
 *     5  the centre -1 differs from the axis sign 1, so the throttle clears
 *     6  the field +0x148 is 1, so the throttle clears
 *     7  the field +0x110 is 1, so the throttle clears
 *     8  the field +0x170 is 1, so the throttle clears
 *     9  the control bit 0x40 is set, so the throttle clears
 *     A  the fields +0x4f4 and +0x110 hold the sign bit only and the control
 *        byte holds 0xbf, so the throttle is 1.0
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void player_throttle_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_00100710;
int DAT_00119880;
int DAT_001197ec;
unsigned char DAT_00118e1d;

static char text[8];
static char object[0x600];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_float(float *p, unsigned int bits)
{
    *(unsigned int *)p = bits;
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static void set_float(int offset, unsigned int bits)
{
    set_bits_float((float *)(object + offset), bits);
}

static unsigned int get_float(int offset)
{
    return *(unsigned int *)(object + offset);
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x600; index++) {
        object[index] = 0;
    }
    DAT_00119880 = 0;
    DAT_001197ec = 0;
    DAT_00118e1d = 0;
    set_bits_float(&DAT_00100710, 0x447fc000);
}

int main(void)
{
    int ok;

    reset();
    DAT_00119880 = 1023;
    DAT_001197ec = 1;
    player_throttle_update(object);
    ok = get_float(0x144) == 0x3f800000;
    put(ok ? '0' : 'x');

    reset();
    DAT_00119880 = -1023;
    DAT_001197ec = -1;
    player_throttle_update(object);
    ok = get_float(0x144) == 0x3f800000;
    put(ok ? '1' : 'x');

    reset();
    DAT_00119880 = 0;
    DAT_001197ec = 1;
    player_throttle_update(object);
    ok = get_float(0x144) == 0x00000000;
    put(ok ? '2' : 'x');

    reset();
    DAT_00119880 = 1023;
    DAT_001197ec = 1;
    set_float(0x144, 0xbf800000);
    player_throttle_update(object);
    ok = get_float(0x144) == 0x00000000;
    put(ok ? '3' : 'x');

    reset();
    DAT_00119880 = 1023;
    DAT_001197ec = 1;
    set_int(0x4f4, 1);
    player_throttle_update(object);
    ok = get_float(0x144) == 0x00000000;
    put(ok ? '4' : 'x');

    reset();
    DAT_00119880 = 1023;
    DAT_001197ec = -1;
    player_throttle_update(object);
    ok = get_float(0x144) == 0x00000000;
    put(ok ? '5' : 'x');

    reset();
    DAT_00119880 = 1023;
    DAT_001197ec = 1;
    set_int(0x148, 1);
    player_throttle_update(object);
    ok = get_float(0x144) == 0x00000000;
    put(ok ? '6' : 'x');

    reset();
    DAT_00119880 = 1023;
    DAT_001197ec = 1;
    set_int(0x110, 1);
    player_throttle_update(object);
    ok = get_float(0x144) == 0x00000000;
    put(ok ? '7' : 'x');

    reset();
    DAT_00119880 = 1023;
    DAT_001197ec = 1;
    set_int(0x170, 1);
    player_throttle_update(object);
    ok = get_float(0x144) == 0x00000000;
    put(ok ? '8' : 'x');

    reset();
    DAT_00119880 = 1023;
    DAT_001197ec = 1;
    DAT_00118e1d = 0x40;
    player_throttle_update(object);
    ok = get_float(0x144) == 0x00000000;
    put(ok ? '9' : 'x');

    reset();
    DAT_00119880 = 1023;
    DAT_001197ec = 1;
    DAT_00118e1d = 0xbf;
    set_int(0x4f4, (int)0x80000000u);
    set_int(0x110, (int)0x80000000u);
    player_throttle_update(object);
    ok = get_float(0x144) == 0x3f800000;
    put(ok ? 'A' : 'x');

    put('D');
    quit();
    return 0;
}
