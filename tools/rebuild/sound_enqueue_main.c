/* The test harness for the rebuilt `sound_enqueue` module.
 *
 * The harness holds the two gates, the sound effect queue with eight entries,
 * and the effect, mode, delay, and pad fields of each entry.  The reset function
 * fills every entry and sets every effect index to the terminator -1.
 *
 * The harness runs these paths:
 *
 *     0  the gate DAT_001d2c8 is clear: no entry changes
 *     1  the gate DAT_00008058 is clear: no entry changes
 *     2  the empty queue: the effect lands at the head, the next effect is -1,
 *        and the pad field keeps its value
 *     3  the same effect at the head: no entry changes
 *     4  the same effect in the middle: no entry changes
 *     5  one entry before the terminator: the effect lands in the second slot
 *     6  two entries before the terminator: the effect lands in the third slot
 *     7  the inserted slot: the pad field keeps its value
 *     8  the delay bits 0x3f333333 and the mode 3: both fields are copied
 *     9  the effect 0 and the mode -5: zero is not the terminator
 *     D  the exit marker
 *
 * The float delays arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void sound_enqueue(int effect, int mode, float delay);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct sound_queue_entry {
    int effect;
    int mode;
    float delay;
    int pad;
};

int DAT_001d2c8;
int DAT_00008058;
struct sound_queue_entry sound_queue[8];

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

static float float_from_bits(unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    return value.f;
}

static unsigned int float_bits(float value)
{
    union {
        float f;
        unsigned int u;
    } converted;

    converted.f = value;
    return converted.u;
}

static void set_entry(int index, int effect, int mode, unsigned int bits,
    int pad)
{
    sound_queue[index].effect = effect;
    sound_queue[index].mode = mode;
    sound_queue[index].delay = float_from_bits(bits);
    sound_queue[index].pad = pad;
}

static int entry_is(int index, int effect, int mode, unsigned int bits)
{
    return sound_queue[index].effect == effect
        && sound_queue[index].mode == mode
        && float_bits(sound_queue[index].delay) == bits;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 8; index++) {
        set_entry(index, -1, 0, 0, 0);
    }
}

int main(void)
{
    reset();
    set_entry(0, 5, 1, 0x3f000000, 0x1111);
    DAT_001d2c8 = 0;
    DAT_00008058 = 1;
    sound_enqueue(9, 3, float_from_bits(0x3f000000));
    put(entry_is(0, 5, 1, 0x3f000000)
        && sound_queue[0].pad == 0x1111
        && sound_queue[1].effect == -1
        ? '0' : 'x');

    reset();
    set_entry(0, 5, 1, 0x3f000000, 0x1111);
    DAT_001d2c8 = 1;
    DAT_00008058 = 0;
    sound_enqueue(9, 3, float_from_bits(0x3f000000));
    put(entry_is(0, 5, 1, 0x3f000000)
        && sound_queue[0].pad == 0x1111
        && sound_queue[1].effect == -1
        ? '1' : 'x');

    reset();
    sound_queue[0].pad = 0xA5A5;
    set_entry(1, 42, 0, 0, 0);
    DAT_001d2c8 = 1;
    DAT_00008058 = 1;
    sound_enqueue(9, 3, float_from_bits(0x3f000000));
    put(entry_is(0, 9, 3, 0x3f000000)
        && sound_queue[0].pad == 0xA5A5
        && sound_queue[1].effect == -1
        ? '2' : 'x');

    reset();
    set_entry(0, 9, 3, 0x3f000000, 0x1111);
    DAT_001d2c8 = 1;
    DAT_00008058 = 1;
    sound_enqueue(9, 7, float_from_bits(0x3e800000));
    put(entry_is(0, 9, 3, 0x3f000000)
        && sound_queue[0].pad == 0x1111
        && sound_queue[1].effect == -1
        ? '3' : 'x');

    reset();
    set_entry(0, 1, 0, 0x3e800000, 0);
    set_entry(1, 9, 3, 0x3f000000, 0);
    DAT_001d2c8 = 1;
    DAT_00008058 = 1;
    sound_enqueue(9, 7, float_from_bits(0x3e800000));
    put(entry_is(0, 1, 0, 0x3e800000)
        && entry_is(1, 9, 3, 0x3f000000)
        && sound_queue[2].effect == -1
        ? '4' : 'x');

    reset();
    set_entry(0, 1, 2, 0x3dcccccd, 0x1111);
    DAT_001d2c8 = 1;
    DAT_00008058 = 1;
    sound_enqueue(9, 3, float_from_bits(0x3f000000));
    put(entry_is(0, 1, 2, 0x3dcccccd)
        && sound_queue[0].pad == 0x1111
        && entry_is(1, 9, 3, 0x3f000000)
        && sound_queue[2].effect == -1
        ? '5' : 'x');

    reset();
    set_entry(0, 1, 0, 0x3e800000, 0);
    set_entry(1, 2, 0, 0x3f000000, 0);
    DAT_001d2c8 = 1;
    DAT_00008058 = 1;
    sound_enqueue(9, 3, float_from_bits(0x3f000000));
    put(entry_is(0, 1, 0, 0x3e800000)
        && entry_is(1, 2, 0, 0x3f000000)
        && entry_is(2, 9, 3, 0x3f000000)
        && sound_queue[3].effect == -1
        ? '6' : 'x');

    reset();
    set_entry(0, 1, 0, 0x3e800000, 0);
    set_entry(1, -1, 0, 0, 0xBEEF);
    DAT_001d2c8 = 1;
    DAT_00008058 = 1;
    sound_enqueue(9, 3, float_from_bits(0x3f000000));
    put(entry_is(1, 9, 3, 0x3f000000)
        && sound_queue[1].pad == 0xBEEF
        && sound_queue[2].effect == -1
        ? '7' : 'x');

    reset();
    DAT_001d2c8 = 1;
    DAT_00008058 = 1;
    sound_enqueue(0x19, 3, float_from_bits(0x3f333333));
    put(entry_is(0, 0x19, 3, 0x3f333333)
        && sound_queue[1].effect == -1
        ? '8' : 'x');

    reset();
    DAT_001d2c8 = 1;
    DAT_00008058 = 1;
    sound_enqueue(0, -5, float_from_bits(0x00000000));
    put(entry_is(0, 0, -5, 0x00000000)
        && sound_queue[1].effect == -1
        ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
