/* The test harness for the rebuilt `object_slot_acquire` module.
 *
 * The harness holds four 0x1c-byte object records, one block object for each
 * record, an eight-entry list, and one entry index.  The record holds the entry
 * slot at +0x18 and the block pointer at +0x0c.  The sentinel 0x55 marks an
 * untouched record slot and the sentinel 0x1234 marks an untouched list value.
 *
 * The harness runs eight paths:
 *
 *     0  a free slot at list value -1: the record takes slot 0 and list[0] takes
 *        the block pointer
 *     1  the first list value is occupied and list[1] is zero: the record takes
 *        slot 1 and list[2] clears
 *     2  the first list value is occupied and list[1] is -1: the record takes
 *        slot 1 and list[2] stays
 *     3  an entry that is already used: the call returns 0xd and changes nothing
 *     4  the second record: only that record takes a slot
 *     5  the last record over two occupied values: the record takes slot 2
 *     6  the second record over three occupied values: the record takes the -1
 *        value at slot 3
 *     7  the first list value is zero: the record takes slot 0 and list[1]
 *        clears
 *     D  the exit marker
 *
 * The table, the blocks, and the list use BSS state, because the minimal image
 * does not load the initialized data page.
 */
#pragma off (check_stack)

#define RECORD_COUNT 4
#define RECORD_SIZE 0x1c
#define LIST_COUNT 8

struct object_record {
    char name[0x0c];
    void *block;
    void *chunks;
    void *file;
    int slot;
};

int object_slot_acquire(void *table, int *list, int *index_address);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static unsigned char table[RECORD_COUNT * RECORD_SIZE];
static unsigned char blocks[RECORD_COUNT][8];
static int list[LIST_COUNT];
static int index;
static char text[8];

static struct object_record *record_at(int number)
{
    return (struct object_record *)(table + number * RECORD_SIZE);
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
    int position;
    int byte;

    for (position = 0; position < RECORD_COUNT * RECORD_SIZE; position++) {
        table[position] = 0;
    }
    for (position = 0; position < RECORD_COUNT; position++) {
        for (byte = 0; byte < 8; byte++) {
            blocks[position][byte] = 0;
        }
        record_at(position)->block = blocks[position];
        record_at(position)->slot = 0x55;
    }
    for (position = 0; position < LIST_COUNT; position++) {
        list[position] = 0x1234;
    }
    index = 0;
}

static void set_slot(int number, int slot)
{
    record_at(number)->slot = slot;
}

int main(void)
{
    int result;
    int ok;

    reset();
    set_slot(0, -1);
    list[0] = -1;
    result = object_slot_acquire(table, list, &index);
    ok = result == 0 && record_at(0)->slot == 0 &&
         list[0] == (int)blocks[0] && list[1] == 0x1234;
    report(ok, '0');

    reset();
    set_slot(0, -1);
    list[0] = (int)blocks[1];
    list[1] = 0;
    result = object_slot_acquire(table, list, &index);
    ok = result == 0 && record_at(0)->slot == 1 &&
         list[1] == (int)blocks[0] && list[2] == 0 &&
         list[0] == (int)blocks[1];
    report(ok, '1');

    reset();
    set_slot(0, -1);
    list[0] = (int)blocks[1];
    list[1] = -1;
    result = object_slot_acquire(table, list, &index);
    ok = result == 0 && record_at(0)->slot == 1 &&
         list[1] == (int)blocks[0] && list[2] == 0x1234;
    report(ok, '2');

    reset();
    set_slot(0, 5);
    list[0] = 7;
    result = object_slot_acquire(table, list, &index);
    ok = result == 0xd && record_at(0)->slot == 5 && list[0] == 7;
    report(ok, '3');

    reset();
    index = 2;
    set_slot(2, -1);
    list[0] = -1;
    result = object_slot_acquire(table, list, &index);
    ok = result == 0 && record_at(2)->slot == 0 &&
         list[0] == (int)blocks[2] && record_at(0)->slot == 0x55;
    report(ok, '4');

    reset();
    index = 3;
    set_slot(3, -1);
    list[0] = (int)blocks[0];
    list[1] = (int)blocks[1];
    list[2] = 0;
    result = object_slot_acquire(table, list, &index);
    ok = result == 0 && record_at(3)->slot == 2 &&
         list[2] == (int)blocks[3] && list[3] == 0;
    report(ok, '5');

    reset();
    index = 1;
    set_slot(1, -1);
    list[0] = (int)blocks[0];
    list[1] = (int)blocks[2];
    list[2] = (int)blocks[3];
    list[3] = -1;
    result = object_slot_acquire(table, list, &index);
    ok = result == 0 && record_at(1)->slot == 3 &&
         list[3] == (int)blocks[1] && list[4] == 0x1234;
    report(ok, '6');

    reset();
    set_slot(0, -1);
    list[0] = 0;
    result = object_slot_acquire(table, list, &index);
    ok = result == 0 && record_at(0)->slot == 0 &&
         list[0] == (int)blocks[0] && list[1] == 0;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
