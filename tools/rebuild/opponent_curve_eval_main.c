/* The test harness for the rebuilt `opponent_curve_eval` module.
 *
 * The harness holds six 56-byte nodes, the two 36-byte state entries, the path
 * record, and the 2D curve sampler that the original reaches through 0x1e7b4.
 * The sampler records its input points and the fraction, and it writes a
 * position and a tangent that the harness can check.
 *
 * The harness compares the float fields by their bit pattern, because the
 * minimal image does not load the initialized data and the compiler constant
 * pool would read as zero.  It assigns the phase and the basis scale as raw bit
 * values at run time.
 *
 * The paths:
 *
 *     0  the clear flag takes the first and the second point of the first
 *        node and the first point of the second node, and the scalar
 *        interpolates from 10 to 20 at the phase 0.25
 *     1  the set flag takes the second point of the first node and the two
 *        points of the second node, and the scalar interpolates from 4 to 12
 *        at the phase 0.5
 *     2  the second track entry and the node offsets select the two nodes of
 *        that entry, with a zero phase
 *     3  the negative control values and the negative scalar values stay
 *        negative through the integer to float conversion
 *     4  the odd scalar delta 3 at the phase 0.5 gives the fraction 0.75
 *     5  the equal scalar values keep the scalar at the first value
 *     6  any non-zero flag selects the second point set and adds the flag to
 *        the scalar phase
 *     7  the sampler output at +0x60, +0x64, +0x6c, and +0x70 stays in place,
 *        and the scalar sits at +0x68
 *     D  the exit marker
 */
#pragma off (check_stack)

void opponent_curve_eval(unsigned int state, void *body);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define NODE_SIZE 56
#define NODE_COUNT 6
#define PATH_SIZE 0x80
#define TRACK_STRIDE 36
#define TRACK_COUNT 2

static char nodes[NODE_SIZE * NODE_COUNT];
static char state[TRACK_STRIDE * TRACK_COUNT];
static char path[PATH_SIZE];

double opponent_curve_basis_scale;

static float sample_points[9];
static float sample_t;
static int sample_calls;
static float sample_out[5];

static char text[8];

