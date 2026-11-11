/* The test harness for the rebuilt `func_113c8` module.
 *
 * The harness holds one opaque object table, one position block, and the
 * `object_chunk_find` stub.  The stub records each call, checks that the caller
 * passes the same name, position, and object pointers, and checks that the
 * position slot equals the call number.  A selected slot makes the stub set the
 * chunk index and return 0.  Any other slot returns the configured non-zero
 * result.
 *
 * The position block holds the object-table slot at `+0x00` and the chunk index
 * at `+0x04`.  The sentinel 0x7777 marks an untouched index.
 *
 * The harness runs these paths:
 *
 *     0  a match at slot 5: the result 0, six calls, the slot 5, and the index
 *        0x55
 *     1  no match: the result 7, 0x78 calls, the last slot 0x77, and the
 *        unchanged index
 *     2  a match at the first slot: the result 0 and one call
 *     3  a match at the last slot 0x77: the result 0 and 0x78 calls
 *     4  no match with the non-zero result 9: the function still returns 7
 *     D  the exit marker
 */
#pragma off (check_stack)

struct object_record {
    char name[0x0c];
    void *block;
    void *chunks;
    void *file;
    int reserved;
};

struct chunk_position {
    int slot;
    int index;
};

int func_113c8(char *name, struct chunk_position *position,
    struct object_record *objects);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char objects_data[0x10];
static struct chunk_position position_cell;
static char wanted[8];
static char text[8];

static int find_calls;
static int find_match;
static int find_result;
static int find_index;
static int slot_error;
static int argument_error;

int object_chunk_find(char *name, struct chunk_position *pos,
    struct object_record *objects)
{
    if (name != wanted || pos != &position_cell
        || objects != (struct object_record *)objects_data) {
        argument_error = 1;
    }
    if (pos->slot != find_calls) {
        slot_error = 1;
    }
    find_calls = find_calls + 1;
    if (pos->slot == find_match) {
        pos->index = find_index;
        return 0;
    }
    return find_result;
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

static void check(int number, int ok)
{
    put(ok ? (char)('0' + number) : 'x');
}

static void reset(void)
{
    position_cell.slot = -1;
    position_cell.index = 0x7777;
    find_calls = 0;
    find_match = -1;
    find_result = 7;
    find_index = 0;
    slot_error = 0;
    argument_error = 0;
}

static int clean(void)
{
    return slot_error == 0 && argument_error == 0;
}

int main(void)
{
    int result;

    reset();
    find_match = 5;
    find_index = 0x55;
    result = func_113c8(wanted, &position_cell,
        (struct object_record *)objects_data);
    check(0, result == 0 && find_calls == 6 && position_cell.slot == 5
             && position_cell.index == 0x55 && clean());

    reset();
    result = func_113c8(wanted, &position_cell,
        (struct object_record *)objects_data);
    check(1, result == 7 && find_calls == 0x78 && position_cell.slot == 0x77
             && position_cell.index == 0x7777 && clean());

    reset();
    find_match = 0;
    find_index = 3;
    result = func_113c8(wanted, &position_cell,
        (struct object_record *)objects_data);
    check(2, result == 0 && find_calls == 1 && position_cell.slot == 0
             && position_cell.index == 3 && clean());

    reset();
    find_match = 0x77;
    find_index = 0x77;
    result = func_113c8(wanted, &position_cell,
        (struct object_record *)objects_data);
    check(3, result == 0 && find_calls == 0x78 && position_cell.slot == 0x77
             && position_cell.index == 0x77 && clean());

    reset();
    find_result = 9;
    result = func_113c8(wanted, &position_cell,
        (struct object_record *)objects_data);
    check(4, result == 7 && find_calls == 0x78 && position_cell.slot == 0x77
             && position_cell.index == 0x7777 && clean());

    put('D');
    quit();
    return 0;
}
