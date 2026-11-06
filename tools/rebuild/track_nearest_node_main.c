/* The test harness for the rebuilt `track_nearest_node` module.
 *
 * The harness holds a 0x100-row spatial grid, three 0x100-cell rows, four cell
 * records, the node and segment entry arrays, the grid and result globals, and
 * the three angle constants.  The query position arrives through
 * `DAT_00108038` and `DAT_00108040`, and the module reads the nearest entry
 * back through `DAT_00108054` or `DAT_00108044`.
 *
 * The harness runs the markers `0` through `9`, `A`, and `D`:
 *
 *     0  the zero mode finds the only node at distance zero and returns its
 *        heading 0.5
 *     1  the scan crosses two rows and two cells and picks the nearer node
 *     2  the flag bit 3 adds pi and the heading stays below two pi
 *     3  the flag bit 3 adds pi and the heading wraps by minus two pi
 *     4  no cell matches, so the function returns 0 and keeps the old heading
 *     5  the only node sits in the last row and cell of the scan
 *     6  the non-zero mode picks the only segment and leaves the heading
 *     7  the non-zero mode picks the nearer of two segments
 *     8  the non-zero mode finds no segment and returns 0
 *     9  the heading reaches two pi exactly and wraps to zero
 *     A  the heading above two pi wraps with the flag clear
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because an FPU memory
 * operand of a literal reads the constant pool at a low address in this
 * toolchain.  The float checks compare the bit patterns exactly.
 */
#pragma off (check_stack)

