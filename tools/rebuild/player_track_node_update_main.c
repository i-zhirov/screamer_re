/* The test harness for the rebuilt `player_track_node_update` module.
 *
 * The harness holds the object record, the four track nodes, the node count,
 * and the scale 0.05.  The `sqrt` stub lives in the local FPU assembly file,
 * because a C body needs literals that the linker reads at a low address.  The
 * harness runs ten paths:
 *
 *     0  the object sits on node 0: the current node takes the node value, the
 *        surface grip, and the flags, and the status is 1
 *     1  the second chain holds node 0: the status is 2
 *     2  the visited chain holds node 1: the status is 2
 *     3  the visited chain is clear at node 1: the status is 1
 *     4  the object sits on the node before the current index: the scan takes
 *        the previous node
 *     5  the object sits on the node after the current index: the scan takes
 *        the next node
 *     6  the object sits between nodes 2 and 3 with no radius hit: the target
 *        index sets to -1 and the status stays
 *     7  the current node is 3: the next index wraps to 0 and the ratio is 0
 *     8  the object sits one segment past node 1: the ratio is 1
 *     9  the object sits half a segment past node 2: the truncated ratio is
 *        0.4
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#define NODE_COUNT 4

struct track_node {
    int radius;
    int value;
    int reserved08;
    float data;
    int flags;
    int x;
    int reserved18;
    int y;
};

void player_track_node_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct track_node track_node_table[NODE_COUNT];
int track_node_count;
double track_node_scale;

static char text[8];
static unsigned char object[0x900];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int get_int(int offset)
{
    return *(int *)(object + offset);
}

static void set_float_bits(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *(float *)(object + offset) = v.f;
}

static unsigned int get_float_bits(int offset)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = *(float *)(object + offset);
    return v.u;
}

static void set_double_bits(double *value, unsigned int low, unsigned int high)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.u[0] = low;
    v.u[1] = high;
    *value = v.d;
}

static void set_node(
    int index, int x, int y, int radius, int value, unsigned int data_bits,
    int flags)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = data_bits;
    track_node_table[index].radius = radius;
    track_node_table[index].value = value;
    track_node_table[index].reserved08 = 0;
    track_node_table[index].data = v.f;
    track_node_table[index].flags = flags;
    track_node_table[index].x = x;
    track_node_table[index].reserved18 = 0;
    track_node_table[index].y = y;
}

static void set_radius(int index, int radius)
{
    track_node_table[index].radius = radius;
}

static void prepare(int index, int x, int y)
{
    int i;

    for (i = 0; i < 0x900; i++) {
        object[i] = 0;
    }
    set_int(0x598, index);
    set_int(0x780, 0x55);
    set_int(0x668, 0x12345678);
    set_int(0x66c, 0x1111);
    set_int(0x678, 0x2222);
    set_int(0x4b0, x);
    set_int(0x4b8, y);
}

static void set_visited(int index)
{
    *(int *)(object + 0x59c + index * 4) = 1;
}

static void set_second(int index)
{
    *(int *)(object + 0x5a0 + index * 4) = 1;
}

static int fields_are(int index, int value, unsigned int data_bits, int flags)
{
    return get_int(0x598) == index
        && get_int(0x668) == index
        && get_int(0x66c) == value
        && get_float_bits(0x670) == data_bits
        && get_int(0x678) == flags
        && get_int(0x58c) == 0x1111
        && get_int(0x590) == 0x2222;
}

int main(void)
{
    set_double_bits(&track_node_scale, 0x9999999a, 0x3fa99999);

    set_node(0, 0, 0, 0, 11, 0x3fc00000, 0);
    set_node(1, 60, 80, 0, 22, 0x40200000, 2);
    set_node(2, 120, 160, 0, 33, 0x40600000, 4);
    set_node(3, 180, 240, 0, 44, 0x40900000, 8);
    track_node_count = NODE_COUNT;

    prepare(0, 0, 0);
    set_radius(0, 1000);
    player_track_node_update(object);
    put(get_int(0x780) == 1 && fields_are(0, 11, 0x3fc00000, 0)
            && get_float_bits(0x67c) == 0x00000000
        ? '0' : 'x');
    set_radius(0, 0);

    prepare(0, 0, 0);
    set_radius(0, 1000);
    set_second(0);
    player_track_node_update(object);
    put(get_int(0x780) == 2 && fields_are(0, 11, 0x3fc00000, 0)
            && get_float_bits(0x67c) == 0x00000000
        ? '1' : 'x');
    set_radius(0, 0);

    prepare(1, 60, 80);
    set_radius(1, 1000);
    set_visited(1);
    player_track_node_update(object);
    put(get_int(0x780) == 2 && fields_are(1, 22, 0x40200000, 2)
            && get_float_bits(0x67c) == 0x00000000
        ? '2' : 'x');
    set_radius(1, 0);

    prepare(1, 60, 80);
    set_radius(1, 1000);
    player_track_node_update(object);
    put(get_int(0x780) == 1 && fields_are(1, 22, 0x40200000, 2)
            && get_float_bits(0x67c) == 0x00000000
        ? '3' : 'x');
    set_radius(1, 0);

    prepare(1, 0, 0);
    set_radius(0, 1000);
    player_track_node_update(object);
    put(get_int(0x780) == 1 && get_int(0x598) == 0
            && get_int(0x668) == 0 && get_int(0x66c) == 11
            && get_float_bits(0x670) == 0x3fc00000
            && get_int(0x678) == 0 && get_int(0x58c) == 11
            && get_int(0x590) == 0
            && get_float_bits(0x67c) == 0x00000000
        ? '4' : 'x');
    set_radius(0, 0);

    prepare(0, 60, 80);
    set_radius(1, 1000);
    player_track_node_update(object);
    put(get_int(0x780) == 1 && fields_are(1, 22, 0x40200000, 2)
            && get_float_bits(0x67c) == 0x00000000
        ? '5' : 'x');
    set_radius(1, 0);

    prepare(2, 150, 200);
    player_track_node_update(object);
    put(get_int(0x780) == 0x55 && get_int(0x668) == -1
            && get_int(0x598) == 2 && get_int(0x66c) == 0x1111
            && get_int(0x678) == 0x2222
            && get_float_bits(0x67c) == 0x3ecccccd
        ? '6' : 'x');

    prepare(3, 180, 240);
    set_radius(3, 1000);
    player_track_node_update(object);
    put(get_int(0x780) == 1 && fields_are(3, 44, 0x40900000, 8)
            && get_float_bits(0x67c) == 0x00000000
        ? '7' : 'x');
    set_radius(3, 0);

    prepare(1, 120, 160);
    set_radius(1, 1000);
    player_track_node_update(object);
    put(get_int(0x780) == 1 && fields_are(1, 22, 0x40200000, 2)
            && get_float_bits(0x67c) == 0x3f800000
        ? '8' : 'x');
    set_radius(1, 0);

    prepare(2, 150, 200);
    set_radius(2, 1000);
    player_track_node_update(object);
    put(get_int(0x780) == 1 && fields_are(2, 33, 0x40600000, 4)
            && get_float_bits(0x67c) == 0x3ecccccd
        ? '9' : 'x');
    set_radius(2, 0);

    put('D');
    quit();
    return 0;
}
