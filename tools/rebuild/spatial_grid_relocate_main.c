/* The test harness for the rebuilt `spatial_grid_relocate` module.
 *
 * The harness holds the grid region, the row table of 0x100 entries, three row
 * arrays of 0x100 cell pointers, and three cells of five pointer slots plus
 * three extra fields.  The region base reaches the function through
 * DAT_00102fa4.  Every row entry is an offset from the region base, and the
 * relocation turns it into an absolute address.  The harness runs four paths:
 *
 *     0  a full tree: three rows, three cells, zero and non-zero slots
 *     1  an empty grid: every row entry is -1, so nothing changes
 *     2  a row with no cells: only the row pointer moves
 *     3  one cell: the zero, non-zero, and -1 slots, and the extra fields
 *     D  the exit marker
 *
 * Every path also checks that DAT_00102fa4 keeps its value, that every -1
 * sentinel stays -1, that every zero slot stays zero, and that the extra cell
 * fields stay untouched.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#define ROW_COUNT 0x100
#define CELL_COUNT 0x100
#define SLOT_COUNT 5
#define EXTRA_COUNT 3
#define CELL_WORDS (SLOT_COUNT + EXTRA_COUNT)
#define EMPTY 0xffffffffu
#define FILL 0xdeadbeefu
#define EXTRA 0xa5a5a5a5u

void spatial_grid_relocate(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_00102fa4;

static unsigned int arena[0x1000];
static unsigned int *row0;
static unsigned int *row2;
static unsigned int *row255;
static unsigned int *cell0;
static unsigned int *cell2;
static unsigned int *cell255;
static unsigned int *bump;

static char text[8];

#define BASE ((unsigned int)arena)
#define OFF(pointer) ((unsigned int)(pointer) - BASE)

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    unsigned int index;

    for (index = 0; index < 0x1000; ++index) {
        arena[index] = FILL;
    }
    bump = arena + ROW_COUNT;
    row0 = bump;
    bump += CELL_COUNT;
    row2 = bump;
    bump += CELL_COUNT;
    row255 = bump;
    bump += CELL_COUNT;
    cell0 = bump;
    bump += CELL_WORDS;
    cell2 = bump;
    bump += CELL_WORDS;
    cell255 = bump;
    bump += CELL_WORDS;
    for (index = 0; index < ROW_COUNT; ++index) {
        arena[index] = EMPTY;
    }
    for (index = 0; index < CELL_COUNT; ++index) {
        row0[index] = EMPTY;
        row2[index] = EMPTY;
        row255[index] = EMPTY;
    }
    for (index = 0; index < CELL_WORDS; ++index) {
        cell0[index] = EXTRA;
        cell2[index] = EXTRA;
        cell255[index] = EXTRA;
    }
    DAT_00102fa4 = BASE;
}

static int empty_rows(unsigned int *row)
{
    unsigned int index;

    for (index = 0; index < CELL_COUNT; ++index) {
        if (row[index] != EMPTY) {
            return 0;
        }
    }
    return 1;
}

static int empty_table(void)
{
    unsigned int index;

    for (index = 0; index < ROW_COUNT; ++index) {
        if (arena[index] != EMPTY) {
            return 0;
        }
    }
    return 1;
}

static int extras(unsigned int *cell)
{
    unsigned int index;

    for (index = SLOT_COUNT; index < CELL_WORDS; ++index) {
        if (cell[index] != EXTRA) {
            return 0;
        }
    }
    return 1;
}

static int full_ok(void)
{
    unsigned int base;

    base = DAT_00102fa4;
    if (base != BASE) {
        return 0;
    }
    if (arena[0] != (unsigned int)row0) {
        return 0;
    }
    if (arena[2] != (unsigned int)row2) {
        return 0;
    }
    if (arena[255] != (unsigned int)row255) {
        return 0;
    }
    if (row0[0] != (unsigned int)cell0) {
        return 0;
    }
    if (row0[1] != EMPTY) {
        return 0;
    }
    if (row0[3] != (unsigned int)cell2) {
        return 0;
    }
    if (row255[255] != (unsigned int)cell255) {
        return 0;
    }
    if (cell0[0] != base + 0x100) {
        return 0;
    }
    if (cell0[1] != 0) {
        return 0;
    }
    if (cell0[2] != base + 0x200) {
        return 0;
    }
    if (cell0[3] != 0) {
        return 0;
    }
    if (cell0[4] != base + 0x300) {
        return 0;
    }
    if (cell2[0] != 0 || cell2[1] != 0 || cell2[2] != 0 || cell2[3] != 0) {
        return 0;
    }
    if (cell2[4] != base + 0x400) {
        return 0;
    }
    if (cell255[0] != (unsigned int)(EMPTY + base)) {
        return 0;
    }
    if (cell255[1] != 0) {
        return 0;
    }
    if (cell255[2] != base + 0x500) {
        return 0;
    }
    if (cell255[3] != 0 || cell255[4] != 0) {
        return 0;
    }
    if (!extras(cell0)) {
        return 0;
    }
    if (!extras(cell2)) {
        return 0;
    }
    if (!extras(cell255)) {
        return 0;
    }
    return 1;
}

int main(void)
{
    reset();
    arena[0] = OFF(row0);
    arena[2] = OFF(row2);
    arena[255] = OFF(row255);
    row0[0] = OFF(cell0);
    row0[3] = OFF(cell2);
    row255[255] = OFF(cell255);
    cell0[0] = 0x100;
    cell0[1] = 0;
    cell0[2] = 0x200;
    cell0[3] = 0;
    cell0[4] = 0x300;
    cell2[0] = 0;
    cell2[1] = 0;
    cell2[2] = 0;
    cell2[3] = 0;
    cell2[4] = 0x400;
    cell255[0] = EMPTY;
    cell255[1] = 0;
    cell255[2] = 0x500;
    cell255[3] = 0;
    cell255[4] = 0;
    spatial_grid_relocate();
    put(full_ok()
        && empty_rows(row2) ? '0' : 'x');

    reset();
    spatial_grid_relocate();
    put(empty_table() && DAT_00102fa4 == BASE ? '1' : 'x');

    reset();
    arena[0] = OFF(row0);
    spatial_grid_relocate();
    put(arena[0] == (unsigned int)row0 && empty_rows(row0)
        && empty_rows(row2) && empty_rows(row255)
        && DAT_00102fa4 == BASE ? '2' : 'x');

    reset();
    arena[7] = OFF(row0);
    row0[42] = OFF(cell0);
    cell0[0] = 0x111;
    cell0[1] = 0;
    cell0[2] = EMPTY;
    cell0[3] = 0x222;
    cell0[4] = 0;
    spatial_grid_relocate();
    put(arena[0] == EMPTY && arena[7] == (unsigned int)row0
        && row0[42] == (unsigned int)cell0
        && cell0[0] == BASE + 0x111 && cell0[1] == 0
        && cell0[2] == (unsigned int)(EMPTY + BASE)
        && cell0[3] == BASE + 0x222 && cell0[4] == 0
        && extras(cell0)
        && DAT_00102fa4 == BASE ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
