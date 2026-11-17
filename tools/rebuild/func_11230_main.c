/* The test harness for the rebuilt `func_11230` module.
 *
 * The harness holds the 0x78-record table of 0x1c bytes and the two release
 * stubs.  The stubs record their pointer and their call count.  The harness
 * runs nine paths:
 *
 *     0  no shared handle at index 5: the two-handle release returns 0 and the
 *        function returns 0
 *     1  a shared handle at index 0x10 from 0x20: the one-handle release
 *        returns 0 and the function returns -1
 *     2  a shared handle at index 0x30 from 0x05: the one-handle release
 *        returns 9 and the function returns 9
 *     3  no shared handle at index 0x40: the two-handle release returns 0xb and
 *        the function returns 0xb
 *     4  a shared handle at index 0x70 from the later index 0x77: the function
 *        returns -1
 *     5  no shared handle at the last index 0x77: the function returns 0
 *     6  a shared handle at index 0 from 0x01: the function returns -1
 *     7  no shared handle at index 0x76: the function returns 0
 *     8  an equal field at +0x0c but a distinct handle at +0x14 keeps the
 *        handle unshared, so the two-handle release runs
 *     D  the exit marker
 *
 * Every path also checks the selected record keeps its flag, padding, and
 * fields, because the function only reads the table.
 */
#pragma off (check_stack)

struct object_entry {
    unsigned char flag;
    char pad_01[0x0b];
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
};

int func_11230(struct object_entry *table, int *index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static struct object_entry table[0x78];
static int index_cell;
static char text[8];

static int one_calls;
static int two_calls;
static struct object_entry *one_entry;
static struct object_entry *two_entry;
static int one_result;
static int two_result;

int release_one(struct object_entry *entry)
{
    one_calls = one_calls + 1;
    one_entry = entry;
    return one_result;
}

int release_two(struct object_entry *entry)
{
    two_calls = two_calls + 1;
    two_entry = entry;
    return two_result;
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

static void reset(void)
{
    int index;
    int slot;

    for (index = 0; index < 0x78; ++index) {
        table[index].flag = 0x5a;
        for (slot = 0; slot < 0x0b; ++slot) {
            table[index].pad_01[slot] = 0x5b;
        }
        table[index].field_0c = 0x0c000000 + index;
        table[index].field_10 = 0x10000000 + index;
        table[index].field_14 = 0x14000000 + index;
        table[index].field_18 = 0x18000000 + index;
    }
    index_cell = 0;
    one_calls = 0;
    two_calls = 0;
    one_entry = 0;
    two_entry = 0;
    one_result = 0;
    two_result = 0;
}

static int untouched(int index, int handle)
{
    int slot;

    if (table[index].flag != 0x5a
        || table[index].field_10 != 0x10000000 + index
        || table[index].field_14 != handle
        || table[index].field_18 != 0x18000000 + index) {
        return 0;
    }
    for (slot = 0; slot < 0x0b; ++slot) {
        if (table[index].pad_01[slot] != 0x5b) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int result;

    reset();
    index_cell = 5;
    result = func_11230(table, &index_cell);
    put(result == 0 && two_calls == 1 && one_calls == 0
        && two_entry == &table[5] && untouched(5, 0x14000005)
        ? '0' : 'x');

    reset();
    index_cell = 0x10;
    table[0x20].field_14 = 0x14000010;
    result = func_11230(table, &index_cell);
    put(result == -1 && one_calls == 1 && two_calls == 0
        && one_entry == &table[0x10] && untouched(0x10, 0x14000010)
        ? '1' : 'x');

    reset();
    index_cell = 0x30;
    table[0x05].field_14 = 0x14000030;
    one_result = 9;
    result = func_11230(table, &index_cell);
    put(result == 9 && one_calls == 1 && two_calls == 0
        && one_entry == &table[0x30] && untouched(0x30, 0x14000030)
        ? '2' : 'x');

    reset();
    index_cell = 0x40;
    two_result = 0x0b;
    result = func_11230(table, &index_cell);
    put(result == 0x0b && two_calls == 1 && one_calls == 0
        && two_entry == &table[0x40] && untouched(0x40, 0x14000040)
        ? '3' : 'x');

    reset();
    index_cell = 0x70;
    table[0x77].field_14 = 0x14000070;
    result = func_11230(table, &index_cell);
    put(result == -1 && one_calls == 1 && two_calls == 0
        && one_entry == &table[0x70] && untouched(0x70, 0x14000070)
        ? '4' : 'x');

    reset();
    index_cell = 0x77;
    result = func_11230(table, &index_cell);
    put(result == 0 && two_calls == 1 && one_calls == 0
        && two_entry == &table[0x77] && untouched(0x77, 0x14000077)
        ? '5' : 'x');

    reset();
    index_cell = 0;
    table[0x01].field_14 = 0x14000000;
    result = func_11230(table, &index_cell);
    put(result == -1 && one_calls == 1 && two_calls == 0
        && one_entry == &table[0] && untouched(0, 0x14000000)
        ? '6' : 'x');

    reset();
    index_cell = 0x76;
    result = func_11230(table, &index_cell);
    put(result == 0 && two_calls == 1 && one_calls == 0
        && two_entry == &table[0x76] && untouched(0x76, 0x14000076)
        ? '7' : 'x');

    reset();
    index_cell = 0x10;
    table[0x11].field_0c = 0x0c000010;
    result = func_11230(table, &index_cell);
    put(result == 0 && two_calls == 1 && one_calls == 0
        && two_entry == &table[0x10] && untouched(0x10, 0x14000010)
        ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
