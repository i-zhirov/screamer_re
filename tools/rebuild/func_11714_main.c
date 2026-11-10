/* The test harness for the rebuilt `func_11714` module.
 *
 * The harness holds a four-record object table at the 0x1c-byte stride, an
 * eight-entry list, and one entry index.  The record holds the slot word at
 * +0x18.  The list holds the linked slot values.  The sentinel 0x55 marks an
 * untouched record slot and the sentinel 0x1234 marks an untouched list value.
 *
 * The harness runs seven paths:
 *
 *     0  a free entry: the function returns 0xc and changes nothing
 *     1  a non-zero next value: the slot takes -1 and the next value stays
 *     2  a zero next value: the slot takes 0
 *     3  the second record: only that record clears, the first record stays
 *     4  the last record: a zero next value clears the list head
 *     5  the slot 6: the list values below the slot stay
 *     6  the negative next value: the slot takes -1
 *     D  the exit marker
 */
#pragma off (check_stack)

int func_11714(void *table, int *list, int *index_address);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct entry {
    char name[0xc];
    char pad[0xc];
    int slot;
};

static char text[8];
static struct entry table[4];
static int list[8];
static int index;

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
    int position;

    for (position = 0; position < 4; position++) {
        table[position].slot = 0x55;
    }
    for (position = 0; position < 8; position++) {
        list[position] = 0x1234;
    }
    index = 0;
}

int main(void)
{
    int result;

    reset();
    table[0].slot = -1;
    list[0] = 7;
    result = func_11714(table, list, &index);
    check(0, result == 0xc && table[0].slot == -1 && list[0] == 7);

    reset();
    table[0].slot = 3;
    list[4] = 1;
    result = func_11714(table, list, &index);
    check(1, result == 0 && table[0].slot == -1 && list[3] == -1 &&
             list[4] == 1);

    reset();
    table[0].slot = 4;
    list[4] = 9;
    list[5] = 0;
    result = func_11714(table, list, &index);
    check(2, result == 0 && table[0].slot == -1 && list[4] == 0);

    reset();
    index = 2;
    table[2].slot = 1;
    list[2] = -5;
    result = func_11714(table, list, &index);
    check(3, result == 0 && table[2].slot == -1 && list[1] == -1 &&
             table[0].slot == 0x55);

    reset();
    index = 3;
    table[3].slot = 0;
    list[0] = 9;
    list[1] = 0;
    result = func_11714(table, list, &index);
    check(4, result == 0 && table[3].slot == -1 && list[0] == 0);

    reset();
    index = 1;
    table[1].slot = 6;
    list[7] = 7;
    result = func_11714(table, list, &index);
    check(5, result == 0 && table[1].slot == -1 && list[6] == -1 &&
             list[5] == 0x1234);

    reset();
    index = 2;
    table[2].slot = 2;
    list[2] = 99;
    list[3] = -1;
    result = func_11714(table, list, &index);
    check(6, result == 0 && table[2].slot == -1 && list[2] == -1 &&
             table[0].slot == 0x55);

    put('D');
    quit();
    return 0;
}
