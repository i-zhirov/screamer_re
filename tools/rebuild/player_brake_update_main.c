/* The test harness for the rebuilt `player_brake_update` module.
 *
 * The harness holds one player object, the pedal axis and its centre sign, the
 * two mode cells, the 1023.0 scale, the 0.1 state limit, the frame delta, and
 * the filter stub.  The stub records its arguments and writes the target value
 * back to the cell.
 *
 * The harness runs nine paths:
 *
 *     0  the axis 1023 takes the direct path, so the brake value is 1.0, the
 *        road speed falls from 10.0 to 8.0, and the deceleration flag +0x110
 *        takes the brake value
 *     1  the mode cell 0x197fc is non-zero, so the filter path sets the brake
 *        value and the filter receives the index 0x52
 *     2  the flag cell 0x197f8 is not 1, so the filter path runs again
 *     3  the axis sign differs from the centre, so the brake value clears and
 *        the road speed stays
 *     4  the axis is zero, so the brake value clears even with a matching sign
 *     5  the state +0x128 is 0.5, so the tail skips and the road speed stays
 *     6  the brake change drives the road speed below zero, so the clamp sets
 *        zero and the deceleration flag takes the brake value
 *     7  the ramp flag +0x2f0 is set, so the ramp flag +0x170 sets and the
 *        deceleration flag stays
 *     8  the ramp flag +0x170 is already set, so no flag changes
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void player_brake_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00119884;
int DAT_00119808;
int DAT_001197fc;
int DAT_001197f8;
float DAT_00100714;
double DAT_00100718;
float DAT_00118e04;

static char text[8];
static char object[0x600];

static int filter_calls;
static unsigned int filter_cell;
static unsigned int filter_target;
static int filter_index;

void filter_update(float *value, float target, int index)
{
    filter_cell = (unsigned int)value;
    filter_target = *(unsigned int *)&target;
    filter_index = index;
    filter_calls = filter_calls + 1;
    *value = target;
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

static void set_bits_float(float *p, unsigned int bits)
{
    *(unsigned int *)p = bits;
}

static void set_bits_double(double *p, unsigned int low, unsigned int high)
{
    unsigned int *words;

    words = (unsigned int *)p;
    words[0] = low;
    words[1] = high;
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
    DAT_00119884 = 0;
    DAT_00119808 = 0;
    DAT_001197fc = 0;
    DAT_001197f8 = 0;
    set_bits_float(&DAT_00100714, 0x447fc000);
    set_bits_double(&DAT_00100718, 0x9999999a, 0x3fb99999);
    set_bits_float(&DAT_00118e04, 0x3f800000);
    filter_calls = 0;
    filter_cell = 0;
    filter_target = 0;
    filter_index = 0;
}

int main(void)
{
    int ok;

    reset();
    DAT_00119884 = 1023;
    DAT_00119808 = 1;
    DAT_001197f8 = 1;
    set_int(0x170, 0);
    set_int(0x2f0, 0);
    set_float(0x110, 0xdeadbeef);
    set_float(0x128, 0x00000000);
    set_float(0x17c, 0x40000000);
    set_float(0x190, 0x00000000);
    set_float(0x50c, 0x41200000);
    player_brake_update(object);
    ok = get_float(0x114) == 0x3f800000
        && get_float(0x50c) == 0x41000000
        && get_float(0x110) == 0x3f800000
        && *(int *)(object + 0x170) == 0
        && filter_calls == 0;
    put(ok ? '0' : 'x');

    reset();
    DAT_00119884 = -600;
    DAT_00119808 = -1;
    DAT_001197fc = 1;
    set_float(0x128, 0x00000000);
    set_float(0x17c, 0x40000000);
    set_float(0x50c, 0x41200000);
    player_brake_update(object);
    ok = get_float(0x114) == 0x3f800000
        && get_float(0x50c) == 0x41000000
        && filter_calls == 1
        && filter_target == 0x3f800000
        && filter_index == 0x52
        && filter_cell == (unsigned int)(object + 0x114);
    put(ok ? '1' : 'x');

    reset();
    DAT_00119884 = 100;
    DAT_00119808 = 1;
    DAT_001197f8 = 0;
    set_float(0x128, 0x00000000);
    set_float(0x50c, 0x41200000);
    player_brake_update(object);
    ok = get_float(0x114) == 0x3f800000
        && get_float(0x50c) == 0x41200000
        && filter_calls == 1
        && filter_index == 0x52;
    put(ok ? '2' : 'x');

    reset();
    DAT_00119884 = 100;
    DAT_00119808 = -1;
    set_int(0x170, 0);
    set_float(0x110, 0xdeadbeef);
    set_float(0x128, 0x00000000);
    set_float(0x17c, 0x40000000);
    set_float(0x50c, 0x41200000);
    player_brake_update(object);
    ok = get_float(0x114) == 0x00000000
        && get_float(0x50c) == 0x41200000
        && get_float(0x110) == 0xdeadbeef
        && *(int *)(object + 0x170) == 0
        && filter_calls == 0;
    put(ok ? '3' : 'x');

    reset();
    DAT_00119884 = 0;
    DAT_00119808 = 1;
    DAT_001197f8 = 1;
    set_float(0x128, 0x00000000);
    set_float(0x50c, 0x41200000);
    player_brake_update(object);
    ok = get_float(0x114) == 0x00000000
        && get_float(0x50c) == 0x41200000
        && filter_calls == 0;
    put(ok ? '4' : 'x');

    reset();
    DAT_00119884 = 1023;
    DAT_00119808 = 1;
    DAT_001197f8 = 1;
    set_int(0x170, 0);
    set_int(0x2f0, 0);
    set_float(0x110, 0xdeadbeef);
    set_float(0x128, 0x3f000000);
    set_float(0x17c, 0x40000000);
    set_float(0x50c, 0x41200000);
    player_brake_update(object);
    ok = get_float(0x114) == 0x3f800000
        && get_float(0x50c) == 0x41200000
        && get_float(0x110) == 0xdeadbeef
        && *(int *)(object + 0x170) == 0;
    put(ok ? '5' : 'x');

    reset();
    DAT_00119884 = 1023;
    DAT_00119808 = 1;
    DAT_001197f8 = 1;
    set_int(0x170, 0);
    set_int(0x2f0, 0);
    set_float(0x110, 0xdeadbeef);
    set_float(0x128, 0x00000000);
    set_float(0x17c, 0x41a00000);
    set_float(0x190, 0xbf800000);
    set_float(0x50c, 0x41200000);
    player_brake_update(object);
    ok = get_float(0x114) == 0x3f800000
        && get_float(0x50c) == 0x00000000
        && get_float(0x110) == 0x3f800000
        && *(int *)(object + 0x170) == 0;
    put(ok ? '6' : 'x');

    reset();
    DAT_00119884 = 1023;
    DAT_00119808 = 1;
    DAT_001197f8 = 1;
    set_int(0x170, 0);
    set_int(0x2f0, 1);
    set_float(0x110, 0xdeadbeef);
    set_float(0x128, 0x00000000);
    set_float(0x17c, 0x40000000);
    set_float(0x190, 0x00000000);
    set_float(0x50c, 0x41200000);
    player_brake_update(object);
    ok = get_float(0x114) == 0x3f800000
        && get_float(0x50c) == 0x41000000
        && get_float(0x110) == 0xdeadbeef
        && *(int *)(object + 0x170) == 1;
    put(ok ? '7' : 'x');

    reset();
    DAT_00119884 = 1023;
    DAT_00119808 = 1;
    DAT_001197f8 = 1;
    set_int(0x170, 1);
    set_int(0x2f0, 0);
    set_float(0x110, 0xdeadbeef);
    set_float(0x128, 0x00000000);
    set_float(0x17c, 0x40000000);
    set_float(0x190, 0x00000000);
    set_float(0x50c, 0x41200000);
    player_brake_update(object);
    ok = get_float(0x114) == 0x3f800000
        && get_float(0x50c) == 0x41000000
        && get_float(0x110) == 0xdeadbeef
        && *(int *)(object + 0x170) == 1;
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
