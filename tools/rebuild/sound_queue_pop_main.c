/* The test harness for the rebuilt `sound_queue_pop` module.
 *
 * The harness holds the status and the sound effect queue with eight entries.
 * Each entry has the effect index, the variant mode, the delay, and one pad
 * field.  The reset function clears every entry and sets every effect index to
 * the terminator -1.  A path then fills the head and the entries that it needs,
 * and the first unfilled entry holds the terminator.
 *
 * The harness runs five paths:
 *
 *     0  the empty queue: the head stays the terminator and the status is -1
 *     1  one entry before the terminator: the terminator moves to the head
 *     2  three entries: the second entry moves to the head, the third moves to
 *        the second slot, and the terminator moves to the third slot
 *     3  the pad field: the shift keeps the pad of each slot while the three
 *        head fields follow the next entry
 *     D  the exit marker
 *
 * The float delays arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void sound_queue_pop(void);
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

int DAT_00119070;
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

static int entry_is(int index, int effect, int mode, unsigned int bits)
{
    return sound_queue[index].effect == effect
        && sound_queue[index].mode == mode
        && delay_bits(index) == bits;
}

static void reset(void)
{
    int index;

    DAT_00119070 = 0;
    for (index = 0; index < 8; index++) {
        set_entry(index, -1, 0, 0, 0);
    }
}

int main(void)
{
    reset();
    sound_queue_pop();
    put(sound_queue[0].effect == -1 && DAT_00119070 == -1 ? '0' : 'x');

    reset();
    set_entry(0, 10, 1, 0x3f000000, 0x1111);
    sound_queue_pop();
    put(sound_queue[0].effect == -1 && DAT_00119070 == -1 ? '1' : 'x');

    reset();
    set_entry(0, 10, 1, 0x3f000000, 0);
    set_entry(1, 11, 2, 0x3ba3d70a, 0);
    set_entry(2, 12, 3, 0x3e4ccccd, 0);
    sound_queue_pop();
    put(entry_is(0, 11, 2, 0x3ba3d70a)
        && entry_is(1, 12, 3, 0x3e4ccccd)
        && sound_queue[2].effect == -1
        && DAT_00119070 == -1 ? '2' : 'x');

    reset();
    set_entry(0, 10, 1, 0x3f000000, 0x1111);
    set_entry(1, 11, 2, 0x3ba3d70a, 0x2222);
    set_entry(2, 12, 3, 0x3e4ccccd, 0x3333);
    sound_queue_pop();
    put(entry_is(0, 11, 2, 0x3ba3d70a)
        && sound_queue[0].pad == 0x1111
        && entry_is(1, 12, 3, 0x3e4ccccd)
        && sound_queue[1].pad == 0x2222
        && sound_queue[2].pad == 0x3333
        && DAT_00119070 == -1 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
