/* The test harness for the rebuilt `map_collision_check` module.
 *
 * The harness holds a 0x100-row spatial grid, three 0x100-cell rows, the nine
 * cell offsets of the table at object 3 offset 0x1cf0, the two slot globals,
 * the world position, and the `collision_cell_test` stub.  The query position
 * is 0 on both axes, so the cell index is 128 on both axes and the scan covers
 * the rows 127, 128, and 129 and the same columns.
 *
 * The stub records the mode, the cell pointer from `DAT_00108034`, and the
 * scripted return value.  A non-zero return reports a contact.  The harness
 * fills the offset table at run time, because the minimal image does not load
 * the initialized data segment.
 *
 * The harness runs nine paths:
 *
 *     0  the empty grid: the scan finds no row and returns 0
 *     1  a row holds no cell: the scan skips every cell and returns 0
 *     2  the centre cell misses: the function returns 0 after one call
 *     3  the centre cell hits: the function returns 1 after one call
 *     4  the centre cell misses and the right cell hits: two calls
 *     5  the mode reaches the callee unchanged
 *     6  the lower row offset hits: the scan reaches row 129 and column 128
 *     7  the negative column and row offset hits: row 127 and column 127
 *     8  the negative row and positive column offset hits: row 127 and
 *        column 129
 *     D  the exit marker
 *
 * The marker goes through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#define GRID_ROWS 0x100
#define GRID_CELLS 0x100

struct cell_offset {
    int column;
    int spare;
    int row;
};

int map_collision_check(int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int *DAT_00102fa4;
struct cell_offset DAT_00101cf0[9];
int *DAT_00108030;
int *DAT_00108034;
int DAT_00108038;
int DAT_00108040;

static int grid_rows[GRID_ROWS];
static int row_first[GRID_CELLS];
static int row_second[GRID_CELLS];
static int row_third[GRID_CELLS];
static char text[8];

static int stub_calls;
static int stub_mode[16];
static int stub_cell[16];
static int stub_return[16];

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

int collision_cell_test(int mode)
{
    int index;
    int result;

    index = stub_calls;
    result = 0;
    if (index < 16) {
        stub_mode[index] = mode;
        stub_cell[index] = *DAT_00108034;
        result = stub_return[index];
    }
    stub_calls = stub_calls + 1;
    return result;
}

static void reset(void)
{
    int index;

    clear_ints(grid_rows, GRID_ROWS);
    clear_ints(row_first, GRID_CELLS);
    clear_ints(row_second, GRID_CELLS);
    clear_ints(row_third, GRID_CELLS);
    DAT_00101cf0[0].column = 0;
    DAT_00101cf0[0].spare = 0;
    DAT_00101cf0[0].row = 0;
    DAT_00101cf0[1].column = 1;
    DAT_00101cf0[1].spare = 0;
    DAT_00101cf0[1].row = 0;
    DAT_00101cf0[2].column = -1;
    DAT_00101cf0[2].spare = 0;
    DAT_00101cf0[2].row = 0;
    DAT_00101cf0[3].column = 0;
    DAT_00101cf0[3].spare = 0;
    DAT_00101cf0[3].row = 1;
    DAT_00101cf0[4].column = 0;
    DAT_00101cf0[4].spare = 0;
    DAT_00101cf0[4].row = -1;
    DAT_00101cf0[5].column = 1;
    DAT_00101cf0[5].spare = 0;
    DAT_00101cf0[5].row = 1;
    DAT_00101cf0[6].column = -1;
    DAT_00101cf0[6].spare = 0;
    DAT_00101cf0[6].row = -1;
    DAT_00101cf0[7].column = 1;
    DAT_00101cf0[7].spare = 0;
    DAT_00101cf0[7].row = -1;
    DAT_00101cf0[8].column = -1;
    DAT_00101cf0[8].spare = 0;
    DAT_00101cf0[8].row = 1;
    DAT_00102fa4 = grid_rows;
    DAT_00108030 = 0;
    DAT_00108034 = 0;
    DAT_00108038 = 0;
    DAT_00108040 = 0;
    stub_calls = 0;
    for (index = 0; index < 16; index++) {
        stub_mode[index] = 0;
        stub_cell[index] = 0;
        stub_return[index] = 0;
    }
}

static void install_rows(void)
{
    grid_rows[127] = (int)row_first;
    grid_rows[128] = (int)row_second;
    grid_rows[129] = (int)row_third;
}

int main(void)
{
    int result;

    reset();
    result = map_collision_check(0);
    put(result == 0 && stub_calls == 0 ? '0' : 'x');

    reset();
    grid_rows[128] = (int)row_second;
    result = map_collision_check(0);
    put(result == 0 && stub_calls == 0 ? '1' : 'x');

    reset();
    install_rows();
    row_second[128] = 0x2000;
    result = map_collision_check(0);
    put(result == 0 && stub_calls == 1 && stub_mode[0] == 0
        && stub_cell[0] == 0x2000 ? '2' : 'x');

    reset();
    install_rows();
    row_second[128] = 0x2100;
    stub_return[0] = 1;
    result = map_collision_check(0);
    put(result == 1 && stub_calls == 1 && stub_cell[0] == 0x2100 ? '3' : 'x');

    reset();
    install_rows();
    row_second[128] = 0x2200;
    row_second[129] = 0x2201;
    stub_return[0] = 0;
    stub_return[1] = 1;
    result = map_collision_check(0);
    put(result == 1 && stub_calls == 2 && stub_cell[0] == 0x2200
        && stub_cell[1] == 0x2201 && stub_mode[1] == 0 ? '4' : 'x');

    reset();
    install_rows();
    row_second[128] = 0x2300;
    stub_return[0] = 1;
    result = map_collision_check(7);
    put(result == 1 && stub_calls == 1 && stub_mode[0] == 7 ? '5' : 'x');

    reset();
    install_rows();
    row_third[128] = 0x2400;
    stub_return[0] = 1;
    result = map_collision_check(0);
    put(result == 1 && stub_calls == 1 && stub_cell[0] == 0x2400
        && DAT_00108034 == &row_third[128] ? '6' : 'x');

    reset();
    install_rows();
    row_first[127] = 0x2500;
    stub_return[0] = 1;
    result = map_collision_check(0);
    put(result == 1 && stub_calls == 1 && stub_cell[0] == 0x2500
        && DAT_00108034 == &row_first[127] ? '7' : 'x');

    reset();
    install_rows();
    row_first[129] = 0x2600;
    stub_return[0] = 1;
    result = map_collision_check(0);
    put(result == 1 && stub_calls == 1 && stub_cell[0] == 0x2600
        && DAT_00108034 == &row_first[129] ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
