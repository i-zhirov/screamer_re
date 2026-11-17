/* The test harness for the rebuilt `opponent_records_init` module.
 *
 * The harness holds the difficulty flag, the active flag, the record count,
 * the opponent record array, the state base, the object table, and the two
 * callee stubs.
 *
 * The harness checks the active flag, the record count guard, the load calls
 * and their pointers, the copied track field, the state entry, the three
 * cleared fields, the sound field, and the closing opponent update.
 *
 * The harness runs four paths:
 *
 *     0  the difficulty flag 0: the clear flag and no record walk
 *     1  the difficulty flag 1 with a zero load value: the record walk without
 *        a load and one opponent update with 1
 *     2  the difficulty flag 2 with a non-zero load value: one load per record
 *        with the object table and the slot at record + 0x18
 *     3  the difficulty flag 1 with a failing second load: the early result and
 *        no opponent update
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#define OPPONENT_RECORD_STRIDE 0xbc

int opponent_records_init(int load);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011d2b8;
int DAT_0010cf58;
int DAT_0010f178;
char DAT_0010f180[4 * 0xbc];
char *DAT_0010f174;
char DAT_00006530[4];

static char text[8];
static int state[8 * 9];

static int lookup_calls;
static char *lookup_table;
static char *lookup_record[8];
static char *lookup_slot[8];
static int fail_index;
static int fail_result;

static int update_calls;
static int update_value;

int object_lookup(char *table, char *record, char *slot)
{
    int which;

    which = lookup_calls;
    if (which < 8) {
        lookup_table = table;
        lookup_record[which] = record;
        lookup_slot[which] = slot;
    }
    lookup_calls = lookup_calls + 1;
    if (which == fail_index) {
        return fail_result;
    }
    return 0;
}

void opponent_update(int value)
{
    update_calls = update_calls + 1;
    update_value = value;
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

static int rec_get(int index, int offset)
{
    return *(int *)(DAT_0010f180 + index * OPPONENT_RECORD_STRIDE + offset);
}

static void rec_set(int index, int offset, int value)
{
    *(int *)(DAT_0010f180 + index * OPPONENT_RECORD_STRIDE + offset) = value;
}

static void reset(void)
{
    int position;

    for (position = 0; position < 4 * 0xbc; position++) {
        DAT_0010f180[position] = 0;
    }
    for (position = 0; position < 8 * 9; position++) {
        state[position] = 0;
    }
    for (position = 0; position < 8; position++) {
        lookup_record[position] = 0;
        lookup_slot[position] = 0;
    }
    DAT_0011d2b8 = 0;
    DAT_0010cf58 = -1;
    DAT_0010f178 = 0;
    DAT_0010f174 = (char *)state;
    lookup_calls = 0;
    lookup_table = 0;
    fail_index = -1;
    fail_result = 0;
    update_calls = 0;
    update_value = 0;
}

static void prepare(int difficulty, int load, int count)
{
    int position;

    reset();
    DAT_0011d2b8 = difficulty;
    DAT_0010f178 = count;
    for (position = 0; position < count; position++) {
        rec_set(position, 0x10, position);
        rec_set(position, 0x38, 0x5555);
        rec_set(position, 0x44, 0x6666);
        rec_set(position, 0x48, 0x7777);
        rec_set(position, 0x4c, 0x8888);
        rec_set(position, 0x50, 0x9999);
        state[position * 9 + 8] = 0x5000 + position;
    }
    (void)load;
}

static int untouched(int index)
{
    return rec_get(index, 0x38) == 0x5555
        && rec_get(index, 0x44) == 0x6666
        && rec_get(index, 0x48) == 0x7777
        && rec_get(index, 0x4c) == 0x8888
        && rec_get(index, 0x50) == 0x9999;
}

static int initialised(int index)
{
    return rec_get(index, 0x44) == index
        && rec_get(index, 0x48) == 0x5000 + index
        && rec_get(index, 0x4c) == 0
        && rec_get(index, 0x50) == 0
        && rec_get(index, 0x38) == -1;
}

int main(void)
{
    int result;

    prepare(0, 0, 3);
    result = opponent_records_init(0);
    put(result == 0
        && DAT_0010cf58 == 0
        && lookup_calls == 0
        && update_calls == 0
        && untouched(0) && untouched(1) && untouched(2)
        ? '0' : 'x');

    prepare(1, 0, 3);
    result = opponent_records_init(0);
    put(result == 0
        && DAT_0010cf58 == 1
        && lookup_calls == 0
        && update_calls == 1
        && update_value == 1
        && initialised(0) && initialised(1) && initialised(2)
        ? '1' : 'x');

    prepare(2, 1, 3);
    result = opponent_records_init(1);
    put(result == 0
        && DAT_0010cf58 == 1
        && lookup_calls == 3
        && lookup_table == DAT_00006530
        && lookup_record[0] == DAT_0010f180
        && lookup_slot[0] == DAT_0010f180 + 0x18
        && lookup_record[1] == DAT_0010f180 + 0xbc
        && lookup_slot[1] == DAT_0010f180 + 0xbc + 0x18
        && lookup_record[2] == DAT_0010f180 + 2 * 0xbc
        && lookup_slot[2] == DAT_0010f180 + 2 * 0xbc + 0x18
        && update_calls == 1
        && update_value == 1
        && initialised(0) && initialised(1) && initialised(2)
        ? '2' : 'x');

    prepare(1, 1, 3);
    fail_index = 1;
    fail_result = 7;
    result = opponent_records_init(1);
    put(result == 7
        && lookup_calls == 2
        && update_calls == 0
        && initialised(0)
        && untouched(1) && untouched(2)
        ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
