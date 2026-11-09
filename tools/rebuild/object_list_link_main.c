/* The test harness for the rebuilt `object_list_link` module.
 *
 * The harness holds the 0x78-record object table at 0x1c bytes, one block
 * object per record index, the list at 0x6350, and two callee stubs.  Each stub
 * records its arguments and returns a scripted value.  The harness does not
 * model the list contents, because the function only passes the list to the two
 * helpers and never reads it.
 *
 * The harness runs nine paths:
 *
 *     0  an empty table: no slot is taken, no slot is tagged, and the call
 *        returns 0
 *     1  one active record with the block bit clear: the slot is taken at
 *        index 0, the tag does not run, and the call returns 0
 *     2  one active record with the block bit set: the slot is taken and the
 *        tag runs at index 0
 *     3  records 0 and 2 are active and record 1 is free: the two slots are
 *        taken and tagged at index 0 and index 2 in table order
 *     4  record 5 is active: the two helpers receive the table, the list, and
 *        the index 5
 *     5  the tag returns 14: the call returns 14 at once and record 1 stays
 *        untouched
 *     6  the slot helper returns 13: the call ignores the result and still runs
 *        the tag
 *     7  the last record 0x77 is active with the block bit clear: the slot is
 *        taken at index 0x77
 *     8  the file field holds 0x100 and the block holds 0x7f: a non-zero,
 *        non-one file record is active, and a clear bit 0x80 does not tag
 *     D  the exit marker
 *
 * The table, the list, and the blocks use BSS state, because the minimal image
 * does not load the initialized data page.
 */
#pragma off (check_stack)

#define RECORD_COUNT 0x78
#define RECORD_SIZE 0x1c
#define BLOCK_FLAG 0x80

struct object_record {
    char name[0x0c];
    void *block;
    void *chunks;
    void *file;
    int reserved;
};

int object_list_link(void *table, void *list);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int object_slot_acquire(void *table, void *list, int *index);
int object_list_bind(void *table, void *list, int *index);

static unsigned char table[RECORD_COUNT * RECORD_SIZE];
static int list[16];
static unsigned char blocks[RECORD_COUNT][8];
static char text[8];

static int acquire_calls;
static void *acquire_table;
static void *acquire_list;
static int acquire_index[16];
static int acquire_result;

static int bind_calls;
static void *bind_table;
static void *bind_list;
static int bind_index[16];
static int bind_result;

int object_slot_acquire(void *table, void *list, int *index)
{
    if (acquire_calls < 16) {
        acquire_index[acquire_calls] = *index;
    }
    acquire_calls++;
    acquire_table = table;
    acquire_list = list;
    return acquire_result;
}

int object_list_bind(void *table, void *list, int *index)
{
    if (bind_calls < 16) {
        bind_index[bind_calls] = *index;
    }
    bind_calls++;
    bind_table = table;
    bind_list = list;
    return bind_result;
}

static struct object_record *record_at(int index)
{
    return (struct object_record *)(table + index * RECORD_SIZE);
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

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

static void reset(void)
{
    int index;
    int byte;

    for (index = 0; index < RECORD_COUNT * RECORD_SIZE; index++) {
        table[index] = 0;
    }
    for (index = 0; index < 16; index++) {
        list[index] = 0;
    }
    for (index = 0; index < RECORD_COUNT; index++) {
        for (byte = 0; byte < 8; byte++) {
            blocks[index][byte] = 0;
        }
    }
    for (index = 0; index < 16; index++) {
        acquire_index[index] = 0;
        bind_index[index] = 0;
    }
    acquire_calls = 0;
    acquire_result = 0;
    acquire_table = 0;
    acquire_list = 0;
    bind_calls = 0;
    bind_result = 0;
    bind_table = 0;
    bind_list = 0;
}

static void set_active(int index, int value)
{
    record_at(index)->file = (void *)value;
}

static void set_block(int index, unsigned char flag)
{
    record_at(index)->block = blocks[index];
    blocks[index][5] = flag;
}

int main(void)
{
    int result;
    int ok;

    reset();
    result = object_list_link(table, list);
    ok = result == 0 && acquire_calls == 0 && bind_calls == 0;
    report(ok, '0');

    reset();
    set_active(0, 1);
    set_block(0, 0);
    result = object_list_link(table, list);
    ok = result == 0
        && acquire_calls == 1 && acquire_index[0] == 0
        && acquire_table == (void *)table && acquire_list == (void *)list
        && bind_calls == 0;
    report(ok, '1');

    reset();
    set_active(0, 1);
    set_block(0, BLOCK_FLAG);
    result = object_list_link(table, list);
    ok = result == 0
        && acquire_calls == 1 && acquire_index[0] == 0
        && bind_calls == 1 && bind_index[0] == 0;
    report(ok, '2');

    reset();
    set_active(0, 1);
    set_block(0, BLOCK_FLAG);
    set_active(2, 1);
    set_block(2, BLOCK_FLAG);
    result = object_list_link(table, list);
    ok = result == 0
        && acquire_calls == 2 && acquire_index[0] == 0 && acquire_index[1] == 2
        && bind_calls == 2 && bind_index[0] == 0 && bind_index[1] == 2;
    report(ok, '3');

    reset();
    set_active(5, 1);
    set_block(5, BLOCK_FLAG);
    result = object_list_link(table, list);
    ok = result == 0
        && acquire_calls == 1 && acquire_index[0] == 5
        && bind_calls == 1 && bind_index[0] == 5
        && bind_table == (void *)table && bind_list == (void *)list;
    report(ok, '4');

    reset();
    set_active(0, 1);
    set_block(0, BLOCK_FLAG);
    set_active(1, 1);
    set_block(1, BLOCK_FLAG);
    bind_result = 0x0e;
    result = object_list_link(table, list);
    ok = result == 0x0e
        && acquire_calls == 1 && acquire_index[0] == 0
        && bind_calls == 1 && bind_index[0] == 0;
    report(ok, '5');

    reset();
    set_active(0, 1);
    set_block(0, BLOCK_FLAG);
    acquire_result = 0x0d;
    result = object_list_link(table, list);
    ok = result == 0
        && acquire_calls == 1 && acquire_index[0] == 0
        && bind_calls == 1 && bind_index[0] == 0;
    report(ok, '6');

    reset();
    set_active(0x77, 1);
    set_block(0x77, 0);
    result = object_list_link(table, list);
    ok = result == 0
        && acquire_calls == 1 && acquire_index[0] == 0x77
        && bind_calls == 0;
    report(ok, '7');

    reset();
    set_active(0, 0x100);
    set_block(0, 0x7f);
    result = object_list_link(table, list);
    ok = result == 0
        && acquire_calls == 1 && acquire_index[0] == 0
        && bind_calls == 0;
    report(ok, '8');

    put('D');
    quit();
    return 0;
}
