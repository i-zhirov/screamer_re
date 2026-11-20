/* The test harness for the rebuilt `object_list_tag_toggle` module.
 *
 * The harness holds eight 0x1c-byte object records, one block object for each
 * record, an eight-entry list, and one entry index.  The record holds the entry
 * slot at +0x18 and the block pointer at +0x0c.  The sentinel 0x55 marks an
 * untouched record slot and the sentinel 0x1234 marks an untouched list value.
 *
 * The harness runs seven paths:
 *
 *     0  record 0 takes list slot 0: list[0] takes the top bit and the call
 *        returns 0
 *     1  record 0 takes list slot 3: list[3] inverts the top bit and the other
 *        list values stay
 *     2  record 0 takes the last list slot 7: the value -1 becomes the top bit
 *        clear
 *     3  an entry with the slot -1: the call returns 0xe and the list stays
 *     4  index 2 selects record 2: the free slot returns 0xe, then the slot 1
 *        takes the top bit and record 0 stays clear
 *     5  index 3 selects record 3: list[6] inverts the top bit of 0x40000000
 *     6  two calls: record 1 sets list[2] and record 5 clears list[4]
 *     D  the exit marker
 *
 * The table, the blocks, and the list use BSS state, because the minimal image
 * does not load the initialized data page.
 */
#pragma off (check_stack)

#define RECORD_COUNT 8
#define RECORD_SIZE 0x1c
#define LIST_COUNT 8

struct object_record {
    char name[0x0c];
    void *block;
    void *chunks;
    void *file;
    int slot;
};

int object_list_tag_toggle(void *table, int *list, int *index_address);
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
    set_slot(0, 0);
    list[0] = 0;
    result = object_list_tag_toggle(table, list, &index);
    ok = result == 0 && list[0] == (int)0x80000000 &&
         list[1] == 0x1234;
    report(ok, '0');

    reset();
    set_slot(0, 3);
    list[3] = 0x12345678;
    result = object_list_tag_toggle(table, list, &index);
    ok = result == 0 && list[3] == (int)0x92345678 &&
         list[2] == 0x1234 && list[4] == 0x1234;
    report(ok, '1');

    reset();
    set_slot(0, 7);
    list[7] = -1;
    result = object_list_tag_toggle(table, list, &index);
    ok = result == 0 && list[7] == (int)0x7fffffff &&
         list[6] == 0x1234;
    report(ok, '2');

    reset();
    set_slot(0, -1);
    list[0] = 0x1234;
    result = object_list_tag_toggle(table, list, &index);
    ok = result == 0xe && list[0] == 0x1234 && list[1] == 0x1234;
    report(ok, '3');

    reset();
    index = 2;
    set_slot(2, -1);
    list[0] = 0x1234;
    result = object_list_tag_toggle(table, list, &index);
    ok = result == 0xe && list[0] == 0x1234;
    set_slot(2, 1);
    list[1] = 0;
    result = object_list_tag_toggle(table, list, &index);
    ok = ok && result == 0 && list[1] == (int)0x80000000 &&
         record_at(0)->slot == 0x55 && list[0] == 0x1234;
    report(ok, '4');

    reset();
    index = 3;
    set_slot(3, 6);
    list[6] = 0x40000000;
    result = object_list_tag_toggle(table, list, &index);
    ok = result == 0 && list[6] == (int)0xc0000000 &&
         list[5] == 0x1234;
    report(ok, '5');

    reset();
    index = 1;
    set_slot(1, 2);
    list[2] = 0;
    result = object_list_tag_toggle(table, list, &index);
    ok = result == 0;
    index = 5;
    set_slot(5, 4);
    list[4] = (int)0x80000000;
    result = object_list_tag_toggle(table, list, &index);
    ok = ok && result == 0 && list[2] == (int)0x80000000 &&
         list[4] == 0 && list[0] == 0x1234;
    report(ok, '6');

    put('D');
    quit();
    return 0;
}
