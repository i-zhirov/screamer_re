/* The test harness for the rebuilt `object_block_tag` module.
 *
 * The harness holds eight 0x1c-byte object records, one block object for each
 * record, and one slot.  The slot holds the object index, so the function reads
 * the index from the first int.
 *
 * The harness runs seven paths:
 *
 *     0  record 0 is loaded and the block byte at +5 is clear: the bit 0x80 is
 *        set and the call returns 0
 *     1  record 0 is loaded and the block byte at +5 holds 0x7f: the result is
 *        0xff and the low bits stay
 *     2  record 0 is loaded and the block byte at +5 holds 0xa5: the bit is
 *        already set, so the byte stays 0xa5 and the call returns 0
 *     3  record 0 is free with the block byte at +5 set to 0x11: the call
 *        returns 0x10 and the block stays 0x11
 *     4  record 5 is loaded: the function changes record 5 and record 0 stays
 *        clear
 *     5  record 7 is loaded and the block byte at +5 holds 0x01: the result is
 *        0x81 and the call returns 0
 *     D  the exit marker
 *
 * The table and the blocks use BSS state, because the minimal image does not
 * load the initialized data page.
 */
#pragma off (check_stack)

#define RECORD_COUNT 8
#define RECORD_SIZE 0x1c

struct object_record {
    char name[0x0c];
    void *block;
    void *chunks;
    void *file;
    int reserved;
};

int object_block_tag(void *table, int *slot);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static unsigned char table[RECORD_COUNT * RECORD_SIZE];
static unsigned char blocks[RECORD_COUNT][8];
static int slot;
static char text[8];

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
    for (index = 0; index < RECORD_COUNT; index++) {
        for (byte = 0; byte < 8; byte++) {
            blocks[index][byte] = 0;
        }
    }
    slot = 0;
}

static void set_loaded(int index, int value)
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
    set_loaded(0, 1);
    set_block(0, 0x00);
    slot = 0;
    result = object_block_tag(table, &slot);
    ok = result == 0 && blocks[0][5] == 0x80;
    report(ok, '0');

    reset();
    set_loaded(0, 1);
    set_block(0, 0x7f);
    slot = 0;
    result = object_block_tag(table, &slot);
    ok = result == 0 && blocks[0][5] == 0xff;
    report(ok, '1');

    reset();
    set_loaded(0, 1);
    set_block(0, 0xa5);
    slot = 0;
    result = object_block_tag(table, &slot);
    ok = result == 0 && blocks[0][5] == 0xa5;
    report(ok, '2');

    reset();
    set_loaded(0, 0);
    set_block(0, 0x11);
    slot = 0;
    result = object_block_tag(table, &slot);
    ok = result == 0x10 && blocks[0][5] == 0x11;
    report(ok, '3');

    reset();
    set_loaded(0, 1);
    set_block(0, 0x00);
    set_loaded(5, 1);
    set_block(5, 0x00);
    slot = 5;
    result = object_block_tag(table, &slot);
    ok = result == 0 && blocks[5][5] == 0x80 && blocks[0][5] == 0x00;
    report(ok, '4');

    reset();
    set_loaded(7, 1);
    set_block(7, 0x01);
    slot = 7;
    result = object_block_tag(table, &slot);
    ok = result == 0 && blocks[7][5] == 0x81;
    report(ok, '5');

    put('D');
    quit();
    return 0;
}