int track_nearest_node(int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00108020;
int *DAT_00108030;
int *DAT_00108034;
int DAT_00108038;
int DAT_00108040;
int *DAT_00108044;
float DAT_00108048;
int DAT_0010804c;
int DAT_00108050;
int *DAT_00108054;
int *DAT_00102fa4;
float DAT_0010040c;
float DAT_00100410;
float DAT_00100414;
unsigned char DAT_0011d274;

struct cell {
    int object_base;
    int reserved04;
    int *nodes;
    int *segments;
    int reserved10;
    int object_count;
    int reserved18;
    int node_count;
    int segment_count;
};

struct node {
    float angle;
    int x;
    int y;
};

struct segment {
    float angle;
    int x;
    int y;
    int extra0;
    int extra1;
    int extra2;
    int extra3;
};

static int grid_rows[0x100];
static int row_first[0x100];
static int row_second[0x100];
static int row_third[0x100];
static struct cell cells[4];
static struct node nodes[8];
static struct segment segments[8];
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

static void clear_ints(int *values, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        values[index] = -1;
    }
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static unsigned int float_bits(float value)
{
    union {
        float f;
        unsigned int u;
    } number;

    number.f = value;
    return number.u;
}

static void set_node(struct node *entry, unsigned int angle, int x, int y)
{
    set_bits_float(&entry->angle, angle);
    entry->x = x;
    entry->y = y;
}

static void set_segment(
    struct segment *entry, unsigned int angle, int x, int y)
{
    set_bits_float(&entry->angle, angle);
    entry->x = x;
    entry->y = y;
    entry->extra0 = 0;
    entry->extra1 = 0;
    entry->extra2 = 0;
    entry->extra3 = 0;
}

static void set_cell(int row, int column, struct cell *value)
{
    int *cells_row;

    cells_row = (int *)grid_rows[row];
    cells_row[column] = (int)value;
}

static void reset(void)
{
    int index;

    clear_ints(grid_rows, 0x100);
    clear_ints(row_first, 0x100);
    clear_ints(row_second, 0x100);
    clear_ints(row_third, 0x100);
    for (index = 0; index < 4; index++) {
        cells[index].object_base = 0;
        cells[index].reserved04 = 0;
        cells[index].nodes = 0;
        cells[index].segments = 0;
        cells[index].reserved10 = 0;
        cells[index].object_count = 0;
        cells[index].reserved18 = 0;
        cells[index].node_count = 0;
        cells[index].segment_count = 0;
    }
    for (index = 0; index < 8; index++) {
        set_node(&nodes[index], 0, 0, 0);
        set_segment(&segments[index], 0, 0, 0);
    }
    grid_rows[127] = (int)row_first;
    grid_rows[128] = (int)row_second;
    grid_rows[129] = (int)row_third;
    DAT_00102fa4 = grid_rows;
    DAT_00108020 = 0;
    DAT_00108030 = 0;
    DAT_00108034 = 0;
    DAT_00108038 = 0;
    DAT_00108040 = 0;
    DAT_00108044 = 0;
    DAT_00108048 = 0.0f;
    DAT_0010804c = 0;
    DAT_00108050 = 0;
    DAT_00108054 = 0;
    DAT_0011d274 = 0;
    set_bits_float(&DAT_0010040c, 0x40490fdb);
    set_bits_float(&DAT_00100410, 0x40c90fdb);
    set_bits_float(&DAT_00100414, 0xc0c90fdb);
}

int main(void)
{
    int found;

    reset();
    set_node(&nodes[0], 0x3f000000, 0, 0);
    cells[0].nodes = (int *)&nodes[0];
    cells[0].node_count = 1;
    set_cell(127, 127, &cells[0]);
    found = track_nearest_node(0);
    put(found == 1
        && DAT_00108020 == 0
        && DAT_00108054 == (int *)&nodes[0]
        && DAT_0010804c == 0
        && DAT_00108050 == 0
        && float_bits(DAT_00108048) == 0x3f000000
        ? '0' : 'x');

    reset();
    set_node(&nodes[0], 0x40000000, 0, 0);
    set_node(&nodes[1], 0x3f800000, 30, 0);
    cells[0].nodes = (int *)&nodes[0];
    cells[0].node_count = 1;
    cells[1].nodes = (int *)&nodes[1];
    cells[1].node_count = 1;
    set_cell(127, 127, &cells[0]);
    set_cell(128, 128, &cells[1]);
    found = track_nearest_node(0);
    put(found == 1
        && DAT_00108020 == 0
        && DAT_00108054 == (int *)&nodes[0]
        && float_bits(DAT_00108048) == 0x40000000
        ? '1' : 'x');

    reset();
    DAT_0011d274 = 8;
    set_node(&nodes[0], 0x3f000000, 0, 0);
    cells[0].nodes = (int *)&nodes[0];
    cells[0].node_count = 1;
    set_cell(127, 127, &cells[0]);
    found = track_nearest_node(0);
    put(found == 1 && float_bits(DAT_00108048) == 0x40690fdb ? '2' : 'x');

    reset();
    DAT_0011d274 = 8;
    set_node(&nodes[0], 0x40800000, 0, 0);
    cells[0].nodes = (int *)&nodes[0];
    cells[0].node_count = 1;
    set_cell(127, 127, &cells[0]);
    found = track_nearest_node(0);
    put(found == 1 && float_bits(DAT_00108048) == 0x3f5bc098 ? '3' : 'x');

    reset();
    set_bits_float(&nodes[0].angle, 0x3e800000);
    nodes[0].x = 7;
    nodes[0].y = 9;
    DAT_00108054 = (int *)&nodes[0];
    found = track_nearest_node(0);
    put(found == 0
        && DAT_00108020 == 0x7ffffff8
        && DAT_00108054 == (int *)&nodes[0]
        && DAT_0010804c == 7
        && DAT_00108050 == 9
        && float_bits(DAT_00108048) == 0x3e800000
        ? '4' : 'x');

    reset();
    set_node(&nodes[0], 0x40400000, 5, 5);
    cells[0].nodes = (int *)&nodes[0];
    cells[0].node_count = 1;
    set_cell(129, 129, &cells[0]);
    found = track_nearest_node(0);
    put(found == 1
        && DAT_00108020 == 50
        && DAT_00108054 == (int *)&nodes[0]
        && float_bits(DAT_00108048) == 0x40400000
        ? '5' : 'x');

    reset();
    set_segment(&segments[0], 0x3e800000, 10, 0);
    cells[0].segments = (int *)&segments[0];
    cells[0].segment_count = 1;
    set_cell(127, 127, &cells[0]);
    set_bits_float(&DAT_00108048, 0x3f800000);
    found = track_nearest_node(1);
    put(found == 1
        && DAT_00108020 == 100
        && DAT_00108044 == (int *)&segments[0]
        && float_bits(DAT_00108048) == 0x3f800000
        ? '6' : 'x');

    reset();
    set_segment(&segments[0], 0x3f000000, 100, 0);
    set_segment(&segments[1], 0x40000000, 0, 0);
    cells[0].segments = (int *)&segments[0];
    cells[0].segment_count = 2;
    set_cell(127, 127, &cells[0]);
    found = track_nearest_node(1);
    put(found == 1
        && DAT_00108020 == 0
        && DAT_00108044 == (int *)&segments[1]
        ? '7' : 'x');

    reset();
    DAT_00108044 = (int *)&segments[0];
    found = track_nearest_node(1);
    put(found == 0
        && DAT_00108020 == 0x7ffffff8
        && DAT_00108044 == (int *)&segments[0]
        ? '8' : 'x');

    reset();
    DAT_0011d274 = 8;
    set_node(&nodes[0], 0x40490fdb, 0, 0);
    cells[0].nodes = (int *)&nodes[0];
    cells[0].node_count = 1;
    set_cell(127, 127, &cells[0]);
    found = track_nearest_node(0);
    put(found == 1 && float_bits(DAT_00108048) == 0x00000000 ? '9' : 'x');

    reset();
    set_node(&nodes[0], 0x40e00000, 0, 0);
    cells[0].nodes = (int *)&nodes[0];
    cells[0].node_count = 1;
    set_cell(127, 127, &cells[0]);
    found = track_nearest_node(0);
    put(found == 1 && float_bits(DAT_00108048) == 0x3f378128 ? 'A' : 'x');

    put('D');
    quit();
    return 0;
}
