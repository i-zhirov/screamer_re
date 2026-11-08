/* The test harness for the rebuilt `track_progress_update` module.
 *
 * The harness holds the object record, the four track nodes, the node count,
 * and the scale 0.05.  The `sqrt` stub lives in the local FPU assembly file,
 * because a C body needs literals that the linker reads at a low address.  The
 * harness runs ten paths:
 *
 *     0  the object sits on node 0: the first node marks and the progress is 0
 *     1  the object sits on node 1 after node 0: the node marks and the
 *        progress is 1
 *     2  the object sits on node 2 after nodes 0 and 1: the progress is 2
 *     3  the object sits on node 3 after nodes 0 to 2: the progress is 3
 *     4  the object returns to node 0 with every node marked: the chain clears,
 *        the lap increments, and the progress is 100
 *     5  the object sits between nodes 1 and 2: the fraction forms and the
 *        progress is 1.2
 *     6  the same hit copies the node value and data to the target fields
 *     7  the object finds no node: the target index sets to -1
 *     8  the previous node is unmarked: the node after the first stays
 *        unmarked
 *     9  the current node is 0 and the object sits on the last node: the
 *        neighbour index wraps and the progress is 3
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void track_progress_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define NODE_COUNT 4

struct track_node {
    int radius;
    int value;
    float data;
    int reserved0c;
    int reserved10;
    int x;
    int reserved18;
    int y;
};

struct track_node track_nodes[NODE_COUNT];
int track_count;
double track_scale;

static char text[8];
static unsigned char object[0x500];

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

static void set_flag(int index, int value)
{
    *(int *)(object + 0x328 + index * 4) = value;
}

static int get_flag(int index)
{
    return *(int *)(object + 0x328 + index * 4);
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

static void set_node(
    int index, int x, int y, int radius, int value, unsigned int data_bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = data_bits;
    track_nodes[index].radius = radius;
    track_nodes[index].value = value;
    track_nodes[index].data = v.f;
    track_nodes[index].reserved0c = 0;
    track_nodes[index].reserved10 = 0;
    track_nodes[index].x = x;
    track_nodes[index].reserved18 = 0;
    track_nodes[index].y = y;
}

static void prepare(int index, int count, int lap, int x, int y)
{
    int i;

    for (i = 0; i < 0x500; i++) {
        object[i] = 0;
    }
    set_int(0x320, index);
    set_int(0x324, count);
    set_int(0x40c, lap);
    set_int(0x3f0, 0x12345678);
    set_int(0x498, x);
    set_int(0x4a0, y);
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

int main(void)
{
    set_double_bits(&track_scale, 0x9999999a, 0x3fa99999);

    set_node(0, 0, 0, 60, 11, 0x3fc00000);
    set_node(1, 100, 0, 60, 22, 0x40200000);
    set_node(2, 200, 0, 60, 33, 0x40600000);
    set_node(3, 300, 0, 60, 44, 0x40900000);
    track_count = NODE_COUNT;

    prepare(0, 0, 0, 0, 0);
    track_progress_update(object);
    put(get_int(0x3f0) == 0 && get_int(0x320) == 0 &&
                get_int(0x3f4) == 11 && get_float_bits(0x3fc) == 0x3fc00000 &&
                get_int(0x324) == 1 && get_flag(0) == 1 &&
                get_int(0x40c) == 0 && get_float_bits(0x404) == 0x00000000 &&
                get_float_bits(0x408) == 0x00000000
            ? '0'
            : 'x');

    prepare(0, 1, 0, 100, 0);
    set_flag(0, 1);
    track_progress_update(object);
    put(get_int(0x3f0) == 1 && get_int(0x320) == 1 &&
                get_int(0x3f4) == 22 && get_float_bits(0x3fc) == 0x40200000 &&
                get_int(0x324) == 2 && get_flag(1) == 1 &&
                get_int(0x40c) == 0 && get_float_bits(0x408) == 0x3f800000
            ? '1'
            : 'x');

    prepare(1, 2, 0, 200, 0);
    set_flag(0, 1);
    set_flag(1, 1);
    track_progress_update(object);
    put(get_int(0x3f0) == 2 && get_int(0x320) == 2 &&
                get_int(0x324) == 3 && get_flag(2) == 1 &&
                get_float_bits(0x408) == 0x40000000
            ? '2'
            : 'x');

    prepare(2, 3, 0, 300, 0);
    set_flag(0, 1);
    set_flag(1, 1);
    set_flag(2, 1);
    track_progress_update(object);
    put(get_int(0x3f0) == 3 && get_int(0x320) == 3 &&
                get_int(0x324) == 4 && get_flag(3) == 1 &&
                get_int(0x40c) == 0 && get_float_bits(0x408) == 0x40400000
            ? '3'
            : 'x');

    prepare(3, 4, 0, 0, 0);
    set_flag(0, 1);
    set_flag(1, 1);
    set_flag(2, 1);
    set_flag(3, 1);
    track_progress_update(object);
    put(get_int(0x3f0) == 0 && get_int(0x320) == 0 &&
                get_int(0x324) == 1 && get_int(0x40c) == 1 &&
                get_flag(1) == 0 && get_flag(2) == 0 && get_flag(3) == 0 &&
                get_float_bits(0x408) == 0x42c80000
            ? '4'
            : 'x');

    prepare(1, 1, 0, 125, 0);
    set_flag(0, 1);
    track_progress_update(object);
    put(get_int(0x3f0) == 1 && get_int(0x320) == 1 &&
                get_int(0x324) == 2 && get_flag(1) == 1 &&
                get_float_bits(0x404) == 0x3e4ccccd &&
                get_float_bits(0x408) == 0x3f99999a
            ? '5'
            : 'x');
    put(get_int(0x3f4) == 22 && get_float_bits(0x3fc) == 0x40200000 ? '6'
                                                                   : 'x');

    prepare(2, 3, 0, 200, -100);
    track_progress_update(object);
    put(get_int(0x3f0) == -1 && get_int(0x320) == 2 &&
                get_float_bits(0x408) == 0x40400000
            ? '7'
            : 'x');

    prepare(2, 1, 0, 200, 0);
    track_progress_update(object);
    put(get_int(0x3f0) == 2 && get_int(0x320) == 2 &&
                get_int(0x324) == 1 && get_flag(2) == 0 &&
                get_float_bits(0x408) == 0x40000000
            ? '8'
            : 'x');

    prepare(0, 3, 0, 300, 0);
    set_flag(0, 1);
    set_flag(1, 1);
    set_flag(2, 1);
    track_progress_update(object);
    put(get_int(0x3f0) == 3 && get_int(0x320) == 3 &&
                get_int(0x324) == 4 && get_flag(3) == 1 &&
                get_float_bits(0x408) == 0x40400000
            ? '9'
            : 'x');

    put('D');
    quit();
    return 0;
}
