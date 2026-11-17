/* The test harness for the rebuilt `sound_queue_promote` module.
 *
 * The harness holds the sound effect queue with eight entries.  Each entry has
 * the effect index, the variant mode, the delay, and one pad field.  The reset
 * function clears every entry and sets every effect index to the terminator -1.
 * A path then fills the head and the entries that it needs, and the first
 * unfilled entry holds the terminator.
 *
 * The harness runs twelve paths:
 *
 *     0  the empty queue: the head stays the terminator
 *     1  one entry below the threshold: the head swaps with itself
 *     2  the head holds the smallest delay below the threshold: no reorder
 *     3  a later entry holds the smallest delay below the threshold: it moves
 *        to the head
 *     4  every delay is at or above the threshold: no reorder
 *     5  two entries below the threshold: the smaller delay moves to the head
 *     6  the delays 0.01, 0.011, and 0.009: only 0.009 moves to the head
 *     7  a negative delay: it moves to the head
 *     8  the pad field: the swap keeps the pad of each slot
 *     9  three equal zero delays: the strict compare keeps the head
 *     a  the head is zero and a later delay is negative: the negative delay
 *        moves to the head
 *     b  the first zero delay is not at the head: it moves to the head
 *     D  the exit marker
 *
 * The float delays arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void sound_queue_promote(void);
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

static void set_entry(int index, int effect, int mode, unsigned int bits,
    int pad)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    sound_queue[index].effect = effect;
    sound_queue[index].mode = mode;
    sound_queue[index].delay = value.f;
    sound_queue[index].pad = pad;
}

static unsigned int delay_bits(int index)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = sound_queue[index].delay;
    return value.u;
}

static int entry_is(int index, int effect, unsigned int bits)
{
    return sound_queue[index].effect == effect
        && delay_bits(index) == bits;
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
    sound_queue_promote();
    put(sound_queue[0].effect == -1 ? '0' : 'x');

    reset();
    set_entry(0, 10, 1, 0x00000000, 0x1111);
    sound_queue_promote();
    put(entry_is(0, 10, 0x00000000)
        && sound_queue[0].mode == 1
        && sound_queue[0].pad == 0x1111
        && sound_queue[1].effect == -1
        ? '1' : 'x');

    reset();
    set_entry(0, 10, 0, 0x3ba3d70a, 0);
    set_entry(1, 11, 0, 0x3f000000, 0);
    set_entry(2, 12, 0, 0x3f333333, 0);
    sound_queue_promote();
    put(entry_is(0, 10, 0x3ba3d70a)
        && entry_is(1, 11, 0x3f000000)
        && entry_is(2, 12, 0x3f333333)
        ? '2' : 'x');

    reset();
    set_entry(0, 10, 0, 0x3f000000, 0);
    set_entry(1, 11, 0, 0x3ba3d70a, 0);
    set_entry(2, 12, 0, 0x3f333333, 0);
    sound_queue_promote();
    put(entry_is(0, 11, 0x3ba3d70a)
        && entry_is(1, 10, 0x3f000000)
        && entry_is(2, 12, 0x3f333333)
        ? '3' : 'x');

    reset();
    set_entry(0, 10, 0, 0x3f000000, 0);
    set_entry(1, 11, 0, 0x3e4ccccd, 0);
    set_entry(2, 12, 0, 0x3f333333, 0);
    sound_queue_promote();
    put(entry_is(0, 10, 0x3f000000)
        && entry_is(1, 11, 0x3e4ccccd)
        && entry_is(2, 12, 0x3f333333)
        ? '4' : 'x');

    reset();
    set_entry(0, 10, 0, 0x3c1374bc, 0);
    set_entry(1, 11, 0, 0x3ba3d70a, 0);
    set_entry(2, 12, 0, 0x3f333333, 0);
    sound_queue_promote();
    put(entry_is(0, 11, 0x3ba3d70a)
        && entry_is(1, 10, 0x3c1374bc)
        && entry_is(2, 12, 0x3f333333)
        ? '5' : 'x');

    reset();
    set_entry(0, 10, 0, 0x3c23d70a, 0);
    set_entry(1, 11, 0, 0x3c343958, 0);
    set_entry(2, 12, 0, 0x3c1374bc, 0);
    sound_queue_promote();
    put(entry_is(0, 12, 0x3c1374bc)
        && entry_is(1, 11, 0x3c343958)
        && entry_is(2, 10, 0x3c23d70a)
        ? '6' : 'x');

    reset();
    set_entry(0, 10, 0, 0x3f000000, 0);
    set_entry(1, 11, 0, 0xbf800000, 0);
    set_entry(2, 12, 0, 0x3e4ccccd, 0);
    sound_queue_promote();
    put(entry_is(0, 11, 0xbf800000)
        && entry_is(1, 10, 0x3f000000)
        && entry_is(2, 12, 0x3e4ccccd)
        ? '7' : 'x');

    reset();
    set_entry(0, 10, 0, 0x3f000000, 0x1111);
    set_entry(1, 11, 0, 0x3ba3d70a, 0x2222);
    sound_queue_promote();
    put(entry_is(0, 11, 0x3ba3d70a)
        && sound_queue[0].pad == 0x1111
        && entry_is(1, 10, 0x3f000000)
        && sound_queue[1].pad == 0x2222
        ? '8' : 'x');

    reset();
    set_entry(0, 10, 0, 0x00000000, 0);
    set_entry(1, 11, 0, 0x00000000, 0);
    set_entry(2, 12, 0, 0x00000000, 0);
    sound_queue_promote();
    put(entry_is(0, 10, 0x00000000)
        && entry_is(1, 11, 0x00000000)
        && entry_is(2, 12, 0x00000000)
        ? '9' : 'x');

    reset();
    set_entry(0, 10, 0, 0x00000000, 0);
    set_entry(1, 11, 0, 0xbf800000, 0);
    sound_queue_promote();
    put(entry_is(0, 11, 0xbf800000)
        && entry_is(1, 10, 0x00000000)
        ? 'a' : 'x');

    reset();
    set_entry(0, 10, 0, 0x3f000000, 0);
    set_entry(1, 11, 0, 0x3e4ccccd, 0);
    set_entry(2, 12, 0, 0x00000000, 0);
    set_entry(3, 13, 0, 0x00000000, 0);
    sound_queue_promote();
    put(entry_is(0, 12, 0x00000000)
        && entry_is(1, 11, 0x3e4ccccd)
        && entry_is(2, 10, 0x3f000000)
        && entry_is(3, 13, 0x00000000)
        ? 'b' : 'x');

    put('D');
    quit();
    return 0;
}