void opponent_curve_sample(float *out, float *points, float t)
{
    int index;

    sample_calls++;
    sample_t = t;
    for (index = 0; index < 9; index++) {
        sample_points[index] = points[index];
    }
    out[0] = points[0] + points[3] + points[6];
    out[1] = points[1] + points[4] + points[7];
    out[3] = points[3] - points[0];
    out[4] = points[4] - points[1];
    for (index = 0; index < 5; index++) {
        sample_out[index] = out[index];
    }
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

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void set_int(void *base, int offset, int value)
{
    *(int *)((char *)base + offset) = value;
}

static int get_int(void *base, int offset)
{
    return *(int *)((char *)base + offset);
}

static void set_bits(void *base, int offset, unsigned int value)
{
    *(unsigned int *)((char *)base + offset) = value;
}

static unsigned int get_bits(void *base, int offset)
{
    return *(unsigned int *)((char *)base + offset);
}

static void set_state_pointer(int track, char *base)
{
    *(unsigned int *)(state + track * TRACK_STRIDE) = (unsigned int)base;
}

static void set_node(int index, int scalar, int next, int x0, int y0,
                     int z0, int x1, int y1, int z1)
{
    char *node = nodes + index * NODE_SIZE;

    set_int(node, 0x0c, scalar);
    set_int(node, 0x10, next);
    set_int(node, 0x14, x0);
    set_int(node, 0x18, y0);
    set_int(node, 0x1c, z0);
    set_int(node, 0x20, x1);
    set_int(node, 0x24, y1);
    set_int(node, 0x28, z1);
}

static void setup(void)
{
    int index;

    for (index = 0; index < (int)sizeof(nodes); index++) {
        nodes[index] = 0;
    }
    for (index = 0; index < (int)sizeof(state); index++) {
        state[index] = 0;
    }
    for (index = 0; index < (int)sizeof(path); index++) {
        path[index] = 0;
    }
    for (index = 0; index < 9; index++) {
        sample_points[index] = 0;
    }
    for (index = 0; index < 5; index++) {
        sample_out[index] = 0;
    }
    sample_t = 0;
    sample_calls = 0;
    set_bits(&opponent_curve_basis_scale, 0, 0x00000000);
    set_bits(&opponent_curve_basis_scale, 4, 0x3fe00000);
    set_state_pointer(0, nodes);
    set_state_pointer(1, nodes + 3 * NODE_SIZE);
}

static void set_path(int track, int node, unsigned int phase, int flag)
{
    set_int(path, 0x00, track);
    set_int(path, 0x04, node);
    set_bits(path, 0x08, phase);
    set_int(path, 0x0c, flag);
}

static int points_match(unsigned int a0, unsigned int a1, unsigned int a3,
                        unsigned int a4, unsigned int a6, unsigned int a7)
{
    return get_bits(sample_points, 0x00) == a0
        && get_bits(sample_points, 0x04) == a1
        && get_bits(sample_points, 0x0c) == a3
        && get_bits(sample_points, 0x10) == a4
        && get_bits(sample_points, 0x18) == a6
        && get_bits(sample_points, 0x1c) == a7;
}

int main(void)
{
    setup();
    set_node(0, 10, 1, 100, 200, 0, 300, 400, 0);
    set_node(1, 20, 0, 500, 600, 0, 700, 800, 0);
    set_path(0, 0, 0x3e800000, 0);
    opponent_curve_eval((unsigned int)state, path);
    check('0', sample_calls == 1
        && get_bits(&sample_t, 0) == 0x3e800000
        && points_match(0x42c80000, 0x43480000, 0x43960000, 0x43c80000,
                        0x43fa0000, 0x44160000)
        && get_bits(path, 0x60) == 0x44610000
        && get_bits(path, 0x64) == 0x44960000
        && get_bits(path, 0x6c) == 0x43480000
        && get_bits(path, 0x70) == 0x43480000
        && get_bits(path, 0x68) == 0x41340000);

    setup();
    set_node(0, 4, 1, 1, 2, 0, 3, 4, 0);
    set_node(1, 12, 0, 5, 6, 0, 7, 8, 0);
    set_path(0, 0, 0x3f000000, 1);
    opponent_curve_eval((unsigned int)state, path);
    check('1', sample_calls == 1
        && get_bits(&sample_t, 0) == 0x3f000000
        && points_match(0x40400000, 0x40800000, 0x40a00000, 0x40c00000,
                        0x40e00000, 0x41000000)
        && get_bits(path, 0x68) == 0x41200000);

    setup();
    set_node(4, 2, 2, -1, -2, 0, -3, -4, 0);
    set_node(5, -8, 0, 10, 20, 0, 30, 40, 0);
    set_path(1, 1, 0x00000000, 0);
    opponent_curve_eval((unsigned int)state, path);
    check('2', sample_calls == 1
        && get_bits(&sample_t, 0) == 0x00000000
        && points_match(0xbf800000, 0xc0000000, 0xc0400000, 0xc0800000,
                        0x41200000, 0x41a00000)
        && get_bits(path, 0x68) == 0x40000000);

    setup();
    set_node(0, -10, 1, 1, 2, 0, -1, -2, 0);
    set_node(1, -20, 0, -3, -4, 0, -5, -6, 0);
    set_path(0, 0, 0x3f000000, 1);
    opponent_curve_eval((unsigned int)state, path);
    check('3', points_match(0xbf800000, 0xc0000000, 0xc0400000, 0xc0800000,
                            0xc0a00000, 0xc0c00000)
        && get_bits(path, 0x68) == 0xc18c0000);

    setup();
    set_node(0, 0, 1, 1, 1, 0, 2, 2, 0);
    set_node(1, 3, 0, 4, 4, 0, 5, 5, 0);
    set_path(0, 0, 0x3f000000, 0);
    opponent_curve_eval((unsigned int)state, path);
    check('4', get_bits(path, 0x68) == 0x3f400000);

    setup();
    set_node(0, 7, 1, 10, 10, 0, 20, 20, 0);
    set_node(1, 7, 0, 30, 30, 0, 40, 40, 0);
    set_path(0, 0, 0x00000000, 0);
    opponent_curve_eval((unsigned int)state, path);
    check('5', sample_calls == 1
        && points_match(0x41200000, 0x41200000, 0x41a00000, 0x41a00000,
                        0x41f00000, 0x41f00000)
        && get_bits(path, 0x68) == 0x40e00000);

    setup();
    set_node(0, 1, 1, 10, 10, 0, 20, 20, 0);
    set_node(1, 5, 0, 30, 30, 0, 40, 40, 0);
    set_path(0, 0, 0x3e800000, 2);
    opponent_curve_eval((unsigned int)state, path);
    check('6', points_match(0x41a00000, 0x41a00000, 0x41f00000, 0x41f00000,
                            0x42200000, 0x42200000)
        && get_bits(path, 0x68) == 0x40b00000);

    setup();
    set_node(0, 2, 1, 1, 2, 0, 3, 4, 0);
    set_node(1, 4, 0, 5, 6, 0, 7, 8, 0);
    set_path(0, 0, 0x3f000000, 0);
    opponent_curve_eval((unsigned int)state, path);
    check('7', sample_calls == 1
        && get_bits(sample_out, 0x00) == 0x41100000
        && get_bits(sample_out, 0x04) == 0x41400000
        && get_bits(sample_out, 0x0c) == 0x40000000
        && get_bits(sample_out, 0x10) == 0x40000000
        && get_bits(path, 0x60) == 0x41100000
        && get_bits(path, 0x64) == 0x41400000
        && get_bits(path, 0x6c) == 0x40000000
        && get_bits(path, 0x70) == 0x40000000
        && get_bits(path, 0x68) == 0x40200000);

    put('D');
    quit();
    return 0;
}
