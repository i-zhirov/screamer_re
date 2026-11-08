/* The test harness for the rebuilt `player_segment_apply` module.
 *
 * The harness holds one player object with the four 0x2c-byte slots at `+0x54`
 * through `+0x104`, the two lateral terms `+0x44` and `+0x4c`, the angle
 * `+0x50`, the two positions `+0x4b0` and `+0x4b8`, and the callee stub.  The
 * stub records its call and the self pointer.
 *
 * The harness runs six paths:
 *
 *     0  the flag +0x40 is clear: the function returns and changes nothing
 *     1  the flag is set and every slot is inactive: the two terms clear and
 *        the angle stays, and the callee runs
 *     2  one active slot: one lateral term takes the slot value and the angle
 *     3  three active slots: the larger absolute x and y terms win
 *        independently and the last angle stays
 *     4  two active slots with opposite equal x terms: the tie keeps the first
 *     5  an inactive big slot and an active negative slot: the active slot wins
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.  The angle checks compare the bit patterns exactly.
 */
#pragma off (check_stack)

void player_segment_apply(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define SLOT_BASE 0x54
#define SLOT_STRIDE 0x2c
#define SLOT_COUNT 4

static char text[8];
static char object[0x600];
static int wheel_calls;
static char *wheel_self;

void wheel_contact_update(char *self)
{
    wheel_calls = wheel_calls + 1;
    wheel_self = self;
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

static void clear_bytes(char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static char *slot_at(int index)
{
    return object + SLOT_BASE + index * SLOT_STRIDE;
}

static void set_int(char *base, int offset, int value)
{
    *(int *)(base + offset) = value;
}

static int get_int(char *base, int offset)
{
    return *(int *)(base + offset);
}

static void set_bits(char *base, int offset, unsigned int bits)
{
    *(unsigned int *)(base + offset) = bits;
}

static unsigned int get_bits(char *base, int offset)
{
    return *(unsigned int *)(base + offset);
}

static void reset(void)
{
    clear_bytes(object, 0x600);
    wheel_calls = 0;
    wheel_self = 0;
}

int main(void)
{
    int ok;

    reset();
    set_int(object, 0x40, 0);
    set_int(object, 0x44, 0x1111);
    set_int(object, 0x4c, 0x2222);
    set_bits(object, 0x50, 0x3f000000);
    set_int(object, 0x4b0, 100);
    set_int(object, 0x4b8, 200);
    set_int(slot_at(0), 0x18, 1);
    set_int(slot_at(0), 0x1c, 500);
    set_int(slot_at(0), 0x24, 600);
    set_bits(slot_at(0), 0x28, 0x40800000);
    player_segment_apply(object);
    ok = wheel_calls == 0
        && get_int(object, 0x44) == 0x1111
        && get_int(object, 0x4c) == 0x2222
        && get_bits(object, 0x50) == 0x3f000000
        && get_int(object, 0x4b0) == 100
        && get_int(object, 0x4b8) == 200;
    put(ok ? '0' : 'x');

    reset();
    set_int(object, 0x40, 1);
    set_int(object, 0x44, 0x1234);
    set_int(object, 0x4c, 0x5678);
    set_bits(object, 0x50, 0x3f000000);
    set_int(object, 0x4b0, 100);
    set_int(object, 0x4b8, 200);
    player_segment_apply(object);
    ok = wheel_calls == 1
        && wheel_self == object
        && get_int(object, 0x44) == 0
        && get_int(object, 0x4c) == 0
        && get_bits(object, 0x50) == 0x3f000000
        && get_int(object, 0x4b0) == 100
        && get_int(object, 0x4b8) == 200;
    put(ok ? '1' : 'x');

    reset();
    set_int(object, 0x40, 1);
    set_int(object, 0x4b0, 100);
    set_int(object, 0x4b8, 200);
    set_int(slot_at(0), 0x18, 1);
    set_int(slot_at(0), 0x1c, 50);
    set_int(slot_at(0), 0x24, 0);
    set_bits(slot_at(0), 0x28, 0x3f800000);
    player_segment_apply(object);
    ok = wheel_calls == 1
        && get_int(object, 0x44) == 50
        && get_int(object, 0x4c) == 0
        && get_bits(object, 0x50) == 0x3f800000
        && get_int(object, 0x4b0) == 150
        && get_int(object, 0x4b8) == 200;
    put(ok ? '2' : 'x');

    reset();
    set_int(object, 0x40, 1);
    set_int(object, 0x4b0, 100);
    set_int(object, 0x4b8, 200);
    set_int(slot_at(0), 0x18, 1);
    set_int(slot_at(0), 0x1c, 10);
    set_int(slot_at(0), 0x24, -20);
    set_bits(slot_at(0), 0x28, 0x40000000);
    set_int(slot_at(1), 0x18, 1);
    set_int(slot_at(1), 0x1c, -40);
    set_int(slot_at(1), 0x24, 5);
    set_bits(slot_at(1), 0x28, 0x40400000);
    set_int(slot_at(2), 0x18, 0);
    set_int(slot_at(2), 0x1c, 999);
    set_int(slot_at(2), 0x24, 999);
    set_bits(slot_at(2), 0x28, 0x40800000);
    set_int(slot_at(3), 0x18, 1);
    set_int(slot_at(3), 0x1c, 25);
    set_int(slot_at(3), 0x24, -30);
    set_bits(slot_at(3), 0x28, 0x40a00000);
    player_segment_apply(object);
    ok = wheel_calls == 1
        && get_int(object, 0x44) == -40
        && get_int(object, 0x4c) == -30
        && get_bits(object, 0x50) == 0x40a00000
        && get_int(object, 0x4b0) == 60
        && get_int(object, 0x4b8) == 170;
    put(ok ? '3' : 'x');

    reset();
    set_int(object, 0x40, 1);
    set_int(slot_at(0), 0x18, 1);
    set_int(slot_at(0), 0x1c, 50);
    set_int(slot_at(0), 0x24, 0);
    set_bits(slot_at(0), 0x28, 0x3f800000);
    set_int(slot_at(1), 0x18, 1);
    set_int(slot_at(1), 0x1c, -50);
    set_int(slot_at(1), 0x24, 0);
    set_bits(slot_at(1), 0x28, 0x40000000);
    player_segment_apply(object);
    ok = wheel_calls == 1
        && get_int(object, 0x44) == 50
        && get_int(object, 0x4c) == 0
        && get_bits(object, 0x50) == 0x3f800000
        && get_int(object, 0x4b0) == 50
        && get_int(object, 0x4b8) == 0;
    put(ok ? '4' : 'x');

    reset();
    set_int(object, 0x40, 1);
    set_int(slot_at(0), 0x18, 0);
    set_int(slot_at(0), 0x1c, 1000);
    set_int(slot_at(0), 0x24, 1000);
    set_bits(slot_at(0), 0x28, 0x40c00000);
    set_int(slot_at(1), 0x18, 1);
    set_int(slot_at(1), 0x1c, -7);
    set_int(slot_at(1), 0x24, 8);
    set_bits(slot_at(1), 0x28, 0x40e00000);
    player_segment_apply(object);
    ok = wheel_calls == 1
        && get_int(object, 0x44) == -7
        && get_int(object, 0x4c) == 8
        && get_bits(object, 0x50) == 0x40e00000
        && get_int(object, 0x4b0) == -7
        && get_int(object, 0x4b8) == 8;
    put(ok ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
